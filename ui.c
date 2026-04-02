#include "ui.h"
#include "os.h"

static UI_Ctx *context;

void ui_pop_parent();


#define ui_defer_loop(begin, end) for(int _i_ = ((begin), 0); !_i_; _i_ += 1, (end))

// ///////////////////////////////////////////////
// NOTE(erb): helpers
// ///////////////////////////////////////////////

UI_Key ui_key_null()
{
	UI_Key result = {0};
	return result;
}

UI_Key ui_key_from_string(String str)
{
	UI_Key result = {0};
	result.content = hash_string(str);
	result.iter = 0;
	return result;
}

b32 ui_key_equals(UI_Key a, UI_Key b)
{
	b32 result = (a.content == b.content && a.iter == b.iter);
	return result;
}

u64 ui_hash_key(UI_Key key)
{
	u64 result = ((key.iter + 1) * key.content) ^ (key.parent * 69420);
	return result;
}

UI_Sizing_Value ui_fixed(f32 value)
{
	UI_Sizing_Value result = {0};
	result.sizing = UI_Sizing_Fixed;
	result.value = value;
	return result;
}

UI_Sizing_Value ui_grow()
{
	UI_Sizing_Value result = {0};
	result.sizing = UI_Sizing_Grow;
	return result;
}

UI_Sizing_Value ui_shrink()
{
	UI_Sizing_Value result = {0};
	result.sizing = UI_Sizing_Shrink;
	return result;
}

UI_Sizing_Value ui_fit()
{
	UI_Sizing_Value result = {0};
	result.sizing = UI_Sizing_Fit;
	return result;
}

void ui_set_next_floating_pos(V2f pos)
{
	context->has_next_floating_pos = true;
	context->next_floating_pos = pos;
}

void ui_set_next_fixed_size(V2f size)
{
	context->has_next_fixed_size = true;
	context->next_fixed_size = size;
}

void ui_set_next_rect(V4f rect)
{
	ui_set_next_floating_pos(rect.p0);
	ui_set_next_fixed_size(rect_size(rect));
}

void ui_set_next_layout_axis(UI_Axis axis)
{
	context->has_next_layout_axis = true;
	context->next_layout_axis = axis;
}

#define ui_font_size(value) ui_defer_loop(ui_push_font_size(value), ui_pop_font_size())
void ui_push_font_size(u32 value)
{
	pf_assert(context->font_size_stack_count + 1 < array_size(context->font_size_stack));
	context->font_size_stack[context->font_size_stack_count++] = value;
}
void ui_pop_font_size()
{
	pf_assert(context->font_size_stack_count > 0);
	context->font_size_stack_count--;
}

#define ui_font_spacing(value) ui_defer_loop(ui_push_font_spacing(value), ui_pop_font_spacing())
void ui_push_font_spacing(u32 value)
{
	pf_assert(context->font_spacing_stack_count + 1 < array_size(context->font_spacing_stack));
	context->font_spacing_stack[context->font_spacing_stack_count++] = value;
}
void ui_pop_font_spacing()
{
	pf_assert(context->font_spacing_stack_count > 0);
	context->font_spacing_stack_count--;
}

#define ui_line_spacing(value) ui_defer_loop(ui_push_line_spacing(value), ui_pop_line_spacing())
void ui_push_line_spacing(u32 value)
{
	pf_assert(context->line_spacing_stack_count + 1 < array_size(context->line_spacing_stack));
	context->line_spacing_stack[context->line_spacing_stack_count++] = value;
}
void ui_pop_line_spacing()
{
	pf_assert(context->line_spacing_stack_count > 0);
	context->line_spacing_stack_count--;
}

#define ui_child_gap(value) ui_defer_loop(ui_push_child_gap(value), ui_pop_child_gap())
void ui_push_child_gap(u32 value)
{
	pf_assert(context->child_gap_stack_count + 1 < array_size(context->child_gap_stack));
	context->child_gap_stack[context->child_gap_stack_count++] = value;
}
void ui_pop_child_gap()
{
	pf_assert(context->child_gap_stack_count > 0);
	context->child_gap_stack_count--;
}

#define ui_font_color(value) ui_defer_loop(ui_push_font_color(value), ui_pop_font_color())
void ui_push_font_color(V4f value)
{
	pf_assert(context->font_color_stack_count + 1 < array_size(context->font_color_stack));
	context->font_color_stack[context->font_color_stack_count++] = value;
}
void ui_pop_font_color()
{
	pf_assert(context->font_color_stack_count > 0);
	context->font_color_stack_count--;
}

#define ui_background_color(value) ui_defer_loop(ui_push_background_color(value), ui_pop_background_color())
void ui_push_background_color(V4f value)
{
	pf_assert(context->background_color_stack_count + 1 < array_size(context->background_color_stack));
	context->background_color_stack[context->background_color_stack_count++] = value;
}
void ui_pop_background_color()
{
	pf_assert(context->background_color_stack_count > 0);
	context->background_color_stack_count--;
}


#define ui_border_color(value) ui_defer_loop(ui_push_border_color(value), ui_pop_border_color())
void ui_push_border_color(V4f value)
{
	pf_assert(context->border_color_stack_count + 1 < array_size(context->border_color_stack));
	context->border_color_stack[context->border_color_stack_count++] = value;
}
void ui_pop_border_color()
{
	pf_assert(context->border_color_stack_count > 0);
	context->border_color_stack_count--;
}

#define ui_padding(value) ui_defer_loop(ui_push_padding(value), ui_pop_padding())
void ui_push_padding(V4f value)
{
	pf_assert(context->padding_stack_count + 1 < array_size(context->padding_stack));
	context->padding_stack[context->padding_stack_count++] = value;
}
void ui_pop_padding()
{
	pf_assert(context->padding_stack_count > 0);
	context->padding_stack_count--;
}

#define ui_sizing_x(value) ui_defer_loop(ui_push_sizing_x(value), ui_pop_sizing_x())
void ui_push_sizing_x(UI_Sizing_Value value)
{
	pf_assert(context->sizing_x_stack_count + 1 < array_size(context->sizing_x_stack));
	context->sizing_x_stack[context->sizing_x_stack_count++] = value;
}
void ui_pop_sizing_x()
{
	pf_assert(context->sizing_x_stack_count > 0);
	context->sizing_x_stack_count--;
}

#define ui_border_size(value) ui_defer_loop(ui_push_border_size(value), ui_pop_border_size())
void ui_push_border_size(f32 value)
{
	pf_assert(context->border_size_stack_count + 1 < array_size(context->border_size_stack));
	context->border_size_stack[context->border_size_stack_count++] = value;
}
void ui_pop_border_size()
{
	pf_assert(context->border_size_stack_count > 0);
	context->border_size_stack_count--;
}

#define ui_corner_rounded_percent(value) ui_defer_loop(ui_push_corner_ronded_percent(value), ui_pop_corner_rounded_percent())
void ui_push_corner_ronded_percent(f32 value)
{
	pf_assert(context->corner_rounded_percent_stack_count + 1 < array_size(context->corner_rounded_percent_stack));
	context->corner_rounded_percent_stack[context->corner_rounded_percent_stack_count++] = value;
}
void ui_pop_corner_rounded_percent()
{
	pf_assert(context->corner_rounded_percent_stack_count > 0);
	context->corner_rounded_percent_stack_count--;
}

#define ui_seed_key(value) ui_defer_loop(ui_push_seed_key(value), ui_pop_seed_key())
void ui_push_seed_key(u64 value)
{
	pf_assert(context->seed_key_stack_count + 1 < array_size(context->seed_key_stack));
	context->seed_key_stack[context->seed_key_stack_count++] = value;
}
void ui_pop_seed_key()
{
	pf_assert(context->seed_key_stack_count > 0);
	context->seed_key_stack_count--;
}
// ///////////////////////////////////////////////
// NOTE(erb): measure text
// ///////////////////////////////////////////////

f32 get_char_width(RL_Font raylib_font, u32 ch)
{
  f32 result = 0;
  
  i32 glyph_idx = GetGlyphIndex(raylib_font, ch);
  f32 glyph_width = raylib_font.glyphs[glyph_idx].advanceX;
  if (glyph_width > 0)
  {
    result = glyph_width;
  }
  else
  {
    result = (raylib_font.recs[glyph_idx].width + raylib_font.glyphs[glyph_idx].offsetX);
    
  }
  
  return result;
}

V2f ui_measure_text_size(UI_Text *text)
{
	// TODO(erb): support new line wrapping
	// TODO(erb): support utf8
	
	f32 base_width = 0;
	
	for (u32 byte_idx = 0; 
       byte_idx < text->content.length;
       byte_idx++)
	{
		i8 letter = *(text->content.value + byte_idx);
		pf_assert(letter != '\n');
		i32 char_width = get_char_width(text->raylib_font, letter); 
		base_width += char_width;
	}
	
	f32 scale_factor = text->font_size / (f32)text->raylib_font.baseSize;
	
	V2f result = {0};
  u32 spacing_count = text->content.length;
  if (spacing_count > 0) 
  {
    spacing_count--;
  }
	result.y = text->font_size;
	result.x = base_width * scale_factor + (f32)(spacing_count*text->spacing);
	
	return result;
}

V2f ui_measure_str_size(String str, RL_Font raylib_font, f32 font_size, f32 spacing, f32 line_spacing)
{
	UI_Text text = {0};
	text.content = str;
	text.raylib_font = raylib_font;
	text.font_size = font_size;
	text.spacing = spacing;
	text.line_spacing = line_spacing;
	
	V2f result = ui_measure_text_size(&text);
	return result;
}

// ///////////////////////////////////////////////
// NOTE(erb): internal stuff
// ///////////////////////////////////////////////


void ui_build_element_cache(UI_Element *element)
{
	for (UI_Element *child = element->first_child;
       child;
       child = child->next)
	{
		ui_build_element_cache(child);
	}
	
	u64 hash = ui_hash_key(element->key);
	
  if (hash != 0)
	{
		u64 idx = hash & (array_size(context->element_cache) - 1);
		UI_Element **free_slot = context->element_cache + idx;
		
		if (*free_slot) 
		{
			element->next_hash = (*free_slot)->next_hash;
			(*free_slot)->next_hash = element;
		}
		else
		{
			*free_slot = element;
		}
	}
}

file_local
void fit_size_element(UI_Element *element, UI_Axis axis)
{
	pf_assert(element);
  pf_assert(axis < UI_Axis_Count);
  
  f32 *element_size = &element->size.c[axis];
  f32 *element_min_size = &element->min_size.c[axis];
	
	if (!(element->flags & UI_Flags_DrawText) && element->sizing[axis] != UI_Sizing_Fixed)
	{
		f32 padding = element->padding.c[axis] + element->padding.c[axis+UI_Axis_Count];
		*element_min_size += padding;
		*element_size += padding;
		
		if (element->layout_axis == axis)
		{
			*element_size += (element->children_count - 1) * element->child_gap;
		}
	}
	
	UI_Element *parent = element->parent;
	if (parent)
	{
		f32 *parent_size = &parent->size.c[axis];
		f32 *parent_min_size = &parent->min_size.c[axis];
		
		if (parent->sizing[axis] != UI_Sizing_Fixed)
		{
			if (parent->layout_axis == axis)
			{
				*parent_size += *element_size;
				*parent_min_size += *element_min_size;
			}
			else if (!(parent->flags & UI_Flags_CrossOverlay))
			{
				*parent_size = max(*parent_size, *element_size);
				*parent_min_size = max(*parent_min_size, *parent_min_size);
			}
		}
	}
}

file_local
void fit_size_step(UI_Element *element, UI_Axis axis)
{
  for (UI_Element *child = element->first_child;
       child;
       child = child->next)
  {
		fit_size_step(child, axis);
  }
  
  fit_size_element(element, axis);
}

file_local
void wrap_text_element(Arena *arena, UI_Element *element)
{
  pf_assert(element);
  pf_assert(arena);
  
  // NOTE(erb): iterate and wrap when needed.
  if ((element->flags & UI_Flags_DrawText) && element->sizing[UI_Axis_X] == UI_Sizing_Shrink)
  {
    pf_assert(element->text);
    UI_Text *text = element->text;
    
    // NOTE(erb): push the first line in
    UI_Line_Node *first_line = push_struct(arena, UI_Line_Node);
    first_line->content = text->content;
    text->first_line = text->last_line = first_line;
    text->line_count++;
    
    u32 last_line_begin = 0;
    u32 longest_line_bytes = 0;
    f32 longest_line_width = element->size.x;
    
    f32 line_width = 0;
    u32 line_bytes = 0;
    
    f32 scale_factor = text->font_size / text->raylib_font.baseSize;
    
    // TODO(erb): handle utf8
    // TODO(erb): handle new line wrapping
    for (u32 byte_idx = 0; 
         byte_idx < text->content.length;)
    {
      i32 codepoint_byte_count = 0;
      i32 letter = GetCodepointNext(text->content.value + byte_idx, &codepoint_byte_count);
      byte_idx += codepoint_byte_count;
      line_bytes++;
      
      line_width += scale_factor * get_char_width(text->raylib_font, letter);
      
      if (byte_idx < text->content.length - 1)
      {
        line_width += text->spacing;
      }
      
      if (line_width > element->size.x)
      {
        u32 wrap_word_width = 0;
        u32 wrap_idx = byte_idx;
        for (; 
             wrap_idx > 0;
             wrap_idx--)
        {
          wrap_word_width += scale_factor * get_char_width(text->raylib_font, wrap_idx);
          if (wrap_idx < line_bytes - 1)
          {
            wrap_word_width += text->spacing;
          }
          
          f32 new_width = (line_width - wrap_word_width);
          if (new_width < element->size.x || new_width < element->min_size.x)
          {
            break;
          }
        }
        
        // NOTE(erb): split the current line
        UI_Line_Node *old_line = text->last_line;
        old_line->content = str_slice(text->content, last_line_begin, wrap_idx);
        
        // NOTE(erb): Append new line
        UI_Line_Node *line = push_struct(arena, UI_Line_Node);
        line->content = str_from(text->content, wrap_idx);
        text->last_line = old_line->next = line;
        text->line_count++;
        last_line_begin = wrap_idx;
        
        longest_line_width = max(longest_line_width, line_width - wrap_word_width);
        line_width = 0;
        longest_line_bytes= max(longest_line_bytes, line_bytes - old_line->content.length);
        line_bytes = 0;
        byte_idx = wrap_idx+1;
      }
    }
    
    element->size.x = longest_line_width;
    element->size.y = (text->line_count * text->font_size) + ((text->line_count - 1) * text->line_spacing);
  }
}

file_local
void wrap_text_step(Arena *arena, UI_Element *element)
{
  
  for (UI_Element *child = element->first_child;
       child;
       child = child->next)
  {
    wrap_text_element(arena, child);
  }
}

file_local
void grow_and_shrink_size(UI_Element *parent, UI_Axis axis)
{
  pf_assert(parent);
  pf_assert(axis < UI_Axis_Count);
  
	f32 padding = parent->padding.c[axis] + parent->padding.c[axis+UI_Axis_Count];
  f32 extra_size = parent->size.c[axis] - padding;
  
	if (parent->layout_axis == axis)
	{
		UI_Element *first_shrinkable = 0;
		UI_Element *first_growable = 0;
		for (UI_Element *child = parent->first_child;
         child;
         child = child->next)
		{
			UI_Sizing sizing = child->sizing[axis];
			if (!first_growable && sizing == UI_Sizing_Grow)
			{
				first_growable = child;
			}
			
			if (!first_shrinkable && (sizing == UI_Sizing_Shrink || sizing == UI_Sizing_Fit))
			{
				first_shrinkable = child;
			}
			
			extra_size -= child->size.c[axis];
		}
		
		extra_size-= (parent->children_count - 1) * parent->child_gap;
		
		if (first_growable)
		{
			while (extra_size > 0.0001)
			{
				f32 smallest = first_growable->size.c[axis];
				f32 second_smallest = MAX_f32;
				f32 size_to_add = extra_size;
				
				u32 growable_count = 0;
				for (UI_Element *child = first_growable;
             child;
             child = child->next)
				{
					f32 size = child->size.c[axis];
					f32 max_size = child->max_size.c[axis];
					
					if (child->sizing[axis] == UI_Sizing_Grow && size < max_size)
					{
						growable_count++;
						if (size < smallest)
						{
							second_smallest = smallest;
							smallest = size;
						}
						if (size > smallest)
						{
							second_smallest= min(second_smallest, size);
							size_to_add = second_smallest - smallest;
						}
					}
				}
				
				if (growable_count == 0) 
				{  
					break;
				}
				
				size_to_add = min(size_to_add, extra_size / growable_count);
        
        if (size_to_add == 0) 
        { 
          break;
        }
        
				u32 grown_count = 0;
				
				for (UI_Element *child = first_growable;
             child;
             child = child->next)
				{
					f32 *size = &child->size.c[axis];
					if (child->sizing[axis] == UI_Sizing_Grow && *size == smallest)
					{
						if (*size < child->max_size.c[axis])
						{
							*size += size_to_add;
							extra_size -= size_to_add;
							grown_count++;
						}
					}
				}	
				
				
				if (grown_count == 0) 
				{
					break;
				}
			}
		}
		
		if (first_shrinkable)
		{
			while (extra_size < -0.0001)
			{
				f32 largets = first_shrinkable->size.c[axis];
				f32 second_largest = 0;
				// NOTE(erb): this value is negative
				f32 size_to_sub = extra_size;
				
				u32 shrinkable_count = 0;
				for (UI_Element *child = first_shrinkable;
             child;
             child = child->next)
				{
					f32 size = child->size.c[axis];
					f32 minsize = child->min_size.c[axis];
					UI_Sizing sizing = child->sizing[axis];
					
					if ((sizing == UI_Sizing_Shrink || sizing == UI_Sizing_Fit) && size > minsize)
					{
						shrinkable_count++;
						if (size > largets)
						{
							second_largest = largets;
							largets = size;
						}
						if (size < largets)
						{
							second_largest = max(second_largest, size);
							size_to_sub = second_largest - largets;
						}
					}
				}
				
				if (shrinkable_count == 0) 
				{  
					break;
				}
				
				size_to_sub = max(size_to_sub, extra_size / shrinkable_count);
				
				u32 shrunk_count = 0;
				for (UI_Element *child = first_shrinkable;
             child;
             child = child->next)
				{
					f32 *size = &child->size.c[axis];
					UI_Sizing sizing = child->sizing[axis];
					
					if ((sizing == UI_Sizing_Shrink || sizing == UI_Sizing_Fit) && *size == largets)
					{
						if (*size > child->min_size.c[axis])
						{
							// NOTE(erb): WidthToSub is negative
							*size += size_to_sub;
							extra_size -= size_to_sub;
							shrunk_count++;
						}
					}	
				}
				
				if (shrunk_count == 0)
				{
					break;
				}
			}
			
		}
		
	}
	else
	{
		for (UI_Element *child = parent->first_child;
         child;
         child = child->next)
		{
			UI_Sizing sizing = child->sizing[axis];
			f32 *size = &child->size.c[axis];
			
			if (sizing == UI_Sizing_Shrink) 
			{
				*size = max(extra_size, child->min_size.c[axis]);
			}
			else if (sizing == UI_Sizing_Grow)
			{
				*size = min(extra_size, child->max_size.c[axis]);
			}
		}
	}
}

file_local
void grow_and_shrink_step(UI_Element *parent, UI_Axis axis)
{
	if (parent)
	{
		grow_and_shrink_size(parent, axis);
		
		for (UI_Element *child = parent->first_child;
         child;
         child = child->next)
		{
			grow_and_shrink_step(child, axis);
		}
	}
}

Index_u32 line_index_from_cursor_delta(f32 delta, UI_Text *text)
{
  pf_assert(text);
  
  f32 line_width = 0;
  f32 scale_factor = text->font_size / (f32)text->raylib_font.baseSize;
  
  Index_u32 index = {0};
  for (;
       index.value < text->content.length;
       index.value++)
  {
    char ch = text->content.value[index.value];
    f32 letter_width = get_char_width(text->raylib_font, ch)*scale_factor;
    if (index.value < text->content.length - 1)
    {
      letter_width += text->spacing;
    }
    
    if (delta <= line_width + letter_width)
    {
      // NOTE(erb): snap to forward or back char
      if (delta >= line_width + letter_width/2)
      {
        index.value++;
      }
      index.exists = true;
      break;
    }
    
    line_width += letter_width;
  }
  
  return index;
}

file_local
void draw_element_cmd(Arena *arena, UI_Draw_Cmd_List *draw_cmds, UI_Element *element)
{
  pf_assert(draw_cmds);
  pf_assert(element);
	
	if (element->flags & UI_Flags_DrawBackGround)
	{
		UI_Draw_Cmd *cmd = sll_push_allocate(arena, draw_cmds, UI_Draw_Cmd);
		cmd->kind = UI_Draw_Kind_Rect;
		cmd->pos = element->pos;
		cmd->size = element->size;
		cmd->color = element->background_color;
		
		if (element->flags & UI_Flags_DrawRounded)
		{
			cmd->corner_rounded_percent = element->corner_rounded_percent;
		}
	}
	
	if (element->flags & UI_Flags_DrawBorder)
	{
		UI_Draw_Cmd *cmd = sll_push_allocate(arena, draw_cmds, UI_Draw_Cmd);
		cmd->kind = UI_Draw_Kind_Rectlines;
		cmd->pos = element->pos;
		cmd->size = element->size;
		cmd->color = element->border_color;
    cmd->border_size = element->border_size;
		
		if (element->flags & UI_Flags_DrawRounded)
		{
			cmd->corner_rounded_percent = element->corner_rounded_percent;
		}
	}
	
	// NOTE(erb): debug
	extern b32 global_debug;
	if (global_debug && (element->flags & UI_Flags_DrawBackGround))
	{
		UI_Draw_Cmd *cmd = sll_push_allocate(arena, draw_cmds, UI_Draw_Cmd);
		cmd->kind = UI_Draw_Kind_Rect;
		cmd->pos = v2f_add(element->pos, element->padding.left_top);
		cmd->size = v2f_sub(element->size, v2f_add(element->padding.left_top, element->padding.right_bottom));
		cmd->color = v4f(1, 1, 0, .5);
	}
	
	if (global_debug)
	{
		UI_Draw_Cmd *cmd = sll_push_allocate(arena, draw_cmds, UI_Draw_Cmd);
		cmd->kind = UI_Draw_Kind_Rectlines;
		cmd->pos = element->pos;
		cmd->size = element->size;
		cmd->color = v4f(1, 1, 0, 1);
	}
	
	if (element->flags & UI_Flags_DrawText)
	{
		UI_Text *text = element->text;
		pf_assert(text);
		
		if (text->line_count > 0)
		{
      f32 line_height = text->font_size + text->line_spacing;
			u32 line_idx = 0;
			for (UI_Line_Node *line = text->first_line;
           line && line_idx < text->line_count;
           line = line->next, line_idx++)
			{
				UI_Draw_Cmd *cmd = sll_push_allocate(arena, draw_cmds, UI_Draw_Cmd);
				
				cmd->pos = element->pos;
				cmd->pos.y += line_idx*line_height;
				
				cmd->kind = UI_Draw_Kind_Text;
				cmd->color = text->color;
				cmd->raylib_font = text->raylib_font;
				cmd->content = line->content;
				cmd->font_size = text->font_size;
				cmd->spacing = text->spacing;
			}
		}
		else
		{
			UI_Draw_Cmd *cmd = sll_push_allocate(arena, draw_cmds, UI_Draw_Cmd);
			cmd->kind = UI_Draw_Kind_Text;
			cmd->pos = element->pos;
			cmd->color = text->color;
			cmd->raylib_font = text->raylib_font;
			cmd->content = text->content;
			cmd->font_size = text->font_size;
			cmd->spacing = text->spacing;
		}
	}
}

file_local
void draw_step(Arena *arena, UI_Draw_Cmd_List *draw_cmds, UI_Element *element)
{
  pf_assert(element);
  
  draw_element_cmd(arena, draw_cmds, element);
	
	V2f child_pos = v2f_add(element->pos, element->padding.left_top);
  for (UI_Element *child = element->first_child;
       child;
       child = child->next)
  {
		if (!(child->flags & UI_Flags_Floating))
		{
			child->pos = child_pos;
			child_pos.c[element->layout_axis] += child->size.c[element->layout_axis] + element->child_gap;
		}
    
		draw_step(arena, draw_cmds, child);
  }
  
}


// ///////////////////////////////////////////////
// NOTE(erb): usage
// ///////////////////////////////////////////////

UI_Element *ui_make_element(UI_Key Key, String str, UI_Flags flags);
void ui_build_element_cache(UI_Element *);
void ui_push_parent(UI_Element *);

void ui_init(Arena *arena)
{
	context = push_struct(arena, UI_Ctx);
	context->arena = push_sub_arena(arena, kb(64));
	context->last_arena = push_sub_arena(arena, kb(64));
  
	ui_push_font_size(20);
	ui_push_font_spacing(2);
	ui_push_line_spacing(2);
	ui_push_child_gap(10);
	ui_push_font_color(v4ff(1));
	ui_push_background_color(v4f(1, 0, 1, 1));
	ui_push_border_color(v4f(.5f, .5f, .5f, 1));
  ui_push_seed_key(~0);
	ui_push_padding(v4ff(0));
	ui_push_sizing_x(ui_fit());
	ui_push_corner_ronded_percent(.2f);
	ui_push_border_size(2);
}

void ui_begin_frame(V4f screen_rect)
{
	ui_set_next_rect(screen_rect);
	ui_push_parent(ui_make_element(ui_key_null(), str_null(), UI_Flags_None));
}

void ui_end_frame()
{
	mem_zero((u8 *)context->element_cache, array_size(context->element_cache)*sizeof(UI_Element *));
	
	ui_build_element_cache(context->root);
	arena_clear(&context->last_arena);
	swap(context->last_arena, context->arena, Arena);
  
	context->element_count = 0;
	context->root = 0;
  context->defered_draw.first = 0;
}

UI_Draw_Cmd_List ui_calculate_and_draw(Arena *arena)
{
	pf_assert(context->current == context->root);
  
  fit_size_step(context->root,        UI_Axis_X);
  grow_and_shrink_step(context->root, UI_Axis_X);
  wrap_text_step(&context->arena,      context->root);
	fit_size_step(context->root,        UI_Axis_Y);
	grow_and_shrink_step(context->root, UI_Axis_Y);
	
	UI_Draw_Cmd_List draw_cmds = {0};
	draw_step(arena, &draw_cmds, context->root);
  
  draw_cmds.last->next = context->defered_draw.first;
  
	return draw_cmds;
}

UI_Element *ui_get_cached_element(UI_Key key)
{
  UI_Element *result = 0;
  
  u64 index = ui_hash_key(key) & (array_size(context->element_cache) - 1);
  UI_Element *test_element = context->element_cache[index];
  for (;
       test_element != 0; 
       test_element = test_element->next_hash)
  {
    if (ui_key_equals(test_element->key, key))
    {
      result = test_element;
      break;
    }
  }
  
  return result;
}

UI_Element *ui_make_element(UI_Key key, String str, UI_Flags flags)
{
	UI_Element *element = push_struct(&context->arena, UI_Element);
	
	element->max_size = v2ff(MAX_f32);
	element->key = key;
  element->key.parent = context->seed_key_stack[context->seed_key_stack_count-1];
  element->flags = flags;
	
	element->padding = context->padding_stack[context->padding_stack_count-1];
	element->child_gap = context->child_gap_stack[context->child_gap_stack_count-1];
	
	if (flags & UI_Flags_DrawBorder)
	{
		element->border_size = context->border_size_stack[context->border_size_stack_count-1];
    element->border_color = context->border_color_stack[context->border_color_stack_count-1];
	}
	
	if (flags & UI_Flags_DrawRounded)
	{
		element->corner_rounded_percent = context->corner_rounded_percent_stack[context->corner_rounded_percent_stack_count-1];
	}
	
	element->sizing[UI_Axis_X] = context->sizing_x_stack[context->sizing_x_stack_count-1].sizing;
	element->size.c[UI_Axis_X] = context->sizing_x_stack[context->sizing_x_stack_count-1].value;
	
	if (element->flags & UI_Flags_DrawBackGround)
	{
		element->background_color = context->background_color_stack[context->background_color_stack_count-1];
	}
	
	if (element->flags & UI_Flags_DrawText)
	{
		UI_Text *text = push_struct(&context->arena, UI_Text);
		text->raylib_font = GetFontDefault();
		text->font_size = context->font_size_stack[context->font_size_stack_count-1];
		text->spacing = context->font_spacing_stack[context->font_spacing_stack_count-1];
		text->line_spacing = context->line_spacing_stack[context->line_spacing_stack_count-1];
		text->color = context->font_color_stack[context->font_color_stack_count-1];
		text->content = str_copy(&context->arena, str);
		text->first_line = 0;
		
		V2f text_size = ui_measure_text_size(text);
		V2f min_text_size = text_size;
		V2f max_text_size = text_size;
    
    if (element->sizing[UI_Axis_X] == UI_Sizing_Shrink) 
    {
      // NOTE(erb): find largest word
      u32 word_count = 0;
      String largest_word = {0};
      for (u32 ch_idx = 0; 
           ch_idx < str.length;
           word_count++) 
      {
        String word = str_next_word(str_from(str, ch_idx));
        if (word.length > largest_word.length)
        {
          largest_word = word;
        }
        ch_idx += word.length;
      }
      //pf_assert(largest_word.length > 0);
      
      V2f largest_word_size = ui_measure_str_size(largest_word, text->raylib_font, text->font_size, 
                                                  text->spacing, text->line_spacing);
      
      max_text_size.y = word_count*text->font_size + (word_count - 1)*text->line_spacing;
      min_text_size.x = largest_word_size.x;
      element->sizing[UI_Axis_Y] = UI_Sizing_Grow;
    }
    
		element->text = text;
		element->size = text_size;
		element->min_size = min_text_size;
		element->max_size = max_text_size;
	}
	
	if (context->has_next_floating_pos)
	{
		element->flags |= UI_Flags_Floating;
		element->pos = context->next_floating_pos;
		context->has_next_floating_pos = false;
	}
	
	if (context->has_next_fixed_size)
	{
		element->sizing[UI_Axis_X] = UI_Sizing_Fixed;
		element->sizing[UI_Axis_Y] = UI_Sizing_Fixed;
		element->size = context->next_fixed_size;
		context->has_next_fixed_size = false;
	}
	
	if (context->has_next_layout_axis)
	{
		element->layout_axis = context->next_layout_axis;
		context->has_next_layout_axis = false;
	}
	
	return element;
}

void ui_push_parent(UI_Element *element)
{
	if (context->root == 0)
	{
		context->root = element;
		element->pos = context->root_pos;
	}
	else
	{
		UI_Element *parent = context->current;
		if (parent->last_child)
		{
			parent->last_child = parent->last_child->next = element;
		}
		else
		{
			parent->last_child = parent->first_child = element;
		}
		
		parent->children_count++;
		element->parent = parent;
	}
	
	context->current = element;
}

void ui_pop_parent()
{
	pf_assert(context->current != 0);
	context->element_count++;
	context->current = context->current->parent;
}

#define ui_parent(parent)     ui_defer_loop(ui_push_parent(parent), ui_pop_parent())
#define ui_null_parent(flags) ui_make_element(ui_key_null(), str_null(), flags)
#define ui_parent_flagged(flags) ui_defer_loop(ui_push_parent(ui_null_parent(flags)), ui_pop_parent())
#define ui_parent_zero()  ui_parent_flagged(UI_Flags_None)

UI_Element_Out ui_element_out(UI_Element *element)
{
	UI_Element_Out result = {0};
	UI_Key key = element->key;
  
	if (ui_key_equals(context->active_element, key)) 
	{
    result.dragging = true;
    if (IsMouseButtonUp(MOUSE_BUTTON_LEFT))
		{
			if (ui_key_equals(context->hot_element, key))
			{
				result.clicked = true;
			}
      context->active_element = ui_key_null();
      result.dragging = false;
		}
	}
	else if (ui_key_equals(context->hot_element, key))
	{
		result.hovered = true;
		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) 
		{
			context->active_element = key;
			result.pressed = true;
		}
		context->hot_element = ui_key_null();
	}
  else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
  {
    result.pressed_outside = true;
  }
  
  UI_Element *cached = ui_get_cached_element(key);
  if (cached)
  {
    if (element->flags & UI_Flags_Clickable)
    {
      result.element = cached;
      
      f32 mouse_x = GetMouseX();
      f32 mouse_y = GetMouseY();
      
      if (cached->pos.x <= mouse_x && 
          mouse_x <= cached->pos.x + cached->size.x &&
          cached->pos.y <= mouse_y && 
          mouse_y <= cached->pos.y + cached->size.y)
      {
        context->hot_element = key;
      }
    }
  }
  
  return result;
}

void ui_text(String str)
{
	ui_parent(ui_make_element(ui_key_from_string(str), str, UI_Flags_DrawText));
}

UI_Element_Out ui_button(String text)
{
	UI_Key key = ui_key_from_string(text);
	UI_Element *element = ui_make_element(key, str_null(), 
                                        UI_Flags_Clickable|
                                        UI_Flags_DrawBackGround|
                                        UI_Flags_DrawRounded|
                                        UI_Flags_DrawBorder);
  ui_seed_key(ui_hash_key(key)) ui_parent(element)
	{
		ui_text(text);
	}
	
	UI_Element_Out result = ui_element_out(element);
  
  element->background_color.a = 0.5f;
  if (result.hovered) 
  {
    element->background_color.a = 0.75f;
  }
  if (result.dragging) 
  {
    element->background_color.a = 1;
    element->background_color.r = max(element->background_color.r * 1.5, 1);
  }
  
	return result;
}


UI_Element_Out ui_toggle(String text)
{
	UI_Key key = ui_key_from_string(text);
  UI_Element *element = ui_make_element(key, str_null(), 
                                        UI_Flags_Clickable|
                                        UI_Flags_DrawBackGround|
                                        UI_Flags_DrawRounded|
                                        UI_Flags_DrawBorder);
  
  ui_seed_key(ui_hash_key(key)) ui_parent(element)
  {
    ui_text(text);
  }
  
	UI_Element_Out out = ui_element_out(element);
  
  if (out.element)
  {
    element->toggled = out.element->toggled;
  }
  if (out.clicked)
  {
    element->toggled = !element->toggled;
  }
  if (element->toggled)
  {
    element->background_color = v4f(1, 1, 1, 1);
  }
  
	return out;
}

b32 ui_try_get_element_top_left(UI_Element *element, V2f *result)
{
  pf_assert(result != 0);
  
  b32 found = false;
  UI_Element *cached = ui_get_cached_element(element->key);
  if (cached) 
  {
    *result = cached->pos;
    found = true;
  }
  
  return found;
}

void ui_push_draw_cursor_and_highlight(Arena *temp_arena, 
                                       UI_Draw_Cmd_List *draw_cmds, 
                                       UI_Text text, 
                                       V2f pos,
                                       b32 active,
                                       Text_Edit_State *text_state)
{
  UI_Draw_Cmd *cursor = sll_push_allocate(temp_arena, &context->defered_draw, UI_Draw_Cmd);
  UI_Text cursor_sub = text;
  cursor_sub.content = str_clip(cursor_sub.content, text_state->cursor);
  cursor->pos = v2f_add(pos, v2f(ui_measure_text_size(&cursor_sub).x, 0));
  cursor->kind = UI_Draw_Kind_Rect;
  cursor->size = v2f(1.5f, cursor_sub.font_size);
  cursor->color = v4f(1, 0, 0, 0.5f);
  
  if (active)
  {
    cursor->color.a = 1.0f;
    
    if (text_state->mark != text_state->cursor)
    {
      UI_Text mark_sub = text;
      mark_sub.content = str_slice(mark_sub.content, 
                                   min(text_state->mark, text_state->cursor),
                                   max(text_state->mark, text_state->cursor));
      f32 highlight_width = ui_measure_text_size(&mark_sub).x;
      
      V2f highligh_pos = v2f(cursor->pos.x, pos.y);
      if (text_state->mark < text_state->cursor) 
      {
        highligh_pos.x -= highlight_width;
      }
      
      UI_Draw_Cmd *highligh = sll_push_allocate(temp_arena, draw_cmds, UI_Draw_Cmd);
      highligh->pos = highligh_pos;
      highligh->kind = UI_Draw_Kind_Rect;
      highligh->size = v2f(highlight_width, mark_sub.font_size);
      highligh->color = v4f(1, 1, 0, 0.5f);
    }
  }
}

void ui_draw_commands_raylib(UI_Draw_Cmd_List *draw_cmds) 
{
	for (UI_Draw_Cmd *draw = draw_cmds->first;
       draw;
       draw = draw->next)
	{
		
		Color raylib_color = {0};
		raylib_color.r = draw->color.r * 255;
		raylib_color.g = draw->color.g * 255;
		raylib_color.b = draw->color.b * 255;
		raylib_color.a = draw->color.a * 255;
		
		switch (draw->kind)
		{
			case UI_Draw_Kind_Text:
			{
				Vector2 raylib_pos = {0};
				raylib_pos.x = draw->pos.x;
				raylib_pos.y = draw->pos.y;
				
				DrawTextEx(draw->raylib_font, str_to_temp256_cstr(draw->content), raylib_pos, 
                   draw->font_size, draw->spacing, raylib_color);
        extern b32 global_debug;
				if (global_debug)
				{
					V2f text_size = ui_measure_str_size(draw->content, draw->raylib_font, 
                                              draw->font_size, draw->spacing, 1);
					DrawRectangleLines(draw->pos.x, draw->pos.y, text_size.x, text_size.y, GREEN);
				}
			} break;
			
			case UI_Draw_Kind_Rect:
			{
				Rectangle raylib_rect = {0};
				raylib_rect.x = draw->pos.x;
				raylib_rect.y = draw->pos.y;
				raylib_rect.width = draw->size.x;
				raylib_rect.height = draw->size.y;
				
				DrawRectangleRounded(raylib_rect, draw->corner_rounded_percent, 50, raylib_color);
			} break;
			
			case UI_Draw_Kind_Rectlines:
			{
				Rectangle raylib_rect = {0};
				raylib_rect.x = draw->pos.x;
				raylib_rect.y = draw->pos.y;
				raylib_rect.width = draw->size.x;
				raylib_rect.height = draw->size.y;
				
				DrawRectangleRoundedLinesEx(raylib_rect, draw->corner_rounded_percent, 50, draw->border_size, raylib_color);
				extern b32 global_debug;
				if (global_debug)
				{
					DrawRectangleLines(draw->pos.x, draw->pos.y, draw->size.x, draw->size.y, BLUE);
					DrawRectangleLines(draw->pos.x + draw->border_size/2.0f, 
                             draw->pos.y + draw->border_size/2.0f, 
                             draw->size.x - draw->border_size/2.0f, 
                             draw->size.y - draw->border_size/2.0f, BLUE);
				}
			} break;
      
      case UI_Draw_Kind_BeginClipRect:
      {
        BeginScissorMode(draw->pos.x, draw->pos.y, draw->size.x, draw->size.y);
      } break;;
      
			case UI_Draw_Kind_EndClipRect:
      {
      } break;;
			
			default: pf_assert(0);
		}
	}
}


// NOTE(erb): text_action_from_eventTable[OS_KEY][OS_CONTROL][OS_SHIFT]
#define bind(key, modifiers, flags, delta) \
[(key)][!!(modifiers & OS_Modifier_Flags_Ctrl)][!!((modifiers) & OS_Modifier_Flags_Shift)] = {(flags),(delta),0}
Text_Action text_action_from_eventTable[][2][2] = 
{
  bind(OS_Key_Code_Left, OS_Modifier_Flags_None, Text_Action_Flags_DeltaPickSelectSide, -1),
  bind(OS_Key_Code_Left, OS_Modifier_Flags_Shift, Text_Action_Flags_KeepMark, -1),
  bind(OS_Key_Code_Left, OS_Modifier_Flags_Ctrl, Text_Action_Flags_WordScan, -1),
  bind(OS_Key_Code_Left, OS_Modifier_Flags_Shift|OS_Modifier_Flags_Ctrl, Text_Action_Flags_KeepMark|Text_Action_Flags_WordScan, -1),
  
  bind(OS_Key_Code_Right, OS_Modifier_Flags_None, Text_Action_Flags_DeltaPickSelectSide, 1),
  bind(OS_Key_Code_Right, OS_Modifier_Flags_Shift, Text_Action_Flags_KeepMark, 1),
  bind(OS_Key_Code_Right, OS_Modifier_Flags_Ctrl, Text_Action_Flags_WordScan, 1),
  bind(OS_Key_Code_Right, OS_Modifier_Flags_Shift|OS_Modifier_Flags_Ctrl, Text_Action_Flags_KeepMark|Text_Action_Flags_WordScan, 1),
  
  bind(OS_Key_Code_Delete, OS_Modifier_Flags_None, Text_Action_Flags_Delete|Text_Action_Flags_ZeroDeltaWithSelect, 1),
  bind(OS_Key_Code_Delete, OS_Modifier_Flags_Ctrl, Text_Action_Flags_Delete|Text_Action_Flags_WordScan|Text_Action_Flags_ZeroDeltaWithSelect, 1),
  
  bind(OS_Key_Code_Backspace, OS_Modifier_Flags_None, Text_Action_Flags_Delete|Text_Action_Flags_ZeroDeltaWithSelect, -1),
  bind(OS_Key_Code_Backspace, OS_Modifier_Flags_Ctrl, Text_Action_Flags_Delete|Text_Action_Flags_WordScan|Text_Action_Flags_ZeroDeltaWithSelect, -1),
  
  bind(OS_Key_Code_C, OS_Modifier_Flags_Ctrl, Text_Action_Flags_Copy,  0),
  bind(OS_Key_Code_X, OS_Modifier_Flags_Ctrl, Text_Action_Flags_Cut,   0),
  bind(OS_Key_Code_V, OS_Modifier_Flags_Ctrl, Text_Action_Flags_Paste, 0),
  
  // emacs style
  bind(OS_Key_Code_F, OS_Modifier_Flags_Ctrl, Text_Action_Flags_DeltaPickSelectSide, 1),
  bind(OS_Key_Code_F, OS_Modifier_Flags_Alt, Text_Action_Flags_WordScan, 1),
  
  bind(OS_Key_Code_B, OS_Modifier_Flags_Ctrl, Text_Action_Flags_DeltaPickSelectSide, -1),
  bind(OS_Key_Code_B, OS_Modifier_Flags_Alt, Text_Action_Flags_WordScan, -1),
};
#undef bind

Text_Action text_action_from_event(OS_Event *event) 
{
  Text_Action action = {0};
  
  if (event->key >= 0 && event->key < array_size(text_action_from_eventTable))
  {
    b32 ctrl = (event->modifiers & OS_Modifier_Flags_Ctrl) != 0;
    b32 shift = (event->modifiers & OS_Modifier_Flags_Shift) != 0;
    action = text_action_from_eventTable[event->key][ctrl][shift];
  }
  
  b32 dealt_with = action.flags != 0 || action.delta != 0;
  
  if (!dealt_with &&
      event->key >= KEY_SPACE && event->key <= KEY_TAB &&
      (event->modifiers & (~OS_Modifier_Flags_Shift)) == 0)
  {
    action.code_point = event->key;
    action.delta = 1;
  }
  
  return action;
}

Text_Cmd text_cmd_from_state_and_action(Arena *arena, String str, Text_Edit_State *state, Text_Action *action)
{
  Text_Cmd cmd = {0};
  cmd.new_cursor = state->cursor;
  cmd.new_mark = state->mark;
  cmd.replace_min = 0;
  cmd.replace_max = 0;
  cmd.replace_str = S("");
  cmd.copy_str = S("");
  
  i64 delta = 0;
  // TODO(erb): navigation
  if (action->flags & Text_Action_Flags_WordScan) 
  {
    Index_u64 index = str_index_word_end_delta(str, state->cursor, action->delta);
    delta = (index.value - state->cursor);
  }
  else 
  {
    delta = action->delta;
  }
  
  if (action->flags & Text_Action_Flags_ZeroDeltaWithSelect)
  {
    delta = 0;
  }
  
  if (state->cursor != state->mark &&
      action->flags & Text_Action_Flags_DeltaPickSelectSide)
  {
    delta = 0;
    if (action->delta > 0) 
    {
      cmd.new_cursor = max(state->cursor, state->mark);
    }
    else if (action->delta < 0)
    {
      cmd.new_cursor = min(state->cursor, state->mark);
    }
  }
  
  // NOTE(erb): apply delta
  cmd.new_cursor = cap(state->cursor + delta, 0, (i64)str.length);
  
  // NOTE(erb): post navigation
  
  // NOTE(erb): copy
  if (action->flags & Text_Action_Flags_Copy || 
      action->flags & Text_Action_Flags_Cut) 
  {
    cmd.copy_str = str_slice(str, state->cursor, state->mark);
  }
  
  // NOTE(erb): delete
  if (action->flags & Text_Action_Flags_Delete || 
      action->flags & Text_Action_Flags_Cut)
  {
    cmd.replace_min = min(cmd.new_cursor, cmd.new_mark);
    cmd.replace_max = max(cmd.new_cursor, cmd.new_mark);
    
    cmd.new_cursor = cmd.new_mark = cmd.replace_min;
  }
  
  // NOTE(erb): insert
  if (action->code_point != 0)
  {
    // NOTE(erb): only ascii chars
    char *single_char = push_array(arena, 1, char);
    *single_char = action->code_point;
    cmd.replace_str.value = single_char;
    cmd.replace_str.length = 1;
    cmd.replace_min = state->cursor;
    cmd.replace_max = state->mark;
  }
  // NOTE(erb): paste
  else if (action->flags & Text_Action_Flags_Paste)
  {
    cmd.replace_str = os_push_clipboard_text(arena);
    cmd.replace_min = state->cursor;
    cmd.replace_max = state->mark;
  }
  
  // NOTE(erb): keep mark
  if (!(action->flags & Text_Action_Flags_KeepMark))
  {
    cmd.new_mark = cmd.new_cursor;
  }
  
  return cmd;
}

b32 ui_element_toggle_focus(UI_Element_Out out)
{
  b32 result = false;
  
  if (out.element)
  {
    result = out.element->toggled;
  }
  
  if (out.pressed)
  {
    result = true;
  }
  else if (out.pressed_outside)
  {
    result = false;
  }
  return result;
}

b32 ui_apply_text_action(Arena *arena, Arena *temp_arena, Text_Action action, Text_Edit_State *state, String_Builder *input_builder)
{
  b32 result = false;
  
  if (action.delta != 0 || action.flags != Text_Action_Flags_None)
  {
    Text_Cmd text_cmd = text_cmd_from_state_and_action(temp_arena, input_builder->buffer, state, &action);
    
    if (state->cursor != text_cmd.new_cursor &&
        state->mark != text_cmd.new_mark)
    {
      result = true;
    }
    
    state->cursor = text_cmd.new_cursor;
    state->mark = text_cmd.new_mark;
    str_build_replace(arena, temp_arena, 
                      input_builder, text_cmd.replace_str, text_cmd.replace_min, text_cmd.replace_max);
    if (text_cmd.copy_str.length > 0) 
    {
      os_set_clipboard_text(text_cmd.copy_str);
    }
  }
  
  return result;
}

void ui_input(Arena *builder_arena, Arena *temp_arena, String_Builder *str_builder, OS_Event_List *event_list)
{
  UI_Element *input = 0; 
  
  UI_Key key = ui_key_from_string(str_builder->buffer);
  
  ui_sizing_x(ui_grow()) 
    ui_parent(ui_make_element(key, str_null(), 
                              UI_Flags_DrawBorder|
                              UI_Flags_DrawRounded|
                              UI_Flags_DrawBackGround|
                              UI_Flags_ClipRectangle)) 
    ui_seed_key(ui_hash_key(key))
  {
    input = ui_make_element(key, 
                            str_builder->buffer, 
                            UI_Flags_DrawText|
                            UI_Flags_Clickable);
    ui_parent(input);
  }
  
  UI_Element_Out out = ui_element_out(input);
  
  if (out.element)
  {
    input->toggled = ui_element_toggle_focus(out);
    
    b32 key_action_happened = false;
    if (out.element->toggled)
    {
      for (OS_Event *event = event_list->first;
           event;
           event = event->next) 
      {
        if (event->kind == OS_Event_Kind_Pressed || event->kind == OS_Event_Kind_PressedRepeat) 
        {
          Text_Action text_action = text_action_from_event(event);
          if (ui_apply_text_action(builder_arena, temp_arena, text_action, &context->text_state, str_builder))
          {
            key_action_happened = true;
          }
        }
      }
    }
    
    if (!key_action_happened && (out.pressed || out.dragging))
    {
      V2f mouse_pos = os_get_mouse_position();
      V2f input_pos = {0};
      if (ui_try_get_element_top_left(input, &input_pos))
      {
        Index_u32 index = line_index_from_cursor_delta(mouse_pos.x - input_pos.x, input->text);
        if (out.dragging)
        {
          context->text_state.cursor = index.value;
        }
        else if (out.pressed) 
        {
          context->text_state.cursor = index.value;
          context->text_state.mark = index.value;
        }
      }
      
      if (input->parent)
      {
        UI_Element* parent = ui_get_cached_element(input->parent->key);
        if (parent && (parent->flags & UI_Flags_ClipRectangle))
        {
          V2f inner_size = v2f_sub(parent->size, v2f_add(parent->padding.left_top, parent->padding.right_bottom));
          f32 x_overflow = mouse_pos.x - (parent->pos.x + inner_size.x);
          if (x_overflow > 0) 
          {
            Index_u32 index = line_index_from_cursor_delta(x_overflow, input->text);
            input->text->content = str_from(input->text->content, index.value);
            context->text_state.cursor -= index.value;
            context->text_state.mark -= index.value;
          }
        }
      }
    }
    
    ui_push_draw_cursor_and_highlight(temp_arena, &context->defered_draw, *input->text, out.element->pos, input->toggled, &context->text_state);
  }
}
