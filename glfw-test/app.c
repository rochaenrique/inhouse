#include "core.c"
#include "platform.c"

typedef struct App_Links
{
  Arena permanent_arena;
  Arena transient_arena;
} App_Links;

UPDATE_AND_RENDER(update_and_render)
{
  supress_unused(exec_directory);
  g_platform = memory->platform;
  
  App_Links *app = (App_Links *)memory->permanent_memory;
  
  if (!memory->is_initialized)
  {
    memory->is_initialized = true;
    arena_init(&app->permanent_arena, 
               memory->permanent_memory + sizeof(App_Links), 
               memory->permanent_memory_size - sizeof(App_Links));
    arena_init(&app->transient_arena, 
               memory->transient_memory, 
               memory->transient_memory_size);
  }
  
  App_Update_Result result = {0};
  
  result.should_kill = (input->should_kill || key_released(input, KeyCode_Escape));
  
  {
    V2f screen_size = v2fi(input->screen_space_size.end);
    V2f cursor = input->cursor_position.end;
    cursor.y = screen_size.y - cursor.y;
    
    append_render_rect_color(renderer, cursor, v2f(100, 100), color_blue);
    append_render_text(renderer, 
                       default_font, 
                       S("hello "), 
                       v2f_scale(screen_size, 0.5f), 
                       20, 
                       0, 
                       color_red);
  }
  
  return result;
}