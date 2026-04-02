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
  
  g_platform.window_frame_begin(window);
  {
    result.should_kill = window->should_kill;
    
    for (Input_Event *event = window->event_list.first;
         event;
         event = event->next)
    {
      if (event->kind == InputEventKind_KeyDown &&
          event->key.code == KeyCode_Escape)
      {
        result.should_kill = true;
        break;
      }
    }
    
    g_platform.renderer_frame_begin(renderer, window->size);
    {
      append_render_rect_color(renderer, v2f(100, 100), v2f(100, 100), color_blue);
      append_render_text(renderer, 
                         default_font, 
                         S("hello "), 
                         v2f_scale(window->size, 0.5f), 
                         20, 
                         0, 
                         color_blue);
    }
    g_platform.renderer_frame_end(renderer, window->size);
  }
  g_platform.window_frame_end(window);
  
  return result;
}