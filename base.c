#include "base.h"

// //////////////////////////////////////////////
// NOTE(erb): bytes
// //////////////////////////////////////////////

u8 *copy_bytes(u8 *source, u8 *dest, u64 count)
{
	while (count-- > 0)
	{
		*dest++ = *source++;
	}
	return dest;
}

void set_bytes(u8 *bytes, u64 count, u8 value)
{
	while (count-- > 0)
	{
		*bytes++ = value;
	}
}

void zero_bytes(u8 *bytes, u64 count)
{
	set_bytes(bytes, count, 0);
}

// //////////////////////////////////////////////
// NOTE(erb): arena
// //////////////////////////////////////////////

void init_arena(Arena *arena, void *memory, u64 size)
{
	arena->size = size;
	arena->base = memory;
	arena->used = 0;
  set_bytes(memory, size, 0xEE);
}

void free_arena(Arena *arena)
{
  pf_free(arena->base);
  arena->used = 0;
  arena->size = 0;
}

Arena allocate_arena(u64 size)
{
	Arena arena = {0};
	void *memory = pf_allocate(size);
	init_arena(&arena, memory, size);
	return arena;
}

void release_arena(Arena *arena)
{
	arena->used = 0;
}

void *arena_push(Arena *arena, u64 size) 
{
  pf_assert(arena->used + size < arena->size);
  void *result = arena->base + arena->used;
  zero_bytes(result, size);
  arena->used += size;
  
  return result;
}

Arena push_sub_arena(Arena *parent, u64 size)
{
	Arena sub_arena = {0};
	void *memory = arena_push(parent, size);
	init_arena(&sub_arena, memory, size);
	return sub_arena;
}


// //////////////////////////////////////////////
// NOTE(erb): hash
// //////////////////////////////////////////////

u64 hash_string(String str)
{
	u64 hash = 0;
	i32 p = 31;
	i32 m = 1e9 + 9;
	u64 ppow = 1;
	
	for (u32 char_idx = 0; 
       char_idx < str.length;
       char_idx ++)
	{
		u8 c = str.value[char_idx];
		hash = (hash + (c - 'a' + 1) * ppow) % m;
		ppow = (ppow * p) % m;
	}
	return hash;
}

// //////////////////////////////////////////////
// NOTE(erb): char helpers
// //////////////////////////////////////////////

b32 is_digit(char c)
{
	b32 result = (c >= '0' && c <= '9');
	return result;
}

b32 is_math_op(char c)
{
  b32 result = (c == '+' || c == '-' || 
                c == '/' || c == '*');
  return result;
}

b32 is_blank(char c)
{
	b32 result = (c == ' ' || c == '\t' || c == '\n');
	return result;
}

b32 is_lower_letter(char ch)
{
	b32 result = (ch >= 'a' && ch <= 'z');
	return result;
}

b32 is_upper_letter(char Ch)
{
	b32 result = (Ch >= 'A' && Ch <= 'Z');
	return result;
}

b32 is_letter(char Ch)
{
	b32 result = is_lower_letter(Ch) || is_upper_letter(Ch);
	return result;
}

// //////////////////////////////////////////////
// NOTE(erb): string
// //////////////////////////////////////////////

char *cstr_copy(char *source, char *dest, u32 size_max)
{
	while (size_max > 0 && *source && *dest)
	{
		*dest++ = *source++;
		size_max--;
	}
	return dest;
}

char *cstr_trim_start(char *str)
{
	while (*str && (*str == ' ' || *str == '\n' || *str == '\t'))
	{
		str++;
	}
	return str;
}

b32 cstr_equals(char *a, char *b)
{
	b32 result = true;
	while (result && *a && *b)
	{
		result = (*a++ == *b++);
	}
  
	return result;
}

u64 cstr_length(char *str)
{
	char *str_it = str;
	while (*str_it++);
  
	u64 result = cap_bottom((u64)(str_it - str) - 1, 0);
	return result;
}

b32 cstr_to_u64(char *str, u64 *value)
{
	u64 max_value = ~(u64)0;
	*value = 0;
  
	while (*str 
         && is_digit(*str) 
         && *value < max_value)
	{
		*value = (*value)*10 + ((*str) - '0');
		str++;
	}
	
	b32 result = (*str == 0);
	return result;
}

b32 cstr_to_u32(char *str, u32 *value)
{
	u32 max_value = ~(u32)0;
	*value = 0;
  
	while (*str 
         && is_digit(*str) 
         && *value < max_value)
	{
		*value = (*value)*10 + ((*str) - '0');
		str++;
	}
	b32 result = (*str == 0);
	return result;
}

String str_null()
{
	String result = {0};
	return result;
}

String str_copy(Arena *arena, String source)
{
  String str = {0};
  
	str.value = push_array(arena, source.length, char);
	str.length = source.length;
	copy_bytes((u8 *)source.value, (u8 *)str.value, source.length);
  
	return str;
}

b32 str_equals(String str_a, String str_b)
{
	b32 result = false;
	
	if (str_a.length == str_b.length)
	{
		result = true;
		
		for (u64 Index = 0;
         Index < str_a.length && (result);
         Index++)
		{
			result = (str_a.value[Index] == str_b.value[Index]);
		}
	}
  
	return result;
}

String str_from(String str, u64 from_idx)
{
  String slice = {0};
	
	if (from_idx <= str.length)
	{
		slice.value = (str.value + from_idx);
		slice.length = str.length - from_idx;
	}
	
	return slice;
}

String str_clip(String str, u64 to_idx)
{
	String slice = {0};
	
	if (to_idx >= 0 && to_idx <= str.length)
	{
		slice.value = str.value;
		slice.length = to_idx;
	}
	
	return slice;
}

String str_slice(String str, u64 from_idx, u64 to_idx)
{
	String slice = {0};
	String to = str_clip(str, to_idx);
	if (to.length > 0)
	{
		slice = str_from(to, from_idx);
	}
	
	return slice;
}

Index_u64 str_index_char(String str, char ch)
{
	Index_u64 result = {0};
	for (u64 ch_idx = 0;
       ch_idx < str.length && !result.exists;
       ch_idx++)
	{
		if (ch == str.value[ch_idx])
		{
			result.value = ch_idx;
			result.exists = true;
		}
	}
  
	return result;
}

Index_u64 str_index_blank(String str)
{
	Index_u64 result = {0};
	for (u64 ch_idx = 0;
       ch_idx < str.length && !result.exists;
       ch_idx++)
	{
		if (is_blank(str.value[ch_idx]))
		{
			result.value = ch_idx;
			result.exists = true;
		}
	}
  
	return result;
}

Index_u64 str_index_not_blank(String str)
{
	Index_u64 result = {0};
	for (u64 ch_index = 0;
       ch_index < str.length && !result.exists;
       ch_index++)
	{
		if (!is_blank(str.value[ch_index]))
		{
			result.value = ch_index;
			result.exists = true;
		}
	}
  
	return result;
}

Index_u64 str_index_blank_backward(String str)
{
	Index_u64 result = {0};
	for (i64 ch_index = str.length - 1;
       ch_index >= 0 && !result.exists;
       ch_index--)
	{
		if (is_blank(str.value[ch_index]))
		{
			result.value = ch_index;
			result.exists = true;
		}
	}
  
	return result;
}

Index_u64 str_index_not_blank_backward(String str)
{
	Index_u64 result = {0};
	for (i64 ch_idx = str.length - 1;
       ch_idx >= 0 && !result.exists;
       ch_idx--)
	{
		if (!is_blank(str.value[ch_idx]))
		{
			result.value = ch_idx;
			result.exists = true;
		}
	}
  
	return result;
}

Index_u64 str_index_letter(String str)
{
	Index_u64 result = {0};
	for (u64 ch_idx = 0;
       ch_idx < str.length && !result.exists;
       ch_idx++)
	{
		if (is_letter(str.value[ch_idx]))
		{
			result.value = ch_idx;
			result.exists = true;
		}
	}
  
	return result;
}

Index_u64 str_index_digit(String str)
{
	Index_u64 result = {0};
	for (u64 ch_idx = 0;
       ch_idx < str.length && !result.exists;
       ch_idx++)
	{
		if (is_digit(str.value[ch_idx]))
		{
			result.value = ch_idx;
			result.exists = true;
		}
	}
  
	return result;
}

Index_u64 str_index_math_op(String str)
{
	Index_u64 result = {0};
	for (u64 ch_idx = 0;
       ch_idx < str.length && !result.exists;
       ch_idx++)
	{
		if (is_math_op(str.value[ch_idx]))
		{
			result.value = ch_idx;
			result.exists = true;
		}
	}
  
	return result;
}

String str_next_word(String str)
{
	String word = {0};
  
	Index_u64 word_begin_idx = str_index_not_blank(str);
	if (word_begin_idx.exists)
	{
		Index_u64 word_end_idx = str_index_blank(str_from(str, word_begin_idx.value+1));
		if (word_end_idx.exists)
		{
			word_end_idx.value++;
		}
		else
		{
			word_end_idx.value = str.length;
		}
    
		word = str_slice(str, word_begin_idx.value, word_end_idx.value);
	}
	
	return word;
}


Index_u64 str_index_word_end_backward(String str)
{
  Index_u64 result = {0};
  
	Index_u64 word_begin_idx = str_index_not_blank_backward(str);
	if (word_begin_idx.exists)
	{
    Index_u64 word_end_idx = str_index_blank_backward(str_clip(str, word_begin_idx.value));
		if (word_end_idx.exists)
		{
      result.value = word_end_idx.value+1;
      result.exists = true;
		}
    else
    {
      result.value = 0;
    }
	}
  
	return result;
}

Index_u64 str_index_word_end(String str)
{
  Index_u64 result = {0};
  
	Index_u64 word_begin_idx = str_index_not_blank(str);
	if (word_begin_idx.exists)
	{
    Index_u64 word_end_idx = str_index_blank(str_from(str, word_begin_idx.value));
		if (word_end_idx.exists)
		{
      result.value = word_end_idx.value + word_begin_idx.value;
      result.exists = true;
		}
    else
    {
      result.value = str.length;
    }
	}
  
	return result;
}

Index_u64 str_index_word_end_delta(String str, u32 start, i32 delta)
{
  Index_u64 result = {0};
  result.value = start;
  
  for (u32 scan_count = abs(delta);
       scan_count > 0; 
       scan_count--) 
  {
    Index_u64 scan = {0};
    if (delta > 0) 
    {
      scan = str_index_word_end(str_from(str, result.value));
      result.value += scan.value;
    }
    else
    {
      scan = str_index_word_end_backward(str_clip(str, result.value));
      result.value = scan.value;
    }
    
    if (scan.exists != 0) 
    {
      break;
    }
  }
  
  result.exists = true;
	return result;
}

b32 str_to_u64(String Str, u64 *value)
{
	u64 maxvalue = ~(u64)0;
	*value = 0;
  
	char *base = Str.value;
	for (u64 Index = 0; 
       Index < Str.length
       && is_digit(*base) 
       && *value < maxvalue;
       base++, Index++)
	{
		*value = (*value)*10 + ((*base) - '0');
	}
	
	b32 result = (base == Str.value+Str.length);
	return result;
}

Index_u64 str_try_to_u64(String str, u64 *value)
{
	u64 max_value = ~(u64)0;
	*value = 0;
  
	Index_u64 result = {0};
	Index_u64 digit_idx = str_index_digit(str);
	
	if (digit_idx.exists)
	{
		result.value = digit_idx.value;
		
		while (result.value < str.length 
           && *value < max_value)
		{
			char ch = str.value[result.value];
			if (is_digit(ch))
			{
				*value = (*value)*10 + (ch - '0');
				result.value++;
			}
			else
			{
				break;
			}
		}
	}
  
	result.exists = (result.value > digit_idx.value);
	return result;
}

char *str_to_temp256_cstr(String str)
{
	pf_assert(Str.length < 256);
	static char temp_buffer[256];
  
	copy_bytes((u8 *)str.value, (u8 *)temp_buffer, str.length);
	temp_buffer[str.length] = 0;
  
	return temp_buffer;
}

String push_cstr_copy(Arena *arena, char *cstr)
{
	u64 len = cstr_length(cstr);
  
	char *dest = push_array(arena, len, char);
	copy_bytes((u8 *)cstr, (u8 *)dest, len);
  
	String result = {0};
	result.value = dest;
	result.length = len;
	return result;
}

b32 str_builder_grow_maybe(Arena *arena, String_Builder *builder, u64 size)
{
	u64 total_size = builder->buffer.length + size;
	b32 result = total_size > builder->capacity;
	
	if (result)
	{
		total_size = max(total_size*2, 256);
		
		char *new_mem = push_array(arena, total_size, char);
		copy_bytes((u8 *)builder->buffer.value, (u8 *)new_mem, builder->buffer.length);
		
		builder->buffer.value = new_mem;
		builder->capacity = total_size;
	}
  
	return result;
}

String str_build_cstr(Arena *arena, String_Builder *builder, char *cstr)
{
	u64 len = cstr_length(cstr);
	str_builder_grow_maybe(arena, builder, len);
  
	char *dest = (builder->buffer.value + builder->buffer.length);
	copy_bytes((u8 *)cstr, (u8 *)dest, len);
	builder->buffer.length += len;
  
	String result = {0};
	result.value = dest;
	result.length = len;
	return result;
}


String str_build_char(Arena *arena, String_Builder *builder, char ch)
{
	str_builder_grow_maybe(arena, builder, 1);
  
	char *dest = (builder->buffer.value + builder->buffer.length);
	*dest = ch;
	builder->buffer.length++;
  
	String result = {0};
	result.value = dest;
	result.length = 1;
	return result;	
}

String str_build_str(Arena *arena, String_Builder *builder, String str)
{
	str_builder_grow_maybe(arena, builder, str.length);
  
	char *dest = (builder->buffer.value + builder->buffer.length);
	copy_bytes((u8 *)str.value, (u8 *)dest, str.length);
	builder->buffer.length += str.length;
  
	return str;
}

String str_build_replace(Arena *arena, Arena *temp_arena, String_Builder *builder, 
                         String replace_str, u64 replace_min, u64 replace_max)
{
  String result = {0};
  if (replace_min <= replace_max &&
      replace_min >= 0 && replace_min < builder->buffer.length && 
      replace_min >= 0 && replace_max <= builder->buffer.length) 
  {
    i32 diff = replace_str.length - (replace_max-replace_min);
    if (diff > 0) 
    {
      str_builder_grow_maybe(arena, builder, diff);
    }
    
    String post_replace = str_copy(temp_arena, str_from(builder->buffer, replace_max));
    u8 *replace_dest = (u8 *)(builder->buffer.value + replace_min);
    u8 *advance = replace_dest + replace_str.length;
    
    copy_bytes((u8 *)post_replace.value, advance, post_replace.length);
    copy_bytes((u8 *)replace_str.value, replace_dest, replace_str.length);
    
    builder->buffer.length += diff;
  }
  
  return result;
}

String join_cargs(Arena *arena, i32 arg_count, char **args)
{
	String_Builder builder = {0};
	
	for (i32 arg_idx = 0;
       arg_idx < arg_count;
       arg_idx++)
	{
		str_build_cstr(arena, &builder, args[arg_idx]);
		
		if (arg_idx < arg_count - 1)
		{
			str_build_char(arena, &builder, ' ');
		}
	}
  
	return builder.buffer;
}

// //////////////////////////////////////////////
// NOTE(erb): Date
// //////////////////////////////////////////////

b32 date_equals(Date a, Date b)
{
	b32 result = a.seconds == b.seconds
		&& a.minutes   == b.minutes
		&& a.hours     == b.hours
		&& a.month_day == b.month_day
		&& a.month     == b.month
		&& a.year      == b.year
		&& a.week_day  == b.week_day
		&& a.year_day  == b.year_day;
	return result;
}

Date add_day(Date date, u32 days)
{
	// TODO(enrique): Polish this to handle months with 31 days
	date.month_day = (date.month_day + days) % 31;
	date.month     = (date.month     + (days/31)) % 13 + 1;
	date.year      = (date.year      + (days/366));
	date.year_day  = (date.year_day  + days) % 366;
	date.week_day  = (date.week_day  + days) % 7;
	return date;
}

// //////////////////////////////////////////////
// NOTE(erb): math
// //////////////////////////////////////////////

v2f V2f(f32 x, f32 y)
{
	v2f result = {0};
	result.x = x;
	result.y = y;
	return result;
}

v2f V2ff(f32 v)
{
	v2f result = {0};
	result.x = result.y = v;
	return result;
}

v2f V2fi(v2i v)
{
	v2f result = {0};
  result.x = (i32)v.x;
	result.y = (i32)v.y;
	return result;
}

v2f v2f_add(v2f a, v2f b)
{
	v2f result = {0};
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return result;
}

v2f v2f_sub(v2f a, v2f b)
{
	v2f result = {0};
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}

v2f v2f_hada(v2f a, v2f b)
{
	v2f result = {0};
	result.x = a.x * b.x;
	result.y = a.y * b.y;
	return result;
}

v2f v2f_hada_div(v2f a, v2f b)
{
	v2f result = {0};
	result.x = a.x / b.x;
	result.y = a.y / b.y;
	return result;
}

v2f v2f_scale(v2f vec, f32 scalar) 
{
	v2f result = {0};
	result.x = vec.x * scalar;
	result.y = vec.y * scalar;
	return result;
}

v2f v2f_abs(v2f v) 
{
  v2f result = {0};
	result.x = abs(v.x);
	result.y = abs(v.y);
  return result;
}

// NOTE(erb): v2i

v2i V2i(i32 x, i32 y)
{
	v2i result = {0};
	result.x = x;
	result.y = y;
	return result;
}

v2i V2ii(i32 v)
{
	v2i result = {0};
	result.x = result.y = v;
	return result;
}

v2i v2i_add(v2i a, v2i b)
{
	v2i result = {0};
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return result;
}

v2i v2i_sub(v2i a, v2i b)
{
	v2i result = {0};
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}

// NOTE(erb): v3f

v3f V3f(f32 x, f32 y, f32 z)
{
	v3f result = {0};
  result.x = x;
  result.y = y;
  result.z = z;
	return result;
}

v3f V3ff(f32 v)
{
	v3f result = {0};
	result.x = result.y = result.z = v;
	return result;
}

// NOTE(erb): v4

v4f V4f(f32 x, f32 y, f32 z, f32 w)
{
	v4f result = {0};
  result.x = x;
  result.y = y;
  result.z = z;
  result.w = w;
	return result;
}

v4f V4ff(f32 v)
{
	v4f result = {0};
  result.x = result.y = result.z = result.w = v;
	
	return result;
}

v2f rect_size(v4f rect) 
{
  v2f result = {0};
  result = v2f_abs(v2f_sub(rect.p1, rect.p0));
  return result;
}
