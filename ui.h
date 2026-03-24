/* Date = December 6th 2025 8:27 am */

#ifndef UI_H
#define UI_H

#include "base.h"

typedef enum UI_Flags
{
	UI_Flags_None = 0,
	UI_Flags_CrossOverlay = (1<<0),
	UI_Flags_DrawText = (1<<1),
	UI_Flags_DrawBackGround = (1<<2),
	UI_Flags_DrawBorder = (1<<3),
	UI_Flags_DrawRounded = (1<<4),
	UI_Flags_Clickable = (1<<5),
	UI_Flags_Floating = (1<<6),
  UI_Flags_ClipRectangle = (1<<7),
} UI_Flags;

typedef struct UI_Key
{
	u32 content;
	u32 iter;
  u64 parent;
} UI_Key;

typedef enum UI_Axis
{
  UI_Axis_X,
  UI_Axis_Y,
  UI_Axis_Count,
} UI_Axis;

typedef enum UI_Sizing
{
	UI_Sizing_Fit,
	UI_Sizing_Grow,
	UI_Sizing_Shrink,
	UI_Sizing_Fixed,
} UI_Sizing;

typedef struct UI_Sizing_Value
{
	UI_Sizing sizing;
	f32 value;
} UI_Sizing_Value;

typedef struct UI_Line_Node
{
  struct UI_Line_Node *next;
  String content;
} UI_Line_Node;

typedef Font RL_Font;

typedef struct UI_Text
{
	v4f color;
  u32 font_size;
  f32 line_spacing;
  f32 spacing;
  String content;
  RL_Font raylib_font;
  u32 line_count;
  UI_Line_Node *first_line;
  UI_Line_Node *last_line;
} UI_Text; 

typedef struct Text_Cmd
{
  i64 replace_min;
  i64 replace_max;
  
  String replace_str;
  String copy_str;
  i64 new_cursor;
  i64 new_mark;
} Text_Cmd;

typedef struct Text_Edit_State
{
  i64 cursor;
  i64 mark;
} Text_Edit_State;

typedef enum Text_Action_Flags
{
  Text_Action_Flags_None = 0,
  Text_Action_Flags_Copy                = (1 << 0),
  Text_Action_Flags_Paste               = (1 << 1),
  Text_Action_Flags_Cut                 = (1 << 2),
  Text_Action_Flags_Delete              = (1 << 3),
  Text_Action_Flags_KeepMark            = (1 << 4),
  Text_Action_Flags_ZeroDeltaWithSelect = (1 << 5),
  Text_Action_Flags_WordScan            = (1 << 6),
  Text_Action_Flags_DeltaPickSelectSide = (1 << 7),
} Text_Action_Flags;

typedef struct Text_Action
{
  Text_Action_Flags flags;
  i64 delta;
  i32 code_point;
} Text_Action;

typedef enum UI_Draw_Kind
{
	UI_Draw_Kind_Text,
	UI_Draw_Kind_Rect,
	UI_Draw_Kind_Rectlines,
  UI_Draw_Kind_BeginClipRect,
  UI_Draw_Kind_EndClipRect,
} UI_Draw_Kind;

typedef struct UI_Draw_Cmd 
{
	UI_Draw_Kind kind;
	v2f pos;
	v2f size;
	v4f color;
	f32 border_size;
	f32 corner_rounded_percent;
	
	RL_Font raylib_font;
	String content;
	f32 font_size;
	f32 spacing;
	
  struct UI_Draw_Cmd *next;
} UI_Draw_Cmd;

typedef struct UI_Draw_Cmd_List
{
  UI_Draw_Cmd *first;
  UI_Draw_Cmd *last;
} UI_Draw_Cmd_List;

typedef struct UI_Element
{
	v2f pos;
	v2f size;
	v2f min_size;
	v2f max_size;
  
	UI_Sizing sizing[UI_Axis_Count];
	UI_Axis layout_axis;
	
	UI_Flags flags;
	
	v4f background_color;
	v4f padding;
	v4f border_color;
	f32 border_size;
	f32 corner_rounded_percent;
  f32 child_gap;
  UI_Text *text;
  
  u32 children_count;
  
	struct UI_Element *first_child;
	struct UI_Element *last_child;
	struct UI_Element *parent;
	struct UI_Element *next;
	
	struct UI_Element *next_hash;
	
	UI_Key key;
  b32 toggled;
  u64 frames_pressed;
  
} UI_Element;

typedef struct UI_Element_Out
{
  UI_Element *element;
	b8 clicked;
	b8 hovered;
	b8 pressed;
  b8 pressed_outside;
  b8 dragging;
} UI_Element_Out;

typedef struct UI_Ctx
{
	Arena *arena;
	Arena *last_arena;
	UI_Element *element_cache[256];
  UI_Draw_Cmd_List defered_draw;
  Text_Edit_State text_state;
	
	UI_Element *current;
	UI_Element *root;
	v2f root_pos;
	u32 element_count;
	
  UI_Key active_element;
	UI_Key hot_element;
	
	// NOTE(erb): values stack
	
	u32 font_size_stack[16];
	u32 font_size_stack_count;
	
	u32 font_spacing_stack[16];
	u32 font_spacing_stack_count;
	
	u32 line_spacing_stack[16];
	u32 line_spacing_stack_count;
	
	u32 child_gap_stack[16];
	u32 child_gap_stack_count;
	
	v4f font_color_stack[16];
	u32 font_color_stack_count;
	
	v4f background_color_stack[16];
	u32 background_color_stack_count;
	
	v4f border_color_stack[16];
	u32 border_color_stack_count;
	
	v4f padding_stack[16];
	u32 padding_stack_count;
	
	UI_Sizing_Value sizing_x_stack[16];
	u32 sizing_x_stack_count;
	
	f32 border_size_stack[16];
	u32 border_size_stack_count;
	
	f32 corner_rounded_percent_stack[16];
	u32 corner_rounded_percent_stack_count;
  
  u64 seed_key_stack[16];
  u64 seed_key_stack_count;
	
	// NOTE(erb): immediatly setting values
	
	b32 has_next_layout_axis;
	UI_Axis next_layout_axis;
	
	b32 has_next_floating_pos;
	v2f next_floating_pos;
	
	b32 has_next_fixed_size;
	v2f next_fixed_size;
} UI_Ctx;

#endif //UI_H
