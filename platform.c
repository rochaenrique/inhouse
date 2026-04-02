#include "platform.h"

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


String debug_input_event_str(Arena *arena, Input_Event* event)
{
  pf_assert(event);
  
  static char buffer[1024];
  
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
               debug_mouse_code_cstr(event->mouse.code),
               str_to_temp256_cstr(debug_modifiers_str(arena, event->mouse.modifiers)));
    } break;
    
    case InputEventKind_CursorMove:
    {
      V2f position = event->cursor_move.position;
      snprintf(buffer, sizeof(buffer), "MouseMove[position=(%f, %f)]", position.x, position.y);
    } break;
    
    case InputEventKind_MouseWheel:
    {
      V2f delta = event->mouse_wheel.delta;
      snprintf(buffer, sizeof(buffer), "MouseWheel[delta=(%f, %f)]", delta.x, delta.y);
    } break;
  }
  
  String str = push_cstr_copy(arena, buffer);
  return str;
}

V2f measure_text_size_ignore_lines_and_tabs(Font_Data *font_data, String str, f32 font_height, f32 spacing)
{
  V2f result = {0};
  
  f32 scale = font_height/font_data->height;
  
  for (u32 char_idx = 0;
       char_idx < str.length;
       char_idx++)
  {
    char ch = str.value[char_idx];
    if (ch != '\n' && ch != '\t') 
    {
      Glyph_Info info = font_data->ascii_glyph_table[(u32)ch];
      
      if (char_idx < str.length - 1) 
      {
        result.x += spacing;
      }
      
      result.x += info.advance_x * scale;
      result.y = max(result.y, info.size.y * scale);
    }
  }
  
  return result;
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
                                V2f dest_p0, V2f dest_p1, 
                                V2f src_p0, V2f src_p1, 
                                f32 corner_radius, f32 edge_softness, 
                                V4f color, u32 texture_id)
{
  // NOTE(erb): push rect
  u32 rect_idx = renderer->rects.count;
  pf_assert(renderer->rects.count <= renderer->rects.capacity);
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
    if (batch->texture_count >= array_size(batch->textures))
    {
      pf_assert(renderer->batch_count < array_size(renderer->batches));
      batch = (renderer->batches + renderer->batch_count++);
      batch->texture_count = 0;
      batch->start_rect_index = rect_idx;
      mem_zero((u8 *)batch->textures, sizeof(batch->textures));
    }
    
    rect->texture_slot = batch->texture_count;
    batch->textures[batch->texture_count++] = texture_id;
  }
  
  return rect;
}

void append_render_rect_color(Render_Data *renderer, V2f pos, V2f size, V4f color)
{
  append_render_rect(renderer, 
                     pos, v2f_add(pos, size), 
                     v2ff(0), renderer->blank_image_texture.size, 
                     0, 0,
                     color, renderer->blank_image_texture.id);
}

void append_render_rect_color_rounded(Render_Data *renderer, V2f pos, V2f size, V4f color, f32 corner_radius)
{
  append_render_rect(renderer, 
                     pos, v2f_add(pos, size), 
                     v2ff(0), renderer->blank_image_texture.size, 
                     corner_radius, 2.0f,
                     color, renderer->blank_image_texture.id);
}

void append_render_rect_texture(Render_Data *renderer, V2f pos, V2f size, Image_Texture texture)
{
  append_render_rect(renderer, 
                     pos, v2f_add(pos, size), 
                     v2ff(0), texture.size, 
                     0, 0,
                     v4ff(1), texture.id);
}

V2f append_render_text(Render_Data *renderer, Font_Data *font_data, String str, V2f position, f32 font_height, f32 spacing, V4f color)
{
  f32 text_height = 0;
  V2f advance_pos = position;
  
  f32 scale = font_height/font_data->height;
  f32 base_line_disp = font_data->below_base_line_height * scale;
  
  for (u32 char_idx = 0;
       char_idx < str.length;
       char_idx++)
  {
    char ch = str.value[char_idx];
    Glyph_Info info = font_data->ascii_glyph_table[(u32)ch];
    
    f32 pos_x = advance_pos.x + info.bearing.x * scale;
    f32 pos_y = position.y - (info.size.y - info.bearing.y) * scale - base_line_disp;
    V2f size = v2f_scale(v2fi(info.size), scale);
    
    // NOTE(erb): push rect
    {
      V2f dest_p0 = v2f(pos_x, pos_y);
      V2f dest_p1 = v2f_add(dest_p0, size);
      V2f src_p0 = v2f(0, info.size.y); // NOTE(erb): flipped due to freetype storage
      V2f src_p1 = v2f(info.size.x, 0);
      append_render_rect(renderer, dest_p0, dest_p1, src_p0, src_p1, 0, 0, color, info.texture_id);
    }
    
    if (char_idx < str.length - 1) 
    {
      advance_pos.x += spacing;
    }
    
    advance_pos.x += info.advance_x * scale;
    text_height = max(text_height, size.y);
  }
  
  V2f text_size = v2f(advance_pos.x, text_height);
  
  return text_size;
}
