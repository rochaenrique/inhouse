# fdo - Fast DO

## Library - `fdo_lib.h fdo_lib.c`
The task handling library/engine. 

Desired features: 
 - [ ] Date handling
 - [ ] Filtering
 - [ ] Sorting
 - [ ] Grouping
 - [ ] Actions on sets of tasks
 
## Cli - `fdo_macos_cli.h fdo_macos_cli.c`

A macos cli front end to the library. 
Uses posix/macos api's for OS calls, no platform layer intended for this. 
Mainly to use / test with before the full app is finished and for practicality.

Desired Features: 
 - [ ] new, update, remove, list Subcommands
 - [ ] Simple and powerfull subcommands with flexible argument handling
 - [ ] Comprehensive filtering on all commands
 - [ ] Good terminal UX
 - [ ] (Maybe) GUI mode with curses
 
## FdoApp - (did not begin)

A feature-full and very interactive GUI native application to organize stuff to do across time. 
With a well defined platform layer for easy porting.

Desired Features:
 - [ ] Flexible grid view of time blocks (hours, days, weeks, months)
 - [ ] Switch between time blocks quickly
 - [ ] Keyboard-centric interactivity with quick and intuitive commands
 - [ ] Move multiple selected tasks at the same time
 - [ ] Edit fields on multiple selected tasks
 - [ ] Quickly add a new and slot a new task whenever
 
# Development 

Everything is in development 

