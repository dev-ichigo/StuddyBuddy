#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "models.h"

/* Lifecycle */
void        InitData(void);          /* resolve path, load file, or create sample data */
void        FreeData(void);          /* release all dynamic arrays                     */

/* Persistence */
int         LoadData(void);          /* returns number of records loaded               */
int         SaveData(void);          /* returns 1 on success, 0 on failure             */
void        CreateSampleData(void);  /* pre-loaded dummy records                       */
const char *GetDataPath(void);       /* path of the file currently in use              */

/* Record helpers (return new index, or -1 on failure) */
int         AddBuddy(const StudyBuddy *b);
int         AddGroup(const Group *g);
int         AddGoal(const Goal *g);
int         RemoveGoal(int goalIndex);
int         AddChatMessage(const ChatMessage *m);
int         AddGroupInvite(const GroupInvite *inv); // <-- TAMBAH INI
int         RemoveGroupInvite(int inviteIndex);     // <-- TAMBAH INI

/* Adds a member to an existing group. Returns 1 on success, 0 if full/invalid. */
int         AddGroupMember(int groupIndex, const char *name);
int         RemoveGroup(int groupIndex);
int         RemoveGroupMember(int groupIndex, const char *memberName);

/* Trims text and replaces characters that would break the file format. */
void        SanitizeText(char *s);

#endif /* FILE_MANAGER_H */
