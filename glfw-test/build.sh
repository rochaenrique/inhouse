#!/bin/sh

clang -Wall -Wextra -W -g2 \
-framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL \
glfw-3.4.bin.MACOS/lib-arm64/libglfw3.a /usr/local/lib/libfreetype.dylib \
-Ifreetype-2.10.0/include -Iglfw-3.4.bin.MACOS/include -I`realpath ..` \
`realpath main.c` -o main 

