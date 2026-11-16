# Product Requirement Document (PRD)

## StudyBuddy Campus Match – Desktop GUI Application

## 1. Document Overview

* **Project Name:** StudyBuddy Campus Match
* **Course:** Dasar Pemrograman (Procedural Programming with C)
* **Target Institution:** Universitas Pembangunan Nasional Veteran Jakarta (UPNVJ)
* **Study Program:** S1 Sistem Informasi (Faculty of Computer Science)
* **Version:** 1.0.0
* **Date:** November 2026
* **Development Team:** Kelompok 3

## 2. Executive Summary

**StudyBuddy Campus Match** is a native desktop application engineered using **C Programming Language** with a graphical user interface (GUI) built via **Raylib / Raygui**. The platform enables university students to find compatible study buddies based on shared courses, organize/join structured study groups with real-time member inspection, manage individual learning goals, and preserve user activity using persistent File I/O storage (`studybuddy_data.txt`).

## 3. Business Goals & Objectives

1. **Academic Collaboration:** Reduce barriers for S1 Information Systems students in forming peer-to-peer study groups.
2. **Procedural C Demonstration:** Showcase fundamental procedural programming concepts in C (`struct`, dynamic array structures, File I/O, functions, and modular architecture).
3. **Enhanced User Experience (UX):** Transition from standard Command Line Interfaces (CLI) to a clean, standalone Desktop GUI without terminal reliance (`-mwindows` compilation build).

## 4. System Architecture & Project Structure

The project follows a clean, modular software design pattern separating core data structures, storage persistence, UI rendering, and execution entries:

```
StudyBuddy-GUI/
│
├── bin/                        # Compiled executable output
│   └── StudyBuddyApp.exe
├── data/                       # Persistent File Storage
│   └── studybuddy_data.txt
├── include/                    # Header Declarations (.h)
│   ├── models.h                # Data Models & Global Variables
│   ├── file_manager.h          # File Storage Operations
│   └── gui_manager.h           # Desktop Window & View State Definitions
└── src/                        # Implementation Files (.c)
    ├── file_manager.c          # Storage IO Implementation
    ├── gui_manager.c           # GUI Render & Event Processing
    └── main.c                  # Application Lifecycles & Entry Point
```

## 5. Functional Requirements (Features)

### 5.1 Home Dashboard (`SCREEN_HOME`)

* Displays overall system metrics:
  * Total registered study buddies.
  * Total active study groups.
  * Total tracked learning goals.
* Serves as the primary landing page upon application boot.

### 5.2 Buddy Match & Catalog (`SCREEN_FIND_BUDDY` & `SCREEN_LIST_BUDDY`)

* **Course Selection:** Predefined dropdown menu selection for target academic subjects:
  1. Pemrograman C
  2. Basis Data
  3. Logika Matematika
  4. Sistem Informasi
* **Catalog Inspection:** Browse all registered students displaying Name, NIM, Focus Course, and Available Schedule.

### 5.3 Student Registration & Group Assignment (`SCREEN_ADD_BUDDY`)

* Input fields for **Full Name**, **NIM**, **Course Focus**, and **Available Schedule**.
* **Pre-inspection:** Enables new students to inspect existing study groups and view current member lists prior to joining.
* **Join or Create Branching:**
  * Option to join an existing active group (subject to maximum quota limits).
  * Option to create a new group (auto-assigning the registrant as group creator).

### 5.4 Study Group Management (`SCREEN_GROUPS`)

* Displays active study groups with member counts (`current / max capacity`).
* Displays explicit list of participant names enrolled in each group.

### 5.5 Learning Goals Tracker (`SCREEN_GOALS`)

* Create customized learning objectives (e.g., *"Mastering Pointer & Memory Management in C"*).
* Toggle target status between `[ON PROGRESS]` and `[SELESAI]`.

### 5.6 Data Persistence (File I/O)

* Automatic sync to `data/studybuddy_data.txt` on application startup and state update.
* Automatic generation of pre-loaded sample data (dummy records) if dataset file is absent on initial launch.

## 6. Non-Functional Requirements

### 6.1 Performance & Resource Management

* Target Frame Rate: **60 FPS** window rendering.
* Low CPU/RAM footprint under **30 MB** RAM usage.
* Launch time under **1 second**.

### 6.2 Compatibility & Environment

* Target OS: Windows 10 / Windows 11 (64-bit).
* Build System: GCC (MinGW-w64).
* UI Engine: **Raylib 4.x / Raygui**.

## 7. Data Models & Schemas

### 7.1 Data Structures (`models.h`)

```c
typedef struct {
    char nama[50];
    char nim[15];
    char matkul[30];
    char hari[20];
} StudyBuddy;

typedef struct {
    char namaGroup[40];
    char matkul[30];
    int jumlahAnggota;
    int maxAnggota;
    char anggotaNama[5][50];
} Group;

typedef struct {
    char deskripsi[100];
    int status; // 0: On Progress, 1: Selesai
} Goal;
```

## 8. Build & Compilation Instructions

To compile the application as a standalone desktop binary without a console terminal window:

```bash
gcc -Iinclude src/main.c src/file_manager.c src/gui_manager.c -o bin/StudyBuddyApp.exe -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows
```

## 9. Deliverables & Checklist

* [x] C Source Code with modular folder structure (`src/`, `include/`, `data/`, `bin/`).
* [x] Native Desktop Graphical Window (Raylib GUI).
* [x] Full persistent storage implementation via File I/O.
* [x] Product Requirement Document (`PRD_StudyBuddy_Campus_Match.md`).
* [ ] Application Executable Output Video Demo (1–6 mins duration).
* [ ] Final Academic Report PDF.