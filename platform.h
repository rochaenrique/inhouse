/* date = March 31st 2026 6:17 pm */
#ifndef PLATFORM_H
#define PLATFORM_H

#include "core.h"

typedef struct Loaded_Image
{
  V2i size;
  i32 channels;
  u8 *data;
} Loaded_Image;

typedef struct Render_Rect 
{
  V2f dest_p0;
  V2f dest_p1;
  V2f src_p0;
  V2f src_p1;
  V4f color;
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
  V2f size;
} Image_Texture;

typedef struct Glyph_Info
{
  u32 texture_id;
  V2i size;
  V2i bearing;
  u32 advance_x;
} Glyph_Info;

typedef struct Render_Batch
{
  u32 textures[16];
  u32 texture_count;
  u64 start_rect_index;
} Render_Batch;

typedef struct Render_Data
{
  u32 vao;
  u32 vbo;
  u32 program;
  
  Render_Rect_Buffer rects;
  
  u32 batch_count;
  Render_Batch batches[512];
  
  Image_Texture blank_image_texture;
  
  i64 shaders_load_time;
} Render_Data;

typedef struct Font_Data
{
  i32 height;
  i32 below_base_line_height;
  Glyph_Info ascii_glyph_table[128];
} Font_Data;

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
    } mouse;
    
    struct
    {
      V2f delta;
    } mouse_wheel;
    
    struct
    {
      V2f position;
    } cursor_move;
  };
} Input_Event;

typedef struct Input_Event_List
{
  Input_Event *first;
  Input_Event *last;
} Input_Event_List;

typedef struct Input_State
{
  V2f cursor_position;
  Modifier_Flags modifiers;
  b8 keys_down[KeyCode_COUNT];
  b8 mouse_down[MouseCode_COUNT];
} Input_State;

typedef struct Window
{
  Arena arena;
  
  V2f size;
  V2f cursor_position;
  
  Arena event_arena;
  Input_Event_List event_list;
  
  V2f frame_scroll_offset;
  
  Input_State *frame_input_state;
  Input_State *past_input_state;
  
  void *platform_handle;
  
  b32 should_kill;
} Window;

// NOTE(erb): allocation
#define PLATFORM_ALLOCATE(name) void* name(u64 size)
typedef PLATFORM_ALLOCATE(Platform_Allocate_Func);

#define PLATFORM_FREE(name) void name(void *memory, u64 size)
typedef PLATFORM_FREE(Platform_Free_Func);

// NOTE(erb): date
#define PLATFORM_GET_TODAY(name) Date name()
typedef PLATFORM_GET_TODAY(Platform_Get_Today_Func);

// NOTE(erb): window
#define PLATFORM_MAKE_WINDOW(name) Window *name(Arena *arena, String title, V2i size)
typedef PLATFORM_MAKE_WINDOW(Platform_Make_Window_Func);

#define PLATFORM_WINDOW_FRAME_BEGIN(name) void name(Window *window)
typedef PLATFORM_WINDOW_FRAME_BEGIN(Platform_Window_Frame_Begin_Func);

#define PLATFORM_WINDOW_FRAME_END(name) void name(Window *window)
typedef PLATFORM_WINDOW_FRAME_END(Platform_Window_Frame_End_Func);

// NOTE(erb): renderer
#define PLATFORM_MAKE_RENDERER(name) Render_Data name(Arena *arena, String vertex_shader_path, String fragment_shader_path)
typedef PLATFORM_MAKE_RENDERER(Platform_Make_Renderer_Func);

#define PLATFORM_RENDERER_FRAME_BEGIN(name) void name(Render_Data *renderer, V2f window_size)
typedef PLATFORM_RENDERER_FRAME_BEGIN(Platform_Renderer_Frame_Begin_Func);

#define PLATFORM_RENDERER_FRAME_END(name) void name(Render_Data *renderer, V2f resolution)
typedef PLATFORM_RENDERER_FRAME_END(Platform_Renderer_Frame_End_Func);

// NOTE(erb): font
#define PLATFORM_LOAD_FONT(name) Font_Data *name(Arena *arena, String path)
typedef PLATFORM_LOAD_FONT(Platform_Load_Font_Func);

typedef struct Platform_Code
{
  Platform_Allocate_Func *allocate;
  Platform_Free_Func *free;
  Platform_Get_Today_Func *get_today;
  
  Platform_Make_Window_Func *make_window;
  Platform_Window_Frame_Begin_Func *window_frame_begin;
  Platform_Window_Frame_End_Func *window_frame_end;
  
  Platform_Make_Renderer_Func *make_renderer;
  Platform_Renderer_Frame_Begin_Func *renderer_frame_begin;
  Platform_Renderer_Frame_End_Func *renderer_frame_end;
  
  Platform_Load_Font_Func *load_font;
  
} Platform_Code;
Platform_Code g_platform;

typedef struct App_Memory
{ 
  u64 permanent_memory_size;
  void *permanent_memory;
  
  u64 transient_memory_size;
  void *transient_memory;
  
  Platform_Code platform;
  
  b32 is_initialized;
} App_Memory;

typedef struct App_Update_Result
{
  b32 should_kill;
} App_Update_Result;

// ////////////////////////////////////////////////
// Entry point
// ////////////////////////////////////////////////
#define UPDATE_AND_RENDER(name) App_Update_Result name(App_Memory *memory, Window *window, Render_Data *renderer, Font_Data *default_font, String exec_directory)
typedef UPDATE_AND_RENDER(Update_And_Render_Func);
// ////////////////////////////////////////////////

#endif //PLATFORM_H
