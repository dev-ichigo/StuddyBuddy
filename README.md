# StudyBuddy Campus Match

Desktop GUI app in C + raylib (Dasar Pemrograman, UPNVJ S1 Sistem Informasi - Kelompok 3).
See `PRD_StudyBuddy_Campus_Match.md` for the full requirements.

## Build (Windows 10/11, MinGW-w64 + raylib 4.x/5.x)

    build.bat

or run the command from the PRD directly:

    gcc -Iinclude src/main.c src/file_manager.c src/gui_manager.c -o bin/StudyBuddyApp.exe -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows

## Run

    bin\StudyBuddyApp.exe

Start it from the project root or from `bin/` - the app finds `data/studybuddy_data.txt` either way.
If the file is missing it is created with sample data.

## Project layout

    bin/       compiled StudyBuddyApp.exe
    data/      studybuddy_data.txt (auto-generated, auto-saved on every change)
    include/   models.h, file_manager.h, gui_manager.h
    src/       main.c, file_manager.c, gui_manager.c

## Screens

Home (dashboard) | Find Buddy (course dropdown) | Buddy List | Add Buddy (2-step: details -> join/create group)
| Study Groups (members per group) | Learning Goals (ON PROGRESS / SELESAI toggle)

## Notes

- Buttons, text fields, dropdowns and scrolling lists are small custom widgets written with plain raylib
  calls (in `gui_manager.c`), so **raygui.h is not required**.
- Data file format (`|` separated): `B|nama|nim|matkul|hari`, `G|group|matkul|count|max|m1;m2`, `T|status|deskripsi`.
- Max 5 members per group (`MAX_GROUP_MEMBERS` in `models.h`).
- ESC un-focuses a text field / closes a dropdown; close the window with the X button.
