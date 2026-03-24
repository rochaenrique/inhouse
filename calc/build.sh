#!/bin/sh

clang -Wall -Wextra -W -g2 -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL ../libraylib.a `realpath calc_main.c` -o calc -I`realpath ../raylib-5.5_macos/include` -I`realpath ../`
