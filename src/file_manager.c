#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "models.h"
#include "file_manager.h"

/*
 * File format (one record per line, '|' separated, '#' = comment):
 *   B|nama|nim|matkul|hari
 *   G|namaGroup|matkul|jumlahAnggota|maxAnggota|member1;member2;...
 *   T|status|deskripsi
 */

#define INITIAL_CAPACITY 8
#define LINE_BUFFER      1024
#define PATH_BUFFER      260

static char dataPath[PATH_BUFFER] = "data/studybuddy_data.txt";

/* ------------------------------------------------------------------ */
/* Internal helpers                                                    */
/* ------------------------------------------------------------------ */

static int EnsureCapacity(void **arr, int *capacity, int count, size_t elemSize)
{
    if (count < *capacity) return 1;

    int newCap = (*capacity == 0) ? INITIAL_CAPACITY : *capacity * 2;
    void *tmp = realloc(*arr, (size_t)newCap * elemSize);
    if (tmp == NULL) return 0;

    *arr = tmp;
    *capacity = newCap;
    return 1;
}

/* Works whether the app is started from the project root or from bin/ */
static void ResolveDataPath(void)
{
    static const char *candidates[] = {
        "data/studybuddy_data.txt",
        "../data/studybuddy_data.txt"
    };
    const int n = (int)(sizeof(candidates) / sizeof(candidates[0]));
    FILE *f;

    for (int i = 0; i < n; i++) {            /* 1) an existing file wins */
        f = fopen(candidates[i], "r");
        if (f) {
            fclose(f);
            snprintf(dataPath, sizeof dataPath, "%s", candidates[i]);
            return;
        }
    }
    for (int i = 0; i < n; i++) {            /* 2) otherwise first writable data/ dir */
        f = fopen(candidates[i], "w");
        if (f) {
            fclose(f);
            snprintf(dataPath, sizeof dataPath, "%s", candidates[i]);
            return;
        }
    }
    snprintf(dataPath, sizeof dataPath, "%s", "studybuddy_data.txt");  /* 3) last resort */
}

static int SplitFields(char *line, char *fields[], int maxFields)
{
    int n = 0;
    fields[n++] = line;
    for (char *p = line; *p && n < maxFields; p++) {
        if (*p == '|') {
            *p = '\0';
            fields[n++] = p + 1;
        }
    }
    return n;
}

static void SampleGroup(const char *name, const char *matkul, int max,
                        const char *m1, const char *m2)
{
    Group g;
    memset(&g, 0, sizeof g);
    snprintf(g.namaGroup, sizeof g.namaGroup, "%s", name);
    snprintf(g.matkul, sizeof g.matkul, "%s", matkul);
    g.maxAnggota = max;
    int idx = AddGroup(&g);
    if (idx >= 0) {
        AddGroupMember(idx, m1);
        AddGroupMember(idx, m2);
    }
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

const char *GetDataPath(void) { return dataPath; }

void SanitizeText(char *s)
{
    for (char *p = s; *p; p++) {
        if (*p == '|' || *p == ';' || *p == '\n' || *p == '\r' || *p == '\t') *p = ' ';
    }
    char *start = s;
    while (*start == ' ') start++;
    if (start != s) memmove(s, start, strlen(start) + 1);

    size_t len = strlen(s);
    while (len > 0 && s[len - 1] == ' ') s[--len] = '\0';
}

int AddBuddy(const StudyBuddy *b)
{
    void *p = buddies;
    if (!EnsureCapacity(&p, &buddyCapacity, buddyCount, sizeof(StudyBuddy))) return -1;
    buddies = (StudyBuddy *)p;
    buddies[buddyCount] = *b;
    return buddyCount++;
}

int AddGroup(const Group *g)
{
    void *p = groups;
    if (!EnsureCapacity(&p, &groupCapacity, groupCount, sizeof(Group))) return -1;
    groups = (Group *)p;
    groups[groupCount] = *g;
    return groupCount++;
}

int AddGoal(const Goal *g)
{
    void *p = goals;
    if (!EnsureCapacity(&p, &goalCapacity, goalCount, sizeof(Goal))) return -1;
    goals = (Goal *)p;
    goals[goalCount] = *g;
    return goalCount++;
}

int AddGroupMember(int groupIndex, const char *name)
{
    if (groupIndex < 0 || groupIndex >= groupCount) return 0;

    Group *g = &groups[groupIndex];
    if (g->jumlahAnggota >= g->maxAnggota || g->jumlahAnggota >= MAX_GROUP_MEMBERS) return 0;

    snprintf(g->anggotaNama[g->jumlahAnggota], MAX_NAMA, "%s", name);
    g->jumlahAnggota++;
    return 1;
}

void CreateSampleData(void)
{
    static const char *sample[][4] = {
        { "Aulia Rahma",   "2410512001", "Pemrograman C",     "Senin 14:00"  },
        { "Bima Pratama",  "2410512015", "Basis Data",        "Selasa 10:00" },
        { "Citra Lestari", "2410512022", "Pemrograman C",     "Rabu 13:00"   },
        { "Dimas Saputra", "2410512030", "Logika Matematika", "Kamis 15:00"  },
        { "Eka Putri",     "2410512041", "Sistem Informasi",  "Jumat 09:00"  },
        { "Fajar Nugroho", "2410512047", "Basis Data",        "Senin 16:00"  }
    };
    const int n = (int)(sizeof(sample) / sizeof(sample[0]));

    for (int i = 0; i < n; i++) {
        StudyBuddy b;
        memset(&b, 0, sizeof b);
        snprintf(b.nama,   sizeof b.nama,   "%s", sample[i][0]);
        snprintf(b.nim,    sizeof b.nim,    "%s", sample[i][1]);
        snprintf(b.matkul, sizeof b.matkul, "%s", sample[i][2]);
        snprintf(b.hari,   sizeof b.hari,   "%s", sample[i][3]);
        AddBuddy(&b);
    }

    SampleGroup("C Warriors",   "Pemrograman C", MAX_GROUP_MEMBERS, "Aulia Rahma",  "Citra Lestari");
    SampleGroup("Data Wizards", "Basis Data",    MAX_GROUP_MEMBERS, "Bima Pratama", "Fajar Nugroho");

    static const struct { const char *desc; int status; } sampleGoals[] = {
        { "Mastering Pointer & Memory Management in C",   0 },
        { "Menyelesaikan latihan normalisasi Basis Data", 0 },
        { "Memahami tabel kebenaran & logika proposisi",  1 }
    };
    for (int i = 0; i < 3; i++) {
        Goal g;
        memset(&g, 0, sizeof g);
        snprintf(g.deskripsi, sizeof g.deskripsi, "%s", sampleGoals[i].desc);
        g.status = sampleGoals[i].status;
        AddGoal(&g);
    }
}

int SaveData(void)
{
    FILE *f = fopen(dataPath, "w");
    if (!f) return 0;

    fprintf(f, "# StudyBuddy Campus Match data file - edited automatically\n");

    for (int i = 0; i < buddyCount; i++) {
        fprintf(f, "B|%s|%s|%s|%s\n",
                buddies[i].nama, buddies[i].nim, buddies[i].matkul, buddies[i].hari);
    }

    for (int i = 0; i < groupCount; i++) {
        fprintf(f, "G|%s|%s|%d|%d|",
                groups[i].namaGroup, groups[i].matkul,
                groups[i].jumlahAnggota, groups[i].maxAnggota);
        for (int k = 0; k < groups[i].jumlahAnggota; k++) {
            fprintf(f, "%s%s", k > 0 ? ";" : "", groups[i].anggotaNama[k]);
        }
        fputc('\n', f);
    }

    for (int i = 0; i < goalCount; i++) {
        fprintf(f, "T|%d|%s\n", goals[i].status, goals[i].deskripsi);
    }

    return fclose(f) == 0;
}

int LoadData(void)
{
    FILE *f = fopen(dataPath, "r");
    if (!f) return 0;

    char line[LINE_BUFFER];
    int loaded = 0;

    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0' || line[0] == '#') continue;

        char *fld[8];
        int n = SplitFields(line, fld, 8);

        if (strcmp(fld[0], "B") == 0 && n >= 5) {
            StudyBuddy b;
            memset(&b, 0, sizeof b);
            snprintf(b.nama,   sizeof b.nama,   "%s", fld[1]);
            snprintf(b.nim,    sizeof b.nim,    "%s", fld[2]);
            snprintf(b.matkul, sizeof b.matkul, "%s", fld[3]);
            snprintf(b.hari,   sizeof b.hari,   "%s", fld[4]);
            if (AddBuddy(&b) >= 0) loaded++;

        } else if (strcmp(fld[0], "G") == 0 && n >= 6) {
            Group g;
            memset(&g, 0, sizeof g);
            snprintf(g.namaGroup, sizeof g.namaGroup, "%s", fld[1]);
            snprintf(g.matkul,    sizeof g.matkul,    "%s", fld[2]);
            g.maxAnggota = atoi(fld[4]);

            for (char *tok = strtok(fld[5], ";");
                 tok != NULL && g.jumlahAnggota < MAX_GROUP_MEMBERS;
                 tok = strtok(NULL, ";")) {
                snprintf(g.anggotaNama[g.jumlahAnggota], MAX_NAMA, "%s", tok);
                g.jumlahAnggota++;
            }
            if (g.maxAnggota > MAX_GROUP_MEMBERS) g.maxAnggota = MAX_GROUP_MEMBERS;
            if (g.maxAnggota < g.jumlahAnggota)   g.maxAnggota = g.jumlahAnggota;
            if (g.maxAnggota < 1)                 g.maxAnggota = 1;
            if (AddGroup(&g) >= 0) loaded++;

        } else if (strcmp(fld[0], "T") == 0 && n >= 3) {
            Goal g;
            memset(&g, 0, sizeof g);
            g.status = (atoi(fld[1]) == 1) ? 1 : 0;
            snprintf(g.deskripsi, sizeof g.deskripsi, "%s", fld[2]);
            if (AddGoal(&g) >= 0) loaded++;
        }
    }

    fclose(f);
    return loaded;
}

void InitData(void)
{
    ResolveDataPath();
    if (LoadData() == 0) {          /* missing or empty file -> sample data */
        CreateSampleData();
        SaveData();
    }
}

void FreeData(void)
{
    free(buddies); buddies = NULL; buddyCount = 0; buddyCapacity = 0;
    free(groups);  groups  = NULL; groupCount = 0; groupCapacity = 0;
    free(goals);   goals   = NULL; goalCount  = 0; goalCapacity  = 0;
}
