#ifndef MODELS_H
#define MODELS_H

/* ---------- Limits ---------- */
#define MAX_NAMA           50
#define MAX_NIM            15
#define MAX_MATKUL         30
#define MAX_HARI           100
#define MAX_GROUP_NAME     40
#define MAX_GROUP_MEMBERS  5
#define MAX_GOAL_DESC      100
#define COURSE_COUNT       4

/* ---------- Data models (PRD section 7.1) ---------- */
typedef struct {
    char nama[MAX_NAMA];
    char nim[MAX_NIM];
    char matkul[MAX_MATKUL];
    char hari[MAX_HARI];
} StudyBuddy;

typedef struct {
    char namaGroup[MAX_GROUP_NAME];
    char matkul[MAX_MATKUL];
    int  jumlahAnggota;
    int  maxAnggota;
    char anggotaNama[MAX_GROUP_MEMBERS][MAX_NAMA];
} Group;

typedef struct {
    char deskripsi[MAX_GOAL_DESC];
    int  status;            /* 0: On Progress, 1: Selesai */
} Goal;

/* ---------- Global state (defined in main.c) ----------
 * Dynamic arrays: grown with realloc() by file_manager.c               */
extern StudyBuddy *buddies;
extern int         buddyCount;
extern int         buddyCapacity;

extern Group      *groups;
extern int         groupCount;
extern int         groupCapacity;

extern Goal       *goals;
extern int         goalCount;
extern int         goalCapacity;

/* Predefined course list used by the dropdown menus */
extern const char *COURSES[COURSE_COUNT];

#endif /* MODELS_H */
