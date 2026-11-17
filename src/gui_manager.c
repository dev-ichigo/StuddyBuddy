#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "raylib.h"

#include "../include/models.h"
#include "../include/file_manager.h"
#include "../include/gui_manager.h"

/* ====================================================================== */
/* Palette & Layout Constants                                             */
/* ====================================================================== */

#define PX(v)             ((int)(v))
#define SIDEBAR_W         230
#define CONTENT_PAD       32
#define ROW_H             86.0f
#define GROUP_CARD_H      188.0f
#define GOAL_ROW_H        80.0f

#define MAX_CHAT_MSGS     60
#define MAX_CHAT_LEN      180
/* Virtual canvas resolution (base design size) */
#define VIRT_H 640.0f

static float GetVirtW(void)
{
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    if (sh <= 0.0f) return 980.0f;
    float w = VIRT_H * (sw / sh);
    return (w < 860.0f) ? 860.0f : w;
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
    /* Pusatkan kanvas secara horizontal/vertikal jika rasio layar monitor berbeda */
    cam.offset.x = (sw - (GetVirtW() * scale)) * 0.5f;
    cam.offset.y = (sh - (VIRT_H * scale)) * 0.5f;
    return cam;
}

static Vector2 GetUiMouse(void)
{
    return GetScreenToWorld2D(GetMousePosition(), GetUiCamera());
}

static const Color COL_BG           = { 246, 248, 252, 255 }; /* #F6F8FC */
static const Color COL_NAVY         = {  30,  58, 110, 255 }; /* #1E3A6E */
static const Color COL_NAVY_DARK    = {  22,  44,  86, 255 };
static const Color COL_NAVY_HOVER   = {  42,  75, 138, 255 };
static const Color COL_PRIMARY      = {  74, 123, 208, 255 }; /* #4A7BD0 */
static const Color COL_PRIMARY_LT   = { 234, 241, 253, 255 }; /* #EAF1FD */
static const Color COL_BORDER       = { 226, 232, 243, 255 };
static const Color COL_TEXT         = {  28,  42,  74, 255 };
static const Color COL_MUTED        = { 118, 130, 158, 255 };

static const Color TILE_BLUE_BG     = { 235, 242, 255, 255 };
static const Color TILE_BLUE_FG     = {  74, 123, 208, 255 };
static const Color TILE_PINK_BG     = { 253, 237, 244, 255 };
static const Color TILE_PINK_FG     = { 214,  82, 138, 255 };
static const Color TILE_MINT_BG     = { 232, 249, 242, 255 };
static const Color TILE_MINT_FG     = {  34, 165, 115, 255 };
static const Color TILE_AMBER_BG    = { 255, 247, 230, 255 };
static const Color TILE_AMBER_FG    = { 225, 152,  34, 255 };

static const Color COL_OK           = {  34, 165, 115, 255 };
static const Color COL_WARN         = { 230, 155,  35, 255 };
static const Color COL_ERR          = { 218,  72,  82, 255 };
static const Color COL_DISABLED     = { 190, 198, 214, 255 };

/* Extended Workflow States (Maps cleanly onto existing AppScreen + Auth/Detail views) */
typedef enum {
    VIEW_SPLASH = 0,
    VIEW_AUTH,          /* Sign Up / Log In */
    VIEW_HOME,          /* Personalized Dashboard */
    VIEW_FIND_BUDDY,    /* Filter & Invite Friends */
    VIEW_SCHEDULE,      /* Match Your Schedule */
    VIEW_GROUPS,        /* Study Groups List */
    VIEW_GROUP_DETAIL,  /* Group Detail & Join */
    VIEW_GOALS,         /* Learning Goals */
    VIEW_CHAT,          /* Group Chat */
    VIEW_COUNT
} WorkflowView;

enum {
    ID_AUTH_NAME,
    ID_AUTH_NIM,
    ID_AUTH_SCHED,
    ID_FIND_SEARCH,
    ID_NEW_GROUP_NAME,
    ID_GOAL_INPUT,
    ID_CHAT_INPUT
};

typedef struct {
    bool open;
    int  selected;
    float scroll;
} DropdownState;

typedef struct {
    int  groupIdx;
    char sender[MAX_NAMA];
    char text[MAX_CHAT_LEN];
    char timeStr[12];
    bool isMe;
} ChatMessage;

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
static bool          isLoginMode   = false; /* false = Sign Up, true = Log In */
static char          authName[MAX_NAMA];
static char          authNim[MAX_NIM];
static char          authSched[MAX_HARI];
static DropdownState authCourseDd  = { false, 0 };
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

static DropdownState authDayDd     = { false, 0 };
static DropdownState authSessionDd = { false, 0 };

/* Find Buddy State */
static int  findFilter = -1;
static char findSearch[64];

/* Match Schedule State (Screen 5 Mockup) */
static int  selectedDayIdx = 1; /* 0=Sen, 1=Sel, 2=Rab, 3=Kam, 4=Jum, 5=Sab, 6=Min */
static bool slotChecked[5] = { false, true, true, false, false };

/* Group Detail & Create State (Screen 6 & 7 Mockup) */
static int  selectedGroupIdx = 0;
static bool showCreateGroupModal = false;
static bool confirmDeleteGroup = false;
static char newGroupName[MAX_GROUP_NAME];
static DropdownState newGroupCourseDd = { false, 0 };

/* Goals State (Screen 8 Mockup) */
static char goalInput[MAX_GOAL_DESC];

/* Chat State (Screen 9 Mockup) */
static ChatMessage chatLogs[MAX_CHAT_MSGS];
static int         chatCount = 0;
static char        chatInput[MAX_CHAT_LEN];

/* Scroll & UI Helpers */
static Rectangle blockRect = { 0 }, nextBlockRect = { 0 };
static bool      blockActive = false, nextBlockActive = false;
static Rectangle clipRect = { 0 };
static bool      clipActive = false;
static float     scrollY[VIEW_COUNT];
#define FIND_PAGE_SIZE   4
#define GROUPS_PAGE_SIZE 4

static int findPage   = 0;
static int groupsPage = 0;

static float     backspaceHeld = 0.0f;

static char  statusMsg[128] = "";
static float statusTimer = 0.0f;
static Color statusColor = { 0 };

/* ====================================================================== */
/* Helpers & Widgets                                                      */
/* ====================================================================== */

static Font GetFontReg(void)  { return (fontRegular.texture.id > 0) ? fontRegular : GetFontDefault(); }
static Font GetFontBold(void) { return (fontBold.texture.id > 0)    ? fontBold    : GetFontReg(); }

// static float ContentX(void) { return (float)(SIDEBAR_W + CONTENT_PAD); }
// static float ContentW(void) {
//     float w = (float)GetScreenWidth() - SIDEBAR_W - (CONTENT_PAD * 2);
//     return (w < 540.0f) ? 540.0f : w;
// }
// static float ContentBottom(void) { return (float)GetScreenHeight() - 26.0f; }

static float ContentX(void)      { return (float)(SIDEBAR_W + CONTENT_PAD); }
static float ContentW(void)      { return GetVirtW() - SIDEBAR_W - (CONTENT_PAD * 2); }
static float ContentBottom(void) { return VIRT_H - 26.0f; }

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
    authCourseDd.open = false;
    newGroupCourseDd.open = false;
    showCreateGroupModal = false;
    scrollY[v] = 0.0f;
    confirmDeleteGroup = false;
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
    DrawRectangleRounded((Rectangle){ x, y, s, s }, 0.35f, 10, COL_NAVY);
    DrawTriangle((Vector2){ x + 19*scale, y + 7*scale },
                 (Vector2){ x + 10*scale, y + 23*scale },
                 (Vector2){ x + 19*scale, y + 23*scale }, WHITE);
    DrawTriangle((Vector2){ x + 21*scale, y + 11*scale },
                 (Vector2){ x + 21*scale, y + 23*scale },
                 (Vector2){ x + 29*scale, y + 23*scale }, (Color){ 195, 220, 255, 255 });
    DrawRectangleRounded((Rectangle){ x + 8*scale, y + 25*scale, 22*scale, 6*scale }, 0.8f, 6, COL_PRIMARY);
}

static bool MouseOver(Rectangle r)
{
    Vector2 m = GetUiMouse();
    if (!CheckCollisionPointRec(m, r)) return false;
    if (clipActive && !CheckCollisionPointRec(m, clipRect)) return false;
    if (blockActive && CheckCollisionPointRec(m, blockRect)) return false;
    return true;
}

static void BeginClip(Rectangle r)
{
    clipRect = r;
    clipActive = true;
    Camera2D cam = GetUiCamera();
    int sx = (int)(r.x * cam.zoom + cam.offset.x);
    int sy = (int)(r.y * cam.zoom + cam.offset.y);
    int sw = (int)(r.width * cam.zoom);
    int sh = (int)(r.height * cam.zoom);
    BeginScissorMode(sx, sy, sw, sh);
}

static void EndClip(void)
{
    clipActive = false;
    EndScissorMode();
}

static void ScrollUpdate(float *offset, Rectangle area, float contentH)
{
    float maxOff = contentH - area.height;
    if (maxOff < 0.0f) maxOff = 0.0f;
    if (MouseOver(area)) *offset -= GetMouseWheelMove() * 42.0f;
    if (*offset < 0.0f) *offset = 0.0f;
    if (*offset > maxOff) *offset = maxOff;
}

static void DrawScrollbar(Rectangle area, float offset, float contentH)
{
    if (contentH <= area.height) return;
    float maxOff = contentH - area.height;
    float thumbH = area.height * area.height / contentH;
    if (thumbH < 32.0f) thumbH = 32.0f;
    float thumbY = area.y + (area.height - thumbH) * (offset / maxOff);
    Rectangle thumb = { area.x + area.width - 6, thumbY, 5, thumbH };
    DrawRectangleRounded(thumb, 1.0f, 6, COL_DISABLED);
}

static bool Button(Rectangle r, const char *label, bool enabled, Color base)
{
    bool hover = enabled && MouseOver(r);
    Color c = !enabled ? COL_DISABLED : (hover ? ColorBrightness(base, -0.10f) : base);
    DrawRectangleRounded(r, 0.45f, 12, c);

    Font f = GetFontBold();
    Vector2 sz = MeasureTextEx(f, label, 16.0f, 0.5f);
    DrawTextEx(f, label, (Vector2){ r.x + (r.width - sz.x) / 2.0f, r.y + (r.height - sz.y) / 2.0f }, 16.0f, 0.5f, WHITE);

    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static bool OutlineButton(Rectangle r, const char *label, Color borderCol, Color textCol)
{
    bool hover = MouseOver(r);
    DrawCard(r, hover ? COL_PRIMARY_LT : WHITE, borderCol, r.height * 0.45f);

    Font f = GetFontBold();
    Vector2 sz = MeasureTextEx(f, label, 15.0f, 0.5f);
    DrawTextEx(f, label, (Vector2){ r.x + (r.width - sz.x) / 2.0f, r.y + (r.height - sz.y) / 2.0f }, 15.0f, 0.5f, textCol);

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
    Vector2 sz = MeasureTextEx(f, label, 14.0f, 0.5f);
    DrawTextEx(f, label, (Vector2){ r.x + (r.width - sz.x) / 2.0f, r.y + (r.height - sz.y) / 2.0f }, 14.0f, 0.5f, fg);

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

    int textY = PX(r.y + (r.height - 16) / 2);
    if (buf[0] == '\0') {
        if (!focused) DrawTextFit(placeholder, PX(r.x + 14), textY, 16, PX(r.width - 28), COL_DISABLED);
        if (focused && ((int)(GetTime() * 2.0) % 2 == 0))
            DrawRectangle(PX(r.x + 14), PX(r.y + 10), 2, PX(r.height - 20), COL_PRIMARY);
    } else {
        const char *shown = buf;
        while (*shown && MeasureTextEx(f, shown, 16.0f, 0.5f).x > PX(r.width) - 30) shown++;
        DrawTextFit(shown, PX(r.x + 14), textY, 16, PX(r.width - 28), COL_TEXT);
        if (focused && ((int)(GetTime() * 2.0) % 2 == 0)) {
            int cx = PX(r.x + 14) + (int)MeasureTextEx(f, shown, 16.0f, 0.5f).x + 2;
            DrawRectangle(cx, PX(r.y + 10), 2, PX(r.height - 20), COL_PRIMARY);
        }
    }
    return submitted;
}

static void Dropdown(DropdownState *d, Rectangle r, const char **items, int count)
{
    Vector2 m = GetUiMouse();
    bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    /* Batasi tinggi kotak list maksimal 4 item agar tidak kepanjangan */
    int visibleCount = (count > 3) ? 3 : count;
    float itemH = r.height;
    float totalH = itemH * count;
    float boxH = itemH * visibleCount;
    float maxScroll = totalH - boxH;
    if (maxScroll < 0.0f) maxScroll = 0.0f;

    Rectangle list = { r.x, r.y + r.height + 4, r.width, boxH };

    /* Update scroll wheel saat kursor berada di atas daftar dropdown */
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
            /* Otomatis posisikan scroll agar item yang sedang terpilih terlihat */
            d->scroll = d->selected * itemH - itemH;
            if (d->scroll < 0.0f) d->scroll = 0.0f;
            if (d->scroll > maxScroll) d->scroll = maxScroll;
        }
    }

    bool hoverHeader = CheckCollisionPointRec(m, r);
    DrawCard(r, WHITE, (d->open || hoverHeader) ? COL_PRIMARY : COL_BORDER, 12.0f);
    DrawTextFit(items[d->selected], PX(r.x + 14), PX(r.y + (r.height - 16) / 2), 16, PX(r.width - 36), COL_TEXT);

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
            DrawTextFit(items[i], PX(it.x + 12), PX(it.y + (it.height - 16) / 2), 16, PX(it.width - 18),
                        i == d->selected ? COL_PRIMARY : COL_TEXT);
        }
        EndScissorMode();

        /* Gambar indikator scrollbar kecil di sisi kanan jika item lebih dari 4 */
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
    float btnH = 34.0f;
    float barW = 310.0f;
    float bx   = cx + (cw - barW) * 0.5f;

    if (Button((Rectangle){ bx, y, btnW, btnH }, "< Prev", *page > 0, COL_PRIMARY)) {
        (*page)--;
    }

    char pageLabel[48];
    snprintf(pageLabel, sizeof pageLabel, "Page %d / %d", *page + 1, totalPages);
    Font fb = GetFontBold();
    Vector2 tsz = MeasureTextEx(fb, pageLabel, 14.0f, 0.5f);
    DrawTextEx(fb, pageLabel,
               (Vector2){ bx + (barW - tsz.x) * 0.5f, y + (btnH - tsz.y) * 0.5f },
               14.0f, 0.5f, COL_NAVY);

    if (Button((Rectangle){ bx + barW - btnW, y, btnW, btnH }, "Next >", *page < totalPages - 1, COL_PRIMARY)) {
        (*page)++;
    }
}

static bool IsCurrentUserAdmin(void)
{
    if (!isLoggedIn) return false;
    if (strcmp(currentUserNim, "2610512113") != 0) return false;

    /* Cocokkan nama secara case-insensitive agar tetap valid meski beda huruf besar/kecil */
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

    /* Reset pagination Find Buddy & Study Groups ke halaman pertama saat login */
    findPage   = 0;
    groupsPage = 0;

    /* Sinkronkan pilihan hari & maksimal 2 slot jam di Match Schedule */
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

static void SeedInitialChat(void)
{
    if (chatCount > 0) return;
    chatCount = 3;
    chatLogs[0] = (ChatMessage){ 0, "Raka Pratama", "Halo semuanya! Aku udah selesai ngerjain latihan nomor 3, ada yang mau diskusi bareng?", "14:12", false };
    chatLogs[1] = (ChatMessage){ 0, "Nayla Putri",  "Aku juga masih agak bingung di bagian pointer nih, bahas bareng yuk!", "14:16", false };
    chatLogs[2] = (ChatMessage){ 0, "Dinda Aulia",  "Gas! Kita ketemu di perpustakaan kampus jam 15.00 ya.", "14:18", false };
}

static void PushChatMessage(int groupIdx, const char *sender, const char *msg, bool isMe)
{
    if (chatCount >= MAX_CHAT_MSGS) {
        for (int i = 1; i < MAX_CHAT_MSGS; i++) chatLogs[i - 1] = chatLogs[i];
        chatCount = MAX_CHAT_MSGS - 1;
    }
    ChatMessage *m = &chatLogs[chatCount++];
    m->groupIdx = groupIdx;
    snprintf(m->sender, sizeof m->sender, "%s", sender);
    snprintf(m->text,   sizeof m->text,   "%s", msg);
    snprintf(m->timeStr, sizeof m->timeStr, "Now");
    m->isMe = isMe;
}

/* ====================================================================== */
/* SCREEN 1: Splash Screen (Mockup Screen 1)                              */
/* ====================================================================== */

static void DrawSplashScreen(void)
{
    float sw = GetVirtW();
    float sh = VIRT_H;

    /* Dekorasi gelombang pastel lembut di bagian bawah */
    DrawCircle((int)(sw * 0.15f), (int)(sh * 0.95f), 220, COL_PRIMARY_LT);
    DrawCircle((int)(sw * 0.85f), (int)(sh * 0.10f), 180, TILE_AMBER_BG);

    float cardW = 460.0f;
    float cardH = 440.0f;
    Rectangle card = { (sw - cardW) / 2.0f, (sh - cardH) / 2.0f, cardW, cardH };
    DrawCard(card, WHITE, COL_BORDER, 24.0f);

    /* Logo Besar di Tengah */
    DrawBrandLogo(card.x + (cardW - 68) / 2.0f, card.y + 44, 1.8f);

    Font fb = GetFontBold();
    Font fr = GetFontReg();

    const char *brand = "StudyBuddy";
    Vector2 bsz = MeasureTextEx(fb, brand, 34.0f, 0.5f);
    DrawTextEx(fb, brand, (Vector2){ card.x + (cardW - bsz.x) / 2.0f, card.y + 130 }, 34.0f, 0.5f, COL_NAVY);

    const char *tag = "Find Your Study Match";
    Vector2 tsz = MeasureTextEx(fr, tag, 16.0f, 0.5f);
    DrawTextEx(fr, tag, (Vector2){ card.x + (cardW - tsz.x) / 2.0f, card.y + 170 }, 16.0f, 0.5f, COL_MUTED);

    /* Banner Ilustrasi Tagline */
    Rectangle pill = { card.x + 48, card.y + 210, cardW - 96, 62 };
    DrawRectangleRounded(pill, 0.3f, 12, COL_PRIMARY_LT);
    const char *quote = "Better Learning Together  |  Study Smarter";
    Vector2 qsz = MeasureTextEx(fb, quote, 14.0f, 0.5f);
    DrawTextEx(fb, quote, (Vector2){ pill.x + (pill.width - qsz.x) / 2.0f, pill.y + 23 }, 14.0f, 0.5f, COL_PRIMARY);

    /* Tombol Get Started (Sign Up) */
    if (Button((Rectangle){ card.x + 48, card.y + 296, cardW - 96, 48 }, "Get Started", true, COL_PRIMARY)) {
        isLoginMode = false;
        authName[0] = authNim[0] = '\0';
        GoView(VIEW_AUTH);
    }

    /* Tombol Log In */
    if (OutlineButton((Rectangle){ card.x + 48, card.y + 358, cardW - 96, 44 }, "Already have an account? Log In", COL_BORDER, COL_NAVY)) {
        isLoginMode = true;
        authNim[0] = '\0';
        GoView(VIEW_AUTH);
    }
}

/* ====================================================================== */
/* SCREEN 2: Sign Up & Log In (Mockup Screen 2)                           */
/* ====================================================================== */

static void HandleAuthSubmit(void)
{
    if (isLoginMode) {
        /* Log In menggunakan NIM */
        if (authNim[0] == '\0') {
            SetStatus("Masukkan NIM untuk masuk ke akunmu.", COL_ERR);
            return;
        }
        for (int i = 0; i < buddyCount; i++) {
            if (strcmp(buddies[i].nim, authNim) == 0) {
                LoginWithBuddyIndex(i);
                char welcome[128];
                snprintf(welcome, sizeof welcome, "Selamat datang kembali, %s!", currentUserName);
                SetStatus(welcome, COL_OK);
                GoView(VIEW_HOME);
                return;
            }
        }
        SetStatus("NIM tidak ditemukan. Silakan Sign Up terlebih dahulu.", COL_ERR);
    } else {
        /* Sign Up akun baru */
        // StudyBuddy b;
        // memset(&b, 0, sizeof b);
        // snprintf(b.nama,   sizeof b.nama,   "%s", authName);
        // snprintf(b.nim,    sizeof b.nim,    "%s", authNim);
        // snprintf(b.hari,   sizeof b.hari,   "%s", authSched);
        // snprintf(b.matkul, sizeof b.matkul, "%s", COURSES[authCourseDd.selected]);
        // SanitizeText(b.nama);
        // SanitizeText(b.nim);
        // SanitizeText(b.hari);

        // if (b.nama[0] == '\0' || b.nim[0] == '\0' || b.hari[0] == '\0') {
        //     SetStatus("Lengkapi Nama, NIM, dan Jadwal kosongmu.", COL_ERR);
        //     return;
        // }
        StudyBuddy b;
        memset(&b, 0, sizeof b);
        snprintf(b.nama,   sizeof b.nama,   "%s", authName);
        snprintf(b.nim,    sizeof b.nim,    "%s", authNim);
        snprintf(b.hari,   sizeof b.hari,   "%s, %s",
                 DAY_OPTIONS[authDayDd.selected],
                 SESSION_OPTIONS[authSessionDd.selected]);
        snprintf(b.matkul, sizeof b.matkul, "%s", COURSES[authCourseDd.selected]);
        SanitizeText(b.nama);
        SanitizeText(b.nim);
        SanitizeText(b.hari);

        if (b.nama[0] == '\0' || b.nim[0] == '\0') {
            SetStatus("Lengkapi Nama dan NIM kamu terlebih dahulu.", COL_ERR);
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
        char msg[128];
        snprintf(msg, sizeof msg, "Akun berhasil dibuat! Halo, %s!", currentUserName);
        SetStatus(msg, COL_OK);
        GoView(VIEW_HOME);
    }
}

static void DrawAuthScreen(void)
{
    float sw = GetVirtW();
    float sh = VIRT_H;

    float cardW = 470.0f;
    float cardH = isLoginMode ? 360.0f : 510.0f;
    Rectangle card = { (sw - cardW) / 2.0f, (sh - cardH) / 2.0f, cardW, cardH };
    DrawCard(card, WHITE, COL_BORDER, 22.0f);

    float x = card.x + 36;
    float w = cardW - 72;
    float y = card.y + 28;

    /* Tombol Back kecil ke Splash */
    if (OutlineButton((Rectangle){ x, y, 76, 32 }, "< Back", COL_BORDER, COL_MUTED)) {
        GoView(VIEW_SPLASH);
    }

    DrawTextBold(isLoginMode ? "Welcome Back!" : "Create Your Account", PX(x), PX(y + 46), 26, PX(w), COL_NAVY);
    DrawTextFit(isLoginMode ? "Masukkan NIM mahasiswa untuk masuk ke dashboard"
                            : "Lengkapi profilmu agar teman belajar mudah menemukanmu",
                PX(x), PX(y + 78), 14, PX(w), COL_MUTED);

    float fy = y + 114;

    if (isLoginMode) {
        DrawTextBold("NIM Mahasiswa", PX(x), PX(fy), 14, 200, COL_TEXT);
        if (TextInput(ID_AUTH_NIM, (Rectangle){ x, fy + 22, w, 44 }, authNim, MAX_NIM, "Contoh: 2610512113", true)) {
            HandleAuthSubmit();
        }

        if (Button((Rectangle){ x, fy + 86, w, 46 }, "Log In", true, COL_PRIMARY)) {
            HandleAuthSubmit();
        }

        /* Quick Demo Login jika sudah ada data buddy di file */
        // if (buddyCount > 0) {
        //     char quickLabel[96];
        //     snprintf(quickLabel, sizeof quickLabel, "Masuk Cepat sebagai %s", buddies[0].nama);
        //     if (OutlineButton((Rectangle){ x, fy + 144, w, 40 }, quickLabel, COL_PRIMARY, COL_PRIMARY)) {
        //         LoginWithBuddyIndex(0);
        //         GoView(VIEW_HOME);
        //     }
        // }
    // } else {
    //     DrawTextBold("Nama Lengkap", PX(x), PX(fy), 14, 200, COL_TEXT);
    //     TextInput(ID_AUTH_NAME, (Rectangle){ x, fy + 22, w, 42 }, authName, MAX_NAMA, "Contoh: Sarah Amalia", false);

    //     DrawTextBold("NIM", PX(x), PX(fy + 74), 14, 200, COL_TEXT);
    //     TextInput(ID_AUTH_NIM, (Rectangle){ x, fy + 96, w, 42 }, authNim, MAX_NIM, "Hanya angka (contoh: 2610512113)", true);

    //     DrawTextBold("Mata Kuliah Utama", PX(x), PX(fy + 148), 14, 200, COL_TEXT);

    //     DrawTextBold("Jadwal Kosong", PX(x), PX(fy + 222), 14, 200, COL_TEXT);
    //     TextInput(ID_AUTH_SCHED, (Rectangle){ x, fy + 244, w, 42 }, authSched, MAX_HARI, "Contoh: Selasa, Kamis 10.00 - 12.00", false);

    //     if (Button((Rectangle){ x, fy + 306, w, 46 }, "Sign Up & Masuk", true, COL_PRIMARY)) {
    //         HandleAuthSubmit();
    //     }

    //     /* Dropdown di-render terakhir supaya menumpuk di atas */
    //     Dropdown(&authCourseDd, (Rectangle){ x, fy + 170, w, 42 }, COURSES, COURSE_COUNT);
    // }
        } else {
            DrawTextBold("Nama Lengkap", PX(x), PX(fy), 14, 200, COL_TEXT);
            TextInput(ID_AUTH_NAME, (Rectangle){ x, fy + 22, w, 42 }, authName, MAX_NAMA, "Contoh: Sarah Amalia", false);

            DrawTextBold("NIM", PX(x), PX(fy + 74), 14, 200, COL_TEXT);
            TextInput(ID_AUTH_NIM, (Rectangle){ x, fy + 96, w, 42 }, authNim, MAX_NIM, "Hanya angka (contoh: 2610512113)", true);

            DrawTextBold("Mata Kuliah Utama", PX(x), PX(fy + 148), 14, 200, COL_TEXT);

            DrawTextBold("Jadwal Kosong (Hari & Sesi)", PX(x), PX(fy + 222), 14, 260, COL_TEXT);

            if (Button((Rectangle){ x, fy + 306, w, 46 }, "Sign Up & Masuk", true, COL_PRIMARY)) {
                HandleAuthSubmit();
            }

            /* Render Dropdown Jadwal (Hari di kiri, Sesi Jam di kanan) */
            float halfW = (w - 12.0f) / 2.0f;
            Dropdown(&authDayDd,     (Rectangle){ x, fy + 244, halfW, 42 }, DAY_OPTIONS, DAY_OPTION_COUNT);
            Dropdown(&authSessionDd, (Rectangle){ x + halfW + 12.0f, fy + 244, halfW, 42 }, SESSION_OPTIONS, SESSION_OPTION_COUNT);

            /* Render Dropdown Mata Kuliah paling akhir agar list-nya di lapisan paling atas */
            Dropdown(&authCourseDd, (Rectangle){ x, fy + 170, w, 42 }, COURSES, COURSE_COUNT);
        }

    /* Switch Mode Link */
    // if (OutlineButton((Rectangle){ x, card.y + cardH - 48, w, 34 },
    //                   isLoginMode ? "Belum punya akun? Sign Up" : "Sudah punya akun? Log In",
    //                   COL_BG, COL_PRIMARY)) {
    //     isLoginMode = !isLoginMode;
    //     focusId = -1;
    // }
}

/* ====================================================================== */
/* Sidebar & Header for Logged-In Workspace                               */
/* ====================================================================== */

static void DrawSidebar(void)
{
    int screenH = (int)VIRT_H;
    DrawRectangle(0, 0, SIDEBAR_W, screenH, COL_NAVY_DARK);

    DrawBrandLogo(20, 24, 1.0f);
    DrawTextBold("StudyBuddy", 66, 23, 22, SIDEBAR_W - 74, WHITE);
    DrawTextFit("Campus Match", 66, 47, 12, SIDEBAR_W - 74, (Color){ 165, 188, 232, 255 });

    struct { const char *label; WorkflowView view; } navItems[] = {
        { "Home",            VIEW_HOME },
        { "Find Buddy",      VIEW_FIND_BUDDY },
        { "Match Schedule",  VIEW_SCHEDULE },
        { "Study Groups",    VIEW_GROUPS },
        { "Learning Goals",  VIEW_GOALS },
        { "Group Chat",      VIEW_CHAT }
    };
    int navCount = 6;

    for (int i = 0; i < navCount; i++) {
        Rectangle r = { 14, 94.0f + i * 48.0f, SIDEBAR_W - 28, 40 };
        bool active = (currentView == navItems[i].view) ||
                      (navItems[i].view == VIEW_GROUPS && currentView == VIEW_GROUP_DETAIL);
        bool hover  = MouseOver(r);

        if (active)     DrawRectangleRounded(r, 0.35f, 10, COL_PRIMARY);
        else if (hover) DrawRectangleRounded(r, 0.35f, 10, COL_NAVY_HOVER);

        if (active) DrawTextBold(navItems[i].label, 28, PX(r.y + 11), 16, SIDEBAR_W - 48, WHITE);
        else        DrawTextFit(navItems[i].label,  28, PX(r.y + 11), 16, SIDEBAR_W - 48, (Color){ 210, 222, 245, 255 });

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) GoView(navItems[i].view);
    }

    /* Profile Card Akun Aktif + Badge ADMIN (sejajar NIM) + Tombol Log Out */
    Rectangle prof = { 14, (float)screenH - 116, SIDEBAR_W - 28, 100 };
    DrawRectangleRounded(prof, 0.22f, 12, COL_NAVY);
    DrawAvatar(prof.x + 26, prof.y + 30, 17, currentUserName, COL_PRIMARY, WHITE);
    DrawTextBold(currentUserName, PX(prof.x + 50), PX(prof.y + 14), 14, PX(prof.width - 62), WHITE);

    if (IsCurrentUserAdmin()) {
        DrawTextFit(currentUserNim, PX(prof.x + 50), PX(prof.y + 33), 12, PX(prof.width - 116), (Color){ 165, 188, 232, 255 });

        Rectangle badge = { prof.x + prof.width - 62, prof.y + 31, 50, 17 };
        DrawRectangleRounded(badge, 0.5f, 6, TILE_AMBER_BG);

        Font fb = GetFontBold();
        const char *admTxt = "ADMIN";
        Vector2 tsz = MeasureTextEx(fb, admTxt, 10.0f, 0.5f);
        DrawTextEx(fb, admTxt,
                   (Vector2){ badge.x + (badge.width - tsz.x) * 0.5f, badge.y + (badge.height - tsz.y) * 0.5f },
                   10.0f, 0.5f, TILE_AMBER_FG);
    } else {
        DrawTextFit(currentUserNim, PX(prof.x + 50), PX(prof.y + 33), 12, PX(prof.width - 62), (Color){ 165, 188, 232, 255 });
    }

    if (Button((Rectangle){ prof.x + 12, prof.y + 58, prof.width - 24, 30 }, "Log Out", true, COL_ERR)) {
        isLoggedIn = false;
        loggedBuddyIdx = -1;
        GoView(VIEW_SPLASH);
    }
}

static void DrawHeader(const char *title, const char *subtitle)
{
    float cx = ContentX();
    float cw = ContentW();
    DrawTextBold(title, PX(cx), 24, 28, PX(cw), COL_NAVY);
    DrawTextFit(subtitle, PX(cx), 58, 15, PX(cw), COL_MUTED);
}

static void DrawToast(void)
{
    if (statusTimer <= 0.0f) return;
    statusTimer -= GetFrameTime();

    float alpha = (statusTimer < 0.5f) ? (statusTimer / 0.5f) : 1.0f;
    if (alpha < 0.0f) alpha = 0.0f;

    Font f = GetFontBold();
    int w = (int)MeasureTextEx(f, statusMsg, 15.0f, 0.5f).x + 44;
    float sw = (float)GetScreenWidth();
    if (w > sw - 60) w = (int)sw - 60;

    float baseX = isLoggedIn ? ContentX() + (ContentW() - w) / 2.0f : (sw - w) / 2.0f;
    Rectangle r = { baseX, (float)GetScreenHeight() - 52, (float)w, 38 };

    DrawRectangleRounded(r, 0.5f, 12, Fade(statusColor, alpha));
    DrawTextBold(statusMsg, PX(r.x + 22), PX(r.y + 11), 15, w - 44, Fade(WHITE, alpha));
}

/* ====================================================================== */
/* SCREEN 3: Personalized Home / Dashboard                                */
/* ====================================================================== */

static bool DrawQuickTile(Rectangle r, const char *title, const char *sub, Color bg, Color fg)
{
    bool hover = MouseOver(r);
    Color cardBg = hover ? ColorBrightness(bg, -0.04f) : bg;
    DrawRectangleRounded(r, 0.22f, 14, cardBg);

    DrawCircle(PX(r.x + 30), PX(r.y + 32), 16, WHITE);
    DrawCircle(PX(r.x + 30), PX(r.y + 32), 7, fg);

    DrawTextBold(title, PX(r.x + 16), PX(r.y + 60), 17, PX(r.width - 32), fg);
    DrawTextFit(sub,    PX(r.x + 16), PX(r.y + 84), 13, PX(r.width - 32), COL_MUTED);

    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static bool IsMemberOfGroup(int groupIdx, const char *userName)
{
    if (groupIdx < 0 || groupIdx >= groupCount) return false;
    for (int k = 0; k < groups[groupIdx].jumlahAnggota; k++) {
        if (strcmp(groups[groupIdx].anggotaNama[k], userName) == 0) return true;
    }
    return false;
}

static void DrawHomeScreen(void)
{
    char greeting[96];
    snprintf(greeting, sizeof greeting, "Hi, %s!", currentUserName);
    DrawHeader(greeting, "Let's find your next study buddy and reach your learning goals!");

    float cx = ContentX();
    float cw = ContentW();
    float gap = 16.0f;
    float tw = (cw - 3 * gap) / 4.0f;
    float ty = 96.0f;

    int doneGoals = 0;
    for (int i = 0; i < goalCount; i++) if (goals[i].status == 1) doneGoals++;

    char subBuddy[40], subGroup[40], subGoal[40];
    snprintf(subBuddy, sizeof subBuddy, "%d teman tersedia", buddyCount);
    snprintf(subGroup, sizeof subGroup, "%d grup aktif", groupCount);
    snprintf(subGoal,  sizeof subGoal,  "%d/%d selesai", doneGoals, goalCount);

    if (DrawQuickTile((Rectangle){ cx, ty, tw, 114 }, "Find Buddy",  subBuddy, TILE_BLUE_BG,  TILE_BLUE_FG))  GoView(VIEW_FIND_BUDDY);
    if (DrawQuickTile((Rectangle){ cx + (tw + gap), ty, tw, 114 }, "My Groups",   subGroup, TILE_PINK_BG,  TILE_PINK_FG))  GoView(VIEW_GROUPS);
    if (DrawQuickTile((Rectangle){ cx + 2*(tw + gap), ty, tw, 114 }, "Schedule",  "Cocokkan jam", TILE_MINT_BG,  TILE_MINT_FG))  GoView(VIEW_SCHEDULE);
    if (DrawQuickTile((Rectangle){ cx + 3*(tw + gap), ty, tw, 114 }, "Goals",     subGoal,  TILE_AMBER_BG, TILE_AMBER_FG)) GoView(VIEW_GOALS);

    /* Cari grup yang sudah diikuti user yang sedang login */
    int joinedGroupIdx = -1;
    int myGroupCount   = 0;
    for (int i = 0; i < groupCount; i++) {
        if (IsMemberOfGroup(i, currentUserName)) {
            if (joinedGroupIdx < 0) joinedGroupIdx = i;
            myGroupCount++;
        }
    }

    /* Upcoming Study Session Banner */
    float banY = ty + 132.0f;
    Rectangle banner = { cx, banY, cw, 108 };
    DrawRectangleRounded(banner, 0.18f, 14, COL_NAVY);
    DrawBrandLogo(banner.x + 22, banner.y + 35, 1.0f);

    float titleY = banner.y + 33.0f;
    float subY   = banner.y + 61.0f;

    if (joinedGroupIdx >= 0) {
        Group *myGroup = &groups[joinedGroupIdx];
        DrawTextBold(myGroup->namaGroup, PX(banner.x + 76), PX(titleY), 20, PX(cw - 270), WHITE);

        char meta[160];
        if (myGroupCount > 1) {
            snprintf(meta, sizeof meta, "%s  |  %d/%d Anggota  |  Kamu tergabung di %d grup aktif",
                     myGroup->matkul, myGroup->jumlahAnggota, myGroup->maxAnggota, myGroupCount);
        } else {
            snprintf(meta, sizeof meta, "%s  |  Jadwalmu: %s  |  %d/%d Anggota",
                     myGroup->matkul, currentUserSched, myGroup->jumlahAnggota, myGroup->maxAnggota);
        }
        DrawTextFit(meta, PX(banner.x + 76), PX(subY), 14, PX(cw - 270), (Color){ 195, 215, 248, 255 });

        if (Button((Rectangle){ banner.x + cw - 172, banner.y + 33, 148, 42 }, "Buka Grup", true, COL_PRIMARY)) {
            selectedGroupIdx = joinedGroupIdx;
            GoView(VIEW_GROUP_DETAIL);
        }
    } else {
        DrawTextBold("Kamu Belum Bergabung di Study Group", PX(banner.x + 76), PX(titleY), 20, PX(cw - 270), WHITE);
        DrawTextFit("Yuk cari dan gabung ke kelompok belajar sesuai mata kuliahmu sekarang!",
                    PX(banner.x + 76), PX(subY), 14, PX(cw - 270), (Color){ 195, 215, 248, 255 });

        if (Button((Rectangle){ banner.x + cw - 172, banner.y + 33, 148, 42 }, "Cari Grup", true, COL_PRIMARY)) {
            GoView(VIEW_GROUPS);
        }
    }
    /* Popular Courses */
    float popY = banY + 128.0f;
    DrawTextBold("Popular Courses", PX(cx), PX(popY), 19, 300, COL_NAVY);

    float pcw = (cw - gap) / 2.0f;
    for (int i = 0; i < COURSE_COUNT; i++) {
        int col = i % 2;
        int row = i / 2;
        Rectangle cr = { cx + col * (pcw + gap), popY + 32 + row * 78.0f, pcw, 66 };
        bool hover = MouseOver(cr);
        DrawCard(cr, WHITE, hover ? COL_PRIMARY : COL_BORDER, 14.0f);

        int count = 0;
        for (int b = 0; b < buddyCount; b++) if (strcmp(buddies[b].matkul, COURSES[i]) == 0) count++;

        DrawRectangleRounded((Rectangle){ cr.x + 14, cr.y + 13, 40, 40 }, 0.3f, 8, COL_PRIMARY_LT);
        DrawCircle(PX(cr.x + 34), PX(cr.y + 33), 8, COL_PRIMARY);

        DrawTextBold(COURSES[i], PX(cr.x + 66), PX(cr.y + 14), 16, PX(pcw - 90), COL_TEXT);
        char cstr[48];
        snprintf(cstr, sizeof cstr, "%d members belajar bersama", count);
        DrawTextFit(cstr, PX(cr.x + 66), PX(cr.y + 37), 13, PX(pcw - 90), COL_MUTED);

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            findFilter = i;
            GoView(VIEW_FIND_BUDDY);
        }
    }
}

/* ====================================================================== */
/* SCREEN 4: Find Buddy (Search, Filter & Invite Friend)                  */
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

static void DrawFindBuddyScreen(void)
{
    DrawHeader("Find Buddy", "Cari dan ajak teman belajar berdasarkan mata kuliah atau jadwal");

    float cx = ContentX();
    float cw = ContentW();

    /* Simpan query sebelumnya untuk mendeteksi perubahan ketikan search */
    char prevSearch[64];
    snprintf(prevSearch, sizeof prevSearch, "%s", findSearch);

    /* Search Bar */
    TextInput(ID_FIND_SEARCH, (Rectangle){ cx, 94, cw, 42 }, findSearch, sizeof findSearch,
              "Cari nama mahasiswa, NIM, atau jadwal...", false);

    if (strcmp(prevSearch, findSearch) != 0) {
        findPage = 0;
    }

    /* Filter Pills */
    float px = cx;
    float py = 148.0f;
    if (PillButton((Rectangle){ px, py, 82, 32 }, "Semua", findFilter == -1)) {
        findFilter = -1;
        findPage = 0;
    }
    px += 90.0f;

    Font f = GetFontReg();
    for (int i = 0; i < COURSE_COUNT; i++) {
        float pw = MeasureTextEx(f, COURSES[i], 14.0f, 0.5f).x + 28.0f;
        if (PillButton((Rectangle){ px, py, pw, 32 }, COURSES[i], findFilter == i)) {
            findFilter = i;
            findPage = 0;
        }
        px += pw + 8.0f;
    }

    /* Kumpulkan indeks teman yang lolos filter */
    int totalMatched = 0;
    for (int i = 0; i < buddyCount; i++) {
        if (strcmp(buddies[i].nim, currentUserNim) == 0) continue;
        if (findFilter >= 0 && strcmp(buddies[i].matkul, COURSES[findFilter]) != 0) continue;
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

    int matchedIdx = 0;
    int rendered   = 0;
    for (int i = 0; i < buddyCount; i++) {
        if (strcmp(buddies[i].nim, currentUserNim) == 0) continue;
        if (findFilter >= 0 && strcmp(buddies[i].matkul, COURSES[findFilter]) != 0) continue;
        if (!ContainsIgnoreCase(buddies[i].nama, findSearch) &&
            !ContainsIgnoreCase(buddies[i].nim, findSearch) &&
            !ContainsIgnoreCase(buddies[i].hari, findSearch)) continue;

        if (matchedIdx >= startIdx && matchedIdx < endIdx) {
            float y = 194.0f + rendered * 84.0f;
            rendered++;

            Rectangle card = { cx, y, cw, 74 };
            DrawCard(card, WHITE, COL_BORDER, 16.0f);

            DrawAvatar(card.x + 38, card.y + 37, 22, buddies[i].nama, COL_PRIMARY_LT, COL_NAVY);
            DrawTextBold(buddies[i].nama, PX(card.x + 74), PX(card.y + 15), 17, PX(card.width - 420), COL_TEXT);

            char sub[96];
            snprintf(sub, sizeof sub, "NIM %s  |  %s", buddies[i].nim, buddies[i].matkul);
            DrawTextFit(sub, PX(card.x + 74), PX(card.y + 40), 14, PX(card.width - 420), COL_MUTED);

            /* Badge Jadwal */
            Rectangle pill = { card.x + card.width - 320, card.y + 21, 185, 32 };
            DrawRectangleRounded(pill, 0.5f, 10, COL_PRIMARY_LT);
            DrawTextFit(buddies[i].hari, PX(pill.x + 14), PX(pill.y + 8), 13, PX(pill.width - 24), COL_PRIMARY);

            /* Tombol Match / Invite */
            if (Button((Rectangle){ card.x + card.width - 122, card.y + 20, 106, 34 }, "Invite", true, COL_PRIMARY)) {
                char msg[128];
                snprintf(msg, sizeof msg, "Permintaan belajar bareng terkirim ke %s!", buddies[i].nama);
                SetStatus(msg, COL_OK);
            }
        }
        matchedIdx++;
    }

    if (totalMatched == 0) {
        DrawTextFit("Tidak ada teman yang cocok dengan filter pencarianmu.", PX(cx), 220, 16, 450, COL_MUTED);
    } else {
        DrawPaginationBar(cx, cw, ContentBottom() - 36.0f, &findPage, totalMatched, FIND_PAGE_SIZE);
    }
}

/* ====================================================================== */
/* SCREEN 5: Match Your Schedule (Mockup Screen 5)                        */
/* ====================================================================== */

static void DrawScheduleScreen(void)
{
    DrawHeader("Match Your Schedule", "Pilih 1 hari dan maksimal 2 slot jam kosongmu untuk disimpan");

    float cx = ContentX();
    float cw = ContentW();

    /* Baris Pemilih Hari: Hanya bisa memilih 1 hari (Senin - Minggu) */
    float dayW = (cw - 6 * 10.0f) / 7.0f;
    for (int d = 0; d < DAY_OPTION_COUNT; d++) {
        Rectangle dr = { cx + d * (dayW + 10.0f), 96, dayW, 48 };
        bool sel = (d == selectedDayIdx);
        bool hover = MouseOver(dr);
        DrawCard(dr, sel ? COL_PRIMARY : (hover ? COL_PRIMARY_LT : WHITE), sel ? COL_PRIMARY : COL_BORDER, 14.0f);

        Font fb = GetFontBold();
        Vector2 s1 = MeasureTextEx(fb, DAY_OPTIONS[d], 15.0f, 0.5f);
        DrawTextEx(fb, DAY_OPTIONS[d], (Vector2){ dr.x + (dayW - s1.x) / 2.0f, dr.y + (dr.height - s1.y) / 2.0f },
                   15.0f, 0.5f, sel ? WHITE : COL_TEXT);

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) selectedDayIdx = d;
    }

    /* Hitung berapa slot jam yang sedang dipilih saat ini (maksimal 2) */
    int checkedCount = 0;
    int activeSlots[2] = { -1, -1 };
    for (int i = 0; i < SESSION_OPTION_COUNT && i < 5; i++) {
        if (slotChecked[i]) {
            if (checkedCount < 2) activeSlots[checkedCount] = i;
            checkedCount++;
        }
    }

    /* Kolom Kiri: Pilihan Jam Kosong (Maksimal 2 slot) */
    float leftW = (cw - 20.0f) * 0.52f;
    float rightW = cw - leftW - 20.0f;
    float topY = 162.0f;

    char slotHeader[64];
    snprintf(slotHeader, sizeof slotHeader, "Pilih Slot Jam Kosongmu (%d/2):", checkedCount);
    DrawTextBold(slotHeader, PX(cx), PX(topY), 16, 300, COL_NAVY);

    for (int s = 0; s < SESSION_OPTION_COUNT; s++) {
        Rectangle sr = { cx, topY + 30 + s * 52.0f, leftW, 44 };
        bool chk = slotChecked[s];
        bool locked = (!chk && checkedCount >= 2);
        bool hover = !locked && MouseOver(sr);

        Color cardBg = chk ? COL_PRIMARY_LT : (locked ? (Color){ 240, 243, 248, 255 } : (hover ? COL_PRIMARY_LT : WHITE));
        Color cardBd = chk ? COL_PRIMARY : COL_BORDER;
        Color txtCol = chk ? COL_NAVY : (locked ? COL_DISABLED : COL_TEXT);

        DrawCard(sr, cardBg, cardBd, 12.0f);
        DrawTextFit(SESSION_OPTIONS[s], PX(sr.x + 16), PX(sr.y + 14), 15, PX(leftW - 95), txtCol);

        if (locked) {
            DrawTextFit("Locked", PX(sr.x + sr.width - 86), PX(sr.y + 15), 12, 50, COL_DISABLED);
        }

        /* Checkbox di kanan */
        Rectangle cb = { sr.x + sr.width - 36, sr.y + 11, 22, 22 };
        DrawRectangleRounded(cb, 0.3f, 6, chk ? COL_PRIMARY : (locked ? (Color){ 220, 225, 235, 255 } : COL_BORDER));
        if (chk) DrawCircle(PX(cb.x + 11), PX(cb.y + 11), 5, WHITE);

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (chk) slotChecked[s] = false;
            else if (checkedCount < 2) slotChecked[s] = true;
        }
    }

    /* Kolom Kanan: Rekomendasi Teman Cocok Berdasarkan Prioritas */
    float rx = cx + leftW + 20.0f;
    DrawTextBold("Rekomendasi Teman Cocok:", PX(rx), PX(topY), 16, 280, COL_NAVY);

    Rectangle matchCard = { rx, topY + 30, rightW, 252 };
    DrawCard(matchCard, WHITE, COL_BORDER, 16.0f);

    char schedTitle[96];
    snprintf(schedTitle, sizeof schedTitle, "Hari %s  |  Fokus: %s", DAY_OPTIONS[selectedDayIdx], currentUserCourse);
    DrawTextBold(schedTitle, PX(matchCard.x + 18), PX(matchCard.y + 18), 14, PX(rightW - 36), COL_PRIMARY);

    /*
     * Kumpulkan kandidat yang memenuhi syarat:
     *   Priority 2: Hari sama + kedua jam kosong sama (hanya jika user memilih 2 jam)
     *   Priority 1: Hari sama + minimal salah satu jam kosong sama
     */
    int recIdx[3]  = { -1, -1, -1 };
    int recPrio[3] = {  0,  0,  0 };
    int recCount   = 0;

    if (checkedCount > 0) {
        for (int prio = 2; prio >= 1 && recCount < 3; prio--) {
            /* Lewati cek Priority 2 jika user hanya mencentang 1 slot jam */
            if (prio == 2 && checkedCount < 2) continue;

            for (int i = 0; i < buddyCount && recCount < 3; i++) {
                if (strcmp(buddies[i].nim, currentUserNim) == 0) continue;

                /* Syarat wajib: Hari harus sama */
                if (strstr(buddies[i].hari, DAY_OPTIONS[selectedDayIdx]) == NULL) continue;

                /* Hitung berapa jam kosong yang cocok */
                int slotMatches = 0;
                for (int k = 0; k < checkedCount; k++) {
                    if (activeSlots[k] >= 0 && strstr(buddies[i].hari, SESSION_OPTIONS[activeSlots[k]]) != NULL) {
                        slotMatches++;
                    }
                }

                int buddyPrio = 0;
                if (checkedCount == 2 && slotMatches == 2) buddyPrio = 2;
                else if (slotMatches >= 1)                 buddyPrio = 1;

                if (buddyPrio == prio) {
                    recIdx[recCount]  = i;
                    recPrio[recCount] = buddyPrio;
                    recCount++;
                }
            }
        }
    }

    if (recCount == 0) {
        const char *emptyMsg = "Belum ada teman dengan hari & jam yang cocok.";
        Font fr = GetFontReg();
        float fsize = 14.0f;
        Vector2 tsz = MeasureTextEx(fr, emptyMsg, fsize, 0.5f);

        /* Area kosong di antara header kartu (y + 44) dan tombol Save (y + 196) */
        float areaTop = matchCard.y + 44.0f;
        float areaBot = matchCard.y + 196.0f;
        float centerX = matchCard.x + (rightW - tsz.x) * 0.5f;
        float centerY = areaTop + ((areaBot - areaTop) - tsz.y) * 0.5f;

        /* Jaga agar tidak keluar batas kiri kartu jika jendela dipersempit */
        if (centerX < matchCard.x + 18.0f) centerX = matchCard.x + 18.0f;

        DrawTextFit(emptyMsg, PX(centerX), PX(centerY), (int)fsize, PX(rightW - 36), COL_MUTED);
    } else {
        for (int m = 0; m < recCount; m++) {
            int bi = recIdx[m];
            float my = matchCard.y + 54 + m * 44.0f;

            DrawAvatar(matchCard.x + 32, my + 14, 14, buddies[bi].nama, COL_PRIMARY_LT, COL_NAVY);
            DrawTextBold(buddies[bi].nama, PX(matchCard.x + 54), PX(my + 2), 14, PX(rightW - 145), COL_TEXT);
            DrawTextFit(buddies[bi].hari,  PX(matchCard.x + 54), PX(my + 20), 12, PX(rightW - 145), COL_MUTED);

            /* Badge tingkat kecocokan di sisi kanan */
            const char *badgeTxt = (recPrio[m] == 2) ? "Exact Match" : "1 Jadwal Cocok";
            Color badgeBg = (recPrio[m] == 2) ? TILE_MINT_BG : COL_PRIMARY_LT;
            Color badgeFg = (recPrio[m] == 2) ? COL_OK       : COL_PRIMARY;

            Rectangle badge = { matchCard.x + rightW - 96, my + 6, 80, 22 };
            DrawRectangleRounded(badge, 0.5f, 8, badgeBg);
            DrawTextBold(badgeTxt, PX(badge.x + 8), PX(badge.y + 5), 11, 68, badgeFg);
        }
    }

    /* Tombol Save: Gabungkan 1 Hari + 1 atau 2 Jam Kosong dan Simpan Otomatis */
    if (Button((Rectangle){ matchCard.x + 18, matchCard.y + 196, rightW - 36, 40 }, "Save Jadwal Kosong", true, COL_PRIMARY)) {
        if (checkedCount == 0) {
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

            snprintf(currentUserSched, sizeof currentUserSched, "%s", newSched);
            if (loggedBuddyIdx >= 0 && loggedBuddyIdx < buddyCount) {
                snprintf(buddies[loggedBuddyIdx].hari, sizeof buddies[loggedBuddyIdx].hari, "%s", newSched);
                Persist();
            }
            SetStatus("Jadwal kosong berhasil disimpan ke profil!", COL_OK);
        }
    }
}

/* ====================================================================== */
/* SCREEN 6 & 7: Study Groups & Group Detail (Mockup Screen 6 & 7)        */
/* ====================================================================== */

static void DrawGroupsScreen(void)
{
    DrawHeader("Study Groups", "Klik kartu grup untuk melihat detail anggota, bergabung, atau chat grup");

    float cx = ContentX();
    float cw = ContentW();

    /* Tombol + Create Group di kanan atas */
    if (Button((Rectangle){ cx + cw - 170, 32, 170, 40 }, "+ Create Group", true, COL_PRIMARY)) {
        showCreateGroupModal = !showCreateGroupModal;
        newGroupName[0] = '\0';
    }

    /* Form cepat buat grup baru jika tombol diklik */
    float listTop = 96.0f;
    if (showCreateGroupModal) {
        Rectangle box = { cx, 96, cw, 64 };
        DrawCard(box, COL_PRIMARY_LT, COL_PRIMARY, 14.0f);

        float inputW = cw - 390.0f;
        if (inputW < 150.0f) inputW = 150.0f;
        TextInput(ID_NEW_GROUP_NAME, (Rectangle){ box.x + 14, box.y + 11, inputW, 42 },
                  newGroupName, MAX_GROUP_NAME, "Nama grup baru...", false);

        if (Button((Rectangle){ box.x + inputW + 244, box.y + 11, 132, 42 }, "Buat Grup", true, COL_OK)) {
            Group g;
            memset(&g, 0, sizeof g);
            snprintf(g.namaGroup, sizeof g.namaGroup, "%s", newGroupName);
            SanitizeText(g.namaGroup);
            if (g.namaGroup[0] == '\0') snprintf(g.namaGroup, sizeof g.namaGroup, "Grup %s", COURSES[newGroupCourseDd.selected]);
            snprintf(g.matkul, sizeof g.matkul, "%s", COURSES[newGroupCourseDd.selected]);
            g.maxAnggota = MAX_GROUP_MEMBERS;

            int idx = AddGroup(&g);
            if (idx >= 0) {
                AddGroupMember(idx, currentUserName);
                Persist();
                showCreateGroupModal = false;
                selectedGroupIdx = idx;
                groupsPage = idx / GROUPS_PAGE_SIZE;
                SetStatus("Grup belajar baru berhasil dibuat!", COL_OK);
                GoView(VIEW_GROUP_DETAIL);
                return;
            }
        }
        Dropdown(&newGroupCourseDd, (Rectangle){ box.x + inputW + 24, box.y + 11, 210, 42 }, COURSES, COURSE_COUNT);
        listTop = 174.0f;
    }

    if (groupCount == 0) {
        DrawTextFit("Belum ada grup belajar. Klik '+ Create Group' di kanan atas!", PX(cx), PX(listTop + 20), 16, 480, COL_MUTED);
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
    float cardH = showCreateGroupModal ? 166.0f : GROUP_CARD_H;

    for (int i = startIdx; i < endIdx; i++) {
        int localIdx = i - startIdx;
        int col = localIdx % 2;
        int row = localIdx / 2;
        Rectangle r = { cx + col * (cardW + 16.0f), listTop + row * (cardH + 14.0f), cardW, cardH };

        bool full = groups[i].jumlahAnggota >= groups[i].maxAnggota;
        bool hover = MouseOver(r);
        DrawCard(r, WHITE, hover ? COL_PRIMARY : COL_BORDER, 16.0f);

        DrawCircle(PX(r.x + 36), PX(r.y + 36), 20, COL_NAVY);
        DrawTextBold("SG", PX(r.x + 25), PX(r.y + 28), 15, 30, WHITE);

        DrawTextBold(groups[i].namaGroup, PX(r.x + 68), PX(r.y + 18), 18, PX(r.width - 140), COL_TEXT);
        DrawTextFit(groups[i].matkul,     PX(r.x + 68), PX(r.y + 42), 14, PX(r.width - 140), COL_MUTED);

        char cnt[24];
        snprintf(cnt, sizeof cnt, "%d/%d", groups[i].jumlahAnggota, groups[i].maxAnggota);
        DrawTextBold(cnt, PX(r.x + r.width - 52), PX(r.y + 20), 15, 45, full ? COL_ERR : COL_PRIMARY);

        /* Avatar anggota */
        for (int k = 0; k < groups[i].jumlahAnggota; k++) {
            DrawAvatar(r.x + 30 + k * 24, r.y + 84, 13, groups[i].anggotaNama[k],
                       (k % 2 == 0) ? COL_PRIMARY_LT : TILE_AMBER_BG, COL_NAVY);
        }

        char names[200] = "";
        for (int k = 0; k < groups[i].jumlahAnggota; k++) {
            if (k > 0) strncat(names, ", ", sizeof(names) - strlen(names) - 1);
            strncat(names, groups[i].anggotaNama[k], sizeof(names) - strlen(names) - 1);
        }
        DrawTextFit(names, PX(r.x + 18), PX(r.y + 110), 13, PX(r.width - 36), COL_TEXT);
        DrawTextBold("Klik untuk detail & chat  ->", PX(r.x + 18), PX(r.y + cardH - 32), 13, PX(r.width - 36), COL_PRIMARY);

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selectedGroupIdx = i;
            GoView(VIEW_GROUP_DETAIL);
        }
    }

    DrawPaginationBar(cx, cw, ContentBottom() - 36.0f, &groupsPage, groupCount, GROUPS_PAGE_SIZE);
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

    /* Cek hak akses Creator (anggota indeks 0) dan Role Khusus Admin */
    bool isCreator = (g->jumlahAnggota > 0 && strcmp(g->anggotaNama[0], currentUserName) == 0);
    bool isAdmin   = IsCurrentUserAdmin();
    bool canDelete = (isCreator || isAdmin);

    float cx = ContentX();
    float cw = ContentW();

    if (OutlineButton((Rectangle){ cx, 26, 94, 34 }, "< Kembali", COL_BORDER, COL_NAVY)) {
        confirmDeleteGroup = false;
        GoView(VIEW_GROUPS);
        return;
    }
    DrawTextBold("Group Detail", PX(cx + 110), 28, 26, 350, COL_NAVY);

    /* Badge Indikator Privilege di Kanan Atas Header */
    if (isAdmin && !isCreator) {
        Rectangle admTag = { cx + cw - 175, 28, 175, 30 };
        DrawRectangleRounded(admTag, 0.5f, 8, TILE_AMBER_BG);
        DrawTextBold("Admin Moderation Mode", PX(admTag.x + 14), PX(admTag.y + 8), 12, 150, TILE_AMBER_FG);
    } else if (isCreator) {
        Rectangle crtTag = { cx + cw - 145, 28, 145, 30 };
        DrawRectangleRounded(crtTag, 0.5f, 8, COL_PRIMARY_LT);
        DrawTextBold("Group Creator", PX(crtTag.x + 22), PX(crtTag.y + 8), 13, 110, COL_PRIMARY);
    }

    /* Header Card Grup */
    Rectangle topCard = { cx, 78, cw, 118 };
    DrawCard(topCard, WHITE, COL_BORDER, 18.0f);

    DrawCircle(PX(topCard.x + 48), PX(topCard.y + 58), 28, COL_NAVY);
    DrawTextBold("SG", PX(topCard.x + 34), PX(topCard.y + 48), 20, 40, WHITE);

    DrawTextBold(g->namaGroup, PX(topCard.x + 92), PX(topCard.y + 24), 24, PX(cw - 320), COL_NAVY);
    char meta[120];
    snprintf(meta, sizeof meta, "%s  |  %d/%d Anggota  |  Next Meeting: Perpustakaan Kampus",
             g->matkul, g->jumlahAnggota, g->maxAnggota);
    DrawTextFit(meta, PX(topCard.x + 92), PX(topCard.y + 56), 14, PX(cw - 320), COL_MUTED);

    /* Tombol Join / Leave / Status Creator di Kartu Atas */
    if (isCreator) {
        DrawRectangleRounded((Rectangle){ topCard.x + cw - 180, topCard.y + 36, 156, 42 }, 0.45f, 10, TILE_MINT_BG);
        DrawTextBold("Creator Aktif", PX(topCard.x + cw - 148), PX(topCard.y + 48), 14, 130, COL_OK);
    } else if (alreadyMember) {
        if (OutlineButton((Rectangle){ topCard.x + cw - 170, topCard.y + 36, 146, 42 },
                          "Leave Group", COL_ERR, COL_ERR)) {
            if (RemoveGroupMember(selectedGroupIdx, currentUserName)) {
                Persist();
                alreadyMember = false;
                SetStatus("Kamu keluar dari grup. Akses chat grup dikunci kembali.", COL_WARN);
            }
        }
    } else {
        if (Button((Rectangle){ topCard.x + cw - 170, topCard.y + 36, 146, 42 },
                   full ? "Grup Penuh" : "+ Join Group", !full, COL_PRIMARY)) {
            AddGroupMember(selectedGroupIdx, currentUserName);
            Persist();
            alreadyMember = true;
            SetStatus("Berhasil bergabung ke grup belajar!", COL_OK);
        }
    }

    /* Daftar Anggota */
    Rectangle memCard = { cx, 212, cw, ContentBottom() - 274 };
    DrawCard(memCard, WHITE, COL_BORDER, 18.0f);
    DrawTextBold("Daftar Anggota Kelompok", PX(memCard.x + 24), PX(memCard.y + 20), 18, 300, COL_NAVY);

    for (int k = 0; k < g->jumlahAnggota; k++) {
        float ay = memCard.y + 58 + k * 46.0f;
        DrawAvatar(memCard.x + 42, ay + 16, 16, g->anggotaNama[k], COL_PRIMARY_LT, COL_NAVY);
        DrawTextBold(g->anggotaNama[k], PX(memCard.x + 70), PX(ay + 7), 16, 300, COL_TEXT);

        if (k == 0) {
            DrawRectangleRounded((Rectangle){ memCard.x + cw - 130, ay + 4, 104, 26 }, 0.5f, 8, TILE_AMBER_BG);
            DrawTextBold("Creator", PX(memCard.x + cw - 104), PX(ay + 10), 13, 80, TILE_AMBER_FG);
        } else {
            DrawTextFit("Member", PX(memCard.x + cw - 104), PX(ay + 10), 13, 80, COL_MUTED);
        }
    }

    /* Baris Tombol Aksi Bawah: Chat Group (Khusus Member) + Pengaturan Hapus/Bubarkan Grup (Creator / Admin) */
    float btnY = ContentBottom() - 46.0f;

    if (!canDelete) {
        if (alreadyMember) {
            if (Button((Rectangle){ cx, btnY, cw, 46 }, "Buka Chat Group", true, COL_PRIMARY)) {
                GoView(VIEW_CHAT);
            }
        } else {
            const char *joinChatLabel = full ? "Grup Penuh (Chat Khusus Anggota)"
                                             : "+ Join Group untuk Membuka Chat";
            if (Button((Rectangle){ cx, btnY, cw, 46 }, joinChatLabel, !full, COL_PRIMARY)) {
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
            if (Button((Rectangle){ cx, btnY, chatW, 46 }, "Buka Chat Group", true, COL_PRIMARY)) {
                GoView(VIEW_CHAT);
            }
        } else {
            const char *joinChatLabel = full ? "Chat Khusus Anggota (Grup Penuh)"
                                             : "+ Join Group untuk Membuka Chat";
            if (Button((Rectangle){ cx, btnY, chatW, 46 }, joinChatLabel, !full, COL_PRIMARY)) {
                AddGroupMember(selectedGroupIdx, currentUserName);
                Persist();
                SetStatus("Berhasil bergabung ke grup belajar!", COL_OK);
            }
        }

        if (OutlineButton((Rectangle){ cx + chatW + 14.0f, btnY, delW, 46 }, delLabel, COL_ERR, COL_ERR)) {
            confirmDeleteGroup = true;
        }
    } else {
        float cancelW  = 110.0f;
        float confirmW = 190.0f;
        float infoW    = cw - cancelW - confirmW - 24.0f;

        Rectangle warnBox = { cx, btnY, infoW, 46 };
        DrawCard(warnBox, (Color){ 254, 238, 240, 255 }, COL_ERR, 12.0f);
        DrawTextBold(isCreator ? "Yakin ingin membubarkan grup ini permanen?"
                               : "Admin: Hapus grup ini karena pelanggaran?",
                     PX(warnBox.x + 16), PX(warnBox.y + 15), 14, PX(infoW - 28), COL_ERR);

        if (Button((Rectangle){ cx + infoW + 12.0f, btnY, confirmW, 46 }, "Ya, Hapus Permanen", true, COL_ERR)) {
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

        if (OutlineButton((Rectangle){ cx + infoW + confirmW + 24.0f, btnY, cancelW, 46 }, "Batal", COL_BORDER, COL_TEXT)) {
            confirmDeleteGroup = false;
        }
    }
}

/* ====================================================================== */
/* SCREEN 8: Learning Goals (Mockup Screen 8)                             */
/* ====================================================================== */

static void DrawGoalsScreen(void)
{
    DrawHeader("Learning Goals", "Pantau progres target belajarmu bersama kelompok");

    float cx = ContentX();
    float cw = ContentW();

    int done = 0;
    for (int i = 0; i < goalCount; i++) if (goals[i].status == 1) done++;

    /* Progress Summary Card di atas (seperti "My Goals 1/3" di Screen 8 Mockup) */
    Rectangle progCard = { cx, 94, cw, 82 };
    DrawCard(progCard, WHITE, COL_BORDER, 16.0f);
    DrawTextBold("My Goals Progress", PX(progCard.x + 20), PX(progCard.y + 16), 16, 250, COL_NAVY);

    char statStr[32];
    snprintf(statStr, sizeof statStr, "%d / %d Selesai", done, goalCount);
    DrawTextBold(statStr, PX(progCard.x + cw - 130), PX(progCard.y + 16), 15, 110, COL_OK);

    Rectangle bar = { progCard.x + 20, progCard.y + 48, cw - 40, 12 };
    DrawRectangleRounded(bar, 1.0f, 8, COL_PRIMARY_LT);
    float ratio = (goalCount > 0) ? (float)done / (float)goalCount : 0.0f;
    if (ratio > 0.0f) {
        DrawRectangleRounded((Rectangle){ bar.x, bar.y, bar.width * ratio, bar.height }, 1.0f, 8, COL_OK);
    }

    /* Input Tambah Goal */
    bool enter = TextInput(ID_GOAL_INPUT, (Rectangle){ cx, 190, cw - 166, 44 },
                           goalInput, MAX_GOAL_DESC, "Tambahkan target belajar baru...", false);

    if (Button((Rectangle){ cx + cw - 152, 190, 152, 44 }, "+ Add Goal", true, COL_PRIMARY) || enter) {
        Goal g;
        memset(&g, 0, sizeof g);
        snprintf(g.deskripsi, sizeof g.deskripsi, "%s", goalInput);
        SanitizeText(g.deskripsi);

        if (g.deskripsi[0] == '\0') {
            SetStatus("Deskripsi target belajar tidak boleh kosong.", COL_ERR);
        } else if (AddGoal(&g) >= 0) {
            goalInput[0] = '\0';
            Persist();
            SetStatus("Target belajar ditambahkan!", COL_OK);
        }
    }

    Rectangle area = { cx, 248.0f, cw, ContentBottom() - 248.0f };
    float contentH = goalCount * GOAL_ROW_H;
    ScrollUpdate(&scrollY[VIEW_GOALS], area, contentH);

    BeginClip(area);
    for (int i = 0; i < goalCount; i++) {
        float y = area.y + i * GOAL_ROW_H - scrollY[VIEW_GOALS];
        if (y + GOAL_ROW_H < area.y || y > area.y + area.height) continue;

        Rectangle row = { area.x, y, area.width - 12, 70 };
        bool isDone = (goals[i].status == 1);
        DrawCard(row, WHITE, COL_BORDER, 16.0f);

        if (isDone) {
            DrawCircle(PX(row.x + 30), PX(y + 35), 12, COL_OK);
            DrawCircle(PX(row.x + 30), PX(y + 35), 5, WHITE);
        } else {
            DrawCircleLines(PX(row.x + 30), PX(y + 35), 12, COL_PRIMARY);
        }

        DrawTextBold(goals[i].deskripsi, PX(row.x + 56), PX(y + 16), 16, PX(row.width - 220), COL_TEXT);
        DrawTextFit(isDone ? "Completed" : "On Progress", PX(row.x + 56), PX(y + 40), 13, 180, isDone ? COL_OK : COL_WARN);

        if (Button((Rectangle){ row.x + row.width - 138, y + 17, 122, 36 },
                   isDone ? "Reopen" : "Selesai", true, isDone ? COL_MUTED : COL_PRIMARY)) {
            goals[i].status = isDone ? 0 : 1;
            Persist();
        }
    }
    EndClip();
    DrawScrollbar(area, scrollY[VIEW_GOALS], contentH);
}

/* ====================================================================== */
/* SCREEN 9: Group Chat (Mockup Screen 9)                                 */
/* ====================================================================== */

static void DrawChatScreen(void)
{
    /* Pengaman: Jika selectedGroupIdx bukan grup milik user, cari grup pertama yang dia ikuti */
    if (!IsMemberOfGroup(selectedGroupIdx, currentUserName)) {
        int found = -1;
        for (int i = 0; i < groupCount; i++) {
            if (IsMemberOfGroup(i, currentUserName)) {
                found = i;
                break;
            }
        }
        if (found >= 0) {
            selectedGroupIdx = found;
        } else {
            DrawHeader("Group Chat", "Ruang diskusi khusus anggota kelompok belajar");
            float cx = ContentX();
            float cw = ContentW();
            Rectangle emptyCard = { cx, 96, cw, 220 };
            DrawCard(emptyCard, WHITE, COL_BORDER, 18.0f);

            const char *title = "Kamu Belum Bergabung di Grup Mana Pun";
            const char *desc  = "Silakan pilih dan gabung ke salah satu kelompok di menu Study Groups terlebih dahulu.";
            Font fb = GetFontBold();
            Font fr = GetFontReg();
            Vector2 tsz = MeasureTextEx(fb, title, 18.0f, 0.5f);
            Vector2 dsz = MeasureTextEx(fr, desc,  14.0f, 0.5f);

            DrawTextEx(fb, title, (Vector2){ cx + (cw - tsz.x) * 0.5f, emptyCard.y + 48 }, 18.0f, 0.5f, COL_NAVY);
            DrawTextEx(fr, desc,  (Vector2){ cx + (cw - dsz.x) * 0.5f, emptyCard.y + 80 }, 14.0f, 0.5f, COL_MUTED);

            if (Button((Rectangle){ cx + (cw - 210.0f) * 0.5f, emptyCard.y + 132, 210.0f, 42 },
                       "Lihat Study Groups", true, COL_PRIMARY)) {
                GoView(VIEW_GROUPS);
            }
            return;
        }
    }

    const char *gName = groups[selectedGroupIdx].namaGroup;
    char sub[96];
    snprintf(sub, sizeof sub, "Diskusi kelompok aktif  |  Login sebagai %s", currentUserName);
    DrawHeader(gName, sub);

    float cx = ContentX();
    float cw = ContentW();
    float bottomY = ContentBottom();

    Rectangle chatBox = { cx, 94, cw, bottomY - 152 };
    DrawCard(chatBox, WHITE, COL_BORDER, 18.0f);

    Font fr = GetFontReg();
    Font fb = GetFontBold();

    BeginClip(chatBox);
    float y = chatBox.y + 18.0f;
    for (int i = 0; i < chatCount; i++) {
        ChatMessage *m = &chatLogs[i];
        float bubbleW = cw * 0.62f;
        float bubbleH = 58.0f;

        if (m->isMe) {
            /* Bubble kanan biru */
            Rectangle b = { chatBox.x + cw - bubbleW - 22, y, bubbleW, bubbleH };
            DrawRectangleRounded(b, 0.35f, 12, COL_PRIMARY);
            DrawTextWrap(m->text, PX(b.x + 16), PX(b.y + 12), 14, PX(bubbleW - 32), 18, WHITE, 2);
            DrawTextFit(m->timeStr, PX(b.x + bubbleW - 44), PX(b.y + bubbleH - 18), 11, 40, (Color){ 215, 232, 255, 255 });
        } else {
            /* Bubble kiri abu-abu lembut dengan avatar */
            DrawAvatar(chatBox.x + 34, y + 20, 16, m->sender, TILE_AMBER_BG, COL_NAVY);
            DrawTextBold(m->sender, PX(chatBox.x + 58), PX(y), 13, 250, COL_NAVY);

            Rectangle b = { chatBox.x + 58, y + 18, bubbleW, bubbleH };
            DrawRectangleRounded(b, 0.35f, 12, COL_BG);
            DrawTextWrap(m->text, PX(b.x + 14), PX(b.y + 10), 14, PX(bubbleW - 28), 18, COL_TEXT, 2);
            y += 16.0f;
        }
        y += bubbleH + 14.0f;
    }
    EndClip();
    (void)fr; (void)fb;

    /* Input Pesan di bawah */
    float iy = bottomY - 44.0f;
    bool enter = TextInput(ID_CHAT_INPUT, (Rectangle){ cx, iy, cw - 124, 44 },
                           chatInput, MAX_CHAT_LEN, "Ketik pesan diskusi kelompok...", false);

    if (Button((Rectangle){ cx + cw - 112, iy, 112, 44 }, "Kirim", true, COL_PRIMARY) || enter) {
        if (chatInput[0] != '\0') {
            PushChatMessage(selectedGroupIdx, currentUserName, chatInput, true);
            chatInput[0] = '\0';
        }
    }
}

/* ====================================================================== */
/* Public API                                                             */
/* ====================================================================== */

void GuiInit(void)
{
    memset(scrollY, 0, sizeof scrollY);
    currentView = VIEW_SPLASH;
    isLoggedIn = false;
    focusId = -1;
    SeedInitialChat();

    fontRegular = LoadFontEx("C:\\Windows\\Fonts\\segoeui.ttf",  64, 0, 250);
    fontBold    = LoadFontEx("C:\\Windows\\Fonts\\segoeuib.ttf", 64, 0, 250);

    if (fontRegular.texture.id > 0) SetTextureFilter(fontRegular.texture, TEXTURE_FILTER_BILINEAR);
    if (fontBold.texture.id > 0)    SetTextureFilter(fontBold.texture,    TEXTURE_FILTER_BILINEAR);
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
        authCourseDd.open = false;
        authDayDd.open = false;
        authSessionDd.open = false;
        newGroupCourseDd.open = false;
    }

    BeginDrawing();
    ClearBackground(COL_BG);

    BeginMode2D(GetUiCamera());

    if (!isLoggedIn) {
        /* Alur Awal Sebelum Masuk Dashboard: Splash Screen -> Sign Up / Log In */
        if (currentView == VIEW_AUTH) DrawAuthScreen();
        else                          DrawSplashScreen();
    } else {
        /* Workspace Utama Setelah Login */
        DrawSidebar();
        switch (currentView) {
            case VIEW_HOME:         DrawHomeScreen();        break;
            case VIEW_FIND_BUDDY:   DrawFindBuddyScreen();   break;
            case VIEW_SCHEDULE:     DrawScheduleScreen();    break;
            case VIEW_GROUPS:       DrawGroupsScreen();      break;
            case VIEW_GROUP_DETAIL: DrawGroupDetailScreen(); break;
            case VIEW_GOALS:        DrawGoalsScreen();       break;
            case VIEW_CHAT:         DrawChatScreen();        break;
            default:                DrawHomeScreen();        break;
        }
    }

    DrawToast();

    EndMode2D();
    EndDrawing();
}