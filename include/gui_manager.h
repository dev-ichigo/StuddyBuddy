#ifndef GUI_MANAGER_H
#define GUI_MANAGER_H

#define WINDOW_WIDTH   1000
#define WINDOW_HEIGHT  640

/* View state: one value per screen (order matches the sidebar menu) */
typedef enum {
    SCREEN_HOME = 0,
    SCREEN_FIND_BUDDY,
    SCREEN_LIST_BUDDY,
    SCREEN_ADD_BUDDY,
    SCREEN_GROUPS,
    SCREEN_GOALS,
    SCREEN_COUNT
} AppScreen;

void GuiInit(void);
void GuiUpdateAndDraw(void);   /* call once per frame */
void GuiShutdown(void);

#endif /* GUI_MANAGER_H */
