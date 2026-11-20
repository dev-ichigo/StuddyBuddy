#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "raylib.h"

#include "../include/models.h"
#include "../include/file_manager.h"
#include "../include/gui_manager.h"

/* ====================================================================== */
/* Palette & Layout Constants                                             */
/* ====================================================================== */

#define PX(v)                 ((int)(v))
#define SIDEBAR_W             224
#define CONTENT_PAD           28
#define ROW_H                 86.0f
#define GROUP_CARD_H          180.0f
#define GOAL_ROW_H            74.0f

#define MAX_USER_GOALS        4
#define MAX_MESSAGES_PER_ROOM 12
#define VIRT_H                640.0f

static float GetVirtW(void)
{
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    if (sh <= 0.0f) return 1020.0f;
    float w = VIRT_H * (sw / sh);
    return (w < 920.0f) ? 920.0f : w;
}

static Camera2D GetUiCamera(void)
{
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float scaleX = sw / GetVirtW();
    float scaleY = sh / VIRT_H;
    float scale  = (scaleX < scaleY) ? scaleX : scaleY;

    Camera2D cam = { 0 };
    cam.zoom = scale;
    cam.offset.x = (sw - (GetVirtW() * scale)) * 0.5f;
    cam.offset.y = (sh - (VIRT_H * scale)) * 0.5f;
    return cam;
}

static Vector2 GetUiMouse(void)
{
    return GetScreenToWorld2D(GetMousePosition(), GetUiCamera());
}

/* Palette sesuai desain studybuddy_website_2.html */
static const Color COL_BG           = { 244, 247, 252, 255 }; /* #F4F7FC */
static const Color COL_NAVY         = {  24,  52,  95, 255 }; /* #18345F (--ink) */
static const Color COL_NAVY_DARK    = {  19,  40,  75, 255 };
static const Color COL_NAVY_HOVER   = { 237, 243, 255, 255 }; /* #EDF3FF */
static const Color COL_PRIMARY      = {  57, 117, 220, 255 }; /* #3975DC (--blue) */
static const Color COL_PRIMARY_LT   = { 237, 243, 255, 255 }; /* #EDF3FF */
static const Color COL_BORDER       = { 230, 235, 244, 255 }; /* #E6EBF4 (--line) */
static const Color COL_TEXT         = {  24,  52,  95, 255 }; /* #18345F */
static const Color COL_MUTED        = { 113, 128, 154, 255 }; /* #71809A (--muted) */

static const Color TILE_BLUE_BG     = { 234, 241, 255, 255 }; /* #EAF1FF */
static const Color TILE_BLUE_FG     = {  57, 117, 220, 255 };
static const Color TILE_PINK_BG     = { 253, 233, 240, 255 }; /* #FDE9F0 */
static const Color TILE_PINK_FG     = { 214,  82, 138, 255 };
static const Color TILE_MINT_BG     = { 223, 245, 238, 255 }; /* #DFF5EE */
static const Color TILE_MINT_FG     = {  34, 165, 115, 255 };
static const Color TILE_AMBER_BG    = { 255, 243, 216, 255 }; /* #FFF3D8 */
static const Color TILE_AMBER_FG    = { 215, 145,  28, 255 };

static const Color COL_OK           = {  67, 189, 142, 255 }; /* #43BD8E */
static const Color COL_WARN         = { 230, 155,  35, 255 };
static const Color COL_ERR          = { 218,  72,  82, 255 };
static const Color COL_DISABLED     = { 190, 198, 214, 255 };

/* Extended Workflow States */
typedef enum {
    VIEW_SPLASH = 0,
    VIEW_AUTH,          /* Sign Up / Log In */
    VIEW_HOME,          /* Personalized Dashboard */
    VIEW_FIND_BUDDY,    /* Filter & Invite Friends */
    VIEW_SCHEDULE,      /* Match Your Schedule */
    VIEW_GROUPS,        /* Study Groups List */
    VIEW_GROUP_DETAIL,  /* Group Detail & Join */
    VIEW_GOALS,         /* Learning Goals */
    VIEW_COUNT
} WorkflowView;

enum {
    ID_AUTH_NAME = 1,
    ID_AUTH_NIM,
    ID_AUTH_PASS,             // <-- TAMBAH
    ID_AUTH_CONFIRM,          // <-- TAMBAH
    ID_BUDDY_SEARCH,
    ID_BUDDY_COURSE_FILTER,
    ID_NEW_GROUP_NAME,
    ID_NEW_GROUP_COURSE,
    ID_INVITE_SEARCH,         // <-- TAMBAH
    ID_SCHED_COURSE,
    ID_GOAL_INPUT,
    ID_CHAT_INPUT
};

typedef struct {
    bool open;
    int  selected;
    float scroll;
} DropdownState;

/* Floating Chat Box State (Tab Kecil di Pojok Kanan Bawah) */
#define MAX_DM_CONTACTS 64
static bool          chatMinimized   = true;
static bool          justOpenedChat  = false;
static char          chatInput[MAX_CHAT_LEN];
static bool          chatInitPos     = false;
static Rectangle     chatWinRect     = { 0, 0, 340.0f, 410.0f };
static bool          chatDragging    = false;
static Vector2       chatDragOffset  = { 0 };
static bool          chatResizing    = false;
static Vector2       chatResizeStart = { 0 };
static Vector2       chatStartSize   = { 0 };
static char          activeChatKey[64]   = "";
static char          activeChatTitle[64] = "Pilih Chat";
static DropdownState chatRoomDd      = { false, 0, 0.0f };

/* Daftar kontak DM yang pernah dibuka khusus per-user */static char          dmOwnerNim[MAX_DM_CONTACTS][MAX_NIM];
static char          dmTargetNim[MAX_DM_CONTACTS][MAX_NIM];
static int           dmContactCount  = 0;

/* ====================================================================== */
/* Global State                                                           */
/* ====================================================================== */

static Font fontRegular = { 0 };
static Font fontBold    = { 0 };

static WorkflowView currentView = VIEW_SPLASH;
static int          focusId     = -1;

/* Active Logged-in User Session */
static bool isLoggedIn     = false;
static int  loggedBuddyIdx = -1;
static char currentUserName[MAX_NAMA] = "";
static char currentUserNim[MAX_NIM]   = "";
static char currentUserCourse[MAX_MATKUL] = "Pemrograman C";
static char currentUserSched[MAX_HARI]    = "Senin, 13.00 - 15.00";

/* Auth Form State */
static bool isLoginMode = false;
static char authName[MAX_NAMA];
static char authNim[MAX_NIM];
static char authPass[MAX_PASS];       // <-- TAMBAH
static char authConfirm[MAX_PASS];    // <-- TAMBAH

static const char *DAY_OPTIONS[] = {
    "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu", "Minggu"
};
#define DAY_OPTION_COUNT 7

static const char *SESSION_OPTIONS[] = {
    "08.00 - 10.00",
    "10.00 - 12.00",
    "13.00 - 15.00",
    "15.00 - 17.00",
    "18.30 - 20.30"
};
#define SESSION_OPTION_COUNT 5

/* Find Buddy State */
static char findSearch[64]              = "";
static char findCourseFilter[MAX_MATKUL]= "";
static bool showFindFilterBox           = false;

/* Match Schedule & Courses State */
static int  selectedDayIdx          = 1;
static bool slotChecked[5]          = { false, true, true, false, false };
static char schedCourse[MAX_MATKUL] = "";

/* Group Detail & Create State */
static int  selectedGroupIdx            = 0;
static bool showCreateGroupModal        = false;
static bool confirmDeleteGroup          = false;
static char newGroupName[MAX_GROUP_NAME]= "";
static char newGroupCourse[MAX_MATKUL]  = "";
static bool showInviteModal             = false; // <-- TAMBAH
static char inviteSearch[64]            = "";    // <-- TAMBAH

/* Goals State */
static char goalInput[MAX_GOAL_DESC];

/* Scroll & UI Helpers */
static Rectangle blockRect = { 0 }, nextBlockRect = { 0 };
static bool      blockActive = false, nextBlockActive = false;
static Rectangle clipRect = { 0 };
static bool      clipActive = false;
static float     scrollY[VIEW_COUNT];

#define FIND_PAGE_SIZE   6
#define GROUPS_PAGE_SIZE 4

static int findPage   = 0;
static int groupsPage = 0;

static float backspaceHeld = 0.0f;

static char  statusMsg[128] = "";
static float statusTimer = 0.0f;
static Color statusColor = { 0 };

/* Animasi Transisi Layar */
static float viewAnimAlpha = 1.0f;
static float viewAnimY     = 0.0f;

/* ====================================================================== */
/* Helpers & Widgets                                                      */
/* ====================================================================== */

static Font GetFontReg(void)  { return (fontRegular.texture.id > 0) ? fontRegular : GetFontDefault(); }
static Font GetFontBold(void) { return (fontBold.texture.id > 0)    ? fontBold    : GetFontReg(); }

static float ContentX(void)      { return (float)(SIDEBAR_W + CONTENT_PAD); }
static float ContentW(void)      { return GetVirtW() - SIDEBAR_W - (CONTENT_PAD * 2); }
static float ContentBottom(void) { return VIRT_H - 22.0f; }

static void SetStatus(const char *msg, Color c)
{
    snprintf(statusMsg, sizeof statusMsg, "%s", msg);
    statusColor = c;
    statusTimer = 3.0f;
}

static void Persist(void)
{
    if (!SaveData()) SetStatus("Gagal menyimpan perubahan ke file!", COL_ERR);
}

static void GoView(WorkflowView v)
{
    currentView = v;
    focusId = -1;
    showCreateGroupModal = false;
    showFindFilterBox = false;
    showInviteModal = false; // <-- TAMBAH
    scrollY[v] = 0.0f;
    confirmDeleteGroup = false;

    viewAnimAlpha = 1.0f;
    viewAnimY     = 0.0f;
}

static void DrawAnimatedBackground(bool isAuthStage)
{
    float sw = GetVirtW();
    float sh = VIRT_H;
    float t  = (float)GetTime();

    if (isAuthStage) {
        DrawRectangleGradientV(0, 0, (int)sw + 2, (int)sh + 2,
                               (Color){ 234, 242, 255, 255 },
                               (Color){ 248, 251, 255, 255 });

        float bob1 = sinf(t * 1.1f) * 14.0f;
        float bob2 = cosf(t * 0.9f) * 16.0f;
        float bob3 = sinf(t * 1.4f + 1.0f) * 10.0f;

        DrawCircleV((Vector2){ sw * 0.14f, sh * 0.92f + bob1 }, 225.0f, Fade(TILE_BLUE_BG, 0.95f));
        DrawCircleV((Vector2){ sw * 0.86f, sh * 0.12f + bob2 }, 185.0f, Fade(TILE_AMBER_BG, 0.90f));
        DrawCircleV((Vector2){ sw * 0.84f, sh * 0.85f + bob3 }, 110.0f, Fade(TILE_MINT_BG, 0.75f));
        DrawCircleV((Vector2){ sw * 0.16f, sh * 0.18f - bob3 },  75.0f, Fade(TILE_PINK_BG, 0.70f));

        DrawCircleLines((int)(sw * 0.5f), (int)(sh * 0.5f), 275.0f + sinf(t * 1.5f) * 6.0f,
                        Fade(COL_PRIMARY, 0.12f));
    } else {
        float wx = (float)SIDEBAR_W;
        float ww = sw - wx;
        DrawRectangleGradientV((int)wx, 0, (int)ww + 2, (int)sh + 2,
                               (Color){ 242, 246, 253, 255 },
                               (Color){ 247, 249, 255, 255 });

        float drift = sinf(t * 0.8f) * 8.0f;
        DrawCircleV((Vector2){ sw - 35.0f, 35.0f + drift }, 150.0f, Fade(TILE_BLUE_BG, 0.75f));
        DrawCircleV((Vector2){ sw - 85.0f, sh - 20.0f - drift }, 185.0f, Fade(TILE_MINT_BG, 0.45f));
        DrawCircleV((Vector2){ wx + 55.0f, sh - 25.0f + drift }, 115.0f, Fade(TILE_AMBER_BG, 0.40f));
    }
}

static void DrawCard(Rectangle r, Color fill, Color border, float radiusPx)
{
    float minSide = (r.width < r.height) ? r.width : r.height;
    float roundness = (minSide > 0) ? (radiusPx * 2.0f) / minSide : 0.1f;
    if (roundness > 1.0f) roundness = 1.0f;

    Rectangle in = { r.x + 1, r.y + 1, r.width - 2, r.height - 2 };
    DrawRectangleRounded(r, roundness, 12, border);
    DrawRectangleRounded(in, roundness, 12, fill);
}

static void DrawTextStyle(Font f, const char *text, int x, int y, int size, int maxW, Color c)
{
    char buf[220];
    snprintf(buf, sizeof buf, "%s", text);

    Vector2 sz = MeasureTextEx(f, buf, (float)size, 0.5f);
    if (sz.x <= maxW) {
        DrawTextEx(f, buf, (Vector2){ (float)x, (float)y }, (float)size, 0.5f, c);
        return;
    }
    size_t len = strlen(buf);
    while (len > 0) {
        buf[--len] = '\0';
        char tmp[230];
        snprintf(tmp, sizeof tmp, "%s...", buf);
        if (MeasureTextEx(f, tmp, (float)size, 0.5f).x <= maxW) {
            DrawTextEx(f, tmp, (Vector2){ (float)x, (float)y }, (float)size, 0.5f, c);
            return;
        }
    }
}

static void DrawTextFit(const char *text, int x, int y, int size, int maxW, Color c)
{
    DrawTextStyle(GetFontReg(), text, x, y, size, maxW, c);
}

static void DrawTextBold(const char *text, int x, int y, int size, int maxW, Color c)
{
    DrawTextStyle(GetFontBold(), text, x, y, size, maxW, c);
}

static int DrawTextWrap(const char *text, int x, int y, int size, int maxW, int lineH, Color c, int maxLines)
{
    char line[220] = "";
    char word[110];
    int lines = 0;
    const char *p = text;
    Font f = GetFontReg();

    while (*p && lines < maxLines) {
        int wl = 0;
        while (*p == ' ') p++;
        while (*p && *p != ' ' && wl < (int)sizeof(word) - 1) word[wl++] = *p++;
        word[wl] = '\0';
        if (wl == 0) break;

        char trial[340];
        if (line[0]) snprintf(trial, sizeof trial, "%s %s", line, word);
        else         snprintf(trial, sizeof trial, "%s", word);

        if (line[0] == '\0' || MeasureTextEx(f, trial, (float)size, 0.5f).x <= maxW) {
            snprintf(line, sizeof line, "%s", trial);
        } else {
            DrawTextFit(line, x, y + lines * lineH, size, maxW, c);
            lines++;
            snprintf(line, sizeof line, "%s", word);
        }
    }
    if (line[0] && lines < maxLines) {
        DrawTextFit(line, x, y + lines * lineH, size, maxW, c);
        lines++;
    }
    return lines;
}

static void DrawAvatar(float cx, float cy, float radius, const char *name, Color bg, Color fg)
{
    DrawCircleV((Vector2){ cx, cy }, radius, bg);

    char init[3] = "SB";
    if (name && name[0]) {
        init[0] = (char)toupper((unsigned char)name[0]);
        init[1] = '\0';
        const char *sp = strchr(name, ' ');
        if (sp && sp[1]) {
            init[1] = (char)toupper((unsigned char)sp[1]);
            init[2] = '\0';
        }
    }
    Font f = GetFontBold();
    float fsize = radius * 0.85f;
    Vector2 sz = MeasureTextEx(f, init, fsize, 0.5f);
    DrawTextEx(f, init, (Vector2){ cx - sz.x / 2.0f, cy - sz.y / 2.0f }, fsize, 0.5f, fg);
}

static void DrawBrandLogo(float x, float y, float scale)
{
    float s = 38.0f * scale;
    DrawRectangleRounded((Rectangle){ x, y, s, s }, 0.35f, 10, COL_PRIMARY);
    DrawTriangle((Vector2){ x + 19*scale, y + 7*scale },
                 (Vector2){ x + 10*scale, y + 23*scale },
                 (Vector2){ x + 19*scale, y + 23*scale }, WHITE);
    DrawTriangle((Vector2){ x + 21*scale, y + 11*scale },
                 (Vector2){ x + 21*scale, y + 23*scale },
                 (Vector2){ x + 29*scale, y + 23*scale }, (Color){ 215, 232, 255, 255 });
    DrawRectangleRounded((Rectangle){ x + 8*scale, y + 25*scale, 22*scale, 6*scale }, 0.8f, 6, COL_NAVY);
}

static bool MouseOver(Rectangle r)
{
    Vector2 m = GetUiMouse();
    if (!CheckCollisionPointRec(m, r)) return false;
    if (clipActive && !CheckCollisionPointRec(m, clipRect)) return false;
    if (blockActive && CheckCollisionPointRec(m, blockRect)) return false;
    return true;
}

static bool Button(Rectangle r, const char *label, bool enabled, Color base)
{
    bool hover = enabled && MouseOver(r);
    Color c = !enabled ? COL_DISABLED : (hover ? ColorBrightness(base, -0.10f) : base);
    DrawRectangleRounded(r, 0.38f, 12, c);

    Font f = GetFontBold();
    float fsize = (r.height <= 32.0f) ? 13.0f : 15.0f;
    Vector2 sz = MeasureTextEx(f, label, fsize, 0.5f);
    DrawTextEx(f, label, (Vector2){ r.x + (r.width - sz.x) / 2.0f, r.y + (r.height - sz.y) / 2.0f }, fsize, 0.5f, WHITE);

    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static bool OutlineButton(Rectangle r, const char *label, Color borderCol, Color textCol)
{
    bool hover = MouseOver(r);
    DrawCard(r, hover ? COL_PRIMARY_LT : WHITE, borderCol, r.height * 0.38f);

    Font f = GetFontBold();
    float fsize = (r.height <= 32.0f) ? 12.0f : 14.0f;
    Vector2 sz = MeasureTextEx(f, label, fsize, 0.5f);
    DrawTextEx(f, label, (Vector2){ r.x + (r.width - sz.x) / 2.0f, r.y + (r.height - sz.y) / 2.0f }, fsize, 0.5f, textCol);

    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static bool PillButton(Rectangle r, const char *label, bool active)
{
    bool hover = MouseOver(r);
    Color bg = active ? COL_PRIMARY : (hover ? COL_PRIMARY_LT : WHITE);
    Color fg = active ? WHITE : COL_MUTED;
    Color bd = active ? COL_PRIMARY : COL_BORDER;

    DrawCard(r, bg, bd, r.height / 2.0f);
    Font f = active ? GetFontBold() : GetFontReg();
    Vector2 sz = MeasureTextEx(f, label, 13.0f, 0.5f);
    DrawTextEx(f, label, (Vector2){ r.x + (r.width - sz.x) / 2.0f, r.y + (r.height - sz.y) / 2.0f }, 13.0f, 0.5f, fg);

    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static bool TextInput(int id, Rectangle r, char *buf, int cap, const char *placeholder, bool digitsOnly)
{
    bool submitted = false;
    bool hover = MouseOver(r);
    Font f = GetFontReg();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (hover) focusId = id;
        else if (focusId == id) focusId = -1;
    }
    bool focused = (focusId == id);

    if (focused) {
        int len = (int)strlen(buf);
        int ch;
        while ((ch = GetCharPressed()) > 0) {
            bool ok = digitsOnly ? (ch >= '0' && ch <= '9')
                                 : (ch >= 32 && ch <= 126 && ch != '|' && ch != ';');
            if (ok && len < cap - 1) {
                buf[len++] = (char)ch;
                buf[len] = '\0';
            }
        }

        if (IsKeyPressed(KEY_BACKSPACE)) {
            backspaceHeld = 0.0f;
            if (len > 0) buf[--len] = '\0';
        } else if (IsKeyDown(KEY_BACKSPACE)) {
            backspaceHeld += GetFrameTime();
            if (backspaceHeld > 0.4f) {
                if (len > 0) buf[--len] = '\0';
                backspaceHeld = 0.36f;
            }
        } else {
            backspaceHeld = 0.0f;
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) submitted = true;
    }

    DrawCard(r, WHITE, focused ? COL_PRIMARY : COL_BORDER, 12.0f);

    int textY = PX(r.y + (r.height - 15) / 2);
    if (buf[0] == '\0') {
        if (!focused) DrawTextFit(placeholder, PX(r.x + 14), textY, 15, PX(r.width - 28), COL_DISABLED);
        if (focused && ((int)(GetTime() * 2.0) % 2 == 0))
            DrawRectangle(PX(r.x + 14), PX(r.y + 9), 2, PX(r.height - 18), COL_PRIMARY);
    } else {
        const char *shown = buf;
        while (*shown && MeasureTextEx(f, shown, 15.0f, 0.5f).x > PX(r.width) - 30) shown++;
        DrawTextFit(shown, PX(r.x + 14), textY, 15, PX(r.width - 28), COL_TEXT);
        if (focused && ((int)(GetTime() * 2.0) % 2 == 0)) {
            int cx = PX(r.x + 14) + (int)MeasureTextEx(f, shown, 15.0f, 0.5f).x + 2;
            DrawRectangle(cx, PX(r.y + 9), 2, PX(r.height - 18), COL_PRIMARY);
        }
    }
    return submitted;
}

static bool TextInputMasked(int id, Rectangle r, char *buf, int cap, const char *placeholder)
{
    bool submitted = false;
    bool hover = MouseOver(r);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (hover) focusId = id;
        else if (focusId == id) focusId = -1;
    }
    bool focused = (focusId == id);

    if (focused) {
        int len = (int)strlen(buf);
        int ch;
        while ((ch = GetCharPressed()) > 0) {
            bool ok = (ch >= 33 && ch <= 126 && ch != '|' && ch != ';');
            if (ok && len < cap - 1) {
                buf[len++] = (char)ch;
                buf[len] = '\0';
            }
        }

        if (IsKeyPressed(KEY_BACKSPACE)) {
            backspaceHeld = 0.0f;
            if (len > 0) buf[--len] = '\0';
        } else if (IsKeyDown(KEY_BACKSPACE)) {
            backspaceHeld += GetFrameTime();
            if (backspaceHeld > 0.4f) {
                if (len > 0) buf[--len] = '\0';
                backspaceHeld = 0.36f;
            }
        } else {
            backspaceHeld = 0.0f;
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) submitted = true;
    }

    DrawCard(r, WHITE, focused ? COL_PRIMARY : COL_BORDER, 12.0f);

    int len = (int)strlen(buf);
    int textY = PX(r.y + (r.height - 15) / 2);
    if (len == 0) {
        if (!focused) DrawTextFit(placeholder, PX(r.x + 14), textY, 15, PX(r.width - 28), COL_DISABLED);
        if (focused && ((int)(GetTime() * 2.0) % 2 == 0))
            DrawRectangle(PX(r.x + 14), PX(r.y + 9), 2, PX(r.height - 18), COL_PRIMARY);
    } else {
        int maxDots = (int)((r.width - 34.0f) / 11.0f);
        int shownDots = (len > maxDots) ? maxDots : len;
        for (int i = 0; i < shownDots; i++) {
            DrawCircleV((Vector2){ r.x + 18.0f + i * 11.0f, r.y + r.height * 0.5f }, 3.8f, COL_TEXT);
        }
        if (focused && ((int)(GetTime() * 2.0) % 2 == 0)) {
            int cx = PX(r.x + 16.0f + shownDots * 11.0f);
            DrawRectangle(cx, PX(r.y + 9), 2, PX(r.height - 18), COL_PRIMARY);
        }
    }
    return submitted;
}

static void Dropdown(DropdownState *d, Rectangle r, const char **items, int count)
{
    Vector2 m = GetUiMouse();
    bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    int visibleCount = (count > 3) ? 3 : count;
    float itemH = r.height;
    float totalH = itemH * count;
    float boxH = itemH * visibleCount;
    float maxScroll = totalH - boxH;
    if (maxScroll < 0.0f) maxScroll = 0.0f;

    Rectangle list = { r.x, r.y + r.height + 4, r.width, boxH };

    if (d->open && maxScroll > 0.0f && CheckCollisionPointRec(m, list)) {
        d->scroll -= GetMouseWheelMove() * 32.0f;
        if (d->scroll < 0.0f) d->scroll = 0.0f;
        if (d->scroll > maxScroll) d->scroll = maxScroll;
    }

    if (clicked) {
        if (d->open) {
            if (CheckCollisionPointRec(m, list)) {
                int idx = (int)((m.y - list.y + d->scroll) / itemH);
                if (idx < 0) idx = 0;
                if (idx >= count) idx = count - 1;
                d->selected = idx;
            }
            d->open = false;
        } else if (CheckCollisionPointRec(m, r) && !(blockActive && CheckCollisionPointRec(m, blockRect))) {
            d->open = true;
            d->scroll = d->selected * itemH - itemH;
            if (d->scroll < 0.0f) d->scroll = 0.0f;
            if (d->scroll > maxScroll) d->scroll = maxScroll;
        }
    }

    bool hoverHeader = CheckCollisionPointRec(m, r);
    DrawCard(r, WHITE, (d->open || hoverHeader) ? COL_PRIMARY : COL_BORDER, 12.0f);
    DrawTextFit(items[d->selected], PX(r.x + 14), PX(r.y + (r.height - 15) / 2), 15, PX(r.width - 36), COL_TEXT);

    float ax = r.x + r.width - 20, ay = r.y + r.height / 2;
    DrawTriangle((Vector2){ ax - 5, ay - 3 }, (Vector2){ ax, ay + 4 }, (Vector2){ ax + 5, ay - 3 }, COL_MUTED);

    if (d->open) {
        nextBlockRect = list;
        nextBlockActive = true;

        DrawCard(list, WHITE, COL_PRIMARY, 10.0f);

        Camera2D cam = GetUiCamera();
        int sx = (int)((list.x + 1) * cam.zoom + cam.offset.x);
        int sy = (int)((list.y + 1) * cam.zoom + cam.offset.y);
        int sw = (int)((list.width - 2) * cam.zoom);
        int sh = (int)((list.height - 2) * cam.zoom);

        BeginScissorMode(sx, sy, sw, sh);
        for (int i = 0; i < count; i++) {
            float iy = list.y + i * itemH - d->scroll;
            if (iy + itemH < list.y || iy > list.y + list.height) continue;

            Rectangle it = { list.x + 2, iy + 1, list.width - (maxScroll > 0.0f ? 10 : 4), itemH - 2 };
            if (CheckCollisionPointRec(m, list) && CheckCollisionPointRec(m, it)) {
                DrawRectangleRounded(it, 0.25f, 8, COL_PRIMARY_LT);
            }
            DrawTextFit(items[i], PX(it.x + 12), PX(it.y + (it.height - 15) / 2), 15, PX(it.width - 18),
                        i == d->selected ? COL_PRIMARY : COL_TEXT);
        }
        EndScissorMode();

        if (maxScroll > 0.0f) {
            float thumbH = boxH * (boxH / totalH);
            if (thumbH < 20.0f) thumbH = 20.0f;
            float thumbY = list.y + 3 + (boxH - 6 - thumbH) * (d->scroll / maxScroll);
            DrawRectangleRounded((Rectangle){ list.x + list.width - 6, thumbY, 4, thumbH }, 1.0f, 4, COL_DISABLED);
        }
    }
}

/* ====================================================================== */
/* Session & Chat Helpers                                                 */
/* ====================================================================== */

static void DrawPaginationBar(float cx, float cw, float y, int *page, int totalItems, int pageSize)
{
    int totalPages = (totalItems + pageSize - 1) / pageSize;
    if (totalPages < 1) totalPages = 1;
    if (*page >= totalPages) *page = totalPages - 1;
    if (*page < 0) *page = 0;

    if (totalItems <= pageSize) return;

    float btnW = 92.0f;
    float btnH = 32.0f;
    float barW = 300.0f;
    float bx   = cx + (cw - barW) * 0.5f;

    if (Button((Rectangle){ bx, y, btnW, btnH }, "< Prev", *page > 0, COL_PRIMARY)) {
        (*page)--;
    }

    char pageLabel[48];
    snprintf(pageLabel, sizeof pageLabel, "Page %d / %d", *page + 1, totalPages);
    Font fb = GetFontBold();
    Vector2 tsz = MeasureTextEx(fb, pageLabel, 13.0f, 0.5f);
    DrawTextEx(fb, pageLabel,
               (Vector2){ bx + (barW - tsz.x) * 0.5f, y + (btnH - tsz.y) * 0.5f },
               13.0f, 0.5f, COL_NAVY);

    if (Button((Rectangle){ bx + barW - btnW, y, btnW, btnH }, "Next >", *page < totalPages - 1, COL_PRIMARY)) {
        (*page)++;
    }
}

static bool IsCurrentUserAdmin(void)
{
    if (!isLoggedIn) return false;
    if (strcmp(currentUserNim, "2610512113") != 0) return false;

    char lowerName[MAX_NAMA];
    snprintf(lowerName, sizeof lowerName, "%s", currentUserName);
    for (int i = 0; lowerName[i]; i++) {
        lowerName[i] = (char)tolower((unsigned char)lowerName[i]);
    }
    return (strcmp(lowerName, "ferdigo achda gamis") == 0);
}

static void LoginWithBuddyIndex(int idx)
{
    if (idx < 0 || idx >= buddyCount) return;
    isLoggedIn = true;
    loggedBuddyIdx = idx;
    snprintf(currentUserName,   sizeof currentUserName,   "%s", buddies[idx].nama);
    snprintf(currentUserNim,    sizeof currentUserNim,    "%s", buddies[idx].nim);
    snprintf(currentUserCourse, sizeof currentUserCourse, "%s", buddies[idx].matkul);
    snprintf(currentUserSched,  sizeof currentUserSched,  "%s", buddies[idx].hari);

    if (strcmp(buddies[idx].matkul, "Belum diatur") != 0) {
        snprintf(schedCourse, sizeof schedCourse, "%s", buddies[idx].matkul);
    } else {
        schedCourse[0] = '\0';
    }

    findPage   = 0;
    groupsPage = 0;

    selectedDayIdx = 0;
    for (int i = 0; i < 5; i++) slotChecked[i] = false;

    for (int d = 0; d < DAY_OPTION_COUNT; d++) {
        if (strstr(buddies[idx].hari, DAY_OPTIONS[d]) != NULL) {
            selectedDayIdx = d;
            break;
        }
    }

    int countChecked = 0;
    for (int s = 0; s < SESSION_OPTION_COUNT && s < 5; s++) {
        if (strstr(buddies[idx].hari, SESSION_OPTIONS[s]) != NULL && countChecked < 2) {
            slotChecked[s] = true;
            countChecked++;
        }
    }
    if (countChecked == 0) slotChecked[0] = true;
}

static void AddUserDmContact(const char *ownerNim, const char *targetNim)
{
    if (!ownerNim || !targetNim || strcmp(ownerNim, targetNim) == 0) return;
    for (int i = 0; i < dmContactCount; i++) {
        if (strcmp(dmOwnerNim[i], ownerNim) == 0 && strcmp(dmTargetNim[i], targetNim) == 0) {
            return;
        }
    }
    if (dmContactCount < MAX_DM_CONTACTS) {
        snprintf(dmOwnerNim[dmContactCount],  MAX_NIM, "%s", ownerNim);
        snprintf(dmTargetNim[dmContactCount], MAX_NIM, "%s", targetNim);
        dmContactCount++;
    }
}

static void MakeDmRoomKey(char *outKey, size_t cap, const char *nimA, const char *nimB)
{
    if (strcmp(nimA, nimB) < 0) snprintf(outKey, cap, "DM:%s:%s", nimA, nimB);
    else                         snprintf(outKey, cap, "DM:%s:%s", nimB, nimA);
}

static void OpenDirectChatWithNim(const char *targetNim, const char *targetName)
{
    if (!targetNim || strcmp(targetNim, currentUserNim) == 0) return;

    AddUserDmContact(currentUserNim, targetNim);

    char rKey[64], title[64];
    MakeDmRoomKey(rKey, sizeof rKey, currentUserNim, targetNim);
    snprintf(title, sizeof title, "[Member] %s", targetName);

    snprintf(activeChatKey,   sizeof activeChatKey,   "%s", rKey);
    snprintf(activeChatTitle, sizeof activeChatTitle, "%s", title);
    chatMinimized   = false;
    chatRoomDd.open = false;
    focusId         = ID_CHAT_INPUT;
    justOpenedChat  = true;
}

static void OpenChatRoom(const char *roomKey, const char *title)
{
    snprintf(activeChatKey,   sizeof activeChatKey,   "%s", roomKey);
    snprintf(activeChatTitle, sizeof activeChatTitle, "%s", title);
    chatMinimized   = false;
    chatRoomDd.open = false;
    focusId         = ID_CHAT_INPUT;
    justOpenedChat  = true;
}

static void SyncDmContactsFromHistory(void)
{
    for (int i = 0; i < chatCount; i++) {
        if (strncmp(chatLogs[i].roomKey, "DM:", 3) == 0) {
            char tmp[64];
            snprintf(tmp, sizeof tmp, "%s", chatLogs[i].roomKey + 3);
            char *sep = strchr(tmp, ':');
            if (sep != NULL) {
                *sep = '\0';
                const char *nimA = tmp;
                const char *nimB = sep + 1;
                AddUserDmContact(nimA, nimB);
                AddUserDmContact(nimB, nimA);
            }
        }
    }
}

static bool TrimRoomChatHistory(const char *roomKey)
{
    if (!roomKey || roomKey[0] == '\0') return false;

    int countInRoom = 0;
    for (int i = 0; i < chatCount; i++) {
        if (strcmp(chatLogs[i].roomKey, roomKey) == 0) {
            countInRoom++;
        }
    }

    if (countInRoom <= MAX_MESSAGES_PER_ROOM) return false;

    int toRemove = countInRoom - MAX_MESSAGES_PER_ROOM;
    int writeIdx = 0;
    for (int readIdx = 0; readIdx < chatCount; readIdx++) {
        if (toRemove > 0 && strcmp(chatLogs[readIdx].roomKey, roomKey) == 0) {
            toRemove--;
            continue;
        }
        if (writeIdx != readIdx) {
            chatLogs[writeIdx] = chatLogs[readIdx];
        }
        writeIdx++;
    }
    chatCount = writeIdx;
    return true;
}

static void TrimAllRoomsChatHistory(void)
{
    bool anyTrimmed = false;
    for (int i = 0; i < chatCount; i++) {
        if (TrimRoomChatHistory(chatLogs[i].roomKey)) {
            anyTrimmed = true;
            i = -1;
        }
    }
    if (anyTrimmed) {
        Persist();
    }
}

static void SeedInitialChat(void)
{
    SyncDmContactsFromHistory();
    TrimAllRoomsChatHistory();
    if (chatCount > 0) return;

    ChatMessage m1 = { "G:C Warriors", "Aulia Rahma",   "Halo semuanya! Ada yang mau bahas latihan pointer malam ini?", "14:12" };
    ChatMessage m2 = { "G:C Warriors", "Citra Lestari", "Gas! Aku masih agak bingung di bagian realloc nih.", "14:16" };
    AddChatMessage(&m1);
    AddChatMessage(&m2);
    Persist();
}

static void PushChatMessage(const char *roomKey, const char *sender, const char *msg)
{
    ChatMessage m;
    memset(&m, 0, sizeof m);
    snprintf(m.roomKey, sizeof m.roomKey, "%s", roomKey);
    snprintf(m.sender,  sizeof m.sender,  "%s", sender);
    snprintf(m.text,    sizeof m.text,    "%s", msg);
    SanitizeText(m.text);
    if (m.text[0] == '\0') return;

    snprintf(m.timeStr, sizeof m.timeStr, "Now");

    if (AddChatMessage(&m) >= 0) {
        TrimRoomChatHistory(roomKey);
        Persist();
    }
}

static bool IsMemberOfGroup(int groupIdx, const char *userName)
{
    if (groupIdx < 0 || groupIdx >= groupCount) return false;
    for (int k = 0; k < groups[groupIdx].jumlahAnggota; k++) {
        if (strcmp(groups[groupIdx].anggotaNama[k], userName) == 0) return true;
    }
    return false;
}

/* ====================================================================== */
/* SCREEN 1: Splash Screen                                                */
/* ====================================================================== */

static void DrawSplashScreen(void)
{
    float sw = GetVirtW();
    float sh = VIRT_H;

    float cardW = 460.0f;
    float cardH = 440.0f;
    Rectangle card = { (sw - cardW) / 2.0f, (sh - cardH) / 2.0f, cardW, cardH };
    DrawCard(card, WHITE, COL_BORDER, 24.0f);

    DrawBrandLogo(card.x + (cardW - 68) / 2.0f, card.y + 44, 1.8f);

    Font fb = GetFontBold();
    Font fr = GetFontReg();

    const char *brand = "StudyBuddy";
    Vector2 bsz = MeasureTextEx(fb, brand, 34.0f, 0.5f);
    DrawTextEx(fb, brand, (Vector2){ card.x + (cardW - bsz.x) / 2.0f, card.y + 130 }, 34.0f, 0.5f, COL_NAVY);

    const char *tag = "Find Your Study Match";
    Vector2 tsz = MeasureTextEx(fr, tag, 16.0f, 0.5f);
    DrawTextEx(fr, tag, (Vector2){ card.x + (cardW - tsz.x) / 2.0f, card.y + 170 }, 16.0f, 0.5f, COL_MUTED);

    Rectangle pill = { card.x + 48, card.y + 210, cardW - 96, 62 };
    DrawRectangleRounded(pill, 0.3f, 12, COL_PRIMARY_LT);
    const char *quote = "Better Learning Together  |  Study Smarter";
    Vector2 qsz = MeasureTextEx(fb, quote, 14.0f, 0.5f);
    DrawTextEx(fb, quote, (Vector2){ pill.x + (pill.width - qsz.x) / 2.0f, pill.y + 23 }, 14.0f, 0.5f, COL_PRIMARY);

    if (Button((Rectangle){ card.x + 48, card.y + 296, cardW - 96, 48 }, "Get Started", true, COL_PRIMARY)) {
        isLoginMode = false;
        authName[0] = authNim[0] = authPass[0] = authConfirm[0] = '\0';
        GoView(VIEW_AUTH);
    }

    if (OutlineButton((Rectangle){ card.x + 48, card.y + 358, cardW - 96, 44 }, "Already have an account? Log In", COL_BORDER, COL_NAVY)) {
        isLoginMode = true;
        authNim[0] = authPass[0] = authConfirm[0] = '\0';
        GoView(VIEW_AUTH);
    }
}

/* ====================================================================== */
/* SCREEN 2: Sign Up & Log In                                             */
/* ====================================================================== */

static void HandleAuthSubmit(void)
{
    if (isLoginMode) {
        if (authNim[0] == '\0' || authPass[0] == '\0') {
            SetStatus("Masukkan NIM dan Password untuk masuk.", COL_ERR);
            return;
        }
        for (int i = 0; i < buddyCount; i++) {
            if (strcmp(buddies[i].nim, authNim) == 0) {
                if (strcmp(buddies[i].password, authPass) != 0) {
                    SetStatus("Password salah! Periksa kembali password akunmu.", COL_ERR);
                    return;
                }
                LoginWithBuddyIndex(i);
                authPass[0] = '\0';
                char welcome[128];
                snprintf(welcome, sizeof welcome, "Selamat datang kembali, %s!", currentUserName);
                SetStatus(welcome, COL_OK);
                GoView(VIEW_HOME);
                return;
            }
        }
        SetStatus("NIM tidak ditemukan. Silakan Sign Up terlebih dahulu.", COL_ERR);
    } else {
        StudyBuddy b;
        memset(&b, 0, sizeof b);
        snprintf(b.nama,     sizeof b.nama,     "%s", authName);
        snprintf(b.nim,      sizeof b.nim,      "%s", authNim);
        snprintf(b.password, sizeof b.password, "%s", authPass);
        snprintf(b.matkul,   sizeof b.matkul,   "Belum diatur");
        snprintf(b.hari,     sizeof b.hari,     "Belum diatur");
        SanitizeText(b.nama);
        SanitizeText(b.nim);
        SanitizeText(b.password);

        if (b.nama[0] == '\0' || b.nim[0] == '\0') {
            SetStatus("Lengkapi Nama dan NIM kamu terlebih dahulu.", COL_ERR);
            return;
        }
        if (strlen(b.password) < 4) {
            SetStatus("Password minimal 4 karakter.", COL_ERR);
            return;
        }
        if (strcmp(b.password, authConfirm) != 0) {
            SetStatus("Konfirmasi password tidak cocok!", COL_ERR);
            return;
        }
        for (int i = 0; i < buddyCount; i++) {
            if (strcmp(buddies[i].nim, b.nim) == 0) {
                SetStatus("NIM ini sudah terdaftar! Silakan pilih Log In.", COL_ERR);
                return;
            }
        }

        int idx = AddBuddy(&b);
        if (idx < 0) {
            SetStatus("Memori penuh - gagal membuat akun.", COL_ERR);
            return;
        }
        Persist();
        LoginWithBuddyIndex(idx);
        authPass[0] = authConfirm[0] = '\0';
        char msg[128];
        snprintf(msg, sizeof msg, "Akun dibuat! Atur matkul & jadwal di Schedule & Courses.", currentUserName);
        SetStatus(msg, COL_OK);
        GoView(VIEW_HOME);
    }
}

static void DrawAuthScreen(void)
{
    float sw = GetVirtW();
    float sh = VIRT_H;

    float cardW = 470.0f;
    float cardH = isLoginMode ? 395.0f : 535.0f;
    Rectangle card = { (sw - cardW) / 2.0f, (sh - cardH) / 2.0f, cardW, cardH };
    DrawCard(card, WHITE, COL_BORDER, 22.0f);

    float x = card.x + 36;
    float w = cardW - 72;
    float y = card.y + 24;

    if (OutlineButton((Rectangle){ x, y, 76, 30 }, "< Back", COL_BORDER, COL_MUTED)) {
        authPass[0] = authConfirm[0] = '\0';
        GoView(VIEW_SPLASH);
    }

    DrawTextBold(isLoginMode ? "Welcome Back!" : "Create Your Account", PX(x), PX(y + 42), 25, PX(w), COL_NAVY);
    DrawTextFit(isLoginMode ? "Masukkan NIM dan password untuk masuk ke dashboard"
                            : "Buat akun dengan Nama, NIM, dan Password",
                PX(x), PX(y + 72), 13, PX(w), COL_MUTED);

    float fy = y + 104;

    if (isLoginMode) {
        DrawTextBold("NIM Mahasiswa", PX(x), PX(fy), 13, 200, COL_TEXT);
        TextInput(ID_AUTH_NIM, (Rectangle){ x, fy + 20, w, 42 }, authNim, MAX_NIM, "Contoh: 2610512113", true);

        DrawTextBold("Password", PX(x), PX(fy + 72), 13, 200, COL_TEXT);
        if (TextInputMasked(ID_AUTH_PASS, (Rectangle){ x, fy + 92, w, 42 }, authPass, MAX_PASS, "Masukkan password...")) {
            HandleAuthSubmit();
        }

        if (Button((Rectangle){ x, fy + 154, w, 46 }, "Log In", true, COL_PRIMARY)) {
            HandleAuthSubmit();
        }
    } else {
        DrawTextBold("Nama Lengkap", PX(x), PX(fy), 13, 200, COL_TEXT);
        TextInput(ID_AUTH_NAME, (Rectangle){ x, fy + 20, w, 40 }, authName, MAX_NAMA, "Contoh: Sarah Amalia", false);

        DrawTextBold("NIM", PX(x), PX(fy + 68), 13, 200, COL_TEXT);
        TextInput(ID_AUTH_NIM, (Rectangle){ x, fy + 88, w, 40 }, authNim, MAX_NIM, "Hanya angka (contoh: 2610512113)", true);

        DrawTextBold("Password", PX(x), PX(fy + 136), 13, 200, COL_TEXT);
        TextInputMasked(ID_AUTH_PASS, (Rectangle){ x, fy + 156, w, 40 }, authPass, MAX_PASS, "Minimal 4 karakter");

        DrawTextBold("Konfirmasi Password", PX(x), PX(fy + 204), 13, 240, COL_TEXT);
        if (TextInputMasked(ID_AUTH_CONFIRM, (Rectangle){ x, fy + 224, w, 40 }, authConfirm, MAX_PASS, "Ketik ulang password")) {
            HandleAuthSubmit();
        }

        if (Button((Rectangle){ x, fy + 282, w, 46 }, "Sign Up & Masuk", true, COL_PRIMARY)) {
            HandleAuthSubmit();
        }
    }
}

/* ====================================================================== */
/* Sidebar, Topbar & Editorial Header (Desain HTML Clean SaaS)            */
/* ====================================================================== */

static const char *GetCurrentCrumbName(void)
{
    switch (currentView) {
        case VIEW_HOME:         return "Home";
        case VIEW_FIND_BUDDY:   return "Find Buddy";
        case VIEW_SCHEDULE:     return "Schedule & Courses";
        case VIEW_GROUPS:       return "Study Groups";
        case VIEW_GROUP_DETAIL: return "Group Detail";
        case VIEW_GOALS:        return "Learning Goals";
        default:                return "Home";
    }
}

static void DrawTopbar(void)
{
    float cx = ContentX();
    float cw = ContentW();

    DrawTextFit("Workspace  /", PX(cx), 22, 13, 120, COL_MUTED);
    DrawTextBold(GetCurrentCrumbName(), PX(cx + 82), 22, 13, 180, COL_NAVY);

    float chipW = 195.0f;
    Rectangle chip = { cx + cw - chipW, 13, chipW, 34 };
    DrawCard(chip, WHITE, COL_BORDER, 17.0f);
    DrawAvatar(chip.x + 18, chip.y + 17, 12, currentUserName, TILE_AMBER_BG, COL_NAVY);
    DrawTextBold(currentUserName, PX(chip.x + 36), PX(chip.y + 10), 12, PX(chipW - 56), COL_NAVY);
    DrawCircle(PX(chip.x + chipW - 12), PX(chip.y + 17), 4, COL_OK);
}

static void DrawSidebar(void)
{
    int screenH = (int)VIRT_H;

    DrawRectangle(0, 0, SIDEBAR_W, screenH, WHITE);
    DrawRectangle(SIDEBAR_W - 1, 0, 1, screenH, COL_BORDER);

    DrawBrandLogo(20, 22, 1.0f);
    DrawTextBold("StudyBuddy", 66, 22, 20, SIDEBAR_W - 76, COL_NAVY);
    DrawTextFit("Find Your Study Match", 66, 44, 11, SIDEBAR_W - 76, COL_MUTED);

    DrawTextBold("MENU", 22, 78, 10, 100, (Color){ 161, 172, 192, 255 });

    struct { const char *label; WorkflowView view; } navItems[] = {
        { "Home",           VIEW_HOME },
        { "Find Buddy",     VIEW_FIND_BUDDY },
        { "Study Groups",   VIEW_GROUPS },
        { "Schedule",       VIEW_SCHEDULE },
        { "Learning Goals", VIEW_GOALS }
    };
    int navCount = 5;

    for (int i = 0; i < navCount; i++) {
        Rectangle r = { 14, 98.0f + i * 44.0f, SIDEBAR_W - 28, 38 };
        bool active = (currentView == navItems[i].view) ||
                      (navItems[i].view == VIEW_GROUPS && currentView == VIEW_GROUP_DETAIL);
        bool hover  = MouseOver(r);

        if (active || hover) {
            DrawRectangleRounded(r, 0.32f, 10, COL_PRIMARY_LT);
        }

        Color txtCol = (active || hover) ? COL_PRIMARY : (Color){ 119, 133, 157, 255 };
        DrawCircle(PX(r.x + 16), PX(r.y + 19), active ? 4.0f : 3.0f,
                   active ? COL_PRIMARY : (Color){ 185, 196, 216, 255 });

        if (active) DrawTextBold(navItems[i].label, PX(r.x + 28), PX(r.y + 11), 14, SIDEBAR_W - 52, txtCol);
        else        DrawTextFit(navItems[i].label,  PX(r.x + 28), PX(r.y + 11), 14, SIDEBAR_W - 52, txtCol);

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) GoView(navItems[i].view);
    }

    Rectangle prof = { 14, (float)screenH - 116, SIDEBAR_W - 28, 102 };
    DrawRectangleRounded(prof, 0.22f, 12, (Color){ 242, 246, 255, 255 });
    DrawAvatar(prof.x + 24, prof.y + 28, 16, currentUserName, COL_PRIMARY, WHITE);
    DrawTextBold(currentUserName, PX(prof.x + 46), PX(prof.y + 14), 13, PX(prof.width - 56), COL_NAVY);

    if (IsCurrentUserAdmin()) {
        DrawTextFit(currentUserNim, PX(prof.x + 46), PX(prof.y + 31), 11, PX(prof.width - 108), COL_MUTED);
        Rectangle badge = { prof.x + prof.width - 58, prof.y + 29, 48, 16 };
        DrawRectangleRounded(badge, 0.5f, 6, TILE_AMBER_BG);
        DrawTextBold("ADMIN", PX(badge.x + 7), PX(badge.y + 3), 10, 40, TILE_AMBER_FG);
    } else {
        DrawTextFit(currentUserNim, PX(prof.x + 46), PX(prof.y + 31), 11, PX(prof.width - 56), COL_MUTED);
    }

    if (Button((Rectangle){ prof.x + 12, prof.y + 60, prof.width - 24, 28 }, "Log Out", true, COL_ERR)) {
        isLoggedIn = false;
        loggedBuddyIdx = -1;
        chatMinimized = true;
        GoView(VIEW_SPLASH);
    }
}

static void DrawEditorialHeader(const char *eyebrow, const char *title, const char *subtitle, const char *rightBadge)
{
    float cx = ContentX();
    float cw = ContentW();

    DrawTextBold(eyebrow, PX(cx), 54, 11, PX(cw - 210), COL_PRIMARY);
    DrawTextBold(title,   PX(cx), 70, 26, PX(cw - 210), COL_NAVY);
    DrawTextFit(subtitle, PX(cx), 100, 14, PX(cw - 210), COL_MUTED);

    if (rightBadge && rightBadge[0] != '\0') {
        Font fr = GetFontReg();
        float bw = MeasureTextEx(fr, rightBadge, 12.0f, 0.5f).x + 26.0f;
        Rectangle br = { cx + cw - bw, 72, bw, 34 };
        DrawCard(br, WHITE, COL_BORDER, 11.0f);
        DrawTextFit(rightBadge, PX(br.x + 13), PX(br.y + 10), 12, PX(bw - 20), COL_MUTED);
    }
}

static void DrawToast(void)
{
    if (statusTimer <= 0.0f) return;
    statusTimer -= GetFrameTime();

    float alpha = (statusTimer < 0.5f) ? (statusTimer / 0.5f) : 1.0f;
    if (alpha < 0.0f) alpha = 0.0f;

    Font f = GetFontBold();
    int w = (int)MeasureTextEx(f, statusMsg, 14.0f, 0.5f).x + 40;
    float sw = GetVirtW();
    if (w > sw - 60) w = (int)sw - 60;

    float baseX = isLoggedIn ? ContentX() + (ContentW() - w) / 2.0f : (sw - w) / 2.0f;
    Rectangle r = { baseX, VIRT_H - 50, (float)w, 36 };

    DrawRectangleRounded(r, 0.45f, 12, Fade(COL_NAVY, alpha));
    DrawTextBold(statusMsg, PX(r.x + 20), PX(r.y + 10), 14, w - 40, Fade(WHITE, alpha));
}

/* ====================================================================== */
/* SCREEN 3: Personalized Home / Dashboard (Layout 2 Kolom HTML)          */
/* ====================================================================== */

static bool DrawQuickTile(Rectangle r, const char *title, const char *sub, Color iconBg, Color iconFg)
{
    bool hover = MouseOver(r);
    DrawCard(r, WHITE, hover ? COL_PRIMARY : COL_BORDER, 15.0f);

    Rectangle icoBox = { r.x + 14, r.y + 13, 36, 36 };
    DrawRectangleRounded(icoBox, 0.32f, 8, iconBg);
    DrawCircle(PX(icoBox.x + 18), PX(icoBox.y + 18), 7, iconFg);

    DrawTextBold(title, PX(r.x + 14), PX(r.y + 56), 14, PX(r.width - 28), COL_NAVY);
    DrawTextFit(sub,    PX(r.x + 14), PX(r.y + 75), 11, PX(r.width - 28), COL_MUTED);

    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static void DrawHomeScreen(void)
{
    char greeting[96];
    snprintf(greeting, sizeof greeting, "Hi, %s!", currentUserName);

    char schedBadge[120];
    snprintf(schedBadge, sizeof schedBadge, "Fokus: %s", currentUserCourse);

    DrawEditorialHeader("WELCOME BACK, LET'S MAKE IT COUNT",
                        greeting,
                        "Ready to learn something new today?",
                        schedBadge);

    float cx = ContentX();
    float cw = ContentW();
    float gap = 14.0f;

    /* Baris 1: 4 Quick Tiles putih dengan kotak ikon pastel */
    float tw = (cw - 3 * gap) / 4.0f;
    float ty = 128.0f;

    int myTotalGoals = 0, myDoneGoals = 0;
    for (int i = 0; i < goalCount; i++) {
        if (strcmp(goals[i].nimPemilik, currentUserNim) == 0) {
            myTotalGoals++;
            if (goals[i].status == 1) myDoneGoals++;
        }
    }

    char subBuddy[40], subGroup[40], subGoal[40];
    snprintf(subBuddy, sizeof subBuddy, "%d teman tersedia", buddyCount);
    snprintf(subGroup, sizeof subGroup, "%d grup aktif", groupCount);
    snprintf(subGoal,  sizeof subGoal,  "%d/%d selesai", myDoneGoals, myTotalGoals);

    if (DrawQuickTile((Rectangle){ cx, ty, tw, 98 }, "Find Buddy", subBuddy, TILE_BLUE_BG,  TILE_BLUE_FG))  GoView(VIEW_FIND_BUDDY);
    if (DrawQuickTile((Rectangle){ cx + (tw + gap), ty, tw, 98 }, "My Groups", subGroup, TILE_PINK_BG,  TILE_PINK_FG))  GoView(VIEW_GROUPS);
    if (DrawQuickTile((Rectangle){ cx + 2*(tw + gap), ty, tw, 98 }, "Schedule", "Plan your time", TILE_MINT_BG,  TILE_MINT_FG))  GoView(VIEW_SCHEDULE);
    if (DrawQuickTile((Rectangle){ cx + 3*(tw + gap), ty, tw, 98 }, "My Goals", subGoal, TILE_AMBER_BG, TILE_AMBER_FG)) GoView(VIEW_GOALS);

    /* Baris 2: Grid 2 Kolom (Kiri 59%, Kanan 41%) */
    float gridY  = ty + 112.0f;
    float gridH  = ContentBottom() - gridY;
    float leftW  = (cw - 18.0f) * 0.59f;
    float rightW = cw - leftW - 18.0f;
    float rx     = cx + leftW + 18.0f;

    /* --- KARTU KIRI: Upcoming Study Session + Popular Courses --- */
    Rectangle leftCard = { cx, gridY, leftW, gridH };
    DrawCard(leftCard, WHITE, COL_BORDER, 18.0f);

    DrawTextBold("Upcoming Study Session", PX(leftCard.x + 20), PX(leftCard.y + 18), 15, 240, COL_NAVY);
    if (OutlineButton((Rectangle){ leftCard.x + leftW - 126, leftCard.y + 13, 108, 26 }, "See schedule >", WHITE, COL_PRIMARY)) {
        GoView(VIEW_SCHEDULE);
    }

    int joinedGroupIdx = -1;
    for (int i = 0; i < groupCount; i++) {
        if (IsMemberOfGroup(i, currentUserName)) { joinedGroupIdx = i; break; }
    }

    Rectangle sesBox = { leftCard.x + 20, leftCard.y + 48, leftW - 40, 68 };
    DrawCard(sesBox, WHITE, COL_BORDER, 14.0f);
    DrawRectangleRounded((Rectangle){ sesBox.x + 12, sesBox.y + 13, 42, 42 }, 0.3f, 8, TILE_BLUE_BG);
    DrawCircle(PX(sesBox.x + 33), PX(sesBox.y + 34), 8, COL_PRIMARY);

    if (joinedGroupIdx >= 0) {
        Group *mg = &groups[joinedGroupIdx];
        DrawTextBold(mg->namaGroup, PX(sesBox.x + 66), PX(sesBox.y + 14), 14, PX(sesBox.width - 155), COL_NAVY);
        char sMeta[120];
        snprintf(sMeta, sizeof sMeta, "%s  |  %s", mg->matkul, currentUserSched);
        DrawTextFit(sMeta, PX(sesBox.x + 66), PX(sesBox.y + 36), 12, PX(sesBox.width - 155), COL_MUTED);

        if (Button((Rectangle){ sesBox.x + sesBox.width - 86, sesBox.y + 18, 74, 30 }, "Open", true, COL_PRIMARY)) {
            selectedGroupIdx = joinedGroupIdx;
            GoView(VIEW_GROUP_DETAIL);
        }
    } else {
        DrawTextBold("Belum Gabung Study Group", PX(sesBox.x + 66), PX(sesBox.y + 14), 14, PX(sesBox.width - 155), COL_NAVY);
        DrawTextFit("Temukan grup sesuai mata kuliahmu", PX(sesBox.x + 66), PX(sesBox.y + 36), 12, PX(sesBox.width - 155), COL_MUTED);

        if (Button((Rectangle){ sesBox.x + sesBox.width - 86, sesBox.y + 18, 74, 30 }, "Explore", true, COL_PRIMARY)) {
            GoView(VIEW_GROUPS);
        }
    }

    float popY = leftCard.y + 134.0f;
    DrawTextBold("Popular Courses", PX(leftCard.x + 20), PX(popY), 15, 220, COL_NAVY);
    if (OutlineButton((Rectangle){ leftCard.x + leftW - 104, popY - 4, 86, 26 }, "See all >", WHITE, COL_PRIMARY)) {
        findCourseFilter[0] = '\0';
        findPage = 0;
        GoView(VIEW_FIND_BUDDY);
    }

    /* Hitung 4 mata kuliah terpopuler secara dinamis dari data buddies[] */
    /* Hitung 4 mata kuliah terpopuler secara dinamis dari data buddies[] */
    #define MAX_UNIQUE_COURSES 128
    char topCourses[MAX_UNIQUE_COURSES][MAX_MATKUL];
    int  topCounts[MAX_UNIQUE_COURSES];
    int  uniqueCourses = 0;

    for (int b = 0; b < buddyCount; b++) {
        if (buddies[b].matkul[0] == '\0' || strcmp(buddies[b].matkul, "Belum diatur") == 0) continue;
        int foundIdx = -1;
        for (int u = 0; u < uniqueCourses; u++) {
            if (strcmp(topCourses[u], buddies[b].matkul) == 0) { foundIdx = u; break; }
        }
        if (foundIdx >= 0) {
            topCounts[foundIdx]++;
        } else if (uniqueCourses < MAX_UNIQUE_COURSES) {
            snprintf(topCourses[uniqueCourses], MAX_MATKUL, "%s", buddies[b].matkul);
            topCounts[uniqueCourses] = 1;
            uniqueCourses++;
        }
    }

    for (int a = 0; a < uniqueCourses - 1; a++) {
        for (int b = a + 1; b < uniqueCourses; b++) {
            if (topCounts[b] > topCounts[a]) {
                int tmpC = topCounts[a]; topCounts[a] = topCounts[b]; topCounts[b] = tmpC;
                char tmpName[MAX_MATKUL];
                snprintf(tmpName, sizeof tmpName, "%s", topCourses[a]);
                snprintf(topCourses[a], MAX_MATKUL, "%s", topCourses[b]);
                snprintf(topCourses[b], MAX_MATKUL, "%s", tmpName);
            }
        }
    }

    int showCourses = (uniqueCourses > 4) ? 4 : uniqueCourses;
    float pcw = (leftW - 52.0f) / 2.0f;
    float pch = (gridH - 176.0f) / 2.0f;
    if (pch < 56.0f) pch = 56.0f;

    for (int i = 0; i < showCourses; i++) {
        int col = i % 2;
        int row = i / 2;
        Rectangle cr = { leftCard.x + 20 + col * (pcw + 12.0f), popY + 30 + row * (pch + 10.0f), pcw, pch };
        bool hover = MouseOver(cr);
        DrawCard(cr, WHITE, hover ? COL_PRIMARY : COL_BORDER, 13.0f);

        DrawTextBold(topCourses[i], PX(cr.x + 14), PX(cr.y + 12), 13, PX(pcw - 28), COL_NAVY);
        char cstr[48];
        snprintf(cstr, sizeof cstr, "%d members  |  Active", topCounts[i]);
        DrawTextFit(cstr, PX(cr.x + 14), PX(cr.y + 33), 11, PX(pcw - 28), COL_MUTED);

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            snprintf(findCourseFilter, sizeof findCourseFilter, "%s", topCourses[i]);
            findPage = 0;
            GoView(VIEW_FIND_BUDDY);
        }
    }

    /* --- KOLOM KANAN ATAS: Suggested Buddies --- */
    float sugH = gridH * 0.57f;
    Rectangle sugCard = { rx, gridY, rightW, sugH };
    DrawCard(sugCard, WHITE, COL_BORDER, 18.0f);

    DrawTextBold("Suggested Buddies", PX(sugCard.x + 18), PX(sugCard.y + 16), 15, 180, COL_NAVY);
    if (OutlineButton((Rectangle){ sugCard.x + rightW - 96, sugCard.y + 12, 80, 26 }, "View all >", WHITE, COL_PRIMARY)) {
        GoView(VIEW_FIND_BUDDY);
    }

    int shownSug = 0;
    for (int i = 0; i < buddyCount && shownSug < 2; i++) {
        if (strcmp(buddies[i].nim, currentUserNim) == 0) continue;

        float by = sugCard.y + 50.0f + shownSug * 58.0f;
        if (shownSug > 0) {
            DrawRectangle(PX(sugCard.x + 18), PX(by - 5), PX(rightW - 36), 1, COL_BORDER);
        }

        DrawAvatar(sugCard.x + 36, by + 22, 18, buddies[i].nama,
                   (shownSug % 2 == 0) ? TILE_BLUE_BG : TILE_PINK_BG, COL_NAVY);
        DrawTextBold(buddies[i].nama,   PX(sugCard.x + 62), PX(by + 6),  13, PX(rightW - 140), COL_NAVY);
        DrawTextFit(buddies[i].matkul,  PX(sugCard.x + 62), PX(by + 24), 11, PX(rightW - 140), COL_MUTED);

        if (Button((Rectangle){ sugCard.x + rightW - 72, by + 9, 56, 28 }, "Chat", true, COL_PRIMARY)) {
            OpenDirectChatWithNim(buddies[i].nim, buddies[i].nama);
        }
        shownSug++;
    }

    /* --- KOLOM KANAN BAWAH: Weekly Learning Goals --- */
    float goalY = gridY + sugH + 14.0f;
    float goalH = gridH - sugH - 14.0f;
    Rectangle goalCard = { rx, goalY, rightW, goalH };
    DrawCard(goalCard, WHITE, COL_BORDER, 18.0f);

    DrawTextBold("Weekly Learning Goals", PX(goalCard.x + 18), PX(goalCard.y + 16), 15, 195, COL_NAVY);
    if (OutlineButton((Rectangle){ goalCard.x + rightW - 90, goalCard.y + 12, 74, 26 }, "Details >", WHITE, COL_PRIMARY)) {
        GoView(VIEW_GOALS);
    }

    DrawTextFit("Completed targets", PX(goalCard.x + 18), PX(goalCard.y + 48), 12, 160, COL_MUTED);
    char gStat[24];
    snprintf(gStat, sizeof gStat, "%d/%d", myDoneGoals, myTotalGoals > 0 ? myTotalGoals : MAX_USER_GOALS);
    DrawTextBold(gStat, PX(goalCard.x + rightW - 52), PX(goalCard.y + 48), 12, 40, COL_NAVY);

    Rectangle bar = { goalCard.x + 18, goalCard.y + 70, rightW - 36, 8 };
    DrawRectangleRounded(bar, 1.0f, 6, (Color){ 237, 241, 247, 255 });
    float ratio = (myTotalGoals > 0) ? (float)myDoneGoals / (float)myTotalGoals : 0.0f;
    if (ratio > 0.0f) {
        DrawRectangleRounded((Rectangle){ bar.x, bar.y, bar.width * ratio, bar.height }, 1.0f, 6, COL_OK);
    }

    DrawTextFit("You're making progress. Keep it up!", PX(goalCard.x + 18), PX(goalCard.y + 88), 11, PX(rightW - 36), COL_MUTED);
}

/* ====================================================================== */
/* SCREEN 4: Find Buddy (Grid Kartu 3 Kolom Sesuai Desain HTML)           */
/* ====================================================================== */

static bool ContainsIgnoreCase(const char *haystack, const char *needle)
{
    if (!needle || needle[0] == '\0') return true;
    char h[200], n[200];
    snprintf(h, sizeof h, "%s", haystack);
    snprintf(n, sizeof n, "%s", needle);
    for (int i = 0; h[i]; i++) h[i] = (char)tolower((unsigned char)h[i]);
    for (int i = 0; n[i]; i++) n[i] = (char)tolower((unsigned char)n[i]);
    return strstr(h, n) != NULL;
}

static void DrawFilterIcon(float cx, float cy, Color col)
{
    DrawLineEx((Vector2){ cx - 7.0f, cy - 4.5f }, (Vector2){ cx + 7.0f, cy - 4.5f }, 2.0f, col);
    DrawLineEx((Vector2){ cx - 4.5f, cy + 0.0f }, (Vector2){ cx + 4.5f, cy + 0.0f }, 2.0f, col);
    DrawLineEx((Vector2){ cx - 2.0f, cy + 4.5f }, (Vector2){ cx + 2.0f, cy + 4.5f }, 2.0f, col);
}

static void DrawFindBuddyScreen(void)
{
    DrawEditorialHeader("BETTER LEARNING, TOGETHER",
                        "Find your study buddy",
                        "Connect with students who share your courses and goals.",
                        NULL);

    float cx = ContentX();
    float cw = ContentW();

    char prevSearch[64];
    snprintf(prevSearch, sizeof prevSearch, "%s", findSearch);

    float filterBtnSize = 40.0f;
    float searchW = cw - filterBtnSize - 12.0f;
    TextInput(ID_BUDDY_SEARCH, (Rectangle){ cx, 128, searchW, 40 }, findSearch, sizeof findSearch,
              "Search name, NIM, or schedule...", false);

    if (strcmp(prevSearch, findSearch) != 0) {
        findPage = 0;
    }

    /* Tombol lingkaran filter mata kuliah */
    Rectangle fBtn = { cx + searchW + 12.0f, 128, filterBtnSize, filterBtnSize };
    Vector2 fCenter = { fBtn.x + filterBtnSize * 0.5f, fBtn.y + filterBtnSize * 0.5f };
    bool fHover = CheckCollisionPointCircle(GetUiMouse(), fCenter, filterBtnSize * 0.5f);
    bool hasFilter = (findCourseFilter[0] != '\0');

    Color btnFill   = (showFindFilterBox || hasFilter) ? COL_PRIMARY : (fHover ? COL_PRIMARY_LT : WHITE);
    Color btnBorder = (showFindFilterBox || hasFilter || fHover) ? COL_PRIMARY : COL_BORDER;
    Color iconCol   = (showFindFilterBox || hasFilter) ? WHITE : COL_PRIMARY;

    DrawCircleV(fCenter, filterBtnSize * 0.5f, btnFill);
    DrawCircleLines((int)fCenter.x, (int)fCenter.y, filterBtnSize * 0.5f, btnBorder);
    DrawFilterIcon(fCenter.x, fCenter.y, iconCol);

    if (hasFilter && !showFindFilterBox) {
        DrawCircleV((Vector2){ fBtn.x + filterBtnSize - 5.0f, fBtn.y + 5.0f }, 4.5f, COL_OK);
    }

    if (fHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        showFindFilterBox = !showFindFilterBox;
        if (showFindFilterBox) focusId = ID_BUDDY_COURSE_FILTER;
    }

    int totalMatched = 0;
    for (int i = 0; i < buddyCount; i++) {
        if (strcmp(buddies[i].nim, currentUserNim) == 0) continue;
        if (!ContainsIgnoreCase(buddies[i].matkul, findCourseFilter)) continue;
        if (!ContainsIgnoreCase(buddies[i].nama, findSearch) &&
            !ContainsIgnoreCase(buddies[i].nim, findSearch) &&
            !ContainsIgnoreCase(buddies[i].hari, findSearch)) continue;
        totalMatched++;
    }

    int totalPages = (totalMatched + FIND_PAGE_SIZE - 1) / FIND_PAGE_SIZE;
    if (totalPages < 1) totalPages = 1;
    if (findPage >= totalPages) findPage = totalPages - 1;
    if (findPage < 0) findPage = 0;

    int startIdx = findPage * FIND_PAGE_SIZE;
    int endIdx   = startIdx + FIND_PAGE_SIZE;

    float gap   = 14.0f;
    float cardW = (cw - 2 * gap) / 3.0f;
    float cardH = 176.0f;
    float gridY = 184.0f;

    int matchedIdx = 0;
    int rendered   = 0;
    for (int i = 0; i < buddyCount; i++) {
        if (strcmp(buddies[i].nim, currentUserNim) == 0) continue;
        if (!ContainsIgnoreCase(buddies[i].matkul, findCourseFilter)) continue;
        if (!ContainsIgnoreCase(buddies[i].nama, findSearch) &&
            !ContainsIgnoreCase(buddies[i].nim, findSearch) &&
            !ContainsIgnoreCase(buddies[i].hari, findSearch)) continue;

        if (matchedIdx >= startIdx && matchedIdx < endIdx) {
            int col = rendered % 3;
            int row = rendered / 3;
            Rectangle card = { cx + col * (cardW + gap), gridY + row * (cardH + gap), cardW, cardH };
            rendered++;

            bool hover = MouseOver(card);
            DrawCard(card, WHITE, hover ? COL_PRIMARY : COL_BORDER, 16.0f);

            DrawAvatar(card.x + 34, card.y + 34, 20, buddies[i].nama,
                       (i % 2 == 0) ? TILE_BLUE_BG : TILE_PINK_BG, COL_NAVY);
            DrawTextBold(buddies[i].nama, PX(card.x + 62), PX(card.y + 18), 14, PX(cardW - 88), COL_NAVY);
            DrawCircle(PX(card.x + cardW - 16), PX(card.y + 25), 4, COL_OK);

            char nimStr[48];
            snprintf(nimStr, sizeof nimStr, "NIM %s", buddies[i].nim);
            DrawTextFit(nimStr, PX(card.x + 62), PX(card.y + 37), 11, PX(cardW - 76), COL_MUTED);

            Rectangle chip = { card.x + 14, card.y + 66, cardW - 28, 24 };
            DrawRectangleRounded(chip, 0.35f, 8, (Color){ 241, 245, 252, 255 });
            DrawTextBold(buddies[i].matkul, PX(chip.x + 10), PX(chip.y + 6), 11, PX(chip.width - 20), COL_PRIMARY);

            DrawTextFit(buddies[i].hari, PX(card.x + 14), PX(card.y + 98), 11, PX(cardW - 28), COL_MUTED);

            float btnW = (cardW - 36.0f) * 0.56f;
            float invW = (cardW - 36.0f) - btnW;
            if (Button((Rectangle){ card.x + 14, card.y + cardH - 42, btnW, 30 }, "Send message", true, COL_PRIMARY)) {
                OpenDirectChatWithNim(buddies[i].nim, buddies[i].nama);
            }
        }
        matchedIdx++;
    }

    if (totalMatched == 0) {
        DrawTextFit("Tidak ada teman yang cocok dengan filter pencarianmu.", PX(cx), 210, 15, 450, COL_MUTED);
    } else {
        DrawPaginationBar(cx, cw, ContentBottom() - 34.0f, &findPage, totalMatched, FIND_PAGE_SIZE);
    }

    /* Popover box filter mata kuliah dirender paling akhir agar tampil di atas kartu */
    if (showFindFilterBox) {
        float popW = 290.0f;
        float popH = 122.0f;
        Rectangle pop = { fBtn.x + filterBtnSize - popW, fBtn.y + filterBtnSize + 8.0f, popW, popH };

        DrawRectangleRounded((Rectangle){ pop.x + 2, pop.y + 4, pop.width, pop.height }, 0.16f, 12, Fade(COL_NAVY, 0.10f));
        DrawCard(pop, WHITE, COL_PRIMARY, 14.0f);

        DrawTextBold("FILTER FOKUS MATA KULIAH", PX(pop.x + 14), PX(pop.y + 12), 10, 200, COL_MUTED);
        bool enter = TextInput(ID_BUDDY_COURSE_FILTER, (Rectangle){ pop.x + 14, pop.y + 30, pop.width - 28, 36 },
                               findCourseFilter, sizeof findCourseFilter, "Ketik nama mata kuliah...", false);

        if (Button((Rectangle){ pop.x + 14, pop.y + 74, 118, 32 }, "Reset", false, COL_MUTED)) {
            findCourseFilter[0] = '\0';
            findPage = 0;
        }
        if (Button((Rectangle){ pop.x + pop.width - 132, pop.y + 74, 118, 32 }, "Terapkan", true, COL_PRIMARY) || enter) {
            findPage = 0;
            showFindFilterBox = false;
        }

        nextBlockActive = true;
        nextBlockRect = pop;
    }
}

/* ====================================================================== */
/* SCREEN 5: Match Your Schedule                                          */
/* ====================================================================== */

static void DrawScheduleScreen(void)
{
    DrawEditorialHeader("MAKE TIME FOR WHAT MATTERS",
                        "Schedule & Courses",
                        "Isi fokus mata kuliah dan pilih 1 hari beserta maksimal 2 slot jam kosongmu.",
                        NULL);

    float cx = ContentX();
    float cw = ContentW();

    /* Baris input mata kuliah/materi fokus bebas (real-time ke panel rekomendasi) */
    DrawTextBold("Fokus Mata Kuliah / Materi:", PX(cx), 124, 13, 240, COL_NAVY);
    TextInput(ID_SCHED_COURSE, (Rectangle){ cx, 144, cw, 40 }, schedCourse, sizeof schedCourse,
              "Ketik nama mata kuliah atau materi (contoh: Kalkulus, Basis Data, Algoritma)...", false);

    /* Bersihkan spasi ujung sementara untuk pencocokan real-time */
    char liveCourse[MAX_MATKUL];
    snprintf(liveCourse, sizeof liveCourse, "%s", schedCourse);
    SanitizeText(liveCourse);
    bool hasLiveCourse = (liveCourse[0] != '\0');

    /* Pilihan 7 hari */
    float dayW = (cw - 6 * 10.0f) / 7.0f;
    for (int d = 0; d < DAY_OPTION_COUNT; d++) {
        Rectangle dr = { cx + d * (dayW + 10.0f), 196, dayW, 42 };
        bool sel = (d == selectedDayIdx);
        bool hover = MouseOver(dr);
        DrawCard(dr, sel ? COL_PRIMARY : (hover ? COL_PRIMARY_LT : WHITE), sel ? COL_PRIMARY : COL_BORDER, 12.0f);

        Font fb = GetFontBold();
        Vector2 s1 = MeasureTextEx(fb, DAY_OPTIONS[d], 14.0f, 0.5f);
        DrawTextEx(fb, DAY_OPTIONS[d], (Vector2){ dr.x + (dayW - s1.x) / 2.0f, dr.y + (dr.height - s1.y) / 2.0f },
                   14.0f, 0.5f, sel ? WHITE : COL_TEXT);

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) selectedDayIdx = d;
    }

    int checkedCount = 0;
    int activeSlots[2] = { -1, -1 };
    for (int i = 0; i < SESSION_OPTION_COUNT && i < 5; i++) {
        if (slotChecked[i]) {
            if (checkedCount < 2) activeSlots[checkedCount] = i;
            checkedCount++;
        }
    }

    float leftW  = (cw - 20.0f) * 0.52f;
    float rightW = cw - leftW - 20.0f;
    float topY   = 252.0f;

    char slotHeader[64];
    snprintf(slotHeader, sizeof slotHeader, "Available time slots (%d/2):", checkedCount);
    DrawTextBold(slotHeader, PX(cx), PX(topY), 15, 300, COL_NAVY);

    for (int s = 0; s < SESSION_OPTION_COUNT; s++) {
        Rectangle sr = { cx, topY + 26 + s * 48.0f, leftW, 40 };
        bool chk = slotChecked[s];
        bool locked = (!chk && checkedCount >= 2);
        bool hover = !locked && MouseOver(sr);

        Color cardBg = chk ? COL_PRIMARY_LT : (locked ? (Color){ 240, 243, 248, 255 } : (hover ? COL_PRIMARY_LT : WHITE));
        Color cardBd = chk ? COL_PRIMARY : COL_BORDER;
        Color txtCol = chk ? COL_NAVY : (locked ? COL_DISABLED : COL_TEXT);

        DrawCard(sr, cardBg, cardBd, 12.0f);
        DrawTextFit(SESSION_OPTIONS[s], PX(sr.x + 16), PX(sr.y + 12), 14, PX(leftW - 95), txtCol);

        if (locked) {
            DrawTextFit("Locked", PX(sr.x + sr.width - 86), PX(sr.y + 13), 12, 50, COL_DISABLED);
        }

        Rectangle cb = { sr.x + sr.width - 34, sr.y + 9, 22, 22 };
        DrawRectangleRounded(cb, 0.3f, 6, chk ? COL_PRIMARY : (locked ? (Color){ 220, 225, 235, 255 } : COL_BORDER));
        if (chk) DrawCircle(PX(cb.x + 11), PX(cb.y + 11), 5, WHITE);

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (chk) slotChecked[s] = false;
            else if (checkedCount < 2) slotChecked[s] = true;
        }
    }

    float rx = cx + leftW + 20.0f;
    DrawTextBold("Recommended time together:", PX(rx), PX(topY), 15, 280, COL_NAVY);

    Rectangle matchCard = { rx, topY + 26, rightW, 234 };
    DrawCard(matchCard, WHITE, COL_BORDER, 16.0f);

    char schedTitle[96];
    snprintf(schedTitle, sizeof schedTitle, "Hari %s  |  Fokus: %s",
             DAY_OPTIONS[selectedDayIdx],
             hasLiveCourse ? liveCourse : currentUserCourse);
    DrawTextBold(schedTitle, PX(matchCard.x + 18), PX(matchCard.y + 14), 13, PX(rightW - 36), COL_PRIMARY);

    /*
     * Sistem Ranking Real-Time:
     * Tier 4: Fokus Matkul Cocok + Hari & Semua Slot Cocok
     * Tier 3: Fokus Matkul Cocok + Hari & Minimal 1 Slot Cocok
     * Tier 2: Fokus Matkul Cocok (meski hari/jam berbeda, selama user sedang mengetik fokus matkul)
     * Tier 1 (Fallback): Fokus Matkul Lain, tapi Hari & Minimal 1 Slot Cocok (hanya muncul jika Tier 2-4 kosong)
     */
    int recIdx[3]  = { -1, -1, -1 };
    int recTier[3] = {  0,  0,  0 };
    int recCount   = 0;

    int minTierAllowed = 2; /* Utama: cari yang fokus mata kuliahnya sesuai dulu */
    for (int pass = 0; pass < 2 && recCount == 0; pass++) {
        if (pass == 1) minTierAllowed = 1; /* Fallback: jika fokus matkul tidak ditemukan, tampilkan jadwal sama */

        for (int tier = 4; tier >= minTierAllowed && recCount < 3; tier--) {
            for (int i = 0; i < buddyCount && recCount < 3; i++) {
                if (strcmp(buddies[i].nim, currentUserNim) == 0) continue;

                /* Cek apakah buddy sudah masuk daftar */
                bool alreadyAdded = false;
                for (int r = 0; r < recCount; r++) {
                    if (recIdx[r] == i) { alreadyAdded = true; break; }
                }
                if (alreadyAdded) continue;

                bool courseMatch = hasLiveCourse &&
                                   strcmp(buddies[i].matkul, "Belum diatur") != 0 &&
                                   ContainsIgnoreCase(buddies[i].matkul, liveCourse);

                bool dayMatch = (strstr(buddies[i].hari, DAY_OPTIONS[selectedDayIdx]) != NULL);
                int slotMatches = 0;
                if (dayMatch && checkedCount > 0) {
                    for (int k = 0; k < checkedCount; k++) {
                        if (activeSlots[k] >= 0 && strstr(buddies[i].hari, SESSION_OPTIONS[activeSlots[k]]) != NULL) {
                            slotMatches++;
                        }
                    }
                }

                int bTier = 0;
                if (courseMatch) {
                    if (checkedCount == 2 && slotMatches == 2) bTier = 4;
                    else if (slotMatches >= 1)                 bTier = 3;
                    else                                       bTier = 2;
                } else if (slotMatches >= 1) {
                    bTier = 1; /* Fokus lain dengan hari & slot jam yang sama */
                }

                if (bTier == tier) {
                    recIdx[recCount]  = i;
                    recTier[recCount] = bTier;
                    recCount++;
                }
            }
        }
    }

    if (recCount == 0) {
        const char *emptyMsg = "Belum ada teman dengan fokus atau jadwal yang cocok.";
        Font fr = GetFontReg();
        Vector2 tsz = MeasureTextEx(fr, emptyMsg, 13.0f, 0.5f);
        float centerX = matchCard.x + (rightW - tsz.x) * 0.5f;
        if (centerX < matchCard.x + 18.0f) centerX = matchCard.x + 18.0f;
        DrawTextFit(emptyMsg, PX(centerX), PX(matchCard.y + 100), 13, PX(rightW - 36), COL_MUTED);
    } else {
        for (int m = 0; m < recCount; m++) {
            int bi = recIdx[m];
            float my = matchCard.y + 44 + m * 43.0f;

            DrawAvatar(matchCard.x + 32, my + 14, 14, buddies[bi].nama, COL_PRIMARY_LT, COL_NAVY);
            DrawTextBold(buddies[bi].nama, PX(matchCard.x + 54), PX(my + 2), 13, PX(rightW - 155), COL_TEXT);

            char subMeta[140];
            snprintf(subMeta, sizeof subMeta, "%s  |  %s", buddies[bi].matkul, buddies[bi].hari);
            DrawTextFit(subMeta, PX(matchCard.x + 54), PX(my + 19), 11, PX(rightW - 155), COL_MUTED);

            const char *badgeTxt = "Jadwal Sama";
            Color badgeBg = TILE_AMBER_BG;
            Color badgeFg = TILE_AMBER_FG;

            if (recTier[m] == 4) {
                badgeTxt = "Best Match";
                badgeBg  = TILE_MINT_BG;
                badgeFg  = COL_OK;
            } else if (recTier[m] == 3) {
                badgeTxt = "Matkul & Slot";
                badgeBg  = TILE_MINT_BG;
                badgeFg  = COL_OK;
            } else if (recTier[m] == 2) {
                badgeTxt = "Matkul Sama";
                badgeBg  = COL_PRIMARY_LT;
                badgeFg  = COL_PRIMARY;
            }

            Rectangle badge = { matchCard.x + rightW - 104, my + 6, 88, 22 };
            DrawRectangleRounded(badge, 0.5f, 8, badgeBg);
            DrawTextBold(badgeTxt, PX(badge.x + 8), PX(badge.y + 5), 11, 76, badgeFg);
        }
    }

    if (Button((Rectangle){ matchCard.x + 18, matchCard.y + 182, rightW - 36, 38 }, "Save course & availability", true, COL_PRIMARY)) {
        if (!hasLiveCourse) {
            SetStatus("Isi fokus mata kuliah / materi terlebih dahulu!", COL_ERR);
        } else if (checkedCount == 0) {
            SetStatus("Pilih minimal 1 slot jam kosong terlebih dahulu!", COL_ERR);
        } else {
            char newSched[MAX_HARI];
            snprintf(newSched, sizeof newSched, "%s", DAY_OPTIONS[selectedDayIdx]);

            int added = 0;
            for (int s = 0; s < SESSION_OPTION_COUNT && added < 2; s++) {
                if (slotChecked[s]) {
                    strncat(newSched, ", ", sizeof(newSched) - strlen(newSched) - 1);
                    strncat(newSched, SESSION_OPTIONS[s], sizeof(newSched) - strlen(newSched) - 1);
                    added++;
                }
            }

            snprintf(currentUserCourse, sizeof currentUserCourse, "%s", liveCourse);
            snprintf(currentUserSched,  sizeof currentUserSched,  "%s", newSched);
            if (loggedBuddyIdx >= 0 && loggedBuddyIdx < buddyCount) {
                snprintf(buddies[loggedBuddyIdx].matkul, sizeof buddies[loggedBuddyIdx].matkul, "%s", liveCourse);
                snprintf(buddies[loggedBuddyIdx].hari,   sizeof buddies[loggedBuddyIdx].hari,   "%s", newSched);
                Persist();
            }
            SetStatus("Mata kuliah & jadwal kosong berhasil disimpan!", COL_OK);
        }
    }
}
/* ====================================================================== */
/* SCREEN 6 & 7: Study Groups & Group Detail                              */
/* ====================================================================== */

static void DrawGroupsScreen(void)
{
    DrawEditorialHeader("LEARN AS A TEAM",
                        "Study Groups",
                        "Find your people, share knowledge, and grow together.",
                        NULL);

    float cx = ContentX();
    float cw = ContentW();

    if (Button((Rectangle){ cx + cw - 156, 68, 156, 36 }, "+ Create Group", true, COL_PRIMARY)) {
        showCreateGroupModal = !showCreateGroupModal;
        newGroupName[0] = '\0';
        if (strcmp(currentUserCourse, "Belum diatur") != 0) {
            snprintf(newGroupCourse, sizeof newGroupCourse, "%s", currentUserCourse);
        } else {
            newGroupCourse[0] = '\0';
        }
    }

    float listTop = 128.0f;

    /* Cek apakah user punya undangan masuk */
    int pendingInviteIdx = -1;
    for (int i = 0; i < inviteCount; i++) {
        if (strcmp(groupInvites[i].targetNim, currentUserNim) == 0) {
            pendingInviteIdx = i;
            break;
        }
    }

    if (pendingInviteIdx >= 0) {
        GroupInvite *inv = &groupInvites[pendingInviteIdx];
        Rectangle invBox = { cx, listTop, cw, 56 };
        DrawCard(invBox, TILE_BLUE_BG, COL_PRIMARY, 14.0f);

        char invMsg[160];
        snprintf(invMsg, sizeof invMsg, "Undangan Grup: %s mengajakmu bergabung ke '%s'",
                 inv->senderName, inv->groupName);
        DrawTextBold(invMsg, PX(invBox.x + 16), PX(invBox.y + 19), 13, PX(cw - 220), COL_NAVY);

        if (Button((Rectangle){ invBox.x + cw - 196, invBox.y + 11, 88, 34 }, "Terima", true, COL_OK)) {
            int targetGrp = -1;
            for (int g = 0; g < groupCount; g++) {
                if (strcmp(groups[g].namaGroup, inv->groupName) == 0) {
                    targetGrp = g;
                    break;
                }
            }
            if (targetGrp < 0) {
                SetStatus("Grup sudah dibubarkan oleh creator.", COL_ERR);
            } else if (IsMemberOfGroup(targetGrp, currentUserName)) {
                SetStatus("Kamu sudah menjadi anggota grup ini.", COL_WARN);
            } else if (AddGroupMember(targetGrp, currentUserName)) {
                SetStatus("Undangan diterima! Kamu bergabung ke grup.", COL_OK);
            } else {
                SetStatus("Gagal bergabung: kapasitas grup sudah penuh.", COL_ERR);
            }
            RemoveGroupInvite(pendingInviteIdx);
            Persist();
        }

        if (OutlineButton((Rectangle){ invBox.x + cw - 100, invBox.y + 11, 84, 34 }, "Tolak", COL_ERR, COL_ERR)) {
            RemoveGroupInvite(pendingInviteIdx);
            Persist();
            SetStatus("Undangan grup ditolak.", COL_WARN);
        }

        listTop += 68.0f;
    }

    float usableW = cw - 168.0f;
    float nameW   = usableW * 0.52f;
    float courseW = usableW - nameW - 12.0f;

    if (showCreateGroupModal) {
        Rectangle box = { cx, listTop, cw, 60 };
        DrawCard(box, WHITE, COL_PRIMARY, 14.0f);

        TextInput(ID_NEW_GROUP_NAME, (Rectangle){ box.x + 12, box.y + 10, nameW, 40 },
                  newGroupName, MAX_GROUP_NAME, "Nama grup baru...", false);

        TextInput(ID_NEW_GROUP_COURSE, (Rectangle){ box.x + 12 + nameW + 12, box.y + 10, courseW, 40 },
                  newGroupCourse, MAX_MATKUL, "Mata kuliah fokus...", false);

        if (Button((Rectangle){ box.x + cw - 144, box.y + 10, 132, 40 }, "Buat Grup", true, COL_OK)) {
            char cleanCourse[MAX_MATKUL];
            snprintf(cleanCourse, sizeof cleanCourse, "%s", newGroupCourse);
            SanitizeText(cleanCourse);

            if (cleanCourse[0] == '\0') {
                SetStatus("Isi mata kuliah fokus grup terlebih dahulu.", COL_ERR);
            } else {
                Group g;
                memset(&g, 0, sizeof g);
                snprintf(g.namaGroup, sizeof g.namaGroup, "%s", newGroupName);
                SanitizeText(g.namaGroup);
                if (g.namaGroup[0] == '\0') snprintf(g.namaGroup, sizeof g.namaGroup, "Grup %s", cleanCourse);
                snprintf(g.matkul, sizeof g.matkul, "%s", cleanCourse);
                g.maxAnggota = MAX_GROUP_MEMBERS;

                int idx = AddGroup(&g);
                if (idx >= 0) {
                    AddGroupMember(idx, currentUserName);
                    Persist();
                    showCreateGroupModal = false;
                    selectedGroupIdx = idx;
                    groupsPage = idx / GROUPS_PAGE_SIZE;
                    SetStatus("Your study group has been created!", COL_OK);
                    GoView(VIEW_GROUP_DETAIL);
                    return;
                }
            }
        }
        listTop += 72.0f;
    }

    if (groupCount == 0) {
        DrawTextFit("Belum ada grup belajar. Klik '+ Create Group' di kanan atas!", PX(cx), PX(listTop + 20), 15, 480, COL_MUTED);
        return;
    }

    int totalPages = (groupCount + GROUPS_PAGE_SIZE - 1) / GROUPS_PAGE_SIZE;
    if (totalPages < 1) totalPages = 1;
    if (groupsPage >= totalPages) groupsPage = totalPages - 1;
    if (groupsPage < 0) groupsPage = 0;

    int startIdx = groupsPage * GROUPS_PAGE_SIZE;
    int endIdx   = startIdx + GROUPS_PAGE_SIZE;
    if (endIdx > groupCount) endIdx = groupCount;

    float cardW = (cw - 16.0f) / 2.0f;
    float cardH = (listTop > 140.0f) ? 148.0f : GROUP_CARD_H;

    for (int i = startIdx; i < endIdx; i++) {
        int localIdx = i - startIdx;
        int col = localIdx % 2;
        int row = localIdx / 2;
        Rectangle r = { cx + col * (cardW + 16.0f), listTop + row * (cardH + 14.0f), cardW, cardH };

        bool full = groups[i].jumlahAnggota >= groups[i].maxAnggota;
        bool hover = MouseOver(r);
        DrawCard(r, WHITE, hover ? COL_PRIMARY : COL_BORDER, 16.0f);

        DrawRectangleRounded((Rectangle){ r.x + 16, r.y + 16, 42, 42 }, 0.3f, 8, TILE_BLUE_BG);
        DrawTextBold("SG", PX(r.x + 26), PX(r.y + 28), 14, 30, COL_PRIMARY);

        DrawTextBold(groups[i].namaGroup, PX(r.x + 70), PX(r.y + 18), 16, PX(r.width - 140), COL_NAVY);
        DrawTextFit(groups[i].matkul,     PX(r.x + 70), PX(r.y + 40), 13, PX(r.width - 140), COL_MUTED);

        char cnt[24];
        snprintf(cnt, sizeof cnt, "%d/%d", groups[i].jumlahAnggota, groups[i].maxAnggota);
        DrawTextBold(cnt, PX(r.x + r.width - 52), PX(r.y + 20), 14, 45, full ? COL_ERR : COL_PRIMARY);

        for (int k = 0; k < groups[i].jumlahAnggota; k++) {
            DrawAvatar(r.x + 30 + k * 24, r.y + 82, 13, groups[i].anggotaNama[k],
                       (k % 2 == 0) ? TILE_BLUE_BG : TILE_AMBER_BG, COL_NAVY);
        }

        char names[200] = "";
        for (int k = 0; k < groups[i].jumlahAnggota; k++) {
            if (k > 0) strncat(names, ", ", sizeof(names) - strlen(names) - 1);
            strncat(names, groups[i].anggotaNama[k], sizeof(names) - strlen(names) - 1);
        }
        DrawTextFit(names, PX(r.x + 18), PX(r.y + 106), 12, PX(r.width - 36), COL_TEXT);
        DrawTextBold("Open group detail & chat  >", PX(r.x + 18), PX(r.y + cardH - 30), 12, PX(r.width - 36), COL_PRIMARY);

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selectedGroupIdx = i;
            GoView(VIEW_GROUP_DETAIL);
        }
    }

    DrawPaginationBar(cx, cw, ContentBottom() - 34.0f, &groupsPage, groupCount, GROUPS_PAGE_SIZE);
}

static void DrawGroupDetailScreen(void)
{
    if (groupCount == 0 || selectedGroupIdx < 0 || selectedGroupIdx >= groupCount) {
        GoView(VIEW_GROUPS);
        return;
    }

    Group *g = &groups[selectedGroupIdx];
    bool full = g->jumlahAnggota >= g->maxAnggota;
    bool alreadyMember = false;
    for (int k = 0; k < g->jumlahAnggota; k++) {
        if (strcmp(g->anggotaNama[k], currentUserName) == 0) alreadyMember = true;
    }

    bool isCreator = (g->jumlahAnggota > 0 && strcmp(g->anggotaNama[0], currentUserName) == 0);
    bool isAdmin   = IsCurrentUserAdmin();
    bool canDelete = (isCreator || isAdmin);

    float cx = ContentX();
    float cw = ContentW();

    if (OutlineButton((Rectangle){ cx, 56, 92, 32 }, "< Back", COL_BORDER, COL_NAVY)) {
        confirmDeleteGroup = false;
        showInviteModal = false;
        GoView(VIEW_GROUPS);
        return;
    }
    DrawTextBold("Group Detail", PX(cx + 106), 58, 24, 350, COL_NAVY);

    if (isAdmin && !isCreator) {
        Rectangle admTag = { cx + cw - 175, 58, 175, 28 };
        DrawRectangleRounded(admTag, 0.5f, 8, TILE_AMBER_BG);
        DrawTextBold("Admin Moderation Mode", PX(admTag.x + 14), PX(admTag.y + 7), 12, 150, TILE_AMBER_FG);
    } else if (isCreator) {
        Rectangle crtTag = { cx + cw - 135, 58, 135, 28 };
        DrawRectangleRounded(crtTag, 0.5f, 8, TILE_BLUE_BG);
        DrawTextBold("Group Creator", PX(crtTag.x + 18), PX(crtTag.y + 7), 12, 110, COL_PRIMARY);
    }

    Rectangle topCard = { cx, 102, cw, 108 };
    DrawCard(topCard, WHITE, COL_BORDER, 18.0f);

    DrawRectangleRounded((Rectangle){ topCard.x + 20, topCard.y + 26, 56, 56 }, 0.3f, 10, TILE_BLUE_BG);
    DrawTextBold("SG", PX(topCard.x + 34), PX(topCard.y + 44), 20, 40, COL_PRIMARY);

    DrawTextBold(g->namaGroup, PX(topCard.x + 92), PX(topCard.y + 22), 22, PX(cw - 320), COL_NAVY);
    char meta[120];
    snprintf(meta, sizeof meta, "%s  |  %d/%d Anggota  |  Next Meeting: Perpustakaan Kampus",
             g->matkul, g->jumlahAnggota, g->maxAnggota);
    DrawTextFit(meta, PX(topCard.x + 92), PX(topCard.y + 52), 13, PX(cw - 320), COL_MUTED);

    if (isCreator) {
        if (Button((Rectangle){ topCard.x + cw - 176, topCard.y + 34, 152, 40 },
                   full ? "Grup Penuh" : "+ Invite", !full, COL_PRIMARY)) {
            showInviteModal = !showInviteModal;
            inviteSearch[0] = '\0';
            if (showInviteModal) focusId = ID_INVITE_SEARCH;
        }
    } else if (alreadyMember) {
        if (OutlineButton((Rectangle){ topCard.x + cw - 168, topCard.y + 34, 144, 40 },
                          "Leave Group", COL_ERR, COL_ERR)) {
            if (RemoveGroupMember(selectedGroupIdx, currentUserName)) {
                Persist();
                alreadyMember = false;
                SetStatus("Kamu keluar dari grup. Akses chat grup dikunci kembali.", COL_WARN);
            }
        }
    } else {
        if (Button((Rectangle){ topCard.x + cw - 168, topCard.y + 34, 144, 40 },
                   full ? "Grup Penuh" : "+ Join Group", !full, COL_PRIMARY)) {
            AddGroupMember(selectedGroupIdx, currentUserName);
            Persist();
            alreadyMember = true;
            SetStatus("Berhasil bergabung ke grup belajar!", COL_OK);
        }
    }

    Rectangle memCard = { cx, 224, cw, ContentBottom() - 284 };
    DrawCard(memCard, WHITE, COL_BORDER, 18.0f);
    DrawTextBold("Daftar Anggota Kelompok", PX(memCard.x + 24), PX(memCard.y + 18), 16, 300, COL_NAVY);

    for (int k = 0; k < g->jumlahAnggota; k++) {
        float ay = memCard.y + 52 + k * 44.0f;
        DrawAvatar(memCard.x + 42, ay + 16, 15, g->anggotaNama[k], TILE_BLUE_BG, COL_NAVY);
        DrawTextBold(g->anggotaNama[k], PX(memCard.x + 70), PX(ay + 7), 15, PX(cw - 360), COL_TEXT);

        const char *memberNim = NULL;
        for (int b = 0; b < buddyCount; b++) {
            if (strcmp(buddies[b].nama, g->anggotaNama[k]) == 0) {
                memberNim = buddies[b].nim;
                break;
            }
        }

        bool isSelf = (strcmp(g->anggotaNama[k], currentUserName) == 0);
        float rightCursor = memCard.x + cw - 24.0f;

        if (k == 0) {
            rightCursor -= 96.0f;
            DrawRectangleRounded((Rectangle){ rightCursor, ay + 3, 96, 26 }, 0.5f, 8, TILE_AMBER_BG);
            DrawTextBold("Creator", PX(rightCursor + 22), PX(ay + 9), 12, 80, TILE_AMBER_FG);
        } else if (isCreator) {
            rightCursor -= 68.0f;
            if (OutlineButton((Rectangle){ rightCursor, ay + 2, 68, 28 }, "Kick", COL_ERR, COL_ERR)) {
                char kickedName[MAX_NAMA];
                snprintf(kickedName, sizeof kickedName, "%s", g->anggotaNama[k]);
                if (RemoveGroupMember(selectedGroupIdx, kickedName)) {
                    Persist();
                    char kickMsg[128];
                    snprintf(kickMsg, sizeof kickMsg, "%s dikeluarkan dari grup.", kickedName);
                    SetStatus(kickMsg, COL_WARN);
                    break;
                }
            }
        } else {
            rightCursor -= 76.0f;
            DrawTextFit(isSelf ? "You" : "Member", PX(rightCursor + 12), PX(ay + 9), 12, 70, COL_MUTED);
        }

        if (!isSelf && memberNim != NULL) {
            rightCursor -= 82.0f;
            if (OutlineButton((Rectangle){ rightCursor, ay + 2, 74, 28 }, "Chat", COL_PRIMARY, COL_PRIMARY)) {
                OpenDirectChatWithNim(memberNim, g->anggotaNama[k]);
            }
        }
    }

    float btnY = ContentBottom() - 44.0f;

    if (!canDelete) {
        if (alreadyMember) {
            if (Button((Rectangle){ cx, btnY, cw, 44 }, "Buka Chat Group", true, COL_PRIMARY)) {
                char rKey[64];
                snprintf(rKey, sizeof rKey, "G:%s", g->namaGroup);
                OpenChatRoom(rKey, g->namaGroup);
            }
        } else {
            const char *joinChatLabel = full ? "Grup Penuh (Chat Khusus Anggota)"
                                             : "+ Join Group untuk Membuka Chat";
            if (Button((Rectangle){ cx, btnY, cw, 44 }, joinChatLabel, !full, COL_PRIMARY)) {
                AddGroupMember(selectedGroupIdx, currentUserName);
                Persist();
                SetStatus("Berhasil bergabung! Sekarang kamu bisa membuka chat grup.", COL_OK);
            }
        }
    } else if (!confirmDeleteGroup) {
        float delW  = isCreator ? 196.0f : 226.0f;
        float chatW = cw - delW - 14.0f;
        const char *delLabel = isCreator ? "Bubarkan Grup" : "Hapus Grup (Admin)";

        if (alreadyMember) {
            if (Button((Rectangle){ cx, btnY, chatW, 44 }, "Buka Chat Group", true, COL_PRIMARY)) {
                char rKey[64];
                snprintf(rKey, sizeof rKey, "G:%s", g->namaGroup);
                OpenChatRoom(rKey, g->namaGroup);
            }
        } else {
            const char *joinChatLabel = full ? "Chat Khusus Anggota (Grup Penuh)"
                                             : "+ Join Group untuk Membuka Chat";
            if (Button((Rectangle){ cx, btnY, chatW, 44 }, joinChatLabel, !full, COL_PRIMARY)) {
                AddGroupMember(selectedGroupIdx, currentUserName);
                Persist();
                SetStatus("Berhasil bergabung ke grup belajar!", COL_OK);
            }
        }

        if (OutlineButton((Rectangle){ cx + chatW + 14.0f, btnY, delW, 44 }, delLabel, COL_ERR, COL_ERR)) {
            confirmDeleteGroup = true;
        }
    } else {
        float cancelW  = 110.0f;
        float confirmW = 190.0f;
        float infoW    = cw - cancelW - confirmW - 24.0f;

        Rectangle warnBox = { cx, btnY, infoW, 44 };
        DrawCard(warnBox, (Color){ 254, 238, 240, 255 }, COL_ERR, 12.0f);
        DrawTextBold(isCreator ? "Yakin ingin membubarkan grup ini permanen?"
                               : "Admin: Hapus grup ini karena pelanggaran?",
                     PX(warnBox.x + 16), PX(warnBox.y + 14), 13, PX(infoW - 28), COL_ERR);

        if (Button((Rectangle){ cx + infoW + 12.0f, btnY, confirmW, 44 }, "Ya, Hapus Permanen", true, COL_ERR)) {
            char deletedName[MAX_GROUP_NAME];
            snprintf(deletedName, sizeof deletedName, "%s", g->namaGroup);

            RemoveGroup(selectedGroupIdx);
            Persist();
            confirmDeleteGroup = false;
            selectedGroupIdx = 0;

            char msg[128];
            snprintf(msg, sizeof msg, isCreator ? "Grup '%s' telah dibubarkan."
                                                : "Admin menghapus grup '%s'.", deletedName);
            SetStatus(msg, COL_WARN);
            GoView(VIEW_GROUPS);
            return;
        }

        if (OutlineButton((Rectangle){ cx + infoW + confirmW + 24.0f, btnY, cancelW, 44 }, "Batal", COL_BORDER, COL_TEXT)) {
            confirmDeleteGroup = false;
        }
    }

    /* Popover Invite Teman khusus Creator Group */
    if (isCreator && showInviteModal) {
        float popW = 340.0f;
        float popH = 268.0f;
        Rectangle pop = { topCard.x + cw - popW - 24.0f, topCard.y + 82.0f, popW, popH };

        DrawRectangleRounded((Rectangle){ pop.x + 2, pop.y + 4, pop.width, pop.height }, 0.10f, 12, Fade(COL_NAVY, 0.12f));
        DrawCard(pop, WHITE, COL_PRIMARY, 16.0f);

        DrawTextBold("INVITE TEMAN KE GRUP", PX(pop.x + 16), PX(pop.y + 14), 11, 200, COL_PRIMARY);
        if (OutlineButton((Rectangle){ pop.x + popW - 72, pop.y + 10, 56, 24 }, "Tutup", COL_BORDER, COL_MUTED)) {
            showInviteModal = false;
        }

        TextInput(ID_INVITE_SEARCH, (Rectangle){ pop.x + 16, pop.y + 40, popW - 32, 34 },
                  inviteSearch, sizeof inviteSearch, "Cari nama atau NIM...", false);

        int shown = 0;
        for (int b = 0; b < buddyCount && shown < 4; b++) {
            if (strcmp(buddies[b].nim, currentUserNim) == 0) continue;
            if (IsMemberOfGroup(selectedGroupIdx, buddies[b].nama)) continue;
            if (!ContainsIgnoreCase(buddies[b].nama, inviteSearch) &&
                !ContainsIgnoreCase(buddies[b].nim, inviteSearch)) continue;

            float ry = pop.y + 84.0f + shown * 42.0f;
            DrawTextBold(buddies[b].nama, PX(pop.x + 16), PX(ry + 4), 13, PX(popW - 125), COL_NAVY);
            DrawTextFit(buddies[b].nim,   PX(pop.x + 16), PX(ry + 20), 11, PX(popW - 125), COL_MUTED);

            bool alreadyInvited = false;
            for (int iv = 0; iv < inviteCount; iv++) {
                if (strcmp(groupInvites[iv].groupName, g->namaGroup) == 0 &&
                    strcmp(groupInvites[iv].targetNim, buddies[b].nim) == 0) {
                    alreadyInvited = true;
                    break;
                }
            }

            if (alreadyInvited) {
                DrawTextBold("Terkirim", PX(pop.x + popW - 82), PX(ry + 10), 11, 66, COL_OK);
            } else if (Button((Rectangle){ pop.x + popW - 88, ry + 4, 72, 28 }, "Invite", !full, COL_PRIMARY)) {
                GroupInvite inv;
                memset(&inv, 0, sizeof inv);
                snprintf(inv.groupName,  sizeof inv.groupName,  "%s", g->namaGroup);
                snprintf(inv.senderName, sizeof inv.senderName, "%s", currentUserName);
                snprintf(inv.targetNim,  sizeof inv.targetNim,  "%s", buddies[b].nim);

                if (AddGroupInvite(&inv) >= 0) {
                    Persist();
                    char msg[128];
                    snprintf(msg, sizeof msg, "Ajakan bergabung ke grup dikirim ke %s!", buddies[b].nama);
                    SetStatus(msg, COL_OK);
                }
            }
            shown++;
        }

        if (shown == 0) {
            DrawTextFit("Tidak ada teman yang tersedia untuk di-invite.", PX(pop.x + 16), PX(pop.y + 115), 12, PX(popW - 32), COL_MUTED);
        }

        nextBlockActive = true;
        nextBlockRect = pop;
    }
}

/* ====================================================================== */
/* SCREEN 8: Learning Goals                                               */
/* ====================================================================== */

static void DrawGoalsScreen(void)
{
    DrawEditorialHeader("ONE STEP AT A TIME",
                        "Learning Goals",
                        "Set meaningful goals and celebrate every little win (Max 4 goals).",
                        NULL);

    float cx = ContentX();
    float cw = ContentW();

    int myTotal = 0;
    int myDone  = 0;
    for (int i = 0; i < goalCount; i++) {
        if (strcmp(goals[i].nimPemilik, currentUserNim) == 0) {
            myTotal++;
            if (goals[i].status == 1) myDone++;
        }
    }

    bool isFull = (myTotal >= MAX_USER_GOALS);

    Rectangle progCard = { cx, 128, cw, 76 };
    DrawCard(progCard, WHITE, COL_BORDER, 16.0f);
    DrawTextBold("My Goals Progress", PX(progCard.x + 20), PX(progCard.y + 14), 15, 250, COL_NAVY);

    char statStr[40];
    snprintf(statStr, sizeof statStr, "%d / %d completed", myDone, myTotal);
    DrawTextBold(statStr, PX(progCard.x + cw - 155), PX(progCard.y + 14), 14, 135, COL_OK);

    Rectangle bar = { progCard.x + 20, progCard.y + 44, cw - 40, 10 };
    DrawRectangleRounded(bar, 1.0f, 8, (Color){ 237, 241, 247, 255 });
    float ratio = (myTotal > 0) ? (float)myDone / (float)myTotal : 0.0f;
    if (ratio > 0.0f) {
        DrawRectangleRounded((Rectangle){ bar.x, bar.y, bar.width * ratio, bar.height }, 1.0f, 8, COL_OK);
    }

    bool enter = TextInput(ID_GOAL_INPUT, (Rectangle){ cx, 216, cw - 162, 42 },
                           goalInput, MAX_GOAL_DESC,
                           isFull ? "Batas maksimal 4 target belajar tercapai..." : "What would you like to achieve?",
                           false);

    if (Button((Rectangle){ cx + cw - 148, 216, 148, 42 },
               isFull ? "Max 4 Goals" : "+ Add Goal", !isFull, COL_PRIMARY) || enter) {
        if (isFull) {
            SetStatus("Maksimal 4 target belajar! Hapus salah satu untuk menambah baru.", COL_WARN);
        } else {
            Goal g;
            memset(&g, 0, sizeof g);
            snprintf(g.nimPemilik, sizeof g.nimPemilik, "%s", currentUserNim);
            snprintf(g.deskripsi,  sizeof g.deskripsi,  "%s", goalInput);
            g.status = 0;
            SanitizeText(g.deskripsi);

            if (g.deskripsi[0] == '\0') {
                SetStatus("Deskripsi target belajar tidak boleh kosong.", COL_ERR);
            } else if (AddGoal(&g) >= 0) {
                goalInput[0] = '\0';
                Persist();
                SetStatus("New goal added!", COL_OK);
            } else {
                SetStatus("Gagal menambahkan target belajar!", COL_ERR);
            }
        }
    }

    if (myTotal == 0) {
        Rectangle emptyCard = { cx, 274.0f, cw, 110.0f };
        DrawCard(emptyCard, WHITE, COL_BORDER, 16.0f);

        const char *msg = "Kamu belum memiliki target belajar. Tambahkan target pertamamu di atas!";
        Font fr = GetFontReg();
        Vector2 tsz = MeasureTextEx(fr, msg, 14.0f, 0.5f);
        DrawTextEx(fr, msg,
                   (Vector2){ emptyCard.x + (cw - tsz.x) * 0.5f, emptyCard.y + (emptyCard.height - tsz.y) * 0.5f },
                   14.0f, 0.5f, COL_MUTED);
        return;
    }

    int renderedIdx = 0;
    for (int i = 0; i < goalCount && renderedIdx < MAX_USER_GOALS; i++) {
        if (strcmp(goals[i].nimPemilik, currentUserNim) != 0) continue;

        float y = 272.0f + renderedIdx * GOAL_ROW_H;
        renderedIdx++;

        Rectangle row = { cx, y, cw, 64 };
        bool isDone = (goals[i].status == 1);
        DrawCard(row, WHITE, COL_BORDER, 15.0f);

        if (isDone) {
            DrawCircle(PX(row.x + 28), PX(y + 32), 11, COL_OK);
            DrawCircle(PX(row.x + 28), PX(y + 32), 4, WHITE);
        } else {
            DrawCircleLines(PX(row.x + 28), PX(y + 32), 11, COL_PRIMARY);
        }

        DrawTextBold(goals[i].deskripsi, PX(row.x + 52), PX(y + 14), 15, PX(row.width - 310), COL_NAVY);
        DrawTextFit(isDone ? "Completed" : "On Progress", PX(row.x + 52), PX(y + 36), 12, 180, isDone ? COL_OK : COL_WARN);

        if (Button((Rectangle){ row.x + row.width - 228, y + 15, 108, 34 },
                   isDone ? "Reopen" : "Selesai", true, isDone ? COL_MUTED : COL_PRIMARY)) {
            goals[i].status = isDone ? 0 : 1;
            Persist();
        }

        if (OutlineButton((Rectangle){ row.x + row.width - 108, y + 15, 94, 34 },
                          "Hapus", COL_ERR, COL_ERR)) {
            RemoveGoal(i);
            Persist();
            SetStatus("Target belajar berhasil dihapus.", COL_WARN);
            break;
        }
    }
}

/* ====================================================================== */
/* FLOATING CHAT BOX (Tab Kecil di Pojok Kanan Bawah, Max 12 Pesan/Room)  */
/* ====================================================================== */

static void DrawFloatingChatBox(void)
{
    float sw = GetVirtW();
    float sh = VIRT_H;
    Vector2 m = GetUiMouse();
    Font fr = GetFontReg();

    #define MAX_CHAT_ROOMS 128
    char roomKeys[MAX_CHAT_ROOMS][64];
    char roomLabels[MAX_CHAT_ROOMS][64];
    const char *ddItems[MAX_CHAT_ROOMS];
    int roomCount = 0;

    for (int i = 0; i < groupCount && roomCount < MAX_CHAT_ROOMS; i++) {
        if (IsMemberOfGroup(i, currentUserName)) {
            snprintf(roomKeys[roomCount],   64, "G:%s", groups[i].namaGroup);
            snprintf(roomLabels[roomCount], 64, "[Grup] %s", groups[i].namaGroup);
            ddItems[roomCount] = roomLabels[roomCount];
            roomCount++;
        }
    }

    for (int d = 0; d < dmContactCount && roomCount < MAX_CHAT_ROOMS; d++) {
        if (strcmp(dmOwnerNim[d], currentUserNim) != 0) continue;

        const char *targetName = dmTargetNim[d];
        for (int b = 0; b < buddyCount; b++) {
            if (strcmp(buddies[b].nim, dmTargetNim[d]) == 0) {
                targetName = buddies[b].nama;
                break;
            }
        }

        MakeDmRoomKey(roomKeys[roomCount], 64, currentUserNim, dmTargetNim[d]);
        snprintf(roomLabels[roomCount], 64, "[Member] %s", targetName);
        ddItems[roomCount] = roomLabels[roomCount];
        roomCount++;
    }

    if (roomCount > 0) {
        int found = -1;
        for (int i = 0; i < roomCount; i++) {
            if (strcmp(roomKeys[i], activeChatKey) == 0) {
                found = i;
                break;
            }
        }
        if (found >= 0) {
            chatRoomDd.selected = found;
        } else {
            chatRoomDd.selected = 0;
            snprintf(activeChatKey,   sizeof activeChatKey,   "%s", roomKeys[0]);
            snprintf(activeChatTitle, sizeof activeChatTitle, "%s", roomLabels[0]);
        }
    } else {
        activeChatKey[0] = '\0';
        snprintf(activeChatTitle, sizeof activeChatTitle, "Belum ada obrolan");
    }

    /* Mode Minimized: Tab Kecil di Pojok Kanan Bawah */
    if (chatMinimized) {
        float minW = 218.0f;
        float minH = 44.0f;
        Rectangle minBtn = { sw - minW - 22.0f, sh - minH - 14.0f, minW, minH };
        bool hover = CheckCollisionPointRec(m, minBtn);

        DrawCard(minBtn, WHITE, hover ? COL_PRIMARY : COL_BORDER, 22.0f);
        DrawCircle(PX(minBtn.x + 22), PX(minBtn.y + minH * 0.5f), 5.0f,
                   roomCount > 0 ? COL_OK : COL_DISABLED);

        char tabLabel[48];
        snprintf(tabLabel, sizeof tabLabel, "Messages (%d)", roomCount);
        DrawTextBold(tabLabel, PX(minBtn.x + 36), PX(minBtn.y + 14), 13, 136, COL_NAVY);

        Rectangle plusBadge = { minBtn.x + minW - 34.0f, minBtn.y + 9.0f, 26.0f, 26.0f };
        DrawRectangleRounded(plusBadge, 0.5f, 8, COL_PRIMARY_LT);
        DrawTextBold("+", PX(plusBadge.x + 9), PX(plusBadge.y + 4), 16, 16, COL_PRIMARY);

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (!chatInitPos) {
                chatWinRect.width  = 340.0f;
                chatWinRect.height = 410.0f;
                chatWinRect.x = sw - chatWinRect.width - 22.0f;
                chatWinRect.y = sh - chatWinRect.height - 14.0f;
                chatInitPos = true;
            }
            chatMinimized  = false;
            focusId        = ID_CHAT_INPUT;
            justOpenedChat = true;
        }
        nextBlockRect = minBtn;
        nextBlockActive = true;
        return;
    }

    if (!chatInitPos) {
        chatWinRect.width  = 340.0f;
        chatWinRect.height = 410.0f;
        chatWinRect.x = sw - chatWinRect.width - 22.0f;
        chatWinRect.y = sh - chatWinRect.height - 14.0f;
        chatInitPos = true;
    }

    float minW = 290.0f, maxW = sw * 0.60f;
    float minH = 320.0f, maxH = sh - 30.0f;

    Rectangle headerRect = { chatWinRect.x, chatWinRect.y, chatWinRect.width, 44.0f };
    Rectangle minRect    = { chatWinRect.x + chatWinRect.width - 38.0f, chatWinRect.y + 8.0f, 28.0f, 28.0f };
    Rectangle resizeGrip = { chatWinRect.x + chatWinRect.width - 24.0f, chatWinRect.y + chatWinRect.height - 24.0f, 24.0f, 24.0f };

    /* Drag & Resize */
    if (!justOpenedChat && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(m, minRect)) {
            chatMinimized = true;
            chatDragging = false;
            chatResizing = false;
            chatRoomDd.open = false;
            return;
        } else if (CheckCollisionPointRec(m, resizeGrip)) {
            chatResizing = true;
            chatDragging = false;
            chatResizeStart = m;
            chatStartSize = (Vector2){ chatWinRect.width, chatWinRect.height };
        } else if (CheckCollisionPointRec(m, headerRect)) {
            chatDragging = true;
            chatResizing = false;
            chatDragOffset = (Vector2){ m.x - chatWinRect.x, m.y - chatWinRect.y };
        }
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        chatDragging = false;
        chatResizing = false;
    }

    if (chatDragging) {
        chatWinRect.x = m.x - chatDragOffset.x;
        chatWinRect.y = m.y - chatDragOffset.y;
    }
    if (chatResizing) {
        chatWinRect.width  = chatStartSize.x + (m.x - chatResizeStart.x);
        chatWinRect.height = chatStartSize.y + (m.y - chatResizeStart.y);
        if (chatWinRect.width < minW)  chatWinRect.width = minW;
        if (chatWinRect.width > maxW)  chatWinRect.width = maxW;
        if (chatWinRect.height < minH) chatWinRect.height = minH;
        if (chatWinRect.height > maxH) chatWinRect.height = maxH;
    }

    if (chatWinRect.x < 10.0f) chatWinRect.x = 10.0f;
    if (chatWinRect.y < 10.0f) chatWinRect.y = 10.0f;
    if (chatWinRect.x + chatWinRect.width > sw - 10.0f)  chatWinRect.x = sw - chatWinRect.width - 10.0f;
    if (chatWinRect.y + chatWinRect.height > sh - 10.0f) chatWinRect.y = sh - chatWinRect.height - 10.0f;

    /* Gambar Frame Kartu Chat */
    DrawRectangleRounded((Rectangle){ chatWinRect.x + 3, chatWinRect.y + 6, chatWinRect.width, chatWinRect.height },
                         0.06f, 12, (Color){ 24, 52, 95, 22 });
    DrawCard(chatWinRect, WHITE, COL_BORDER, 18.0f);

    DrawRectangleRounded((Rectangle){ chatWinRect.x + 1, chatWinRect.y + 1, chatWinRect.width - 2, 43.0f },
                         0.28f, 10, COL_PRIMARY);
    DrawRectangle(PX(chatWinRect.x + 1), PX(chatWinRect.y + 26), PX(chatWinRect.width - 2), 18, COL_PRIMARY);

    DrawCircle(PX(chatWinRect.x + 18), PX(chatWinRect.y + 22), 4.5f, roomCount > 0 ? COL_OK : COL_DISABLED);
    DrawTextBold("Messages", PX(chatWinRect.x + 30), PX(chatWinRect.y + 14), 14, 150, WHITE);

    bool hoverMin = CheckCollisionPointRec(m, minRect);
    DrawRectangleRounded(minRect, 0.35f, 8, hoverMin ? COL_NAVY_DARK : (Color){ 42, 96, 194, 255 });
    DrawRectangle(PX(minRect.x + 8), PX(minRect.y + 13), 12, 2, WHITE);

    float pad = 12.0f;
    float innerW = chatWinRect.width - pad * 2.0f;

    Rectangle ddRect  = { chatWinRect.x + pad, chatWinRect.y + 52.0f, innerW, 34.0f };
    Rectangle msgArea = { chatWinRect.x + pad, chatWinRect.y + 94.0f, innerW, chatWinRect.height - 148.0f };
    DrawRectangleRounded(msgArea, 0.06f, 10, (Color){ 247, 249, 253, 255 });

    /* Area Bubble Chat */
    Camera2D cam = GetUiCamera();
    int sx = (int)(msgArea.x * cam.zoom + cam.offset.x);
    int sy = (int)(msgArea.y * cam.zoom + cam.offset.y);
    int swClip = (int)(msgArea.width * cam.zoom);
    int shClip = (int)(msgArea.height * cam.zoom);

    BeginScissorMode(sx, sy, swClip, shClip);

    float y = msgArea.y + 10.0f;
    int shownMsgs = 0;
    if (roomCount > 0) {
        int visIdx[MAX_MESSAGES_PER_ROOM];
        int visCount = 0;

        for (int i = chatCount - 1; i >= 0 && visCount < MAX_MESSAGES_PER_ROOM; i--) {
            if (strcmp(chatLogs[i].roomKey, activeChatKey) == 0) {
                visIdx[visCount++] = i;
            }
        }

        for (int v = visCount - 1; v >= 0; v--) {
            ChatMessage *cm = &chatLogs[visIdx[v]];
            shownMsgs++;

            bool isMine = (strcmp(cm->sender, currentUserName) == 0);
            float bubbleW = innerW * 0.72f;
            float bubbleH = 46.0f;

            if (isMine) {
                Rectangle b = { msgArea.x + innerW - bubbleW - 8.0f, y, bubbleW, bubbleH };
                DrawRectangleRounded(b, 0.32f, 10, COL_PRIMARY);
                DrawTextWrap(cm->text, PX(b.x + 10), PX(b.y + 8), 12, PX(bubbleW - 20), 15, WHITE, 2);
            } else {
                DrawTextBold(cm->sender, PX(msgArea.x + 10), PX(y), 11, PX(innerW - 20), COL_NAVY);
                Rectangle b = { msgArea.x + 8.0f, y + 14.0f, bubbleW, bubbleH };
                DrawCard(b, WHITE, COL_BORDER, 12.0f);
                DrawTextWrap(cm->text, PX(b.x + 10), PX(b.y + 8), 12, PX(bubbleW - 20), 15, COL_TEXT, 2);
                y += 14.0f;
            }
            y += bubbleH + 8.0f;
        }
    }

    if (roomCount == 0) {
        const char *noRoomMsg = "Gabung grup atau klik 'Chat' pada teman untuk mulai.";
        Vector2 tsz = MeasureTextEx(fr, noRoomMsg, 12.0f, 0.5f);
        float tx = msgArea.x + (msgArea.width - tsz.x) * 0.5f;
        if (tx < msgArea.x + 10.0f) tx = msgArea.x + 10.0f;
        DrawTextWrap(noRoomMsg, PX(tx), PX(msgArea.y + msgArea.height * 0.42f), 12, PX(msgArea.width - 20), 16, COL_MUTED, 3);
    } else if (shownMsgs == 0) {
        const char *emptyMsg = "Belum ada pesan. Sapa duluan!";
        Vector2 tsz = MeasureTextEx(fr, emptyMsg, 12.0f, 0.5f);
        DrawTextEx(fr, emptyMsg,
                   (Vector2){ msgArea.x + (msgArea.width - tsz.x) * 0.5f, msgArea.y + msgArea.height * 0.45f },
                   12.0f, 0.5f, COL_MUTED);
    }

    EndScissorMode();

    /* Input Pesan + Tombol Send */
    float iy = chatWinRect.y + chatWinRect.height - 44.0f;
    float sendW = 64.0f;
    Rectangle inputBoxRect = { chatWinRect.x + pad, iy, innerW - sendW - 8.0f, 34.0f };
    Rectangle sendBtnRect  = { chatWinRect.x + pad + innerW - sendW, iy, sendW, 34.0f };

    bool savedInputBlock = blockActive;
    blockActive = false;

    if (chatRoomDd.open) {
        int visDd = (roomCount > 3) ? 3 : roomCount;
        blockRect = (Rectangle){ ddRect.x, ddRect.y + ddRect.height + 4.0f, ddRect.width, ddRect.height * visDd };
        blockActive = true;
    }

    bool enter = TextInput(ID_CHAT_INPUT, inputBoxRect, chatInput, MAX_CHAT_LEN,
                           roomCount > 0 ? "Type a message..." : "Pilih grup/teman dulu...", false);

    /* Pertahankan fokus pada frame pembukaan chat */
    if (justOpenedChat) {
        focusId = ID_CHAT_INPUT;
        justOpenedChat = false;
    }

    bool sendClicked = Button(sendBtnRect, "Send", roomCount > 0, COL_PRIMARY);

    if ((sendClicked || enter) && chatInput[0] != '\0' && roomCount > 0) {
        if (strncmp(activeChatKey, "DM:", 3) == 0) {
            for (int d = 0; d < dmContactCount; d++) {
                if (strcmp(dmOwnerNim[d], currentUserNim) == 0) {
                    char testKey[64];
                    MakeDmRoomKey(testKey, sizeof testKey, currentUserNim, dmTargetNim[d]);
                    if (strcmp(testKey, activeChatKey) == 0) {
                        AddUserDmContact(dmTargetNim[d], currentUserNim);
                        break;
                    }
                }
            }
        }
        PushChatMessage(activeChatKey, currentUserName, chatInput);
        chatInput[0] = '\0';
        focusId = ID_CHAT_INPUT;
    }

    /* Dropdown Pemilih Room */
    blockActive = false;
    if (roomCount > 0) {
        Dropdown(&chatRoomDd, ddRect, ddItems, roomCount);
        if (chatRoomDd.selected >= 0 && chatRoomDd.selected < roomCount) {
            snprintf(activeChatKey,   sizeof activeChatKey,   "%s", roomKeys[chatRoomDd.selected]);
            snprintf(activeChatTitle, sizeof activeChatTitle, "%s", roomLabels[chatRoomDd.selected]);
        }
    } else {
        DrawCard(ddRect, (Color){ 244, 246, 250, 255 }, COL_BORDER, 10.0f);
        DrawTextFit("Belum ada grup atau kontak DM", PX(ddRect.x + 12), PX(ddRect.y + 9), 13, PX(innerW - 24), COL_MUTED);
    }

    blockActive = savedInputBlock;

    DrawLineEx((Vector2){ chatWinRect.x + chatWinRect.width - 13, chatWinRect.y + chatWinRect.height - 5 },
               (Vector2){ chatWinRect.x + chatWinRect.width - 5,  chatWinRect.y + chatWinRect.height - 13 }, 1.5f, COL_MUTED);
    DrawLineEx((Vector2){ chatWinRect.x + chatWinRect.width - 9,  chatWinRect.y + chatWinRect.height - 5 },
               (Vector2){ chatWinRect.x + chatWinRect.width - 5,  chatWinRect.y + chatWinRect.height - 9 },  1.5f, COL_MUTED);

    nextBlockRect = chatWinRect;
    nextBlockActive = true;
}

/* ====================================================================== */
/* Public Entry Points                                                    */
/* ====================================================================== */

void GuiInit(void)
{
    fontRegular = LoadFontEx("C:\\Windows\\Fonts\\segoeui.ttf",  32, 0, 128);
    fontBold    = LoadFontEx("C:\\Windows\\Fonts\\segoeuib.ttf", 32, 0, 128);
    if (fontRegular.texture.id > 0) SetTextureFilter(fontRegular.texture, TEXTURE_FILTER_BILINEAR);
    if (fontBold.texture.id > 0)    SetTextureFilter(fontBold.texture, TEXTURE_FILTER_BILINEAR);

    SeedInitialChat();
}

void GuiShutdown(void)
{
    if (fontRegular.texture.id > 0) UnloadFont(fontRegular);
    if (fontBold.texture.id > 0)    UnloadFont(fontBold);
}

void GuiUpdateAndDraw(void)
{
    blockActive = nextBlockActive;
    blockRect = nextBlockRect;
    nextBlockActive = false;

    if (focusId < 0) { while (GetCharPressed() > 0) {} }
    if (IsKeyPressed(KEY_ESCAPE)) {
        focusId = -1;
        showFindFilterBox = false;
        showInviteModal = false; // <-- TAMBAH
        chatRoomDd.open = false;
    }

    BeginDrawing();
    ClearBackground(COL_BG);

    Camera2D baseCam = GetUiCamera();
    BeginMode2D(baseCam);

    DrawAnimatedBackground(!isLoggedIn);

    if (!isLoggedIn) {
        if (currentView == VIEW_AUTH) DrawAuthScreen();
        else                          DrawSplashScreen();
    } else {
        DrawSidebar();
        DrawTopbar();
        switch (currentView) {
            case VIEW_HOME:         DrawHomeScreen();        break;
            case VIEW_FIND_BUDDY:   DrawFindBuddyScreen();   break;
            case VIEW_SCHEDULE:     DrawScheduleScreen();    break;
            case VIEW_GROUPS:       DrawGroupsScreen();      break;
            case VIEW_GROUP_DETAIL: DrawGroupDetailScreen(); break;
            case VIEW_GOALS:        DrawGoalsScreen();       break;
            default:                DrawHomeScreen();        break;
        }
        DrawFloatingChatBox();
    }

    DrawToast();

    EndMode2D();
    EndDrawing();
}