@ECHO OFF

REM -- compile script for MinGW --

REM Change this path to where you have MinGW\bin.
SET PATH=%PATH%;C:\MinGW\bin

del release\protracker.exe 2>NUL

windres src\protracker.rc src\resource.o

echo Compiling, please wait...
mingw32-gcc src\*.c src\gfx\*.c src/resource.o -lmingw32 -lSDL2main -mwindows -lSDL2 -lm -Wall -Wno-unused-result -Wshadow -Winit-self -Wextra -Wunused -Wunreachable-code -Wredundant-decls -Wswitch-default -march=native -mtune=native -O3 -s -o release\protracker.exe
del src\*.o src\gfx\*.o 2>NUL

echo Done! The binary (protracker.exe) is in the folder named 'release'.

pause
