#include "base.h"
#include "os.h"

void os_set_clipboard_text(String str)
{
  SetClipboardText(str_to_temp256_cstr(str));
}

v2f os_get_mouse_position() 
{
  v2f result = {0};
  Vector2 pos = GetMousePosition();
  result.x = pos.x;
  result.y = pos.y;
  return result;
}
