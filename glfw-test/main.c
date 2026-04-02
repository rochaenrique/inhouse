#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <limits.h>
#include <copyfile.h>
#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/errno.h>

#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "core.c"
#include "platform.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#define debug_break() __builtin_debugtrap()

typedef struct OSX_App_Library
{
  void *handle;
  Update_And_Render_Func *update_and_render;
  i64 load_time;
} OSX_App_Library;

typedef struct OSX_Read_File_Result
{
  void *data;
  u64 size;
  i64 load_time;
} OSX_Read_File_Result;

PLATFORM_ALLOCATE(osx_allocate)
{
  void *result = 0;
  i32 protection = PROT_READ|PROT_WRITE;
  i32 flags = MAP_PRIVATE|MAP_ANON;
  i32 fd = -1;
  i32 offset = 0;
  
#if INTERNAL
  // NOTE(erb): start_address and MAP_FIXED flag fix the address for the allocation (for debuging)
  void *start_address = (void *)0x1;
  result = mmap(start_address, size, protection, flags|MAP_FIXED, fd, offset);
#else 
  result = mmap(0, size, protection, flags, fd, offset);
#endif
  
  pf_assert(result);
  return result;
}

PLATFORM_FREE(osx_free)
{
  i32 result = munmap(memory, size);
  pf_assert(result != 0);
}

Date osx_tm_to_date(struct tm *time)
{
	Date result = {0};
	result.seconds  = time->tm_sec;
	result.minutes  = time->tm_min;
	result.hours    = time->tm_hour;
	result.month_day = time->tm_mday;
	result.month    = time->tm_mon;
	result.year     = time->tm_year + 1900;
	result.week_day  = time->tm_wday;
	result.year_day  = time->tm_yday;
	return result;
}

PLATFORM_GET_TODAY(osx_get_today)
{
	struct timeval today;
	pf_assert(gettimeofday(&today, 0) == 0);
	struct tm *time = localtime(&today.tv_sec);
	
	Date result = osx_tm_to_date(time);
	return result;
}

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

#define GL_STACK_OVERFLOW 0x0503
#define GL_STACK_UNDERFLOW 0x0504
#define GL_CONTEXT_LOST 0x0507
#define GL_TABLE_TOO_LARGE1 0x8031

char *debug_gl_error_code_cstr(i32 code)
{
  switch (code)
  {
    case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
    case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
    case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
    case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_CONTEXT_LOST: return "GL_CONTEXT_LOST";
    case GL_TABLE_TOO_LARGE1: return "GL_TABLE_TOO_LARGE1";
    default: case GL_NO_ERROR: return "GL_NO_ERROR";
  }
}

// NOTE(erb): from https://wikis.khronos.org/opengl/OpenGL_Error
char *debug_gl_error_code_description(i32 code)
{
  switch (code)
  {
    case GL_INVALID_ENUM: return "Given when an enumeration parameter is not a legal enumeration for that function. This is given only for local problems; if the spec allows the enumeration in certain circumstances, where other parameters or state dictate those circumstances, then GL_INVALID_OPERATION is the result instead.";
    case GL_INVALID_VALUE: return "Given when a value parameter is not a legal value for that function. This is only given for local problems; if the spec allows the value in certain circumstances, where other parameters or state dictate those circumstances, then GL_INVALID_OPERATION is the result instead.";
    case GL_INVALID_OPERATION: return "Given when the set of state for a command is not legal for the parameters given to that command. It is also given for commands where combinations of parameters define what the legal parameters are.";
    case GL_STACK_OVERFLOW: return "Given when a stack pushing operation cannot be done because it would overflow the limit of that stack's size.";
    case GL_STACK_UNDERFLOW: return "Given when a stack popping operation cannot be done because the stack is already at its lowest point.";
    case GL_OUT_OF_MEMORY: return "Given when performing an operation that can allocate memory, and the memory cannot be allocated. The results of OpenGL functions that return this error are undefined; it is allowable for partial execution of an operation to happen in this circumstance.";
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "Given when doing anything that would attempt to read from or write/render to a framebuffer that is not complete.";
    case GL_CONTEXT_LOST: return "Given if the OpenGL context has been lost, due to a graphics card reset.";
    case GL_TABLE_TOO_LARGE1: return "Part of the ARB_imaging extension.";
    default: case GL_NO_ERROR: return "No error";
  }
}

#define gl(call) \
do \
{ \
gl_clear_error(); \
call; \
i32 error = gl_poll_error(); \
if (error) \
{ \
pf_log("%s:%d: [GL CALL ERROR] (0x%x) : \"%s\" : %s : %s\n", __FILE__, __LINE__, error, #call, \
debug_gl_error_code_cstr(error), debug_gl_error_code_description(error)); \
} \
} \
while (0)

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

OSX_Read_File_Result osx_read_file(Arena *arena, String file_path)
{
  OSX_Read_File_Result result = {};
  i32 file_descriptor = -1;
  
  Scratch scratch = scratch_begin(arena);
  {
    char *cstr_path = push_cstr(scratch.arena, file_path);
    file_descriptor = open(cstr_path, O_RDONLY);
  }
  scratch_end(&scratch);
  
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
      result.load_time = file_stat.st_atimespec.tv_sec;
    }
    else
    {
      pf_log("Failed to get file stat (%.*s): %s\n", str_expand(file_path), strerror(errno));
    }
    
    close(file_descriptor);
  }
  else
  {
    pf_log("Failed to open file (%.*s): %s\n", str_expand(file_path), strerror(errno));
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

OSX_Read_File_Result fallback_vertex_shader(OSX_Read_File_Result file)
{
  if (!file.data)
  {
    file.data = (u8 *)fallback_vertex_shader_source;
    file.size = S(fallback_vertex_shader_source).length;
  }
  return file;
}

OSX_Read_File_Result fallback_fragment_shader(OSX_Read_File_Result file)
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
  texture.size = v2fi(image->size);
  
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

Loaded_Image generate_blank_image(Arena *arena, u32 width, u32 height)
{
  Loaded_Image blank_image = {0};
  blank_image.size = v2i(width, height);
  blank_image.channels = 4;
  u64 size = blank_image.channels * width * height;
  blank_image.data = push_array(arena, size, unsigned char);
  for (u64 idx = 0;
       idx < size;
       idx++)
  {
    *(blank_image.data + idx) = (1 << 8) - 1;
  }
  
  return blank_image;
}

PLATFORM_RENDERER_FRAME_END(osx_opengl_render_frame_end)
{
  // NOTE(erb): resolution
  gl(glUseProgram(renderer->program));
  gl(glUniform2f(glGetUniformLocation(renderer->program, "u_resolution"), resolution.x, resolution.y));
  
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
    gl(glBufferData(GL_ARRAY_BUFFER, sizeof(Render_Rect)*rect_count, rects_begin, GL_STREAM_DRAW));
    
    // NOTE(erb): texture slots
    for (u32 texture_idx = 0;
         texture_idx < batch->texture_count;
         texture_idx++) 
    {
      u32 tex = batch->textures[texture_idx];
      gl(glActiveTexture(GL_TEXTURE0 + texture_idx));
      gl(glBindTexture(GL_TEXTURE_2D, tex));
    }
    
    u32 textures[16] = { 
      0, 1, 2, 3, 
      4, 5, 6, 7,
      8, 9, 10, 11, 
      12, 13, 14, 15,
    };
    gl(glUseProgram(renderer->program));
    gl(glUniform1iv(glGetUniformLocation(renderer->program, "u_textures"), batch->texture_count, (GLint *)textures));
    
    // NOTE(erb): 
    gl(glEnable(GL_BLEND));
    gl(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    
    // NOTE(erb): draw
    {
      gl(glBindVertexArray(renderer->vao));
      gl(glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, rect_count));
    }
    
    // NOTE(erb): unbind stuff
    {
      gl(glBindVertexArray(0));
      gl(glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo));
      gl(glBufferData(GL_ARRAY_BUFFER, sizeof(Render_Rect)*rect_count, 0, GL_STREAM_DRAW));
    }
  }
}

#define DIR "/Users/enrique/dev/inhouse/glfw-test"

FT_Library freetype_library;

PLATFORM_LOAD_FONT(osx_freetype_load_font)
{
  if (freetype_library == 0) 
  {
    if (FT_Init_FreeType(&freetype_library))
    {
      pf_log("Failed to initialize freetype!\n");
      pf_assert(false);
    }
  }
  
  FT_Face font_face;
  
  Scratch scratch = scratch_begin(arena);
  {
    char *cstr_path = push_cstr(scratch.arena, path);
    if (FT_New_Face(freetype_library, cstr_path, 0, &font_face))
    {
      pf_log("Failed to load font (%s)!\n", cstr_path);
      pf_assert(false);
    }
  }
  scratch_end(&scratch);
  
  FT_Set_Pixel_Sizes(font_face, 0, 48);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  
  Font_Data *font_data = push_struct(arena, Font_Data);
  font_data->height = (font_face->height >> 6);
  font_data->below_base_line_height = (font_face->descender >> 6);
  
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
      info.size = v2i(font_face->glyph->bitmap.width, font_face->glyph->bitmap.rows);
      info.bearing =  v2i(font_face->glyph->bitmap_left, font_face->glyph->bitmap_top);
      info.advance_x = (font_face->glyph->advance.x >> 6);
      
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

u32 make_shaderes_program(Arena *arena, String vertex_shader_path, String fragment_shader_path, b32 should_log, i64 *load_time)
{
  Scratch scratch = scratch_begin(arena);
  // NOTE(erb): shaders setup
#if 0
  OSX_Read_File_Result vertex_shader_file = fallback_vertex_shader(osx_read_file(scratch.arena, vertex_shader_path));
  OSX_Read_File_Result fragment_shader_file = fallback_fragment_shader(osx_read_file(scratch.arena, fragment_shader_path));
#else 
  OSX_Read_File_Result vertex_shader_file = osx_read_file(scratch.arena, vertex_shader_path);
  OSX_Read_File_Result fragment_shader_file = osx_read_file(scratch.arena, fragment_shader_path);
#endif
  
  u32 result = 0;
  
  if (vertex_shader_file.data != 0 && 
      fragment_shader_file.data != 0)
  {
    result = make_program((char *)vertex_shader_file.data, (char *)fragment_shader_file.data, should_log);
    *load_time = max(vertex_shader_file.load_time, fragment_shader_file.load_time);
  }
  
  scratch_end(&scratch);
  return result;
}


PLATFORM_MAKE_RENDERER(osx_opengl_make_renderer)
{
  Render_Data renderer = {0};
  Loaded_Image blank_image = generate_blank_image(arena, 800, 800);
  renderer.blank_image_texture = make_texture_from_image(&blank_image);
  renderer.rects.capacity = 100000;
  renderer.rects.buffer = push_array(arena, renderer.rects.capacity, Render_Rect);
  
  // NOTE(erb): shaders setup
  renderer.program = make_shaderes_program(arena, vertex_shader_path, fragment_shader_path, false, &renderer.shaders_load_time);
  
  // NOTE(erb): buffers setup
  gl(glGenVertexArrays(1, &renderer.vao));
  gl(glBindVertexArray(renderer.vao));
  
  gl(glGenBuffers(1, &renderer.vbo));
  gl(glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo));
  {
    Buffer_Attribs Attribs = {0};
    push_struct_attrib_f32(&Attribs, Render_Rect, dest_p0, V2f);
    push_struct_attrib_f32(&Attribs, Render_Rect, dest_p1, V2f);
    push_struct_attrib_f32(&Attribs, Render_Rect, src_p0, V2f);
    push_struct_attrib_f32(&Attribs, Render_Rect, src_p1, V2f);
    push_struct_attrib_f32(&Attribs, Render_Rect, color, V4f);
    push_struct_attrib_f32(&Attribs, Render_Rect, corner_radius, f32);
    push_struct_attrib_f32(&Attribs, Render_Rect, edge_softness, f32);
    push_struct_attrib_f32(&Attribs, Render_Rect, texture_slot, f32);
  }
  
  return renderer;
}

PLATFORM_RENDERER_FRAME_BEGIN(osx_opengl_render_frame_begin)
{
  renderer->rects.count = 0;
  renderer->batch_count = 1;
  renderer->batches->start_rect_index = 0;
  renderer->batches->texture_count = 0;
  
  gl(glViewport(0, 0, window_size.x, window_size.y));
  gl(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));
}

void verify_key_codes_with_glfw_keys()
{
  pf_assert(KeyCode_Space == GLFW_KEY_SPACE);
  pf_assert(KeyCode_Apostrophe == GLFW_KEY_APOSTROPHE);
  pf_assert(KeyCode_Comma == GLFW_KEY_COMMA);
  pf_assert(KeyCode_Minus == GLFW_KEY_MINUS);
  pf_assert(KeyCode_Period == GLFW_KEY_PERIOD);
  pf_assert(KeyCode_Slash == GLFW_KEY_SLASH);
  pf_assert(KeyCode_Zero == GLFW_KEY_0);
  pf_assert(KeyCode_One == GLFW_KEY_1);
  pf_assert(KeyCode_Two == GLFW_KEY_2);
  pf_assert(KeyCode_Three == GLFW_KEY_3);
  pf_assert(KeyCode_Four == GLFW_KEY_4);
  pf_assert(KeyCode_Five == GLFW_KEY_5);
  pf_assert(KeyCode_Six == GLFW_KEY_6);
  pf_assert(KeyCode_Seven == GLFW_KEY_7);
  pf_assert(KeyCode_Eight == GLFW_KEY_8);
  pf_assert(KeyCode_Nine == GLFW_KEY_9);
  pf_assert(KeyCode_Semicolon == GLFW_KEY_SEMICOLON);
  pf_assert(KeyCode_Equal == GLFW_KEY_EQUAL);
  pf_assert(KeyCode_A == GLFW_KEY_A);
  pf_assert(KeyCode_B == GLFW_KEY_B);
  pf_assert(KeyCode_C == GLFW_KEY_C);
  pf_assert(KeyCode_D == GLFW_KEY_D);
  pf_assert(KeyCode_E == GLFW_KEY_E);
  pf_assert(KeyCode_F == GLFW_KEY_F);
  pf_assert(KeyCode_G == GLFW_KEY_G);
  pf_assert(KeyCode_H == GLFW_KEY_H);
  pf_assert(KeyCode_I == GLFW_KEY_I);
  pf_assert(KeyCode_J == GLFW_KEY_J);
  pf_assert(KeyCode_K == GLFW_KEY_K);
  pf_assert(KeyCode_L == GLFW_KEY_L);
  pf_assert(KeyCode_M == GLFW_KEY_M);
  pf_assert(KeyCode_N == GLFW_KEY_N);
  pf_assert(KeyCode_O == GLFW_KEY_O);
  pf_assert(KeyCode_P == GLFW_KEY_P);
  pf_assert(KeyCode_Q == GLFW_KEY_Q);
  pf_assert(KeyCode_R == GLFW_KEY_R);
  pf_assert(KeyCode_S == GLFW_KEY_S);
  pf_assert(KeyCode_T == GLFW_KEY_T);
  pf_assert(KeyCode_U == GLFW_KEY_U);
  pf_assert(KeyCode_V == GLFW_KEY_V);
  pf_assert(KeyCode_W == GLFW_KEY_W);
  pf_assert(KeyCode_X == GLFW_KEY_X);
  pf_assert(KeyCode_Y == GLFW_KEY_Y);
  pf_assert(KeyCode_Z == GLFW_KEY_Z);
  pf_assert(KeyCode_LeftBracket == GLFW_KEY_LEFT_BRACKET);
  pf_assert(KeyCode_Backslash == GLFW_KEY_BACKSLASH);
  pf_assert(KeyCode_RightBracket == GLFW_KEY_RIGHT_BRACKET);
  pf_assert(KeyCode_Backtick == GLFW_KEY_GRAVE_ACCENT);
  pf_assert(KeyCode_Escape == GLFW_KEY_ESCAPE);
  pf_assert(KeyCode_Enter == GLFW_KEY_ENTER);
  pf_assert(KeyCode_Tab == GLFW_KEY_TAB);
  pf_assert(KeyCode_Backspace == GLFW_KEY_BACKSPACE);
  pf_assert(KeyCode_Insert == GLFW_KEY_INSERT);
  pf_assert(KeyCode_Delete == GLFW_KEY_DELETE);
  pf_assert(KeyCode_Right == GLFW_KEY_RIGHT);
  pf_assert(KeyCode_Left == GLFW_KEY_LEFT);
  pf_assert(KeyCode_Down == GLFW_KEY_DOWN);
  pf_assert(KeyCode_Up == GLFW_KEY_UP);
  pf_assert(KeyCode_PageUp == GLFW_KEY_PAGE_UP);
  pf_assert(KeyCode_PageDown == GLFW_KEY_PAGE_DOWN);
  pf_assert(KeyCode_Home == GLFW_KEY_HOME);
  pf_assert(KeyCode_End == GLFW_KEY_END);
  pf_assert(KeyCode_CapsLock == GLFW_KEY_CAPS_LOCK);
  pf_assert(KeyCode_ScrollLock == GLFW_KEY_SCROLL_LOCK);
  pf_assert(KeyCode_NumLock == GLFW_KEY_NUM_LOCK);
  pf_assert(KeyCode_PrintScreen == GLFW_KEY_PRINT_SCREEN);
  pf_assert(KeyCode_Pause == GLFW_KEY_PAUSE);
  pf_assert(KeyCode_F1 == GLFW_KEY_F1);
  pf_assert(KeyCode_F2 == GLFW_KEY_F2);
  pf_assert(KeyCode_F3 == GLFW_KEY_F3);
  pf_assert(KeyCode_F4 == GLFW_KEY_F4);
  pf_assert(KeyCode_F5 == GLFW_KEY_F5);
  pf_assert(KeyCode_F6 == GLFW_KEY_F6);
  pf_assert(KeyCode_F7 == GLFW_KEY_F7);
  pf_assert(KeyCode_F8 == GLFW_KEY_F8);
  pf_assert(KeyCode_F9 == GLFW_KEY_F9);
  pf_assert(KeyCode_F10 == GLFW_KEY_F10);
  pf_assert(KeyCode_F11 == GLFW_KEY_F11);
  pf_assert(KeyCode_F12 == GLFW_KEY_F12);
  pf_assert(KeyCode_LeftShift == GLFW_KEY_LEFT_SHIFT);
  pf_assert(KeyCode_LeftControl == GLFW_KEY_LEFT_CONTROL);
  pf_assert(KeyCode_LeftAlt == GLFW_KEY_LEFT_ALT);
  pf_assert(KeyCode_LeftSuper == GLFW_KEY_LEFT_SUPER);
  pf_assert(KeyCode_RightShift == GLFW_KEY_RIGHT_SHIFT);
  pf_assert(KeyCode_RightControl == GLFW_KEY_RIGHT_CONTROL);
  pf_assert(KeyCode_RightAlt == GLFW_KEY_RIGHT_ALT);
  pf_assert(KeyCode_RightSuper == GLFW_KEY_RIGHT_SUPER);
}

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

Key_Code glfw_key_to_key_code(i32 key)
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

GLFWwindow *get_glfw_window(Window *window)
{
  GLFWwindow *result = (GLFWwindow *)window->platform_handle;
  return result;
}

void key_callback(GLFWwindow *glfw_window, int key, int scancode, int action, int mods) 
{
  // NOTE(erb): scancode is ignored for now
  supress_unused(scancode);
  
  Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
  pf_assert(window);
  
  Key_Code code = glfw_key_to_key_code(key);
  
  if (code != KeyCode_None && (action == GLFW_PRESS || action == GLFW_RELEASE))
  {
    Input_Event *event = sll_push_allocate(&window->event_arena, &window->event_list, Input_Event);
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
  
  Input_Event *event = sll_push_allocate(&window->event_arena, &window->event_list, Input_Event);
  event->kind = InputEventKind_Text;
  event->text.code_point = codepoint;
}

void mouse_callback(GLFWwindow* glfw_window, int button, int action, int mods)
{
  Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
  pf_assert(window);
  
  Mouse_Code code = mouse_code_from_glfw(button);
  
  if (code != MouseCode_None)
  {
    Input_Event *event = sll_push_allocate(&window->event_arena, &window->event_list, Input_Event);
    if (action == GLFW_PRESS) 
    {
      event->kind = InputEventKind_MouseDown;
    }
    else if (action == GLFW_RELEASE) 
    {
      event->kind = InputEventKind_MouseUp;
    }
    event->mouse.code = code;
    event->mouse.modifiers = modifiers_from_glfw_mods(mods);
    
  }
}

void mouse_scroll_callback(GLFWwindow* glfw_window, double xoffset, double yoffset)
{
  Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
  pf_assert(window);
  
  Input_Event *event = sll_push_allocate(&window->event_arena, &window->event_list, Input_Event);
  event->kind = InputEventKind_MouseWheel;
  event->mouse_wheel.delta = v2f((f32)xoffset, (f32)yoffset);
};

void cursor_position_callback(GLFWwindow* glfw_window, double xpos, double ypos)
{
  Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
  pf_assert(window);
  
  Input_Event *event = sll_push_allocate(&window->event_arena, &window->event_list, Input_Event);
  event->kind = InputEventKind_CursorMove;
  event->cursor_move.position = v2f((f32)xpos, (f32)ypos);
};

V2f glfw_window_size(GLFWwindow *window)
{
  i32 width, height;
  glfwGetFramebufferSize(window, &width, &height);
  V2f result = v2f((f32)width, (f32)height);
  return result;
}

V2f glfw_cursor_position(GLFWwindow *window) 
{
  f32 scale_x, scale_y;
  glfwGetWindowContentScale(window, &scale_x, &scale_y);
  
  f64 x, y;
  glfwGetCursorPos(window, &x, &y);
  V2f result = v2f(x * scale_x, y * scale_y);
  return result;
}

PLATFORM_MAKE_WINDOW(osx_glfw_make_window)
{
  Window *window = 0;
  
  verify_key_codes_with_glfw_keys();
  
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
      window->event_arena = push_sub_arena(arena, kb(128));
      window->frame_input_state = push_struct(arena, Input_State);
      window->past_input_state = push_struct(arena, Input_State);
      
      window->platform_handle = (void *)glfw_window;
      
      glfwMakeContextCurrent(glfw_window);
      
      glfwSwapInterval(0); // NOTE(erb): no vsync
      
      // NOTE(erb): user pointer
      // NOTE(erb): this might be weird/dangerous to do, setting the user pointer to the own parent structure
      glfwSetWindowUserPointer(glfw_window, window);
      
      // NOTE(erb): callbacks
      glfwSetKeyCallback(glfw_window, key_callback);
      glfwSetCharCallback(glfw_window, character_callback);
      glfwSetMouseButtonCallback(glfw_window, mouse_callback);
      glfwSetCursorPosCallback(glfw_window, cursor_position_callback);
      glfwSetScrollCallback(glfw_window, mouse_scroll_callback);
      
      
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

PLATFORM_WINDOW_FRAME_BEGIN(osx_glfw_window_frame_begin)
{
  GLFWwindow *glfw_window = get_glfw_window(window);
  
  glfwMakeContextCurrent(glfw_window);
  glfwPollEvents();
  
  // NOTE(erb): write to current
  if (glfwWindowShouldClose(glfw_window)) 
  {
    Input_Event *event = sll_push_allocate(&window->event_arena, &window->event_list, Input_Event);
    event->kind = InputEventKind_Core;
    event->core.should_close = true;
    window->should_kill = true;
  }
  
  window->size = glfw_window_size(glfw_window);
  window->cursor_position = glfw_cursor_position(glfw_window);
  
  Input_State *input_state = window->frame_input_state;
  
  for (Input_Event *event = window->event_list.first;
       event;
       event = event->next) 
  {
    switch (event->kind)
    {
      default: {} break;
      
      case InputEventKind_None: pf_assert(false && "Got a InputEventKind_None");
      
      case InputEventKind_KeyDown:
      case InputEventKind_KeyUp:
      {
        input_state->keys_down[event->key.code] = (event->kind == InputEventKind_KeyDown);
      } break;
      
      case InputEventKind_MouseDown:
      case InputEventKind_MouseUp:
      {
        input_state->mouse_down[event->mouse.code] = (event->kind == InputEventKind_MouseDown);
      } break;
      
      case InputEventKind_CursorMove:
      {
        input_state->cursor_position = event->cursor_move.position;
      } break;
      
      case InputEventKind_MouseWheel:
      {
        window->frame_scroll_offset = event->mouse_wheel.delta;
      } break;
    }
  }
}

PLATFORM_WINDOW_FRAME_END(osx_glfw_window_frame_end)
{
  glfwSwapBuffers(get_glfw_window(window));
  
  mem_copy((u8 *)window->frame_input_state, (u8 *)window->past_input_state, sizeof(Input_State));
  
  window->event_list.first = 0;
  window->event_list.last = 0;
  
  arena_release(&window->event_arena);
}

void close_glfw_window(Window *window)
{
  glfwDestroyWindow(get_glfw_window(window));
}

b32 key_down(Window *window, Key_Code code)
{
  b32 result = window->frame_input_state->keys_down[code];
  return result;
}

b32 key_up(Window *window, Key_Code code)
{
  b32 result = !window->frame_input_state->keys_down[code];
  return result;
}

b32 key_pressed(Window *window, Key_Code code)
{
  b32 result = (window->past_input_state->keys_down[code] &&
                !window->frame_input_state->keys_down[code]);
  return result;
}

b32 mouse_button_down(Window *window, Mouse_Code code)
{
  b32 result = window->frame_input_state->mouse_down[code];
  return result;
}

b32 mouse_button_up(Window *window, Mouse_Code code)
{
  b32 result = !window->frame_input_state->mouse_down[code];
  return result;
}

b32 mouse_button_pressed(Window *window, Mouse_Code code)
{
  b32 result = (window->past_input_state->mouse_down[code] &&
                !window->frame_input_state->mouse_down[code]);
  return result;
}

u64 cycle_counter()
{
  u64 val;
  asm volatile("mrs %0, cntvct_el0" : "=r" (val));
  return val;
}

UPDATE_AND_RENDER(update_and_render_stub) 
{
  // NOTE(erb): empty
  supress_unused(memory);
  supress_unused(window);
  supress_unused(renderer);
  supress_unused(default_font);
  supress_unused(exec_directory);
  
  App_Update_Result result = {0};
  return result;
}

b32 osx_has_file_changed(String file_path, i64 compare_to_time)
{
  b32 result = 0;
  
  struct stat file_stat = {};
  i32 error = stat(str_to_temp512_cstr(file_path), &file_stat);
  if (error == 0)
  {
    result = file_stat.st_mtimespec.tv_sec > compare_to_time;
  }
  else
  {
    pf_log("Failed to get file stats (%.*s)\n", str_expand(file_path));
  }
  
  return result;
}

void osx_copy_file(String source_path, String dest_path)
{
  i32 source_fd = open(str_to_temp512_cstr(source_path), O_RDONLY);
  if (source_fd != -1) 
  {
    i32 dest_fd = open(str_to_temp512_cstr(dest_path), O_CREAT|O_TRUNC|O_WRONLY);
    if (dest_fd != -1)
    {
      i32 result = fcopyfile(source_fd, dest_fd, 0, COPYFILE_ALL);
      if (result < 0) 
      {
        pf_log("Failed to copy (%.*s) to (%.*s)\n", str_expand(source_path), str_expand(dest_path));
      }
      
      close(source_fd);
    }
    else
    {
      pf_log("Failed to open (%.*s)\n", str_expand(dest_path));
    }
    
    close(source_fd);
  }
  else
  {
    pf_log("Failed to open (%.*s)\n", str_expand(source_path));
  }
}

OSX_App_Library osx_load_app_library(String library_path, String temp_library_path)
{
  OSX_App_Library result = {0};
  result.update_and_render = update_and_render_stub;
  
  struct stat file_stat = {};
  i32 stat_error = stat(str_to_temp512_cstr(library_path), &file_stat);
  if (stat_error == 0)
  {
    // NOTE(erb): copy the file to a temporary path to avoid lock
    osx_copy_file(library_path, temp_library_path);
    
    void *handle = dlopen(str_to_temp512_cstr(temp_library_path), 0);
    if (handle)
    {
      result.update_and_render = (Update_And_Render_Func *)dlsym(handle, "update_and_render");
      result.handle = handle;
      result.load_time = file_stat.st_mtimespec.tv_sec;
    }
    else
    {
      pf_log("Failed to load library dll (%.*s), original: (%.*s)\n", 
             str_expand(temp_library_path), 
             str_expand(library_path));
    }
    
    if (!result.update_and_render) 
    {
      result.update_and_render = update_and_render_stub;
    }
  }
  
  return result;
}

void osx_unload_app_library(OSX_App_Library *library)
{
  i32 error = dlclose(library->handle);
  if (error)
  {
    pf_log("Failed to unload library\n");
  }
  
  library->handle = 0;
  library->update_and_render = update_and_render_stub;
}

String osx_get_executable_directory(Arena *arena)
{
  char path_buffer[PATH_MAX];
  u32 path_size = PATH_MAX;
  b32 error = _NSGetExecutablePath(path_buffer, &path_size);
  pf_assert(!error);
  
  // NOTE(erb): get the directory
  String path = {0};
  path.value = path_buffer;
  path.length = cstr_length(path.value);
  
  Index_u64 path_sep = str_index_char_backward(path, '/');
  pf_assert(path_sep.exists);
  
  String result = str_copy(arena, str_clip(path, path_sep.value));
  return result;
}

int main(void)
{
  // NOTE(erb): platform arena 
  Arena platform_permanent_arena = {0};
  Arena platform_transient_arena = {0};
  {
    u64 permanent_arena_size = mb(64);
    void *permanent_arena_memory = osx_allocate(permanent_arena_size);
    arena_init(&platform_permanent_arena, permanent_arena_memory, permanent_arena_size);
    
    u64 transient_arena_size = mb(128);
    void *transient_arena_memory = osx_allocate(transient_arena_size);
    arena_init(&platform_transient_arena, transient_arena_memory, transient_arena_size);
  }
  
  // NOTE(erb): executable directory
  String exec_directory = osx_get_executable_directory(&platform_permanent_arena);
  
  // NOTE(erb): paths setup
  String library_path = str_concat(&platform_permanent_arena, exec_directory, S("/app.dylib"));
  String temp_library_path = str_concat(&platform_permanent_arena, exec_directory, S("/temp_app.dylib"));
  String font_path = str_concat(&platform_permanent_arena, exec_directory, S("/calibri.ttf"));
  String vertex_shader_path = str_concat(&platform_permanent_arena, exec_directory, S("/vertex_shader_rect.glsl"));
  String fragment_shader_path = str_concat(&platform_permanent_arena, exec_directory, S("/fragment_shader_rect.glsl"));
  
  // NOTE(erb): first app dll load
  OSX_App_Library app_library = osx_load_app_library(library_path, temp_library_path);
  
  // NOTE(erb): platform functions
  {
    g_platform.allocate = osx_allocate;
    g_platform.free = osx_free;
    g_platform.get_today = osx_get_today;
    
    g_platform.make_window = osx_glfw_make_window;
    g_platform.window_frame_begin = osx_glfw_window_frame_begin;
    g_platform.window_frame_end = osx_glfw_window_frame_end;
    
    g_platform.make_renderer = osx_opengl_make_renderer;
    g_platform.renderer_frame_begin = osx_opengl_render_frame_begin;
    g_platform.renderer_frame_end = osx_opengl_render_frame_end;
    
    g_platform.load_font = osx_freetype_load_font;
  }
  
  // NOTE(erb): application memory setup
  App_Memory memory = {0};
  {
    memory.permanent_memory_size = mb(64);
    memory.permanent_memory = osx_allocate(memory.permanent_memory_size);
    
    memory.transient_memory_size = gb(1);
    memory.transient_memory = osx_allocate(memory.transient_memory_size);
    
    memory.platform = g_platform;
  }
  
  Window *window = 0;
  Render_Data renderer = {0};
  Font_Data *default_font = 0;
  {
    
    window = osx_glfw_make_window(&platform_permanent_arena, S("Window"), v2i(600, 900));
    pf_assert(window != 0);
    default_font = osx_freetype_load_font(&platform_permanent_arena, font_path);
    pf_assert(default_font != 0);
    renderer = osx_opengl_make_renderer(&platform_permanent_arena, vertex_shader_path, fragment_shader_path);
  }
  
  u64 tick = 0;
  for (;;)
  {
    tick++;
    float frame_begin = glfwGetTime();
    u64 cycles_begin = cycle_counter();
    
    App_Update_Result update_result = app_library.update_and_render(&memory, window, &renderer, default_font, exec_directory);
    
    if (osx_has_file_changed(library_path, app_library.load_time))
    {
      osx_unload_app_library(&app_library);
      app_library = osx_load_app_library(library_path, temp_library_path);
    }
    
    if (osx_has_file_changed(vertex_shader_path, renderer.shaders_load_time) ||
        osx_has_file_changed(fragment_shader_path, renderer.shaders_load_time))
    {
      renderer.program = make_shaderes_program(&platform_transient_arena, 
                                               vertex_shader_path, 
                                               fragment_shader_path, 
                                               false, 
                                               &renderer.shaders_load_time);
    }
    
    u64 cycles = cycle_counter() - cycles_begin;
    f64 frame_time = glfwGetTime() - frame_begin;
    f64 fps = 1.0f/frame_time;
    if (tick % 500 == 0) 
    {
      //pf_log("fps(%f) cycles(%llu)\n", fps, cycles);
    }
    
    if (update_result.should_kill)
    {
      break;
    }
  }
  
  gl(glDeleteVertexArrays(1, &renderer.vao));
  gl(glDeleteBuffers(1, &renderer.vbo));
  gl(glDeleteProgram(renderer.program));
  
  close_glfw_window(window);
  
  FT_Done_FreeType(freetype_library);
  glfwTerminate();
  return 0;
}
