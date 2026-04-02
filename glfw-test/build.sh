#!/bin/sh

rm -rf *app.dylib

clang -Wall -Wextra -W -g2 \
-I`realpath ..` \
`realpath app.c` -shared -o app.dylib

clang -Wall -Wextra -W -g2 \
-framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL \
glfw-3.4.bin.MACOS/lib-arm64/libglfw3.a /usr/local/lib/libfreetype.dylib app.dylib \
-Ifreetype-2.10.0/include -Iglfw-3.4.bin.MACOS/include -I`realpath ..` \
`realpath main.c` -o main 

