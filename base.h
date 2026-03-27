#ifndef _H_BASE_
#define _H_BASE_

#include <stdint.h>
#include <unistd.h>

typedef uint8_t  b8;
typedef uint32_t b32;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

#define file_local static
#define function_persist static

#define false 0
#define true 1

#define array_size(arr) ((sizeof(arr) / sizeof(*(arr))))

#define S(str) (String){.value = str, .length = (sizeof(str) - 1) }

// TODO(erb): add proper f32 max value
#define MAX_f32 10000000000.0f

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define abs(x) (x < 0 ? -x : x)
#define cap_bottom(value, cap) max(value, cap)
#define cap_top(value, cap) min(value, cap)
#define cap(value, b, t) cap_top(cap_bottom(value, b), t)
#define cap01(value) cap(value, 0, 1)

#define kb(X) (X*1024)
#define mb(X) (kb(X)*1024)
#define gb(X) ((u64)mb(X)*1024)

#define swap(a, b, Type) \
do { Type swap_temp = a; a = b; b = swap_temp; } while (0)

#define DATE_PRINT_FMT "%s %2d/%2d"
#define DATE_PRINT_FMT_ARGS(Date) AbrWeekdayStrings[(Date).WeekDay], (Date).MonthDay, (Date).Month+1
#define STR_PRINT_FMT "%.*s"
#define STR_PRINT_FMT_ARGS(Str) (int)Str.length, Str.value

typedef struct Date
{
	u32 seconds;  // 0-60
	u32 minutes;  // 0-59
	u32 hours;    // 0-23
	u32 month_day; // 1-31
	u32 month;    // 0-11
	u32 year;     // 0-year
	u32 week_day;  // 0-6, Sunday = 0
	u32 year_day;  // 0-365
} Date; 

typedef struct Read_File_result
{
	void *data;
	u64 size;
} Read_File_result;

typedef struct String
{
	char *value;
	u64 length;
} String;

typedef struct Index_u64
{
	b32 exists;
	u64 value;
} Index_u64;

typedef struct Index_u32
{
  b32 exists;
  u32 value;
} Index_u32;

typedef struct String_Builder
{
	String buffer;
	u64 capacity;
} String_Builder;

typedef struct Arena
{
	u64 size;
	u64 used;
	void *base;
} Arena;

typedef union V2f
{
	struct { f32 x, y; };
	f32 c[2];
} V2f;

typedef union V2i
{
	struct { i32 x, y; };
  i32 c[2];
} V2i;

typedef union V3f
{
	struct { f32 x, y, z; };
  struct { V2f xy; f32 _pad_z; };
  struct { V2f _pad_x; f32 yz; };
	f32 c[3];
	
} V3f;

typedef union V4f
{
	struct { f32 x, y, z, w; };
  struct { V2f xy, zw; };
	struct { V3f xyz; f32 _pad_w; };
  
	struct { f32 r, g, b, a; };
  struct { V3f rgb; f32 _pad_a; };
  
	struct { V2f p0, p1; };
	struct { f32 left, top, right, bottom; };
	struct { V2f left_top, right_bottom; };
	f32 c[4];
} V4f;

// ////////////////////////////////////////////////
// PLATFORM pf
// ////////////////////////////////////////////////
#define pf_log(fmt, ...)  
#define pf_log_error(fmt, ...) 
#define pf_assert(condition) 

void *pf_allocate(u64 size);
void pf_free(void *);
Date pf_get_today();
// ////////////////////////////////////////////////

#define push_struct(arena, Type) ((Type *)arena_push((arena), sizeof(Type)))
#define push_array(arena, count, Type) ((Type *)arena_push((arena), sizeof(Type)*(count)))

#define sll_push_allocate(arena, list, Type) \
((!(list)->first) \
? ((list)->first = (list)->last = push_struct((arena), Type)) \
: ((list)->last = (list)->last->next = push_struct((arena), Type)))

#define copy_struct(src, dest) copy_bytes((u8 *)src, (u8 *)dest, sizeof(*src))

#define zero_struct(stru) zero_bytes(stru, sizeof(*stru))

#endif // _H_BASE_

