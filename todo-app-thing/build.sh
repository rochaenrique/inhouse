#!/bin/sh

clang -Wall -Wextra -W -g2 -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL "../libraylib.a" `realpath fdo_raylib_mac.c` -o fdui -I"../raylib-5.5_macos/include" -I"../"
