#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define MAX_STUDENTS 100

typedef struct {
    char  regNo[20];
    char  name[50];
    float marks[3];
    float total;
    float average;
    char  grade[5];
} Student;

Student students[MAX_STUDENTS];
int     studentCount = 0;

// ─────────────────────────────────────────────
//  SAVE/LOAD via localStorage (WASM) or file (native)
// ─────────────────────────────────────────────
void saveToFile() {
#ifdef __EMSCRIPTEN__
    // Build a CSV string and store in localStorage
    char buf[1024 * 100];
    int  pos = 0;
    pos += sprintf(buf + pos, "%d\n", studentCount);
    for (int i = 0; i < studentCount; i++) {
        Student *s = &students[i];
        pos += sprintf(buf + pos, "%s|%s|%.1f|%.1f|%.1f\n",
                       s->regNo, s->name,
                       s->marks[0], s->marks[1], s->marks[2]);
    }
    // Pass string to JS localStorage
    EM_ASM({ localStorage.setItem('spa_data', UTF8ToString($0)); }, buf);
    printf("\n[+] %d record(s) saved.\n", studentCount);
#else
    FILE *fp = fopen("students.dat", "wb");
    if (!fp) { printf("\n[!] Could not save file.\n"); return; }
    fwrite(&studentCount, sizeof(int),     1,            fp);
    fwrite(students,      sizeof(Student), studentCount, fp);
    fclose(fp);
    printf("\n[+] %d record(s) saved.\n", studentCount);
#endif
}

void loadFromFile() {
#ifdef __EMSCRIPTEN__
    // Read CSV string back from localStorage
    char *data = (char *)EM_ASM_PTR({
        var s = localStorage.getItem('spa_data');
        if (!s) return 0;
        var len  = lengthBytesUTF8(s) + 1;
        var heap = _malloc(len);
        stringToUTF8(s, heap, len);
        return heap;
    });
    if (!data) {
        printf("\n[i] No saved data. Starting fresh.\n");
        return;
    }
    // Parse CSV
    char *line = strtok(data, "\n");
    if (!line) { free(data); return; }
    studentCount = atoi(line);
    for (int i = 0; i < studentCount; i++) {
        line = strtok(NULL, "\n");
        if (!line) break;
        Student *s = &students[i];
        // parse: regNo|name|m1|m2|m3
        char tmp[256];
        strncpy(tmp, line, sizeof(tmp)-1);
        char *tok = strtok(tmp, "|");
        if (tok) strncpy(s->regNo,  tok, 19);
        tok = strtok(NULL, "|");
        if (tok) strncpy(s->name,   tok, 49);
        tok = strtok(NULL, "|"); if (tok) s->marks[0] = atof(tok);
        tok = strtok(NULL, "|"); if (tok) s->marks[1] = atof(tok);
        tok = strtok(NULL, "|"); if (tok) s->marks[2] = atof(tok);
        // recalculate
        s->total   = s->marks[0] + s->marks[1] + s->marks[2];
        s->average = s->total / 3.0f;
        if      (s->average >= 80) strcpy(s->grade, "A");
        else if (s->average >= 65) strcpy(s->grade, "B");
        else if (s->average >= 50) strcpy(s->grade, "C");
        else                       strcpy(s->grade, "Fail");
    }
    free(data);
    printf("\n[+] %d record(s) loaded.\n", studentCount);
#else
    FILE *fp = fopen("students.dat", "rb");
    if (!fp) { printf("\n[i] No saved data. Starting fresh.\n"); return; }
    fread(&studentCount, sizeof(int),     1,            fp);
    fread(students,      sizeof(Student), studentCount, fp);
    fclose(fp);
    printf("\n[+] %d record(s) loaded.\n", studentCount);
#endif
}

// ─────────────────────────────────────────────
//  HELPER
// ─────────────────────────────────────────────
void calculateResult(Student *s) {
    s->total   = s->marks[0] + s->marks[1] + s->marks[2];
    s->average = s->total / 3.0f;
    if      (s->average >= 80) strcpy(s->grade, "A");
    else if (s->average >= 65) strcpy(s->grade, "B");
    else if (s->average >= 50) strcpy(s->grade, "C");
    else                       strcpy(s->grade, "Fail");
}

int findByRegNo(const char *regNo) {
    for (int i = 0; i < studentCount; i++)
        if (strcmp(students[i].regNo, regNo) == 0) return i;
    return -1;
}

// ─────────────────────────────────────────────
//  FEATURES
// ─────────────────────────────────────────────
void addStudent() {
    if (studentCount >= MAX_STUDENTS) {
        printf("\n[!] Student limit reached.\n"); return;
    }
    Student s;
    printf("\n--- Add New Student ---\n");
    printf("Enter Reg No : "); scanf("%19s",      s.regNo);
    printf("Enter Name   : "); scanf(" %49[^\n]", s.name);
    printf("Enter marks for 3 subjects:\n");
    for (int i = 0; i < 3; i++) {
        printf("  Subject %d : ", i + 1);
        if (scanf("%f", &s.marks[i]) != 1) {
            int c; while ((c = getchar()) != '\n' && c != EOF);
            printf("[!] Invalid input.\n"); i--; continue;
        }
        if (s.marks[i] < 0 || s.marks[i] > 100) {
            printf("[!] Marks must be 0-100.\n"); i--;
        }
    }
    calculateResult(&s);
    students[studentCount++] = s;
    printf("\n[+] Added! Total: %.1f | Avg: %.1f | Grade: %s\n",
           s.total, s.average, s.grade);
}

void displayAll() {
    if (studentCount == 0) { printf("\n[!] No records.\n"); return; }
    printf("\n%-12s %-20s %6s %6s %6s %8s %7s %6s\n",
           "RegNo","Name","Sub1","Sub2","Sub3","Total","Avg","Grade");
    printf("%-12s %-20s %6s %6s %6s %8s %7s %6s\n",
           "------------","--------------------",
           "------","------","------","--------","-------","------");
    for (int i = 0; i < studentCount; i++) {
        Student *s = &students[i];
        printf("%-12s %-20s %6.1f %6.1f %6.1f %8.1f %7.1f %6s\n",
               s->regNo, s->name,
               s->marks[0], s->marks[1], s->marks[2],
               s->total, s->average, s->grade);
    }
    printf("\nTotal: %d\n", studentCount);
}

void searchStudent() {
    char regNo[20];
    printf("\n--- Search ---\nEnter Reg No: ");
    scanf("%19s", regNo);
    int idx = findByRegNo(regNo);
    if (idx == -1) { printf("\n[!] Not found: %s\n", regNo); return; }
    Student *s = &students[idx];
    printf("\nReg No  : %s\nName    : %s\n"
           "Sub1-3  : %.1f | %.1f | %.1f\n"
           "Total   : %.1f\nAverage : %.1f\nGrade   : %s\n",
           s->regNo, s->name,
           s->marks[0], s->marks[1], s->marks[2],
           s->total, s->average, s->grade);
}

void updateMarks() {
    char regNo[20];
    printf("\n--- Update Marks ---\nEnter Reg No: ");
    scanf("%19s", regNo);
    int idx = findByRegNo(regNo);
    if (idx == -1) { printf("\n[!] Not found: %s\n", regNo); return; }
    Student *s = &students[idx];
    printf("Updating: %s\n", s->name);
    for (int i = 0; i < 3; i++) {
        printf("  Subject %d (now %.1f): ", i+1, s->marks[i]);
        if (scanf("%f", &s->marks[i]) != 1) {
            int c; while ((c = getchar()) != '\n' && c != EOF);
            i--; continue;
        }
        if (s->marks[i] < 0 || s->marks[i] > 100) {
            printf("[!] Must be 0-100.\n"); i--;
        }
    }
    calculateResult(s);
    printf("\n[+] Updated! Total: %.1f | Avg: %.1f | Grade: %s\n",
           s->total, s->average, s->grade);
}

void displayTopper() {
    if (studentCount == 0) { printf("\n[!] No records.\n"); return; }
    float maxT = students[0].total;
    for (int i = 1; i < studentCount; i++)
        if (students[i].total > maxT) maxT = students[i].total;
    printf("\n--- Topper(s) ---\n");
    for (int i = 0; i < studentCount; i++)
        if (students[i].total == maxT)
            printf("%-12s %-20s Total:%.1f Avg:%.1f Grade:%s\n",
                   students[i].regNo, students[i].name,
                   students[i].total, students[i].average, students[i].grade);
}

void classStats() {
    if (studentCount == 0) { printf("\n[!] No records.\n"); return; }
    float sum=0, high=students[0].total, low=students[0].total;
    char hName[50], lName[50];
    strcpy(hName, students[0].name);
    strcpy(lName,  students[0].name);
    for (int i = 0; i < studentCount; i++) {
        sum += students[i].total;
        if (students[i].total > high) { high=students[i].total; strcpy(hName,students[i].name); }
        if (students[i].total < low)  { low =students[i].total; strcpy(lName,students[i].name); }
    }
    printf("\n--- Class Statistics ---\n"
           "  Students : %d\n  Avg Total: %.2f\n"
           "  Highest  : %.1f (%s)\n  Lowest   : %.1f (%s)\n",
           studentCount, sum/studentCount, high, hName, low, lName);
}

void gradeDistribution() {
    if (studentCount == 0) { printf("\n[!] No records.\n"); return; }
    int cA=0,cB=0,cC=0,cF=0;
    for (int i=0;i<studentCount;i++) {
        if      (!strcmp(students[i].grade,"A"))    cA++;
        else if (!strcmp(students[i].grade,"B"))    cB++;
        else if (!strcmp(students[i].grade,"C"))    cC++;
        else if (!strcmp(students[i].grade,"Fail")) cF++;
    }
    printf("\n--- Grade Distribution ---\n");
    printf("  A    : %d (%.1f%%) [", cA, cA*100.0f/studentCount);
    for(int i=0;i<cA;i++) { printf("#"); } printf("]\n");
    printf("  B    : %d (%.1f%%) [", cB, cB*100.0f/studentCount);
    for(int i=0;i<cB;i++) { printf("#"); } printf("]\n");
    printf("  C    : %d (%.1f%%) [", cC, cC*100.0f/studentCount);
    for(int i=0;i<cC;i++) { printf("#"); } printf("]\n");
    printf("  Fail : %d (%.1f%%) [", cF, cF*100.0f/studentCount);
    for(int i=0;i<cF;i++) { printf("#"); } printf("]\n");
}

void displayMenu() {
    printf("\n========================================\n");
    printf("   Smart Student Performance Analyzer   \n");
    printf("========================================\n");
    printf(" 1. Add Student\n");
    printf(" 2. Display All Students\n");
    printf(" 3. Save Records\n");
    printf(" 4. Search by Reg No\n");
    printf(" 5. Update Marks\n");
    printf(" 6. Display Topper(s)\n");
    printf(" 7. Class Statistics\n");
    printf(" 8. Grade Distribution\n");
    printf(" 0. Exit\n");
    printf("========================================\n");
    printf("Enter choice: ");
}

int main() {
    loadFromFile();
    int choice;
    do {
        displayMenu();
        if (scanf("%d", &choice) != 1) {
            int c; while ((c = getchar()) != '\n' && c != EOF);
            printf("[!] Enter a number.\n");
            continue;
        }
        switch (choice) {
            case 1: addStudent();        break;
            case 2: displayAll();        break;
            case 3: saveToFile();        break;
            case 4: searchStudent();     break;
            case 5: updateMarks();       break;
            case 6: displayTopper();     break;
            case 7: classStats();        break;
            case 8: gradeDistribution(); break;
            case 0: saveToFile(); printf("\nGoodbye!\n"); break;
            default: printf("\n[!] Invalid choice.\n");
        }
    } while (choice != 0);
    return 0;
}