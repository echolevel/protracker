#!/bin/sh

rm release/protracker &> /dev/null

echo Compiling, please wait...
cc src/*.c src/gfx/*.c -I/usr/local/include -L/usr/local/lib -lSDL2 -lm -Wall -Wno-unused-result -Wc++-compat -Wshadow -Winit-self -Wextra -Wunused -Wunreachable-code -Wredundant-decls -Wswitch-default -march=native -mtune=native -O3 -o release/protracker

rm src/*.o src/gfx/*.o &> /dev/null

echo Done! The binary \(protracker\) is in the folder named \'release\'.
echo To run it, type ./protracker in the release folder.
