#include <stddef.h>
#include "raylib.h"

#include "models.h"
#include "file_manager.h"
#include "gui_manager.h"

/* ---------- Global state declared in models.h ---------- */
StudyBuddy *buddies       = NULL;
int         buddyCount    = 0;
int         buddyCapacity = 0;

Group      *groups        = NULL;
int         groupCount    = 0;
int         groupCapacity = 0;

Goal       *goals         = NULL;
int         goalCount     = 0;
int         goalCapacity  = 0;

ChatMessage *chatLogs     = NULL;
int          chatCount    = 0;
int          chatCapacity = 0;

const char *COURSES[COURSE_COUNT] = {
    "Pemrograman C",
    "Basis Data",
    "Logika Matematika",
    "Sistem Informasi"
};

int main(void)
{
    InitData();

    // Aktifkan Anti-Aliasing sebelum jendela dibuat
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "StudyBuddy Campus Match");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    // Filter font agar halus
    SetTextureFilter(GetFontDefault().texture, TEXTURE_FILTER_BILINEAR);

    GuiInit();

    while (!WindowShouldClose()) {
        GuiUpdateAndDraw();
    }

    GuiShutdown();
    CloseWindow();

    SaveData();
    FreeData();
    return 0;
}