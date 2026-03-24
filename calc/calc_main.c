#include <stdlib.h>

#include "raylib.h"
#include "base.c"

#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>


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
#define pf_assert(cond)										\
do { if (!(cond)) { fprintf(stdout, "%s:%d: ASSERT at %s()\n", __FILE__, __LINE__, __func__); abort(); } } while (0)

#define debug_break() __builtin_debugtrap()

#include "os.c"
#include "ui.c"

b32 global_debug = false;
u64 frame_counter = 0;

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


#define button_row(value) ui_defer_loop(push_button_row(value), pop_button_row())
void push_button_row()
{
	ui_set_next_layout_axis(UI_Axis_X); 
  ui_push_parent(ui_null_parent(UI_Flags_None));
  ui_push_sizing_x(ui_grow());
}

void pop_button_row()
{
  ui_pop_sizing_x();
  ui_pop_parent();
}

typedef enum Expr_Op
{
  Expr_Op_None,
  Expr_Op_Plus,
  Expr_Op_minus,
  Expr_Op_Mult,
  Expr_Op_Div,
} Expr_Op;

typedef enum Expr_Value_Kind
{
  Expr_Value_Kind_None,
  Expr_Value_Kind_Number,
  Expr_Value_Kind_Op,
} Expr_Value_Kind;


typedef struct Expr_Value
{
  Expr_Value_Kind kind;
  union
  {
    f64 number;
    Expr_Op op;
  };
} Expr_Value;

typedef struct Expr
{
  Expr_Value value;
  struct Expr *left;
  struct Expr *right;
  
  struct Expr *prev;
  struct Expr *next;
} Expr;

Expr_Op char_to_math_op_kind(char C)
{
  Expr_Op result = Expr_Op_None;
  
  switch (C)
  {
    case '+': result = Expr_Op_Plus;  break;
    case '-': result = Expr_Op_minus; break;
    case '*': result = Expr_Op_Mult;  break;
    case '/': result = Expr_Op_Div;   break;
  }
  
  return result;
}

u32 OpPrecedence(Expr_Op Kind)
{
  u32 precedence_table[] = {
    [Expr_Op_None]  = 0,
    [Expr_Op_Plus]  = 1,
    [Expr_Op_minus] = 1,
    [Expr_Op_Mult]  = 2,
    [Expr_Op_Div]   = 2,
  };
  
  u32 result = precedence_table[Kind];
  return result;
}

Expr_Value peek_expr(String str, u64 *consume_idx)
{
  Expr_Value result = {0};
  
  Index_u64 index = str_index_math_op(str);
  
  if (index.exists && index.value == 0)
  {
    result.kind = Expr_Value_Kind_Op;
    char ch = str.value[index.value];
    result.op = char_to_math_op_kind(ch); 
    
    index.value++;
    
    if (consume_idx != 0)
    {
      *consume_idx = 1;
    }
  }
  else 
  {
    result.kind = Expr_Value_Kind_Number;
    
    u64 number = 0;
    if (str_to_u64(str_clip(str, max(index.value, 1)), &number)) 
    {
      result.number = number;
    }
    else
    {
      result.number = -1;
    }
    
    if (consume_idx != 0)
    {
      *consume_idx = index.value;
    }
  }
  
  return result;
}

typedef struct Expr_List 
{
  Expr *first;
  Expr *last;
} Expr_List; 

void debug_print_expr_value(Expr_Value value)
{
  switch (value.kind) 
  {
    case Expr_Value_Kind_None: {} break;
    
    case Expr_Value_Kind_Number:
    {
      printf("number(%f)", value.number);
    } break;
    
    case Expr_Value_Kind_Op: 
    {
      printf("op(");
      switch (value.op)
      {
        case Expr_Op_None:  printf("none");  break;
        case Expr_Op_Plus:  printf("plus");  break;
        case Expr_Op_minus: printf("minus"); break;
        case Expr_Op_Mult:  printf("mult");  break;
        case Expr_Op_Div:   printf("div");   break;
      }
      printf(")");
    } break;
  }
}

void debug_print_expr(Expr *node)
{
  if (node)
  {
    
    if (node->left) 
    {
      printf("( ");
      debug_print_expr(node->left);
      printf(" )");
    }
    
    debug_print_expr_value(node->value);
    
    if (node->right) 
    {
      printf("( ");
      debug_print_expr(node->right);
      printf(" )");
    }
  }
}

void debug_print_list(Expr_List list)
{
  for (Expr *node = list.first;
       node;
       node = node->next)
  {
    debug_print_expr(node);
  }
  
  printf("\n\n");
}

b32 is_number_or_expr_safe(Expr *node)
{
  b32 result = false;
  if (node != 0) 
  {
    
    b32 is_number = (node && 
                     node->value.kind == Expr_Value_Kind_Number);
    b32 is_expr = (node && 
                   node->value.kind == Expr_Value_Kind_Op && 
                   (node->left != 0 || node->right != 0));
    result = is_number || is_expr;
  }
  return result;
}

Expr *parent_op_expr(Expr_List *list, Expr_Op op)
{
  Expr *parent = 0;
  
  // NOTE(erb): pull out operator
  for (Expr *node = list->first;
       node;
       node = node->next) 
  {
    if (node->value.kind == Expr_Value_Kind_Op && 
        node->value.op == op) 
    {
      Expr *prev = node->prev;
      Expr *next = node->next;
      if (is_number_or_expr_safe(prev) && is_number_or_expr_safe(next))
      {
        // NOTE(erb): build expr tree
        node->left = prev;
        node->right = next;
        
        // NOTE(erb): link with new prev (substitution)
        node->prev = prev->prev; 
        if (prev->prev)
        {
          prev->prev->next = node;
        }
        
        // note(erb): link with new next (substitution)
        node->next = next->next;
        if (next->next)
        {
          next->next->prev = node;
        }
        
        // note(erb): unlink children
        prev->prev = 0;
        prev->next = 0;
        next->prev = 0;
        next->next = 0;
        
        parent = node;
      }
    }
  }
  
  return parent;
}

Expr *parse_expr(Arena *arena, String str)
{
  Expr_List list = {0};
  
  String stream = str;
  
  // NOTE(erb): Build a tagged list
  for (;;) 
  {
    if (stream.length == 0)
    {
      break;
    }
    
    u64 consume = 0;
    Expr_Value peek = peek_expr(stream, &consume);
    stream = str_from(stream, max(consume, 1));
    
    Expr *last = list.last;
    Expr *node = sll_push_allocate(arena, &list, Expr);
    node->value = peek;
    node->prev = last;
  }
  
  // NOTE(erb): pull out operators
  Expr *result = 0;
  Expr *parent = 0;
  
  parent = parent_op_expr(&list, Expr_Op_Mult);
  if (parent != 0)
  {
    result = parent;
  }
  parent = parent_op_expr(&list, Expr_Op_Div);
  if (parent != 0)
  {
    result = parent;
  }
  parent = parent_op_expr(&list, Expr_Op_Plus);
  if (parent != 0)
  {
    result = parent;
  }
  parent = parent_op_expr(&list, Expr_Op_minus);
  if (parent != 0)
  {
    result = parent;
  }
  
  return result;
}

f64 eval_expr(Expr *node)
{
  f64 result = 0;
  if (node)
  {
    if (node->value.kind == Expr_Value_Kind_Number) 
    {
      result = node->value.number;
    }
    else if (node->value.kind == Expr_Value_Kind_Op) 
    {
      f64 left = eval_expr(node->left);
      f64 right = eval_expr(node->right);
      
      switch (node->value.op)
      {
        case Expr_Op_None: {}; break;
        case Expr_Op_Plus:  result = left + right; break;
        case Expr_Op_minus: result = left - right; break;
        case Expr_Op_Mult:  result = left * right; break;
        case Expr_Op_Div:   result = left / right; break;
      }
    }
  }
  return result;
}

b32 is_math_op_event(OS_Event *event)
{
  b32 result = ((is_math_op(event->key) && event->modifiers == OS_Modifier_Flags_None) ||
                (event->key == OS_Key_Code_Slash) ||
                (event->key == OS_Key_Code_Eight && (event->modifiers & OS_Modifier_Flags_Shift)));
  return result;
}

#if 0

void draw_stuff()


#endif

int main(void)
{
  Arena *scratch = allocate_arena(mb(256));
  Arena *frame_arena = allocate_arena(mb(256));
  ui_init(scratch);
  
  InitWindow(600, 600, "window");
  SetTargetFPS(60);
  
  String_Builder input_builder = {0};
  str_builder_grow_maybe(scratch, &input_builder, 512);
  
  while (!WindowShouldClose()) 
  {
    global_debug = IsKeyDown(KEY_TAB);
    OS_Event_List event_list = os_poll_events(frame_arena);
    b32 should_eval = false;
    
    for (OS_Event *event = event_list.first;
         event;
         event = event->next)
    {
      if (event->kind == OS_Event_Kind_Pressed &&
          ((is_digit(event->key) && event->modifiers == OS_Modifier_Flags_None) || 
           is_math_op_event(event)))
      {
        str_build_char(scratch, &input_builder, event->key);
      }
      else if (event->key == OS_Key_Code_Enter && 
               event->kind == OS_Event_Kind_Pressed)
      {
        should_eval = true;
      }
      else if (event->key == OS_Key_Code_Backspace && 
               event->kind == OS_Event_Kind_Pressed)
      {
        if (input_builder.buffer.length > 0) 
        {
          input_builder.buffer.length--;
        }
      }
      
    }
    
    ui_begin_frame(V4f(0, 0, GetScreenWidth(), GetScreenHeight()));
    {
      ui_set_next_layout_axis(UI_Axis_Y);
      ui_padding(V4ff(16)) 
        ui_background_color(V4f(0.1, 0.6, 0.2, 1))
        ui_sizing_x(ui_grow()) 
        ui_parent_zero() // NOTE(erb): ui stuff
      {
        
        ui_set_next_layout_axis(UI_Axis_X);
        ui_padding(V4ff(2))
          ui_sizing_x(ui_grow())
          ui_parent_zero()
        {
          ui_text(S("EXPR>"));
          ui_text(input_builder.buffer);
        }
        
#define str_button(str) \
do { if (ui_button((str)).clicked) str_build_str(scratch, &input_builder, (str)); } while (0)
        
        ui_set_next_layout_axis(UI_Axis_Y); 
        ui_background_color(V4f(0.5f, 0.5f, 0.5f, 1)) ui_parent_zero() 
        {
          
          button_row()
          {
            if (ui_button(S("DEL")).clicked)
            {
              if (input_builder.buffer.length > 0) 
              {
                input_builder.buffer.length--;
              }
            }
            
            if (ui_button(S("AC")).clicked)
            {
              input_builder.buffer.length = 0;
            }
            
            str_button(S("%"));
            str_button(S("/"));
          }
          
          String button_strings[] = {
            S("7"), S("8"), S("9"), S("*"),
            S("4"), S("5"), S("6"), S("-"),
            S("1"), S("2"), S("3"), S("+"),
          };
          
          
          for (u32 row = 0;
               row < 3;
               row++) 
          {
            button_row()
            {
              for (u32 col = 0;
                   col < 4;
                   col++) 
              {
                ui_seed_key((row+1)*(col+1))
                {
                  u32 index = row*4 + col;
                  str_button(button_strings[index]);
                }
              }
            }
          }
          
          
#undef str_button
        }
      }
    }
    
    if (should_eval)
    {
      Expr *root = parse_expr(frame_arena, input_builder.buffer);
      f64 result = eval_expr(root);
      
      char *resultStr = push_array(frame_arena, 256, char);
      snprintf(resultStr, 256, "%.3f", result);
      input_builder.buffer.length = 0;
      str_build_cstr(scratch, &input_builder, resultStr);
    }
    
    UI_Draw_Cmd_List draw_cmds = ui_calculate_and_draw(frame_arena);
    
    BeginDrawing();
    {
      ClearBackground(BLANK);
      ui_draw_commands_raylib(&draw_cmds);
    }
    EndDrawing();
    
    ui_end_frame();
  }
  
  CloseWindow();
  return 0;
}