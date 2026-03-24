// TODO(erb) glfw-test stuff

// IMMEDIATE
// - os_poll_events function
//   - Key mapping
//   - IsButtonPressed
//   - IsKeyPressed
//   - IsModifierPressed
// - ui_measure_str_size function
// - rects improvements
//    - rect lines
//    - Gradiants
// - Text improvements
//    - font_size
//    - spacing
// - os_push_clipboard_text function
// - implement SetnextHovercursor and SetnextClickcursor UI functions
// - Multiple windows
// - Multiple panels
// - Panel tabs
// - Cache textures
// - UTF8

// POST IMMEDIATE
// - Stack based log api
//   - Usage: pushLogString("someFunction()", someFunctionArgs()); popLogString();
// - Clean up a decent enough os_window and app_renderer API/lib
// - Finish calculator demo
//   - Add UI alignment
// - File Explorer (dir walker) OR calendar app (calendar notebook)
// - path Drop Callback GLFW

#include <stdio.h>
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>

#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "base.c"

#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef pf_log
#undef pf_log
#endif
#define pf_log(Fmt, ...) fprintf(stdout, Fmt, ##__VA_ARGS__)

#ifdef PlatformError
#undef PlatformError
#endif
#define PlatformError(Fmt, ...) fprintf(stderr, Fmt, ##__VA_ARGS__)

#ifdef pf_assert
#undef pf_assert
#endif
#define pf_assert(Condition)										\
do { if (!(Condition)) { fprintf(stdout, "%s:%d: ASSERT at %s()\n", __FILE__, __LINE__, __func__); abort(); } } while (0)

#define debug_break() __builtin_debugtrap()

typedef struct Loaded_Image
{
  v2i size;
  i32 channels;
  u8 *data;
} Loaded_Image;

typedef struct Render_Rect 
{
  v2f dest_p0;
  v2f dest_p1;
  v2f src_p0;
  v2f src_p1;
  v4f color;
  f32 corner_radius;
  f32 edge_softness;
  f32 texture_slot;
} Render_Rect;

typedef struct Buffer_Attribs
{
  u32 count;
} Buffer_Attribs;

typedef struct Render_Rect_Buffer
{
  u64 count;
  u64 capacity;
  Render_Rect *buffer;
} Render_Rect_Buffer;

typedef struct Image_Texture
{
  u32 id;
  v2f size;
} Image_Texture;

typedef struct Glyph_Info
{
  u32 texture_id;
  v2i size;
  v2i bearing;
  u32 advance;
} Glyph_Info;

typedef struct Render_Batch
{
  GLuint textures[16];
  u32 texture_count;
  u64 start_rect_index;
} Render_Batch;

typedef struct Render_Data
{
  GLuint vao;
  GLuint vbo;
  GLuint program;
  
  Render_Rect_Buffer rects;
  
  u32 batch_count;
  Render_Batch batches[16];
  
  Image_Texture blank_image_texture;
} Render_Data;

typedef struct Font_Data
{
  Glyph_Info ascii_glyph_table[128];
} Font_Data;

typedef struct Read_File_Result
{
  u64 size;
  u8 *data;
} Read_File_Result;

void gl_clear_error()
{
  while (glGetError() != GL_NO_ERROR);
}

i32 gl_poll_error()
{
  GLint error;
  while ((error = glGetError()))
  {
    return error;
  }
  return 0;
}

#define gl(call) \
do \
{ \
gl_clear_error(); \
call; \
i32 error = gl_poll_error(); \
if (error) \
{ \
pf_log("%s:%d: GL ERROR %d: %s\n", __FILE__, __LINE__, error, #call); \
} \
} \
while (0)

void *pf_allocate(u64 size)
{
  void *result = malloc(size);
  pf_assert(result);
  return result;
}

void pf_free(void *Ptr)
{
  free(Ptr);
}

Loaded_Image load_image(char *path)
{
  Loaded_Image result = {0};
  stbi_set_flip_vertically_on_load(1);
  result.data = stbi_load(path, &result.size.x, &result.size.y, &result.channels, 0);
  
  if (!result.data)
  {
    printf("Failed to load %s\n", path);
  }
  
  return result;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) 
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
  {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }
}

u32 create_compile_shader(i32 shader_type, const char *src)
{
  u32 shader = glCreateShader(shader_type);
  
  gl(glShaderSource(shader, 1, &src, 0));
  gl(glCompileShader(shader));
  
  i32 success;
  gl(glGetShaderiv(shader, GL_COMPILE_STATUS, &success));
  if (!success)
  {
    char info_log[512];
    gl(glGetShaderInfoLog(shader, 512, 0, info_log));
    printf("ERROR: Shader: %s\n", info_log);
    printf("%s\n", src);
  }
  
  return shader;
}

Read_File_Result mac_read_file(Arena *arena, String file_path)
{
  Read_File_Result result = {};
  i32 file_descriptor = open(str_to_temp256_cstr(file_path), O_RDONLY);
  if (file_descriptor != -1)
  {
    struct stat file_stat = {};
    if (fstat(file_descriptor, &file_stat) == 0)
    {
      u64 size = (u64)file_stat.st_size;
      void *buffer = (void *)push_array(arena, size+1, u8);
      pf_assert(read(file_descriptor, buffer, size) != -1);
      
      ((u8 *)buffer)[size] = 0;
      
      result.data = buffer;
      result.size = size;
    }
  }
  return result;
}

u32 make_program(const char *vertex_src, const char *fragment_src, b32 should_print_log)
{
  // NOTE(erb): shaders setup
  u32 vertex_shader = create_compile_shader(GL_VERTEX_SHADER, vertex_src);
  u32 fragment_shader = create_compile_shader(GL_FRAGMENT_SHADER, fragment_src);
  
  u32 shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);
  {
    i32 success;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success)
    {
      char info_log[512];
      glGetProgramInfoLog(shader_program, 512, 0, info_log);
      printf("ERROR: Shader LINK: %s\n", info_log);
    }
  }
  
  if (should_print_log)
  {
    pf_log("VERTEX ================\n%s\n", vertex_src);
    pf_log("FRAGME ================\n%s\n", fragment_src);
  }
  
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  
  return shader_program;
}

#define fallback_vertex_shader_source \
"#version 330 core\n" \
"//FALLBACK VERTEX SHADER\n"\
"layout (location = 0) in vec2 apos;\n" \
"layout (location = 1) in vec3 acolor;\n" \
"layout (location = 2) in vec2 aTexCoord;\n" \
"out vec3 ourcolor;\n" \
"out vec2 TexCoord;\n" \
"void main()\n" \
"{\n" \
"    gl_position = vec4(apos, 0.0, 1.0);\n" \
"    ourcolor = acolor;\n" \
"    TexCoord = aTexCoord;\n" \
"}\n"

#define fallback_fragment_shader_source \
"#version 330 core\n" \
"//FALLBACK FRAGMENT SHADER\n"\
"out vec4 Fragcolor;" \
"in vec3 ourcolor;" \
"in vec2 TexCoord;" \
"uniform sampler2D ourTexture;" \
"void main()\n" \
"{\n" \
"    Fragcolor = texture(ourTexture, TexCoord) * vec4(ourcolor, 1.0);\n" \
"}\n"

Read_File_Result fallback_vertex_shader(Read_File_Result file)
{
  if (!file.data)
  {
    file.data = (u8 *)fallback_vertex_shader_source;
    file.size = S(fallback_vertex_shader_source).length;
  }
  return file;
}

Read_File_Result fallback_fragment_shader(Read_File_Result file)
{
  if (!file.data)
  {
    file.data = (u8 *)fallback_fragment_shader_source;
    file.size = S(fallback_fragment_shader_source).length;
  }
  return file;
}

b32 init_window()
{
  // NOTE(erb): initialize library
  if (!glfwInit()) return false;
  
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE); // NOTE(erb): apple forward compatibilty
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  
  GLFWwindow* window;
  window = glfwCreateWindow(640, 480, "GLFW Window", 0, 0);
  if (!window)
  {
    printf("GLFW failed\n");
    glfwTerminate();
    return 0;
  }
  
  glfwMakeContextCurrent(window);
  glfwSwapInterval(0); // NOTE(erb): no vsync
  
  // NOTE(erb): callbacks
  glfwSetKeyCallback(window, key_callback);
  
  // NOTE(erb): opengl setup
  printf("LOG: OpenGL version: %s\n", glGetString(GL_VERSION));
  printf("LOG: OpenGL Vendor: %s\n", glGetString(GL_VENDOR));
  printf("LOG: OpenGL Renderer: %s\n", glGetString(GL_RENDERER));
  printf("LOG: OpenGL Version: %s\n", glGetString(GL_VERSION));
  printf("LOG: OpenGL GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
  
  i32 max_vertex_attribs;
  gl(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attribs));
  printf("LOG: OpenGL max vertex_shader Attributes Allowed: %d\n", max_vertex_attribs);
  
  gl(glEnable(GL_BLEND));
  gl(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
  
  return 1;
}

Image_Texture make_texture_from_image(Loaded_Image *image)
{
  pf_assert(image);
  pf_assert(image->data);
  
  u32 texture_id;
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  
  GLuint gl_image_format = 0;
  if (image->channels == 3) 
  {
    gl_image_format = GL_RGB;
  }
  else if (image->channels == 4) 
  {
    gl_image_format = GL_RGBA;
  }
  
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image->size.x, image->size.y, 0, gl_image_format, GL_UNSIGNED_BYTE, image->data);
  glGenerateMipmap(GL_TEXTURE_2D);
  
  Image_Texture texture = {0};
  texture.id = texture_id;
  texture.size = V2fi(image->size);
  
  return texture;
}

Image_Texture make_texture_from_image_file(char *path)
{
  Image_Texture texture = {0};
  Loaded_Image image = load_image(path);
  if (!image.data) 
  {
    printf("Unable to load: %s\n", path);
  }
  else
  {
    printf("Loaded: %s (%d, %d)\n", path, image.size.x, image.size.y);
    texture = make_texture_from_image(&image);
    stbi_image_free(image.data);
  }
  return texture;
}

v2f Getwindow_size()
{
  i32 width, height;
  glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
  v2f result = V2f((f32)width, (f32)height);
  return result;
}

v2f get_mouse_pos() 
{
  f32 scale_x, scale_y;
  glfwGetWindowContentScale(glfwGetCurrentContext(), &scale_x, &scale_y);
  
  f64 x, y;
  glfwGetCursorPos(glfwGetCurrentContext(), &x, &y);
  v2f result = V2f(x * scale_x, y * scale_y);
  return result;
}

#define push_struct_attrib_f32(attrib_ctx, Struct_Type, field_name, Field_Type) \
_push_struct_attrib_f32(attrib_ctx, sizeof(Struct_Type), sizeof(Field_Type), offsetof(Struct_Type, field_name)) 

void _push_struct_attrib_f32(Buffer_Attribs *attrib_ctx, u64 struct_size, u64 field_size, u64 field_offset) 
{
  pf_assert(field_size % sizeof(f32) == 0);
  glEnableVertexAttribArray(attrib_ctx->count);
  
  f32 component_count = field_size/sizeof(f32);
  glVertexAttribPointer(attrib_ctx->count, component_count, GL_FLOAT, GL_FALSE, struct_size, (void *)field_offset);
  glVertexAttribDivisor(attrib_ctx->count, 1);
  attrib_ctx->count++;
}

Index_u32 get_texture_idx(Render_Batch *batch, u32 texture_id)
{
  Index_u32 result = {0};
  
  for (u32 texture_idx = 0;
       texture_idx < batch->texture_count;
       texture_idx++)
  {
    if (texture_id == batch->textures[texture_idx])
    {
      result.value = texture_idx;
      result.exists = true;
      break;
    }
  }
  
  return result;
}

Render_Rect *append_render_rect(Render_Data *renderer, 
                                v2f dest_p0, v2f dest_p1, 
                                v2f src_p0, v2f src_p1, 
                                f32 corner_radius, f32 edge_softness, 
                                v4f color, u32 texture_id)
{
  // NOTE(erb): push rect
  u32 rect_idx = renderer->rects.count;
  Render_Rect *rect = (renderer->rects.buffer + renderer->rects.count++);
  rect->dest_p0 = dest_p0;
  rect->dest_p1 = dest_p1;
  rect->src_p0 = src_p0;
  rect->src_p1 = src_p1;
  rect->corner_radius = corner_radius;
  rect->edge_softness = edge_softness;
  rect->color = color;
  
  Render_Batch *batch = (renderer->batches + renderer->batch_count - 1);
  
  Index_u32 found_texture_slot = get_texture_idx(batch, texture_id);
  
  if (found_texture_slot.exists)
  {
    rect->texture_slot = found_texture_slot.value;
  }
  else
  {
    // NOTE(erb): push new batch
    if (batch->texture_count == array_size(batch->textures))
    {
      pf_assert(renderer->batch_count <= array_size(renderer->batches));
      batch = (renderer->batches + renderer->batch_count++);
      batch->texture_count = 0;
      batch->start_rect_index = rect_idx;
    }
    
    rect->texture_slot = batch->texture_count;
    batch->textures[batch->texture_count++] = texture_id;
  }
  
  return rect;
}

void append_render_rect_color(Render_Data *renderer, v2f pos, v2f size, v4f color)
{
  append_render_rect(renderer, 
                     pos, v2f_add(pos, size), 
                     V2ff(0), renderer->blank_image_texture.size, 
                     0, 0,
                     color, renderer->blank_image_texture.id);
}

void append_render_rect_color_rounded(Render_Data *renderer, v2f pos, v2f size, v4f color, f32 corner_radius)
{
  append_render_rect(renderer, 
                     pos, v2f_add(pos, size), 
                     V2ff(0), renderer->blank_image_texture.size, 
                     corner_radius, 2.0f,
                     color, renderer->blank_image_texture.id);
}

void append_render_rect_texture(Render_Data *renderer, v2f pos, v2f size, Image_Texture texture)
{
  append_render_rect(renderer, 
                     pos, v2f_add(pos, size), 
                     V2ff(0), texture.size, 
                     0, 0,
                     V4ff(1), texture.id);
}


void AppendRenderText(Render_Data *renderer, Font_Data *font_data, String str, v2f position, f32 scale, v4f color)
{
  v2f advance_pos = position;
  
  for (u32 char_idx = 0;
       char_idx < str.length;
       char_idx++)
  {
    char ch = str.value[char_idx];
    Glyph_Info info = font_data->ascii_glyph_table[(u32)ch];
    
    f32 pos_x = advance_pos.x + info.bearing.x * scale;
    f32 pos_y = position.y - (info.size.y - info.bearing.y) * scale;
    v2f glyph_size = v2f_scale(V2fi(info.size), scale);
    
    // NOTE(erb): push rect
    {
      v2f dest_p0 = V2f(pos_x, pos_y);
      v2f dest_p1 = v2f_add(dest_p0, glyph_size);
      v2f src_p0 = V2f(0, info.size.y); // NOTE(erb): flipped due to freetype storage
      v2f src_p1 = V2f(info.size.x, 0);
      append_render_rect(renderer, dest_p0, dest_p1, src_p0, src_p1, 0, 0, color, info.texture_id);
    }
    
    advance_pos.x += (info.advance >> 6) * scale;
  }
}

void debug_print_rects(Render_Rect_Buffer *rects)
{
  for (u32 idx = 0;
       idx< rects->count;
       idx++)
  {
    Render_Rect *rec = rects->buffer + idx;
    pf_log("Dst[P0(%f, %f) P1(%f, %f)] color(%f, %f, %f, %f)\n", 
           rec->dest_p0.x, rec->dest_p0.y, 
           rec->dest_p1.x, rec->dest_p1.y, 
           rec->color.r, rec->color.g, rec->color.b, rec->color.a);
  }
}

Loaded_Image generate_blank_image(Arena *arena, u32 width, u32 height)
{
  Loaded_Image blank_image = {0};
  blank_image.size = V2i(width, height);
  blank_image.channels = 4;
  blank_image.data = push_array(arena, width * height, unsigned char);
  for (u64 idx = 0;
       idx < (u64)(width * height * blank_image.channels);
       idx++)
  {
    *(blank_image.data + idx) = (1 << 8) - 1;
  }
  
  return blank_image;
}

void render_batches(Render_Data *renderer, v2f window_size)
{
  // NOTE(erb): resolution
  gl(glUseProgram(renderer->program));
  gl(glUniform2f(glGetUniformLocation(renderer->program, "u_resolution"), window_size.x, window_size.y));
  
  // NOTE(erb): batches
  for (u32 batch_idx = 0;
       batch_idx < renderer->batch_count;
       batch_idx++)
  {
    Render_Batch *batch = (renderer->batches + batch_idx);
    u32 end_rect_idx = renderer->rects.count;
    if (batch_idx < renderer->batch_count - 1)
    {
      end_rect_idx = renderer->batches[batch_idx+1].start_rect_index;
    }
    
    u32 rect_count = end_rect_idx - batch->start_rect_index;
    Render_Rect *rects_begin = renderer->rects.buffer + batch->start_rect_index;
    
    // NOTE(erb): bind buffer
    gl(glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo));
    gl(glBufferData(GL_ARRAY_BUFFER, sizeof(Render_Rect)*rect_count, rects_begin, GL_DYNAMIC_DRAW));
    
    // NOTE(erb): texture slots
    for (u32 texture_idx = 0;
         texture_idx < batch->texture_count;
         texture_idx++) 
    {
      u32 Tex = batch->textures[texture_idx];
      gl(glActiveTexture(GL_TEXTURE0 + texture_idx));
      gl(glBindTexture(GL_TEXTURE_2D, Tex));
    }
    
    u32 textures[16] = { 
      0, 1, 2, 3, 
      4, 5, 6, 7,
      8, 9, 10, 11, 
      12, 13, 14, 15,
    };
    gl(glUseProgram(renderer->program));
    gl(glUniform1iv(glGetUniformLocation(renderer->program, "u_textures"), array_size(textures), (GLint *)textures));
    
    // NOTE(erb): draw
    gl(glEnable(GL_BLEND));
    gl(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    
    gl(glBindVertexArray(renderer->vao));
    gl(glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, rect_count));
    gl(glBindVertexArray(0));
  }
}

#define DIR "/Users/enrique/dev/inhouse/glfw-test"

FT_Library freetype_library;

Font_Data *load_font(Arena *arena, String path)
{
  if (freetype_library == 0) 
  {
    if (FT_Init_FreeType(&freetype_library))
    {
      pf_log("Failed to initialize freetype!\n");
      pf_assert(false);
      return 0;
    }
  }
  FT_Face font_face;
  if (FT_New_Face(freetype_library, str_to_temp256_cstr(path), 0, &font_face))
  {
    pf_log("Failed to load font!");
    pf_assert(false);
    return 0;
  }
  
  Font_Data *font_data = push_struct(arena, Font_Data);
  
  FT_Set_Pixel_Sizes(font_face, 0, 48);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  
  for (u32 char_idx = 0;
       char_idx < array_size(font_data->ascii_glyph_table);
       char_idx++) 
  {
    if (!FT_Load_Char(font_face, char_idx, FT_LOAD_RENDER))
    {
      u32 texture_id;
      gl(glGenTextures(1, &texture_id));
      gl(glBindTexture(GL_TEXTURE_2D, texture_id));
      gl(glTexImage2D(GL_TEXTURE_2D, 
                      0, 
                      GL_R8, 
                      font_face->glyph->bitmap.width,
                      font_face->glyph->bitmap.rows,
                      0,
                      GL_RED,
                      GL_UNSIGNED_BYTE, 
                      font_face->glyph->bitmap.buffer));
      
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
      
      gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
      gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
      gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
      gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
      
      Glyph_Info info = {0};
      info.texture_id = texture_id;
      info.size = V2i(font_face->glyph->bitmap.width, font_face->glyph->bitmap.rows);
      info.bearing = V2i(font_face->glyph->bitmap_left, font_face->glyph->bitmap_top);
      info.advance = font_face->glyph->advance.x;
      
      font_data->ascii_glyph_table[char_idx] = info;
    }
    else
    {
      pf_log("Error: Freetype: Failed to load Glyph %c\n", char_idx);
    }
  }
  
  FT_Done_Face(font_face);
  
  return font_data;
}

Render_Data make_renderer(Arena *arena, String vertex_shader_path, String fragment_shader_path)
{
  Render_Data renderer = {0};
  Loaded_Image blank_image = generate_blank_image(arena, 5000, 5000);
  renderer.blank_image_texture = make_texture_from_image(&blank_image);
  renderer.rects.capacity = 1000;
  renderer.rects.buffer = push_array(arena, renderer.rects.capacity, Render_Rect);
  
  // NOTE(erb): shaders setup
  Read_File_Result vertex_shader_file = fallback_vertex_shader(mac_read_file(arena, vertex_shader_path));
  Read_File_Result fragment_shader_file = fallback_fragment_shader(mac_read_file(arena, fragment_shader_path));
  renderer.program = make_program((char *)vertex_shader_file.data, (char *)fragment_shader_file.data, false);
  
  // NOTE(erb): buffers setup
  gl(glGenVertexArrays(1, &renderer.vao));
  gl(glBindVertexArray(renderer.vao));
  
  gl(glGenBuffers(1, &renderer.vbo));
  gl(glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo));
  {
    Buffer_Attribs Attribs = {0};
    push_struct_attrib_f32(&Attribs, Render_Rect, dest_p0, v2f);
    push_struct_attrib_f32(&Attribs, Render_Rect, dest_p1, v2f);
    push_struct_attrib_f32(&Attribs, Render_Rect, src_p0, v2f);
    push_struct_attrib_f32(&Attribs, Render_Rect, src_p1, v2f);
    push_struct_attrib_f32(&Attribs, Render_Rect, color, v4f);
    push_struct_attrib_f32(&Attribs, Render_Rect, corner_radius, f32);
    push_struct_attrib_f32(&Attribs, Render_Rect, edge_softness, f32);
    push_struct_attrib_f32(&Attribs, Render_Rect, texture_slot, f32);
  }
  
  return renderer;
}

void render_frame_begin(Render_Data *renderer, v2f window_size)
{
  renderer->rects.count = 0;
  renderer->batch_count = 1;
  renderer->batches->start_rect_index = 0;
  renderer->batches->texture_count = 0;
  
  gl(glViewport(0, 0, window_size.x, window_size.y));
  gl(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));
}

int main(void)
{
  if (!init_window()) 
  {
    pf_log("Failed to initialize window!\n");
    pf_assert(false);
    return 1;
  }
  
  Arena *scratch = allocate_arena(gb(1));
  
  Render_Data renderer = make_renderer(scratch, S(DIR"/vertex_shader_rect.glsl"), S(DIR"/fragment_shader_rect.glsl"));
  Image_Texture texture = make_texture_from_image_file(DIR"/smile.png");
  
  // NOTE(erb): load font
  Font_Data *font_data = load_font(scratch, S(DIR"/calibri.ttf"));
  FT_Done_FreeType(freetype_library);
  
  f64 min_fps = MAX_f32;
  f64 max_fps = 0.0f;
  u64 tick = 0;
  GLFWwindow *window = glfwGetCurrentContext();
  while (!glfwWindowShouldClose(window))
  {
    tick++;
    float frame_begin = glfwGetTime();
    
    // NOTE(erb): window setup
    v2f window_size = Getwindow_size();
    v2f mouse_pos = get_mouse_pos();
    
    // NOTE(erb): BEGIN DRAWING
    {
      render_frame_begin(&renderer, window_size);
      
      mouse_pos.y = window_size.y - mouse_pos.y;
      v2f P2 = {0};
      P2.x = mouse_pos.x + window_size.x * 0.1f;
      P2.y = mouse_pos.y - window_size.y * 0.1f;
      
      AppendRenderText(&renderer, font_data, S("0123456789abcdfghijklmnopqrstuvwxyz"), v2f_scale(window_size, 0.2f), 1, V4f(1, 1, 1, 1));
      append_render_rect_color_rounded(&renderer, V2f(200, 200), V2f(100, 100), V4f(0, 0, 1, 1), 10.0f);
      append_render_rect_texture(&renderer, V2f(100, 100), V2f(100, 100), texture);
      
      // NOTE(erb): render
      render_batches(&renderer, window_size);
      
      glfwSwapBuffers(window);
    }
    // NOTE(erb): END DRAWING
    glfwPollEvents();
    
    if (tick % 1000 == 0) 
    {
      f64 frame_time = glfwGetTime() - frame_begin;
      f64 fps = 1.0f/frame_time;
      
      if (tick % 3 == 0) 
      {
        min_fps = min(min_fps, fps);
        max_fps = max(max_fps, fps);
      }
      
      pf_log("\rfps(%f) [%f, %f], time(%f)", fps, min_fps, max_fps, frame_time);
      fflush(stdout);
    }
  }
  
  gl(glDeleteVertexArrays(1, &renderer.vao));
  gl(glDeleteBuffers(1, &renderer.vbo));
  gl(glDeleteProgram(renderer.program));
  
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}

