#include "core.c"
#include "platform.c"

typedef struct App_Links
{
  Arena permanent_arena;
  Arena transient_arena;
} App_Links;

V2f v2f_reverse_y(V2f v, f32 y)
{
  V2f result = {0};
  result.x = v.x;
  result.y = y - v.y;
  return result;
}

V2f screen_canonical_position(Input_State *input, V2f pos)
{
  V2f screen = v2fi(input->screen_space_size.end);
  V2f result = v2f_reverse_y(pos, screen.y);
  return result;
}

V2f screen_canonical_cursor(Input_State *input)
{
  V2f cursor = input->cursor_reverse_y.end;
  V2f result = screen_canonical_position(input, cursor);
  return result;
}

V4f rect_top_left_at(V4f rect, V2f at)
{
  V2f size = rect_size(rect);
  V2f bottom_left = v2f_sub(at, size);
  V4f result = v4f_point_add(bottom_left, size);
  return result;
}

V4f rect_top_right_at(V4f rect, V2f at)
{
  V2f size = rect_size(rect);
  V2f bottom_left = v2f_sub(at, v2f(0, size.y));
  V4f result = v4f_point_add(bottom_left, size);
  return result;
}

V4f rect_bottom_left_at(V4f rect, V2f at)
{
  V2f size = rect_size(rect);
  V4f result = v4f_point_add(at, size);
  return result;
}

V4f rect_bottom_right_at(V4f rect, V2f at)
{
  V2f size = rect_size(rect);
  V2f bottom_left = v2f_sub(at, v2f(size.x, 0));
  V4f result = v4f_point_add(bottom_left, size);
  return result;
}

V4f rect_bottom_at(V4f rect, V2f at)
{
  V2f size = rect_size(rect);
  V4f result = {0};
  result.p0.x = rect.p0.x;
  result.p0.y = at.y;
  result.p1.x = rect.p1.x;
  result.p1.y = at.y + size.y;
  return result;
}

V4f rect_top_at(V4f rect, V2f at)
{
  V2f size = rect_size(rect);
  V4f result = {0};
  result.p0.x = rect.p0.x;
  result.p0.y = at.y - size.y;
  result.p1.x = rect.p1.x;
  result.p1.y = at.y;
  return result;
}

V4f rect_left_at(V4f rect, V2f at)
{
  V2f size = rect_size(rect);
  V4f result = {0};
  result.p0.x = at.x;
  result.p0.y = rect.p0.y;
  result.p1.x = at.x + size.x;
  result.p1.y = rect.p1.y;
  return result;
}

V4f rect_right_at(V4f rect, V2f at)
{
  V2f size = rect_size(rect);
  V4f result = {0};
  result.p0.x = at.x - size.x;
  result.p0.y = rect.p0.y;
  result.p1.x = at.x;
  result.p1.y = rect.p1.y;
  return result;
}

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
    V2f cursor = screen_canonical_cursor(input);
    
    V2f position = v2f_scale(screen_size, 0.5f);
    
    V2f half_position = v2f_sub(position, v2f(50, 50));
    
    render_rect(renderer, 
                rect_right_at(v4f_point_add(half_position, v2ff(100)), position), 
                color_blue);
    
    render_rect(renderer, v4f_point_add(position, v2ff(10)), color_white);
    
    render_text(renderer, 
                default_font, 
                str_format(&app->transient_arena, "(%.1f, %.1f)", v2_expand(cursor)), 
                position, 
                20, 
                0, 
                color_white);
    
  }
  arena_release(&app->transient_arena);
  
  return result;
}