// TODO(erb) glfw-test stuff

// MINIMUM NECESSARY
// - text rendering
//   - make sure text is rendering correctly meaning the position in render_text(pos) 
//     should be the top left of the bounding box of the resulting text
//   - change render_text to accept a text height in pixels
//   - add spacing
// - implement ui_measure_str_size()
// - opengl texture issues
//   - reproduce: render a bunch of text on the screen and glBindTexture() starts to error
//   - hyphothesis: might need to free textures and stuff
// - opengl vertex buffer handling
//   - figure out if you need to delete the buffer after using it
// - rects improvements
//    - rect lines
//    - gradiants
// - correct pixel coordinate system
//   - figure out app space vs screen space
//   - check how raylib does it
// - UTF8 strings 
// - UTF8 rendering
// - Custom GLFW allocator

// APP IDEAS
// - File Explorer (dir walker) OR calendar app (calendar notebook)

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
do { if (!(Condition)) { fprintf(stdout, "%s:%d: ASSERT [%s] at %s()\n", __FILE__, __LINE__, #Condition, __func__); abort(); } } while (0)

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
  Render_Batch batches[128];
  
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

v2f append_render_text(Render_Data *renderer, Font_Data *font_data, String str, v2f position, f32 scale, v4f color)
{
  v2f text_size = {0};
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
    advance_pos.x += (info.advance >> 6);
    text_size.x += glyph_size.x;
    text_size.y = max(text_size.y, glyph_size.y);
  }
  
  return text_size;
}

v2f measure_text_size_ignore_lines_and_tabs(Font_Data *font_data, String str, f32 scale)
{
  v2f result = {0};
  
  for (u32 char_idx = 0;
       char_idx < str.length;
       char_idx++)
  {
    char ch = str.value[char_idx];
    if (ch != '\n' && ch != '\t') 
    {
      Glyph_Info info = font_data->ascii_glyph_table[(u32)ch];
      
      f32 width = info.advance >> 6;
      f32 height = (info.size.y + info.bearing.y) * scale;
      
      result.x += width;
      result.y = height;
    }
  }
  
  return result;
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

typedef enum Modifier_Flags
{
  ModifierFlags_None    = 0,
  ModifierFlags_Shift   = (1 << 0),
  ModifierFlags_Control = (1 << 1),
  ModifierFlags_Alt     = (1 << 2),
  ModifierFlags_Super   = (1 << 3),
  ModifierFlags_Caps    = (1 << 4),
  ModifierFlags_NumLock = (1 << 5),
} Modifier_Flags;

typedef enum Mouse_Code
{
  MouseCode_None,
  MouseCode_Left,
  MouseCode_Right,
  MouseCode_Middle,
  MouseCode_COUNT,
} Mouse_Code;

char *mouse_code_cstrings[] =
{
  [MouseCode_Left] = "Mouse_Code_Left",
  [MouseCode_Right] = "Mouse_Code_Right",
  [MouseCode_Middle] = "Mouse_Code_Middle",
};

typedef enum Key_Code
{
  KeyCode_None = 0,
  KeyCode_Space           = 32,
  KeyCode_Apostrophe      = 39,
  KeyCode_Comma           = 44,
  KeyCode_Minus           = 45,
  KeyCode_Period          = 46,
  KeyCode_Slash           = 47,
  KeyCode_Zero            = 48,
  KeyCode_One             = 49,
  KeyCode_Two             = 50,
  KeyCode_Three           = 51,
  KeyCode_Four            = 52,
  KeyCode_Five            = 53,
  KeyCode_Six             = 54,
  KeyCode_Seven           = 55,
  KeyCode_Eight           = 56,
  KeyCode_Nine            = 57,
  KeyCode_Semicolon       = 59,
  KeyCode_Equal           = 61,
  KeyCode_A               = 65,
  KeyCode_B               = 66,
  KeyCode_C               = 67,
  KeyCode_D               = 68,
  KeyCode_E               = 69,
  KeyCode_F               = 70,
  KeyCode_G               = 71,
  KeyCode_H               = 72,
  KeyCode_I               = 73,
  KeyCode_J               = 74,
  KeyCode_K               = 75,
  KeyCode_L               = 76,
  KeyCode_M               = 77,
  KeyCode_N               = 78,
  KeyCode_O               = 79,
  KeyCode_P               = 80,
  KeyCode_Q               = 81,
  KeyCode_R               = 82,
  KeyCode_S               = 83,
  KeyCode_T               = 84,
  KeyCode_U               = 85,
  KeyCode_V               = 86,
  KeyCode_W               = 87,
  KeyCode_X               = 88,
  KeyCode_Y               = 89,
  KeyCode_Z               = 90,
  KeyCode_LeftBracket     = 91,
  KeyCode_Backslash       = 92,
  KeyCode_RightBracket    = 93,
  KeyCode_Backtick        = 96,
  KeyCode_Escape          = 256,
  KeyCode_Enter           = 257,
  KeyCode_Tab             = 258,
  KeyCode_Backspace       = 259,
  KeyCode_Insert          = 260,
  KeyCode_Delete          = 261,
  KeyCode_Right           = 262,
  KeyCode_Left            = 263,
  KeyCode_Down            = 264,
  KeyCode_Up              = 265,
  KeyCode_PageUp          = 266,
  KeyCode_PageDown        = 267,
  KeyCode_Home            = 268,
  KeyCode_End             = 269,
  KeyCode_CapsLock        = 280,
  KeyCode_ScrollLock      = 281,
  KeyCode_NumLock         = 282,
  KeyCode_PrintScreen     = 283,
  KeyCode_Pause           = 284,
  KeyCode_F1              = 290,
  KeyCode_F2              = 291,
  KeyCode_F3              = 292,
  KeyCode_F4              = 293,
  KeyCode_F5              = 294,
  KeyCode_F6              = 295,
  KeyCode_F7              = 296,
  KeyCode_F8              = 297,
  KeyCode_F9              = 298,
  KeyCode_F10             = 299,
  KeyCode_F11             = 300,
  KeyCode_F12             = 301,
  KeyCode_LeftShift       = 340,
  KeyCode_LeftControl     = 341,
  KeyCode_LeftAlt         = 342,
  KeyCode_LeftSuper       = 343,
  KeyCode_RightShift      = 344,
  KeyCode_RightControl    = 345,
  KeyCode_RightAlt        = 346,
  KeyCode_RightSuper      = 347,
  KeyCode_COUNT,
} Key_Code;

char *key_code_cstrings[] = 
{
  [KeyCode_None] = "KeyCode_None",
  [KeyCode_Space] = "KeyCode_Space",
  [KeyCode_Apostrophe] = "KeyCode_Apostrophe",
  [KeyCode_Comma] = "KeyCode_Comma",
  [KeyCode_Minus] = "KeyCode_Minus",
  [KeyCode_Period] = "KeyCode_Period",
  [KeyCode_Slash] = "KeyCode_Slash",
  [KeyCode_Zero] = "KeyCode_Zero",
  [KeyCode_One] = "KeyCode_One",
  [KeyCode_Two] = "KeyCode_Two",
  [KeyCode_Three] = "KeyCode_Three",
  [KeyCode_Four] = "KeyCode_Four",
  [KeyCode_Five] = "KeyCode_Five",
  [KeyCode_Six] = "KeyCode_Six",
  [KeyCode_Seven] = "KeyCode_Seven",
  [KeyCode_Eight] = "KeyCode_Eight",
  [KeyCode_Nine] = "KeyCode_Nine",
  [KeyCode_Semicolon] = "KeyCode_Semicolon",
  [KeyCode_Equal] = "KeyCode_Equal",
  [KeyCode_A] = "KeyCode_A",
  [KeyCode_B] = "KeyCode_B",
  [KeyCode_C] = "KeyCode_C",
  [KeyCode_D] = "KeyCode_D",
  [KeyCode_E] = "KeyCode_E",
  [KeyCode_F] = "KeyCode_F",
  [KeyCode_G] = "KeyCode_G",
  [KeyCode_H] = "KeyCode_H",
  [KeyCode_I] = "KeyCode_I",
  [KeyCode_J] = "KeyCode_J",
  [KeyCode_K] = "KeyCode_K",
  [KeyCode_L] = "KeyCode_L",
  [KeyCode_M] = "KeyCode_M",
  [KeyCode_N] = "KeyCode_N",
  [KeyCode_O] = "KeyCode_O",
  [KeyCode_P] = "KeyCode_P",
  [KeyCode_Q] = "KeyCode_Q",
  [KeyCode_R] = "KeyCode_R",
  [KeyCode_S] = "KeyCode_S",
  [KeyCode_T] = "KeyCode_T",
  [KeyCode_U] = "KeyCode_U",
  [KeyCode_V] = "KeyCode_V",
  [KeyCode_W] = "KeyCode_W",
  [KeyCode_X] = "KeyCode_X",
  [KeyCode_Y] = "KeyCode_Y",
  [KeyCode_Z] = "KeyCode_Z",
  [KeyCode_LeftBracket] = "KeyCode_LeftBracket",
  [KeyCode_Backslash] = "KeyCode_Backslash",
  [KeyCode_RightBracket] = "KeyCode_RightBracket",
  [KeyCode_Backtick] = "KeyCode_Backtick",
  [KeyCode_Escape] = "KeyCode_Escape",
  [KeyCode_Enter] = "KeyCode_Enter",
  [KeyCode_Tab] = "KeyCode_Tab",
  [KeyCode_Backspace] = "KeyCode_Backspace",
  [KeyCode_Insert] = "KeyCode_Insert",
  [KeyCode_Delete] = "KeyCode_Delete",
  [KeyCode_Right] = "KeyCode_Right",
  [KeyCode_Left] = "KeyCode_Left",
  [KeyCode_Down] = "KeyCode_Down",
  [KeyCode_Up] = "KeyCode_Up",
  [KeyCode_PageUp] = "KeyCode_PageUp",
  [KeyCode_PageDown] = "KeyCode_PageDown",
  [KeyCode_Home] = "KeyCode_Home",
  [KeyCode_End] = "KeyCode_End",
  [KeyCode_CapsLock] = "KeyCode_CapsLock",
  [KeyCode_ScrollLock] = "KeyCode_ScrollLock",
  [KeyCode_NumLock] = "KeyCode_NumLock",
  [KeyCode_PrintScreen] = "KeyCode_PrintScreen",
  [KeyCode_Pause] = "KeyCode_Pause",
  [KeyCode_F1] = "KeyCode_F1",
  [KeyCode_F2] = "KeyCode_F2",
  [KeyCode_F3] = "KeyCode_F3",
  [KeyCode_F4] = "KeyCode_F4",
  [KeyCode_F5] = "KeyCode_F5",
  [KeyCode_F6] = "KeyCode_F6",
  [KeyCode_F7] = "KeyCode_F7",
  [KeyCode_F8] = "KeyCode_F8",
  [KeyCode_F9] = "KeyCode_F9",
  [KeyCode_F10] = "KeyCode_F10",
  [KeyCode_F11] = "KeyCode_F11",
  [KeyCode_F12] = "KeyCode_F12",
  [KeyCode_LeftShift] = "KeyCode_LeftShift",
  [KeyCode_LeftControl] = "KeyCode_LeftControl",
  [KeyCode_LeftAlt] = "KeyCode_LeftAlt",
  [KeyCode_LeftSuper] = "KeyCode_LeftSuper",
  [KeyCode_RightShift] = "KeyCode_RightShift",
  [KeyCode_RightControl] = "KeyCode_RightControl",
  [KeyCode_RightAlt] = "KeyCode_RightAlt",
  [KeyCode_RightSuper] = "KeyCode_RightSuper",
};

typedef enum Input_Event_Kind
{
  InputEventKind_None,
  InputEventKind_Core,
  InputEventKind_Text,
  InputEventKind_KeyDown,
  InputEventKind_KeyUp,
  InputEventKind_MouseDown,
  InputEventKind_MouseUp,
  InputEventKind_MouseWheel,
  InputEventKind_CursorMove,
} Input_Event_Kind;

typedef struct Input_Event
{
  Input_Event_Kind kind;
  struct Input_Event *next;
  
  union 
  {
    struct
    {
      b32 should_close;
    } core;
    
    struct
    {
      u32 code_point;
      struct Input_Event *next;
    } text;
    
    struct 
    {
      Key_Code code;
      Modifier_Flags modifiers;
    } key;
    
    struct
    {
      Mouse_Code code;
      Modifier_Flags modifiers;
    } mouse_button;
    
    struct
    {
      v2f delta;
    } mouse_wheel;
    
    struct
    {
      v2f position;
    } cursor_move;
  };
} Input_Event;

typedef struct Input_Event_List
{
  Input_Event *first;
  Input_Event *last;
} Input_Event_List;

typedef struct Platform_Window_Handle 
{
  GLFWwindow *glfw_window;
} Platform_Window_Handle;

typedef struct Window
{
  Arena *event_arena;
  Input_Event_List event_list;
  
  v2f accumulated_scroll_offset;
  
  Platform_Window_Handle handle;
} Window;

Modifier_Flags modifiers_from_glfw_mods(i32 mods)
{
  Modifier_Flags result = 0;
  
  if (mods & GLFW_MOD_SHIFT)
  {
    result |= ModifierFlags_Shift;
  }
  if (mods & GLFW_MOD_CONTROL)
  {
    result |= ModifierFlags_Control;
  }
  if (mods & GLFW_MOD_ALT)
  {
    result |= ModifierFlags_Alt;
  }
  if (mods & GLFW_MOD_SUPER)
  {
    result |= ModifierFlags_Super;
  }
  if (mods & GLFW_MOD_CAPS_LOCK)
  {
    result |= ModifierFlags_Caps;
  }
  if (mods & GLFW_MOD_NUM_LOCK)
  {
    result |= ModifierFlags_NumLock;
  }
  
  return result;
};

Key_Code key_code_from_glfw(i32 key)
{
  Key_Code result = KeyCode_None;
  
  if (0 < key && key < KeyCode_COUNT) 
  {
    result = (Key_Code)key;
  }
  
  return result;
}

Mouse_Code mouse_code_from_glfw(i32 button) 
{
  Mouse_Code code = MouseCode_None;
  switch (button)
  {
    case GLFW_MOUSE_BUTTON_LEFT: 
    {
      code = MouseCode_Left;
    } break;
    
    case GLFW_MOUSE_BUTTON_RIGHT: 
    {
      code = MouseCode_Right;
    } break;
    
    case GLFW_MOUSE_BUTTON_MIDDLE: 
    {
      code = MouseCode_Middle;
    } break;
  }
  
  return code;
}

void key_callback(GLFWwindow *glfw_window, int key, int scancode, int action, int mods) 
{
  // NOTE(erb): scancode is ignored for now
  
  Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
  pf_assert(window);
  
  Key_Code code = key_code_from_glfw(key);
  
  if (code != KeyCode_None)
  {
    Input_Event *event = sll_push_allocate(window->event_arena, &window->event_list, Input_Event);
    if (action == GLFW_PRESS) 
    {
      event->kind = InputEventKind_KeyDown;
    }
    else if (action == GLFW_RELEASE) 
    {
      event->kind = InputEventKind_KeyUp;
    }
    event->key.code = code;
    event->key.modifiers = modifiers_from_glfw_mods(mods);
  }
}

void character_callback(GLFWwindow* glfw_window, unsigned int codepoint)
{
  Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
  pf_assert(window);
  
  Input_Event *event = sll_push_allocate(window->event_arena, &window->event_list, Input_Event);
  event->kind = InputEventKind_Text;
  event->text.code_point = codepoint;
  
  // TODO(erb): build input state
}

void mouse_button_callback(GLFWwindow* glfw_window, int button, int action, int mods)
{
  Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
  pf_assert(window);
  
  Mouse_Code code = mouse_code_from_glfw(button);
  
  if (code != MouseCode_None)
  {
    Input_Event *event = sll_push_allocate(window->event_arena, &window->event_list, Input_Event);
    if (action == GLFW_PRESS) 
    {
      event->kind = InputEventKind_MouseDown;
    }
    else if (action == GLFW_RELEASE) 
    {
      event->kind = InputEventKind_MouseUp;
    }
    event->mouse_button.code = code;
    event->mouse_button.modifiers = modifiers_from_glfw_mods(mods);
    
  }
}

void mouse_scroll_callback(GLFWwindow* glfw_window, double xoffset, double yoffset)
{
  Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
  pf_assert(window);
  
  Input_Event *event = sll_push_allocate(window->event_arena, &window->event_list, Input_Event);
  event->mouse_wheel.delta = V2f((f32)xoffset, (f32)yoffset);
  
  // NOTE(erb): build input state
  window->accumulated_scroll_offset = v2f_add(window->accumulated_scroll_offset, event->mouse_wheel.delta);
};


void cursor_position_callback(GLFWwindow* glfw_window, double xpos, double ypos)
{
  Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
  pf_assert(window);
  
  Input_Event *event = sll_push_allocate(window->event_arena, &window->event_list, Input_Event);
  event->cursor_move.position = V2f((f32)xpos, (f32)ypos);
};


void window_end_frame(Window *window)
{
  glfwSwapBuffers(window->handle.glfw_window);
  
  window->event_list.first = 0;
  window->event_list.last = 0;
  
  release_arena(window->event_arena);
}

Input_Event_List os_poll_events(Window *window)
{
  GLFWwindow *glfw_window = window->handle.glfw_window;
  
  // NOTE(erb): write to current
  if (glfwWindowShouldClose(glfw_window)) 
  {
    Input_Event *event = sll_push_allocate(window->event_arena, &window->event_list, Input_Event);
    event->kind = InputEventKind_Core;
    event->core.should_close = true;
  }
  
  glfwMakeContextCurrent(glfw_window);
  glfwPollEvents();
  
  return window->event_list;
}

char *debug_key_code_cstr(Key_Code code)
{
  char *result = "unknown";
  if (0 <= code && code < KeyCode_COUNT) 
  {
    result = key_code_cstrings[(i32)code];
  }
  return result;
}

char *debug_mouse_code_cstr(Mouse_Code code)
{
  char *result = "unknown";
  if (0 <= code && code < MouseCode_COUNT) 
  {
    result = mouse_code_cstrings[(i32)code];
  }
  return result;
}

String debug_modifiers_str(Arena *arena, Modifier_Flags flags)
{
  String result = {0};
  
  if (flags == ModifierFlags_None) 
  {
    result = S("None");
  }
  else
  {
    String_Builder builder = {0};
    
    if (flags & ModifierFlags_Shift)
    {
      str_build_str(arena, &builder, S("|Shift"));
    }
    if (flags & ModifierFlags_Control)
    {
      str_build_str(arena, &builder, S("Control"));
    }
    if (flags & ModifierFlags_Alt)
    {
      str_build_str(arena, &builder, S("|Alt"));
    }
    if (flags & ModifierFlags_Super)
    {
      str_build_str(arena, &builder, S("|Super"));
    }
    if (flags & ModifierFlags_Caps)
    {
      str_build_str(arena, &builder, S("|Caps"));
    }
    if (flags & ModifierFlags_NumLock)
    {
      str_build_str(arena, &builder, S("|NumLock"));
    }
    
    result = builder.buffer;
  }
  
  return result;
}

char *debug_input_event_str(Arena *arena, Input_Event* event)
{
  pf_assert(event);
  
  static char buffer[256];
  
  switch (event->kind)
  {
    default:
    case InputEventKind_None:
    {
      snprintf(buffer, sizeof(buffer), "None");
    } break;
    
    case InputEventKind_Text:
    {
      snprintf(buffer, sizeof(buffer), "Text[codepoint=%d]", event->text.code_point);
    } break;
    
    case InputEventKind_KeyDown:
    case InputEventKind_KeyUp:
    {
      snprintf(buffer, sizeof(buffer), "Key_%s[code=%s, modifiers=%s]", 
               event->kind == InputEventKind_KeyUp ? "Up" : "Down", 
               debug_key_code_cstr(event->key.code),
               str_to_temp256_cstr(debug_modifiers_str(arena, event->key.modifiers)));
    } break;
    
    case InputEventKind_MouseUp:
    case InputEventKind_MouseDown:
    {
      snprintf(buffer, sizeof(buffer), "Mouse_%s[code=%s, modifiers=%s]", 
               event->kind == InputEventKind_MouseUp ? "Up" : "Down", 
               debug_mouse_code_cstr(event->mouse_button.code),
               str_to_temp256_cstr(debug_modifiers_str(arena, event->mouse_button.modifiers)));
    } break;
    
    case InputEventKind_CursorMove:
    {
      v2f position = event->cursor_move.position;
      snprintf(buffer, sizeof(buffer), "MouseMove[position=(%f, %f)]", position.x, position.y);
    } break;
    
    case InputEventKind_MouseWheel:
    {
      v2f delta = event->mouse_wheel.delta;
      snprintf(buffer, sizeof(buffer), "MouseWheel[delta=(%f, %f)]", delta.x, delta.y);
    } break;
  }
  
  return buffer;
}

Window *make_glfw_window(Arena *arena, String title, v2i size)
{
  Window *window = 0;
  
  if (glfwInit()) 
  {
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE); // NOTE(erb): apple forward compatibilty
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* glfw_window = glfwCreateWindow(size.x, size.y, str_to_temp256_cstr(title), 0, 0);
    if (glfw_window) 
    {
      window = push_struct(arena, Window);
      window->handle.glfw_window = glfw_window;
      window->event_arena = push_sub_arena(arena, kb(128));
      
      glfwMakeContextCurrent(glfw_window);
      
      glfwSwapInterval(0); // NOTE(erb): no vsync
      
      // NOTE(erb): user pointer
      // NOTE(erb): this might be weird/dangerous to do, setting the user pointer to the own parent structure
      glfwSetWindowUserPointer(glfw_window, window);
      
      // NOTE(erb): callbacks
      glfwSetKeyCallback(glfw_window, key_callback);
      glfwSetCharCallback(glfw_window, character_callback);
      glfwSetMouseButtonCallback(glfw_window, mouse_button_callback);
      
      // NOTE(erb): opengl info
      printf("LOG: OpenGL version: %s\n", glGetString(GL_VERSION));
      printf("LOG: OpenGL Vendor: %s\n", glGetString(GL_VENDOR));
      printf("LOG: OpenGL Renderer: %s\n", glGetString(GL_RENDERER));
      printf("LOG: OpenGL Version: %s\n", glGetString(GL_VERSION));
      printf("LOG: OpenGL GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
      i32 max_vertex_attribs;
      gl(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attribs));
      printf("LOG: OpenGL max vertex_shader Attributes Allowed: %d\n", max_vertex_attribs);
      
      // NOTE(erb): opengl setup
      gl(glEnable(GL_BLEND));
      gl(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    }
  }
  
  return window;
}

void close_glfw_window(Window *window)
{
  glfwDestroyWindow(window->handle.glfw_window);
}

v2f get_window_size(Window *window)
{
  i32 width, height;
  glfwGetFramebufferSize(window->handle.glfw_window, &width, &height);
  v2f result = V2f((f32)width, (f32)height);
  return result;
}

v2f get_cursor_position(Window *window) 
{
  GLFWwindow *glfw_window = window->handle.glfw_window;
  
  f32 scale_x, scale_y;
  glfwGetWindowContentScale(glfw_window, &scale_x, &scale_y);
  
  f64 x, y;
  glfwGetCursorPos(glfw_window, &x, &y);
  v2f result = V2f(x * scale_x, y * scale_y);
  return result;
}

#define color(R, G, B, A) (v4f){ .r = R, .g = G, .b = B, .a = A }

v4f color_white = color(1, 1, 1, 1);
v4f color_red = color(1, 0, 0, 1);
v4f color_green = color(0, 1, 0, 1);
v4f color_blue = color(0, 0, 1, 1);
v4f color_debug = color(1, 0, 1, 1);

#undef color

int main(void)
{
  Arena *scratch = allocate_arena(gb(1));
  Arena *frame_arena = allocate_arena(mb(128));
  
  Window *window = make_glfw_window(scratch, S("Hello World"), V2i(600, 900));
  pf_assert(window != 0);
  
  Render_Data renderer = make_renderer(scratch, S(DIR"/vertex_shader_rect.glsl"), S(DIR"/fragment_shader_rect.glsl"));
  Image_Texture texture = make_texture_from_image_file(DIR"/smile.png");
  
  // NOTE(erb): load font
  Font_Data *font_data = load_font(scratch, S(DIR"/calibri.ttf"));
  FT_Done_FreeType(freetype_library);
  
  f64 min_fps = MAX_f32;
  f64 max_fps = 0.0f;
  u64 tick = 0;
  
  b32 running = true;
  
  while (running)
  {
    tick++;
    float frame_begin = glfwGetTime();
    
    v2f window_size = get_window_size(window);
    render_frame_begin(&renderer, window_size);
    
    Input_Event_List events = os_poll_events(window);
    
    for (Input_Event *event = events.first; 
         event;
         event = event->next)
    {
      if (event->kind == InputEventKind_Core &&
          event->core.should_close) 
      {
        running = false;
      }
      else if (event->kind == InputEventKind_KeyDown &&
               event->key.code == KeyCode_Escape)
      {
        running = false;
      }
    }
    
    // NOTE(erb): BEGIN DRAWING
    {
      v2f mouse_pos = get_cursor_position(window);
      
      mouse_pos.y = window_size.y - mouse_pos.y;
      v2f p2 = {0};
      p2.x = mouse_pos.x + window_size.x * 0.1f;
      p2.y = mouse_pos.y - window_size.y * 0.1f;
      
      {
        v2f position = v2f_scale(window_size, 0.2f);
        f32 scale = 1;
        String str = S("0123456789abcdfghijklmnopqrstuvwxyz");
        v2f size = measure_text_size_ignore_lines_and_tabs(font_data, str, scale);
        
        append_render_rect_color(&renderer, position, size, color_red);
        append_render_text(&renderer, font_data, str, position, scale, color_white);
      }
      append_render_rect_color_rounded(&renderer, V2f(200, 200), V2f(100, 100), V4f(0, 0, 1, 1), 10.0f);
      append_render_rect_texture(&renderer, V2f(100, 100), V2f(100, 100), texture);
      
      // NOTE(erb): render
      
    }
    // NOTE(erb): END DRAWING
    
    render_batches(&renderer, window_size);
    window_end_frame(window);
    
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
  
  close_glfw_window(window);
  glfwTerminate();
  return 0;
}
