#include "raylib.h"
#include "fdo_core.c"

#include <stdio.h>
#include <stdlib.h>
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
#define pf_assert(Condition)										\
do { if (!(Condition)) { fprintf(stdout, "%s:%d: ASSERT at %s()\n", __FILE__, __LINE__, __func__); abort(); } } while (0)

#define DebugBreak() __builtin_debugtrap()

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

#include "fdo_os.c"
#include "fdo_ui.c"

b32 global_debug = false;
u64 frame_counter = 0;
u32 DebugCounter = 500;

string MacGetFdoFilePath(arena *arena)
{
	string Path = {0};
	char *EnvVar = getenv("HOME");
	if (EnvVar)
	{
		String_Builder builder = {0};
		str_build_cstr(arena, &builder, EnvVar);
		str_build_str(arena, &builder, S("/.fdo"));
		
		Path = builder.buffer;
	}
	
	return Path;
}

Date MacTmToDate(struct tm *Tm)
{
	Date result = {0};
	result.Seconds  = Tm->tm_sec;
	result.minutes  = Tm->tm_min;
	result.Hours    = Tm->tm_hour;
	result.MonthDay = Tm->tm_mday;
	result.Month    = Tm->tm_mon;
	result.year     = Tm->tm_year + 1900;
	result.WeekDay  = Tm->tm_wday;
	result.yearDay  = Tm->tm_yday;
	return result;
}

struct tm MacDateToTm(Date *Date)
{
	struct tm result = {0};
	result.tm_sec  = Date->Seconds;
	result.tm_min  = Date->minutes;
	result.tm_hour = Date->Hours;
	result.tm_mday = Date->MonthDay;
	result.tm_mon  = Date->Month;
	result.tm_year = Date->Year - 1900;
	result.tm_wday = Date->WeekDay;
	result.tm_yday = Date->YearDay;
	return result;
}

Date PlatformDateDayOffset(Date Date, i32 DayOffset)
{
	struct tm Tm = {0};
	Tm.tm_mday = Date.MonthDay; 
	Tm.tm_mon  = Date.Month;
	Tm.tm_year = Date.year - 1900;
  
	time_t Time = mktime(&Tm) + 24*60*60*DayOffset;
	struct tm *Tmresult = localtime(&Time);
  
	Date result = MacTmToDate(Tmresult);
	return result;
}

Date PlatformGetToday()
{
	struct timeval Today;
	pf_assert(gettimeofday(&Today, 0) == 0);
	struct tm *Tm = localtime(&Today.tv_sec);
	
	Date result = MacTmToDate(Tm);
	return result;
}

read_file_result MacReadFile(string FilePath)
{
	read_file_result result = {};
	i32 FileDescriptor = open(str_to_temp256_cstr(FilePath), O_RDONLY);
	if (FileDescriptor != -1)
	{
		struct stat Stat = {};
		if (fstat(FileDescriptor, &Stat) == 0)
		{
			u64 size = (u64)Stat.st_size;
			void *buffer = pf_allocate(size);
			pf_assert(read(FileDescriptor, buffer, size) != -1);
      
			result.Data = buffer;
			result.size = size;
		}
	}
	return result;
}

b32 MacWriteFdoToFile(fdo_state *State, string FilePath)
{
	b32 result = false;
	i32 FileDescriptor = open(str_to_temp256_cstr(FilePath), O_WRONLY|O_CREAT, 0666);
	if (FileDescriptor != -1)
	{
		task_view *TaskView = &State->TaskView;
		
		fdo_file_header Header = {};
		Header.LabelA = 'F';
		Header.LabelB = 'D';
		Header.LabelC = 'O';
		Header.TaskCount = TaskView->Count;
		Header.nextId = State->nextId;
		Header.maxTitlelength = State->maxTitlelength;
		Header.contentsOffset = sizeof(fdo_file_header);
		
		u64 Written = write(FileDescriptor, (void *)&Header, sizeof(fdo_file_header));
		pf_assert(Written == sizeof(fdo_file_header));
    
		for (u64 TaskIndex = 0;
         TaskIndex < TaskView->Count;
         TaskIndex++)
		{
			task *Task = TaskView->Tasks[TaskIndex];
			
			packed_task_data PackedTask = {0};
			PackedTask.Id = Task->Id;
			PackedTask.Priority = Task->Priority;
			PackedTask.Date = Task->Date;
			PackedTask.Titlelength = Task->Title.length;
			
			Written = write(FileDescriptor, (void *)&PackedTask, sizeof(PackedTask));
      pf_assert(Written == sizeof(PackedTask));
      
			Written = write(FileDescriptor, (void *)Task->Title.value, Task->Title.length*sizeof(char));
			pf_assert(Written == Task->Title.length*sizeof(char));
		}
		pf_assert(close(FileDescriptor) != -1);
    result = true;
	}
	else
	{
		pf_log("error: failed to open file '"STR_PRINT_FMT"' for writing\n", 
           STR_PRINT_FMT_ARGS(FilePath));
	}
  
	return result;
}


fdo_state PlatformLoadFdoState(read_file_result FdoFile)
{
	pf_assert(FdoFile.size > 0 && FdoFile.Data);
	fdo_state State = {0};
	
	fdo_file_header *Header = (fdo_file_header *)FdoFile.Data;
	b32 IsValid = (FdoFile.size >= sizeof(fdo_file_header))
		&& (Header->LabelA == 'F')
		&& (Header->LabelB == 'D')
		&& (Header->LabelC == 'O');
  
	if (IsValid)
	{
		void *TaskData = (FdoFile.Data + Header->contentsOffset);
		u64 memorysize = Header->TaskCount*sizeof(task) * 4;
		void *memory = pf_allocate(memorysize);
    
		init_arena(&State.arena, memory, memorysize);
    
		State.nextId = Header->nextId;
		State.maxTitlelength = Header->maxTitlelength;
    
		State.TaskView.capacity = Header->TaskCount * 2;
		State.TaskView.Tasks = push_array(&State.arena, State.TaskView.capacity, task *);
    
		u64 Offset = 0;
		for (u64 TaskIndex = 0;
         TaskIndex < Header->TaskCount;
         TaskIndex++)
		{
			packed_task_data *PackedTask = (packed_task_data *)(TaskData + Offset);
			char *TitleBytes = (char *)(TaskData + Offset + sizeof(packed_task_data));
      
			task *NewTask = push_struct(&State.arena, task);
			NewTask->Id = PackedTask->Id;
			NewTask->Priority = PackedTask->Priority;
			NewTask->Date = PackedTask->Date;
      
			NewTask->Title.value = push_array(&State.arena, PackedTask->Titlelength, char);
			NewTask->Title.length = PackedTask->Titlelength;
			CopyBytes((u8 *)TitleBytes, (u8 *)NewTask->Title.value, PackedTask->Titlelength);
      
			State.TaskView.Tasks[State.TaskView.Count++] = NewTask;
      
			Offset += (sizeof(packed_task_data) + PackedTask->Titlelength);
		}
    
		State.Initialized = true;
	}
	else
	{
		PlatformError("error: unexpected file format\n");
	}
  
	return State;
}

typedef int(compare_func)(const void *, const void *);

task_view *SortTasks(arena *frame_arena, task_view *TaskView, compare_func CompareFunc)
{
	task_view *CloneView = push_struct(frame_arena, task_view);
	CloneView->Tasks = push_array(frame_arena, TaskView->Count, task *);
	CopyBytes((u8 *)TaskView->Tasks, (u8 *)CloneView->Tasks, TaskView->Count*sizeof(task *));
	CloneView->capacity = TaskView->Count;
	CloneView->Count = TaskView->Count;
	
	qsort(CloneView->Tasks, CloneView->Count, sizeof(task *), CompareFunc);
  
	return CloneView;
}

i32 DateCmpTasksP(const void *TaskAP, const void *TaskBP)
{
	i32 result = 0;
	if (TaskAP && TaskBP)
	{
		task *TaskA = *((task **)TaskAP);
		task *TaskB = *((task **)TaskBP);
    
		struct tm TmA = MacDateToTm(&TaskA->Date);
		struct tm TmB = MacDateToTm(&TaskB->Date);
    
		result = mktime(&TmA) - mktime(&TmB);
	}
	return result;
}

// NOTE(enrique): This does NOT return the difference in days
i32 DayCompare(Date DateA, Date DateB)
{
	i32 result = (Datea.year - Dateb.year)*365 + (Datea.yearDay - Dateb.yearDay);
	return result;
}

void UITest()
{
  static b32 MenuBarOpen = false;
	static b32 OtherMenuBarOpen = false;
	
  ui_set_next_layout_axis(UI_Axis_X);
  ui_parent_zero()
  {
    ui_text(S("Very nice menu"));
    
    ui_set_next_layout_axis(UI_Axis_Y);
    ui_parent_zero()
    {
      if (ui_button(S("File")).clicked)
      {
        MenuBarOpen = !MenuBarOpen;
      }
      
      if (MenuBarOpen)
      {
        ui_set_next_layout_axis(UI_Axis_Y);
        ui_background_color(v4f(.5f, .5f, .5f, 1)) ui_parent_flagged(UI_Flags_CrossOverlay)
        {
          ui_button(S("New Tab"));
          
          ui_set_next_layout_axis(UI_Axis_X);
          ui_parent_zero()
          {
            if (ui_button(S("New Window")).clicked)
            {
              OtherMenuBarOpen = !OtherMenuBarOpen;
            }
            
            if (OtherMenuBarOpen)
            {
              ui_set_next_layout_axis(UI_Axis_Y);
              ui_parent_zero()
              {
                ui_button(S("Here"));
                ui_button(S("Other"));
              }
            }
            
          }
          
          ui_button(S("New Other thing"));
          
        }
      }
      
    }
    
    ui_text(S("4coder project: fdo"));
  }
}

int main(void)
{
	arena *scratch = allocate_arena(MB(256));
  arena *frame_arena = allocate_arena(MB(256));
	
	ui_init(scratch);
	
  //SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(400, 300, "window");
  
  fdo_state FdoState = {0};
  
  SetTargetFPS(60);
	
  String_Builder input_builder = {0};
  str_builder_grow_maybe(scratch, &input_builder, 512);
  str_build_str(scratch, &input_builder, S("the quick brown fox jumps over the lazy dog"
                                           "the quick brown fox jumps over the lazy dog"
                                           "the quick brown fox jumps over the lazy dog"));
  while (!WindowShouldClose())
  {
    OS_Event_List event_list = os_poll_events(frame_arena);
    
    global_debug = IsKeyDown(KEY_TAB);
		ui_begin_frame(v4f(0, 0, GetScreenWidth(), GetScreenHeight()));
		
    if (!FdoState.Initialized)
    {
			
#if 1
			u32 Initialmemorysize = sizeof(task) * 200;
			void *memory = pf_allocate(Initialmemorysize);
			init_arena(&FdoState.arena, memory, Initialmemorysize);
			
			FdoState.nextId = 0;
			FdoState.TaskView.Tasks = push_array(&FdoState.arena, 100, task *);
			FdoState.TaskView.capacity = 100;
			FdoState.TaskView.Count = 0;
			FdoState.Initialized = true;
			
			Date Today = PlatformGetToday();
			
			AddTask(&FdoState, S("One two three task!"), Today, 0);
			/* 
						AddTask(&FdoState, S("Two three task!"), Today, 0);
						AddTask(&FdoState, S("Three task!"), Today, 0);
			 */
#else
      string FdoFilePath = MacGetFdoFilePath(scratch);
      if (FdoFilePath.length > 0)
      {
        read_file_result FdoFile = MacReadFile(FdoFilePath);
        if (FdoFile.size > 0)
        {
          FdoState = PlatformLoadFdoState(FdoFile);
        }
        else
        {
          // TODO(enrique): Log error
        }
      }
      else
      {
        // TODO(enrique): Log error
      }
#endif
    }
		
		//task_view *DateSortedTasks = SortTasks(frame_arena, &FdoState.TaskView, DateCmpTasksP);
		
#if 0
    UITest();
#endif
    
    ui_set_next_rect(v4f(0, 0, GetScreenWidth(), GetScreenHeight()));
    ui_set_next_layout_axis(UI_Axis_Y);
    ui_padding(v4f(32, 32, 32, 32)) ui_parent_zero()
    {
      
      ui_background_color(v4f(0.1, 0.6, 0.2, 1))
      {
        ui_padding(v4f(2, 2, 2, 2))
        {
          ui_input(scratch, frame_arena, &input_builder, &event_list);
        }
        
        ui_toggle(S("times"));
      }
    }
    
    UI_Draw_Cmd_List draw_cmds = ui_calculate_and_draw(frame_arena);
    
    BeginDrawing();
    {
      ClearBackground(BLANK);
      DrawUICommands(&draw_cmds);
    }
    EndDrawing();
    
    ui_end_frame();
    release_arena(frame_arena);
    frame_counter++;
  }
  
  CloseWindow();
  return 0;
} 
