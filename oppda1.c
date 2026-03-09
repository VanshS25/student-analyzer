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
int studentCount = 0;
int state = 0;
Student tempStudent;
int updateIdx = -1;

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

void saveToFile() {
#ifdef __EMSCRIPTEN__
    char buf[1024 * 64];
    int pos = 0;
    pos += sprintf(buf + pos, "%d\n", studentCount);
    for (int i = 0; i < studentCount; i++) {
        Student *s = &students[i];
        pos += sprintf(buf + pos, "%s|%s|%.1f|%.1f|%.1f\n",
                       s->regNo, s->name,
                       s->marks[0], s->marks[1], s->marks[2]);
    }
    EM_ASM({ localStorage.setItem('spa_data', UTF8ToString($0)); }, buf);
    printf("[+] %d record(s) saved.\n", studentCount);
#else
    FILE *fp = fopen("students.dat","wb");
    if (!fp) { printf("[!] Cannot save.\n"); return; }
    fwrite(&studentCount, sizeof(int), 1, fp);
    fwrite(students, sizeof(Student), studentCount, fp);
    fclose(fp);
    printf("[+] %d record(s) saved.\n", studentCount);
#endif
}

void loadFromFile() {
#ifdef __EMSCRIPTEN__
    char *data = (char *)EM_ASM_PTR({
        var s = localStorage.getItem('spa_data');
        if (!s) return 0;
        var len = lengthBytesUTF8(s) + 1;
        var heap = _malloc(len);
        stringToUTF8(s, heap, len);
        return heap;
    });
    if (!data) { printf("[i] No saved data. Starting fresh.\n"); return; }
    char *line = strtok(data, "\n");
    if (!line) { free(data); return; }
    studentCount = atoi(line);
    for (int i = 0; i < studentCount; i++) {
        line = strtok(NULL, "\n");
        if (!line) break;
        char tmp[256]; strncpy(tmp, line, 255);
        char *tok = strtok(tmp, "|");
        if (tok) strncpy(students[i].regNo, tok, 19);
        tok = strtok(NULL, "|");
        if (tok) strncpy(students[i].name, tok, 49);
        tok = strtok(NULL, "|"); if (tok) students[i].marks[0] = atof(tok);
        tok = strtok(NULL, "|"); if (tok) students[i].marks[1] = atof(tok);
        tok = strtok(NULL, "|"); if (tok) students[i].marks[2] = atof(tok);
        calculateResult(&students[i]);
    }
    free(data);
    printf("[+] %d record(s) loaded.\n", studentCount);
#else
    FILE *fp = fopen("students.dat","rb");
    if (!fp) { printf("[i] No saved data. Starting fresh.\n"); return; }
    fread(&studentCount, sizeof(int), 1, fp);
    fread(students, sizeof(Student), studentCount, fp);
    fclose(fp);
    printf("[+] %d record(s) loaded.\n", studentCount);
#endif
}

void displayAll() {
    if (studentCount == 0) { printf("[!] No records found.\n"); return; }
    printf("\n%-12s %-20s %6s %6s %6s %8s %7s %6s\n",
           "RegNo","Name","Sub1","Sub2","Sub3","Total","Avg","Grade");
    printf("--------------------------------------------------------------------\n");
    for (int i = 0; i < studentCount; i++) {
        Student *s = &students[i];
        printf("%-12s %-20s %6.1f %6.1f %6.1f %8.1f %7.1f %6s\n",
               s->regNo, s->name,
               s->marks[0], s->marks[1], s->marks[2],
               s->total, s->average, s->grade);
    }
    printf("\nTotal: %d student(s)\n", studentCount);
}

void displayTopper() {
    if (studentCount == 0) { printf("[!] No records.\n"); return; }
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
    if (studentCount == 0) { printf("[!] No records.\n"); return; }
    float sum=0, high=students[0].total, low=students[0].total;
    char hName[50], lName[50];
    strcpy(hName, students[0].name); strcpy(lName, students[0].name);
    for (int i = 0; i < studentCount; i++) {
        sum += students[i].total;
        if (students[i].total > high) { high=students[i].total; strcpy(hName,students[i].name); }
        if (students[i].total < low)  { low =students[i].total; strcpy(lName,students[i].name); }
    }
    printf("\n--- Class Statistics ---\n");
    printf("  Students : %d\n  Avg Total: %.2f\n  Highest  : %.1f (%s)\n  Lowest   : %.1f (%s)\n",
           studentCount, sum/studentCount, high, hName, low, lName);
}

void gradeDistribution() {
    if (studentCount == 0) { printf("[!] No records.\n"); return; }
    int cA=0,cB=0,cC=0,cF=0;
    for (int i=0;i<studentCount;i++) {
        if      (!strcmp(students[i].grade,"A"))    cA++;
        else if (!strcmp(students[i].grade,"B"))    cB++;
        else if (!strcmp(students[i].grade,"C"))    cC++;
        else if (!strcmp(students[i].grade,"Fail")) cF++;
    }
    printf("\n--- Grade Distribution ---\n");
    printf("  A    : %d (%.1f%%)\n", cA, cA*100.0f/studentCount);
    printf("  B    : %d (%.1f%%)\n", cB, cB*100.0f/studentCount);
    printf("  C    : %d (%.1f%%)\n", cC, cC*100.0f/studentCount);
    printf("  Fail : %d (%.1f%%)\n", cF, cF*100.0f/studentCount);
}

void showMenu() {
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
    printf("Enter choice:\n");
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void processInput(const char *input) {
    switch (state) {
        case 0: {
            int choice = atoi(input);
            switch (choice) {
                case 1:
                    printf("\n--- Add New Student ---\n");
                    printf("Enter Reg No:\n");
                    state = 1; break;
                case 2: displayAll();          showMenu(); break;
                case 3: saveToFile();          showMenu(); break;
                case 4:
                    printf("\n--- Search ---\n");
                    printf("Enter Reg No:\n");
                    state = 6; break;
                case 5:
                    printf("\n--- Update Marks ---\n");
                    printf("Enter Reg No:\n");
                    state = 7; break;
                case 6: displayTopper();       showMenu(); break;
                case 7: classStats();          showMenu(); break;
                case 8: gradeDistribution();   showMenu(); break;
                case 0:
                    saveToFile();
                    printf("\nGoodbye!\n"); break;
                default:
                    printf("[!] Invalid choice.\n");
                    showMenu(); break;
            }
            break;
        }
        case 1:
            strncpy(tempStudent.regNo, input, 19);
            if (findByRegNo(tempStudent.regNo) != -1) {
                printf("[!] Reg No already exists! Try again:\n");
                break;
            }
            printf("Enter Name:\n");
            state = 2; break;

        case 2:
            strncpy(tempStudent.name, input, 49);
            printf("Enter Subject 1 marks (0-100):\n");
            state = 3; break;

        case 3:
            tempStudent.marks[0] = atof(input);
            if (tempStudent.marks[0] < 0 || tempStudent.marks[0] > 100) {
                printf("[!] Must be 0-100. Enter Subject 1:\n"); break;
            }
            printf("Enter Subject 2 marks (0-100):\n");
            state = 4; break;

        case 4:
            tempStudent.marks[1] = atof(input);
            if (tempStudent.marks[1] < 0 || tempStudent.marks[1] > 100) {
                printf("[!] Must be 0-100. Enter Subject 2:\n"); break;
            }
            printf("Enter Subject 3 marks (0-100):\n");
            state = 5; break;

        case 5:
            tempStudent.marks[2] = atof(input);
            if (tempStudent.marks[2] < 0 || tempStudent.marks[2] > 100) {
                printf("[!] Must be 0-100. Enter Subject 3:\n"); break;
            }
            calculateResult(&tempStudent);
            students[studentCount++] = tempStudent;
            printf("[+] Added! Total:%.1f Avg:%.1f Grade:%s\n",
                   tempStudent.total, tempStudent.average, tempStudent.grade);
            state = 0; showMenu(); break;

        case 6: {
            int idx = findByRegNo(input);
            if (idx == -1) {
                printf("[!] Not found: %s\n", input);
            } else {
                Student *s = &students[idx];
                printf("\nReg No  : %s\nName    : %s\nSub1    : %.1f\nSub2    : %.1f\nSub3    : %.1f\nTotal   : %.1f\nAverage : %.1f\nGrade   : %s\n",
                       s->regNo, s->name,
                       s->marks[0], s->marks[1], s->marks[2],
                       s->total, s->average, s->grade);
            }
            state = 0; showMenu(); break;
        }
        case 7: {
            int idx = findByRegNo(input);
            if (idx == -1) {
                printf("[!] Not found: %s\nEnter Reg No:\n", input); break;
            }
            updateIdx = idx;
            printf("Updating: %s\nEnter Subject 1 (current: %.1f):\n",
                   students[idx].name, students[idx].marks[0]);
            state = 8; break;
        }
        case 8:
            students[updateIdx].marks[0] = atof(input);
            if (students[updateIdx].marks[0] < 0 || students[updateIdx].marks[0] > 100) {
                printf("[!] Must be 0-100. Enter Subject 1:\n"); break;
            }
            printf("Enter Subject 2 (current: %.1f):\n", students[updateIdx].marks[1]);
            state = 9; break;

        case 9:
            students[updateIdx].marks[1] = atof(input);
            if (students[updateIdx].marks[1] < 0 || students[updateIdx].marks[1] > 100) {
                printf("[!] Must be 0-100. Enter Subject 2:\n"); break;
            }
            printf("Enter Subject 3 (current: %.1f):\n", students[updateIdx].marks[2]);
            state = 10; break;

        case 10:
            students[updateIdx].marks[2] = atof(input);
            if (students[updateIdx].marks[2] < 0 || students[updateIdx].marks[2] > 100) {
                printf("[!] Must be 0-100. Enter Subject 3:\n"); break;
            }
            calculateResult(&students[updateIdx]);
            printf("[+] Updated! Total:%.1f Avg:%.1f Grade:%s\n",
                   students[updateIdx].total, students[updateIdx].average,
                   students[updateIdx].grade);
            state = 0; showMenu(); break;
    }
}

int main() {
    loadFromFile();
    showMenu();
    return 0;
}