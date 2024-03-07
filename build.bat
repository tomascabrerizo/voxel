@echo off

if not exist .\build mkdir .\build

echo ----------------------------------------
echo Build game ...
echo ----------------------------------------

set TARGET=voxel
set CFLAGS=/std:c11 /W2 /nologo /Od /Zi /EHsc
set LIBS=shell32.lib SDL2.lib
set SOURCES=.\thirdparty\glad\src\glad.c src\build_all.c
set OUT_DIR=/Fo.\build\ /Fe.\build\%TARGET% /Fm.\build\
set INC_DIR=/I.\src /I.\thirdparty /I.\thirdparty\SDL2\include /I.\thirdparty\glad\include
set LNK_DIR=/LIBPATH:.\thirdparty\SDL2\lib\x64

cl %CFLAGS% %INC_DIR% %SOURCES% %OUT_DIR% /link %LNK_DIR% %LIBS% /SUBSYSTEM:CONSOLE

xcopy /y .\thirdparty\SDL2\lib\x64\SDL2.dll .\build
xcopy /y /i /s /e .\res .\build\res
