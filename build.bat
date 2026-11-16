@echo off
REM StudyBuddy Campus Match - build script (MinGW-w64 + raylib)
if not exist bin mkdir bin
gcc -Iinclude src/main.c src/file_manager.c src/gui_manager.c -o bin/StudyBuddyApp.exe -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows
if %errorlevel% neq 0 (
    echo Build FAILED
    exit /b 1
)
echo Build OK: bin\StudyBuddyApp.exe
