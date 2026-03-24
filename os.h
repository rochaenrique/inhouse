/* Date = February 19th 2026 11:00 am */

#ifndef OS_H
#define OS_H

typedef enum OS_Key_Code
{
  OS_Key_Code_None = 0,
  OS_Key_Code_Apostrophe      = 39,
  OS_Key_Code_Comma           = 44,
  OS_Key_Code_minus           = 45,
  OS_Key_Code_Period          = 46,
  OS_Key_Code_Slash           = 47,
  OS_Key_Code_Zero            = 48,
  OS_Key_Code_One             = 49,
  OS_Key_Code_Two             = 50,
  OS_Key_Code_Three           = 51,
  OS_Key_Code_Four            = 52,
  OS_Key_Code_Five            = 53,
  OS_Key_Code_Six             = 54,
  OS_Key_Code_Seven           = 55,
  OS_Key_Code_Eight           = 56,
  OS_Key_Code_Nine            = 57,
  OS_Key_Code_Semicolon       = 59,
  OS_Key_Code_Equal           = 61,
  OS_Key_Code_A               = 65,
  OS_Key_Code_B               = 66,
  OS_Key_Code_C               = 67,
  OS_Key_Code_D               = 68,
  OS_Key_Code_E               = 69,
  OS_Key_Code_F               = 70,
  OS_Key_Code_G               = 71,
  OS_Key_Code_H               = 72,
  OS_Key_Code_I               = 73,
  OS_Key_Code_J               = 74,
  OS_Key_Code_K               = 75,
  OS_Key_Code_L               = 76,
  OS_Key_Code_M               = 77,
  OS_Key_Code_N               = 78,
  OS_Key_Code_O               = 79,
  OS_Key_Code_P               = 80,
  OS_Key_Code_Q               = 81,
  OS_Key_Code_R               = 82,
  OS_Key_Code_S               = 83,
  OS_Key_Code_T               = 84,
  OS_Key_Code_U               = 85,
  OS_Key_Code_V               = 86,
  OS_Key_Code_W               = 87,
  OS_Key_Code_X               = 88,
  OS_Key_Code_Y               = 89,
  OS_Key_Code_Z               = 90,
  OS_Key_Code_LeftBracket     = 91,
  OS_Key_Code_Backslash       = 92,
  OS_Key_Code_RightBracket    = 93,
  OS_Key_Code_Backtick        = 96,
  OS_Key_Code_Space           = 32,
  OS_Key_Code_Escape          = 256,
  OS_Key_Code_Enter           = 257,
  OS_Key_Code_Tab             = 258,
  OS_Key_Code_Backspace       = 259,
  OS_Key_Code_Insert          = 260,
  OS_Key_Code_Delete          = 261,
  OS_Key_Code_Right           = 262,
  OS_Key_Code_Left            = 263,
  OS_Key_Code_Down            = 264,
  OS_Key_Code_Up              = 265,
  OS_Key_Code_PageUp          = 266,
  OS_Key_Code_PageDown        = 267,
  OS_Key_Code_Home            = 268,
  OS_Key_Code_End             = 269,
  OS_Key_Code_CapsLock        = 280,
  OS_Key_Code_ScrollLock      = 281,
  OS_Key_Code_NumLock         = 282,
  OS_Key_Code_PrintScreen     = 283,
  OS_Key_Code_Pause           = 284,
  OS_Key_Code_F1              = 290,
  OS_Key_Code_F2              = 291,
  OS_Key_Code_F3              = 292,
  OS_Key_Code_F4              = 293,
  OS_Key_Code_F5              = 294,
  OS_Key_Code_F6              = 295,
  OS_Key_Code_F7              = 296,
  OS_Key_Code_F8              = 297,
  OS_Key_Code_F9              = 298,
  OS_Key_Code_F10             = 299,
  OS_Key_Code_F11             = 300,
  OS_Key_Code_F12             = 301,
  OS_Key_Code_LeftShift       = 340,
  OS_Key_Code_LeftControl     = 341,
  OS_Key_Code_LeftAlt         = 342,
  OS_Key_Code_LeftSuper       = 343,
  OS_Key_Code_RightShift      = 344,
  OS_Key_Code_RightControl    = 345,
  OS_Key_Code_RightAlt        = 346,
  OS_Key_Code_RightSuper      = 347,
  OS_Key_Code_COUNT,
} OS_Key_Code;

typedef enum OS_Modifier_Flags
{
  OS_Modifier_Flags_None = 0,
  OS_Modifier_Flags_Shift = (1 << 0),
  OS_Modifier_Flags_Ctrl  = (1 << 1),
  OS_Modifier_Flags_Alt   = (1 << 2),
  OS_Modifier_Flags_Super = (1 << 3),
} OS_Modifier_Flags;

typedef enum OS_Event_kind 
{
  OS_Event_Kind_None = 0,
  OS_Event_Kind_Pressed,
  OS_Event_Kind_PressedRepeat,
  OS_Event_Kind_Down,
  OS_Event_Kind_Released,
  OS_Event_Kind_Up,
} OS_Event_kind;

typedef struct OS_Event 
{
  OS_Key_Code key;
  OS_Modifier_Flags modifiers; 
  OS_Event_kind kind;
  
  struct OS_Event *next;
} OS_Event;

typedef struct OS_Event_List
{
  OS_Event *first;
  OS_Event *last;
} OS_Event_List;

int ray_key_value_mapping[] = {
  [KEY_NULL] = OS_Key_Code_None,
  [KEY_APOSTROPHE] = OS_Key_Code_Apostrophe,
  [KEY_COMMA] = OS_Key_Code_Comma,
  [KEY_MINUS] = OS_Key_Code_minus,
  [KEY_PERIOD] = OS_Key_Code_Period,
  [KEY_SLASH] = OS_Key_Code_Slash,
  [KEY_ZERO] = OS_Key_Code_Zero,
  [KEY_ONE] = OS_Key_Code_One,
  [KEY_TWO] = OS_Key_Code_Two,
  [KEY_THREE] = OS_Key_Code_Three,
  [KEY_FOUR] = OS_Key_Code_Four,
  [KEY_FIVE] = OS_Key_Code_Five,
  [KEY_SIX] = OS_Key_Code_Six,
  [KEY_SEVEN] = OS_Key_Code_Seven,
  [KEY_EIGHT] = OS_Key_Code_Eight,
  [KEY_NINE] = OS_Key_Code_Nine,
  [KEY_SEMICOLON] = OS_Key_Code_Semicolon,
  [KEY_EQUAL] = OS_Key_Code_Equal,
  [KEY_A] = OS_Key_Code_A,
  [KEY_B] = OS_Key_Code_B,
  [KEY_C] = OS_Key_Code_C,
  [KEY_D] = OS_Key_Code_D,
  [KEY_E] = OS_Key_Code_E,
  [KEY_F] = OS_Key_Code_F,
  [KEY_G] = OS_Key_Code_G,
  [KEY_H] = OS_Key_Code_H,
  [KEY_I] = OS_Key_Code_I,
  [KEY_J] = OS_Key_Code_J,
  [KEY_K] = OS_Key_Code_K,
  [KEY_L] = OS_Key_Code_L,
  [KEY_M] = OS_Key_Code_M,
  [KEY_N] = OS_Key_Code_N,
  [KEY_O] = OS_Key_Code_O,
  [KEY_P] = OS_Key_Code_P,
  [KEY_Q] = OS_Key_Code_Q,
  [KEY_R] = OS_Key_Code_R,
  [KEY_S] = OS_Key_Code_S,
  [KEY_T] = OS_Key_Code_T,
  [KEY_U] = OS_Key_Code_U,
  [KEY_V] = OS_Key_Code_V,
  [KEY_W] = OS_Key_Code_W,
  [KEY_X] = OS_Key_Code_X,
  [KEY_Y] = OS_Key_Code_Y,
  [KEY_Z] = OS_Key_Code_Z,
  [KEY_LEFT_BRACKET] =OS_Key_Code_LeftBracket,
  [KEY_BACKSLASH] = OS_Key_Code_Backslash,
  [KEY_RIGHT_BRACKET] =OS_Key_Code_RightBracket,
  [KEY_GRAVE] = OS_Key_Code_Backtick,
  [KEY_SPACE] = OS_Key_Code_Space,
  [KEY_ESCAPE] = OS_Key_Code_Escape,
  [KEY_ENTER] = OS_Key_Code_Enter,
  [KEY_TAB] = OS_Key_Code_Tab,
  [KEY_BACKSPACE] = OS_Key_Code_Backspace,
  [KEY_INSERT] = OS_Key_Code_Insert,
  [KEY_DELETE] = OS_Key_Code_Delete,
  [KEY_RIGHT] = OS_Key_Code_Right,
  [KEY_LEFT] = OS_Key_Code_Left,
  [KEY_DOWN] = OS_Key_Code_Down,
  [KEY_UP] = OS_Key_Code_Up,
  [KEY_PAGE_UP] = OS_Key_Code_PageUp,
  [KEY_PAGE_DOWN] = OS_Key_Code_PageDown,
  [KEY_HOME] = OS_Key_Code_Home,
  [KEY_END] = OS_Key_Code_End,
  [KEY_CAPS_LOCK] = OS_Key_Code_CapsLock,
  [KEY_SCROLL_LOCK] = OS_Key_Code_ScrollLock,
  [KEY_NUM_LOCK] = OS_Key_Code_NumLock,
  [KEY_PRINT_SCREEN] = OS_Key_Code_PrintScreen,
  [KEY_PAUSE] = OS_Key_Code_Pause,
  [KEY_F1] = OS_Key_Code_F1,
  [KEY_F2] = OS_Key_Code_F2,
  [KEY_F3] = OS_Key_Code_F3,
  [KEY_F4] = OS_Key_Code_F4,
  [KEY_F5] = OS_Key_Code_F5,
  [KEY_F6] = OS_Key_Code_F6,
  [KEY_F7] = OS_Key_Code_F7,
  [KEY_F8] = OS_Key_Code_F8,
  [KEY_F9] = OS_Key_Code_F9,
  [KEY_F10] = OS_Key_Code_F10,
  [KEY_F11] = OS_Key_Code_F11,
  [KEY_F12] = OS_Key_Code_F12,
  [KEY_LEFT_SHIFT] =  OS_Key_Code_LeftShift,
  [KEY_LEFT_CONTROL] = OS_Key_Code_LeftControl,
  [KEY_LEFT_ALT] = OS_Key_Code_LeftAlt,
  [KEY_LEFT_SUPER] = OS_Key_Code_LeftSuper,
  [KEY_RIGHT_SHIFT] = OS_Key_Code_LeftShift,
  [KEY_RIGHT_CONTROL] = OS_Key_Code_LeftControl,
  [KEY_RIGHT_ALT] = OS_Key_Code_LeftAlt,
  [KEY_RIGHT_SUPER] = OS_Key_Code_LeftSuper,
};

char *get_key_name(OS_Key_Code key)
{
  switch (key) 
  {
    case OS_Key_Code_None           : return "None           ";
    case OS_Key_Code_Apostrophe     : return "Apostrophe     ";
    case OS_Key_Code_Comma          : return "Comma          ";
    case OS_Key_Code_minus          : return "minus          ";
    case OS_Key_Code_Period         : return "Period         ";
    case OS_Key_Code_Slash          : return "Slash          ";
    case OS_Key_Code_Zero           : return "Zero           ";
    case OS_Key_Code_One            : return "One            ";
    case OS_Key_Code_Two            : return "Two            ";
    case OS_Key_Code_Three          : return "Three          ";
    case OS_Key_Code_Four           : return "Four           ";
    case OS_Key_Code_Five           : return "Five           ";
    case OS_Key_Code_Six            : return "Six            ";
    case OS_Key_Code_Seven          : return "Seven          ";
    case OS_Key_Code_Eight          : return "Eight          ";
    case OS_Key_Code_Nine           : return "Nine           ";
    case OS_Key_Code_Semicolon      : return "Semicolon      ";
    case OS_Key_Code_Equal          : return "Equal          ";
    case OS_Key_Code_A              : return "A              ";
    case OS_Key_Code_B              : return "B              ";
    case OS_Key_Code_C              : return "C              ";
    case OS_Key_Code_D              : return "D              ";
    case OS_Key_Code_E              : return "E              ";
    case OS_Key_Code_F              : return "F              ";
    case OS_Key_Code_G              : return "G              ";
    case OS_Key_Code_H              : return "H              ";
    case OS_Key_Code_I              : return "I              ";
    case OS_Key_Code_J              : return "J              ";
    case OS_Key_Code_K              : return "K              ";
    case OS_Key_Code_L              : return "L              ";
    case OS_Key_Code_M              : return "M              ";
    case OS_Key_Code_N              : return "N              ";
    case OS_Key_Code_O              : return "O              ";
    case OS_Key_Code_P              : return "P              ";
    case OS_Key_Code_Q              : return "Q              ";
    case OS_Key_Code_R              : return "R              ";
    case OS_Key_Code_S              : return "S              ";
    case OS_Key_Code_T              : return "T              ";
    case OS_Key_Code_U              : return "U              ";
    case OS_Key_Code_V              : return "V              ";
    case OS_Key_Code_W              : return "W              ";
    case OS_Key_Code_X              : return "X              ";
    case OS_Key_Code_Y              : return "Y              ";
    case OS_Key_Code_Z              : return "Z              ";
    case OS_Key_Code_LeftBracket    : return "LeftBracket    ";
    case OS_Key_Code_Backslash      : return "Backslash      ";
    case OS_Key_Code_RightBracket   : return "RightBracket   ";
    case OS_Key_Code_Backtick       : return "Backtick       ";
    case OS_Key_Code_Space          : return "Space          ";
    case OS_Key_Code_Escape         : return "Escape         ";
    case OS_Key_Code_Enter          : return "Enter          ";
    case OS_Key_Code_Tab            : return "Tab            ";
    case OS_Key_Code_Backspace      : return "Backspace      ";
    case OS_Key_Code_Insert         : return "Insert         ";
    case OS_Key_Code_Delete         : return "Delete         ";
    case OS_Key_Code_Right          : return "Right          ";
    case OS_Key_Code_Left           : return "Left           ";
    case OS_Key_Code_Down           : return "Down           ";
    case OS_Key_Code_Up             : return "Up             ";
    case OS_Key_Code_PageUp         : return "PageUp         ";
    case OS_Key_Code_PageDown       : return "PageDown       ";
    case OS_Key_Code_Home           : return "Home           ";
    case OS_Key_Code_End            : return "End            ";
    case OS_Key_Code_CapsLock       : return "CapsLock       ";
    case OS_Key_Code_ScrollLock     : return "ScrollLock     ";
    case OS_Key_Code_NumLock        : return "NumLock        ";
    case OS_Key_Code_PrintScreen    : return "PrintScreen    ";
    case OS_Key_Code_Pause          : return "Pause          ";
    case OS_Key_Code_F1             : return "F1             ";
    case OS_Key_Code_F2             : return "F2             ";
    case OS_Key_Code_F3             : return "F3             ";
    case OS_Key_Code_F4             : return "F4             ";
    case OS_Key_Code_F5             : return "F5             ";
    case OS_Key_Code_F6             : return "F6             ";
    case OS_Key_Code_F7             : return "F7             ";
    case OS_Key_Code_F8             : return "F8             ";
    case OS_Key_Code_F9             : return "F9             ";
    case OS_Key_Code_F10            : return "F10            ";
    case OS_Key_Code_F11            : return "F11            ";
    case OS_Key_Code_F12            : return "F12            ";
    case OS_Key_Code_LeftShift      : return "LeftShift      ";
    case OS_Key_Code_LeftControl    : return "LeftControl    ";
    case OS_Key_Code_LeftAlt        : return "LeftAlt        ";
    case OS_Key_Code_LeftSuper      : return "LeftSuper      ";
    case OS_Key_Code_RightShift     : return "RightShift     ";
    case OS_Key_Code_RightControl   : return "RightControl   ";
    case OS_Key_Code_RightAlt       : return "RightAlt       ";
    case OS_Key_Code_RightSuper     : return "RightSuper     ";
    default: return "Unknown";
  }
};

void os_debug_print_event(OS_Event *event)
{
  pf_log("E: %s", get_key_name(event->key));
  
  pf_log(" K: ");
  
  if (event->kind == OS_Event_Kind_Pressed)
  {
    pf_log("Pressed");
  }
  else if (event->kind == OS_Event_Kind_PressedRepeat)
  {
    pf_log("PressedRepeat");
  }
  else if (event->kind == OS_Event_Kind_Down)
  {
    pf_log("Down");
  }
  else if (event->kind == OS_Event_Kind_Released)
  {
    pf_log("Released");
  }
  else if (event->kind == OS_Event_Kind_Up)
  {
    pf_log("Up");
  }
  
  pf_log(" M:");
  if (event->modifiers & OS_Modifier_Flags_Shift)
  {
    pf_log("Shift;");
  }
  if (event->modifiers & OS_Modifier_Flags_Ctrl)
  {
    pf_log("Ctrl;");
  }
  if (event->modifiers & OS_Modifier_Flags_Alt)
  {
    pf_log("Alt;");
  }
  if (event->modifiers & OS_Modifier_Flags_Super)
  {
    pf_log("Super;");
  }
  pf_log("\n");
}

OS_Event_List os_poll_events(Arena *arena) 
{
  OS_Event_List event_list = {0};
  
  for (u32 ray_key = 0; 
       ray_key < array_size(ray_key_value_mapping); 
       ray_key++)
  {
    OS_Key_Code os_key = ray_key_value_mapping[ray_key];
    if (os_key != OS_Key_Code_None) 
    {
      OS_Event_kind kind = OS_Event_Kind_None;
      
      if (IsKeyPressed(ray_key))
      {
        kind = OS_Event_Kind_Pressed;
      }
      else if (IsKeyPressedRepeat(ray_key))
      {
        kind = OS_Event_Kind_PressedRepeat;
      }
      else if (IsKeyDown(ray_key))
      {
        kind = OS_Event_Kind_Down;
      }
      else if (IsKeyReleased(ray_key))
      {
        kind = OS_Event_Kind_Released; 
      }
      else if (IsKeyUp(ray_key))
      {
        kind = OS_Event_Kind_Up;
      }
      
      if (kind != OS_Event_Kind_Up && kind != OS_Event_Kind_Released) 
      {
        OS_Event *event = sll_push_allocate(arena, &event_list, OS_Event);
        event->key = os_key;
        event->kind = kind;
        event->modifiers |= (IsKeyDown(KEY_LEFT_SHIFT)   || IsKeyDown(KEY_RIGHT_SHIFT))   * OS_Modifier_Flags_Shift;
        event->modifiers |= (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) * OS_Modifier_Flags_Ctrl;
        event->modifiers |= (IsKeyDown(KEY_LEFT_ALT)     || IsKeyDown(KEY_RIGHT_ALT))     * OS_Modifier_Flags_Alt;
        event->modifiers |= (IsKeyDown(KEY_LEFT_SUPER)   || IsKeyDown(KEY_RIGHT_SUPER))   * OS_Modifier_Flags_Super;
      }
    }
  }
  
#if 1
  for (OS_Event *event = event_list.first;
       event;
       event = event->next) 
  {
    os_debug_print_event(event);
  }
#endif
  return event_list;
}

String os_push_clipboard_text(Arena *arena)
{
  String result = 
    push_cstr_copy(arena, (char *)GetClipboardText());
  return result;
}

#endif //OS_H
