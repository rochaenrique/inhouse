# Minimum Necessary
Stuff to do so it's basically a reduced raylib

 - [ ] Text rendering
   - [ ] Make sure text is rendering correctly meaning the position in render_text(pos) should be the top left of the bounding box of the resulting text
   - [ ] Change render_text to accept a text height in pixels
   - [ ] Add spacing
   - [ ] Implement ui_measure_str_size()

 - [ ] Memory Management
   - [ ] Switch to mmap instead of malloc and free
   - [ ] Smart temp arena system
   - [ ] Custom GLFW allocator

- [ ] Hot reloading
   - [ ] Hot reload app code dll (not platform code)
   - [ ] Hot reload shaders
   - [ ] Hot reload textures

 - [ ] OS Utilities
   - [ ] Low level timer / clock
   - [ ] Read entire file
   - [ ] Datetime support
   - [ ] Executable path/directory
   - [ ] CPU Information (page faults, cache misses, cycle counter, etc)

 - [ ] Window Details
   - [ ] Max frame time 
   - [ ] Debug UI tree / table

 - [ ] Rects improvements
   - [ ] Rect lines
   - [ ] Gradients

 - [ ] Fix Pixel Coordinate System
   - [ ] Figure out app space vs screen space
   - [ ] Check how raylib does it

 - [ ] Handle UTF8
   - [ ] rendering
   - [ ] strings 

 - [ ] Profiler

# TEST APPS
 - File Explorer (dir walker)
 - Calendar app (calendar notebook)
