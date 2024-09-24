#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <dos.h>

#define MAX_INPUT 50
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define MAP_WIDTH 11
#define MAP_HEIGHT 11

typedef struct {
    char description[100];
    int north, south, east, west;
    char item[20];
} Room;

Room rooms[] = {
    {"Dimly lit room", 1, -1, 2, -1, "key"},
    {"Cold, damp corridor", -1, 0, -1, -1, ""},
    {"Small library", -1, 3, -1, 0, "book"},
    {"Cozy study", 2, -1, -1, -1, ""}
};

int currentRoom = 0;
char inventory[50] = "";
int gameWon = 0;

char map[MAP_HEIGHT][MAP_WIDTH];

int roomPositions[][2] = {
    {5, 5},  /* Room 0 */
    {5, 3},  /* Room 1 */
    {7, 5},  /* Room 2 */
    {7, 7}   /* Room 3 */
};

void clearScreen() {
    union REGS regs;
    regs.h.ah = 0x06;  /* Scroll up function */
    regs.h.al = 0;     /* Clear entire screen */
    regs.h.ch = 0;     /* Upper left corner row */
    regs.h.cl = 0;     /* Upper left corner col */
    regs.h.dh = 24;    /* Lower right corner row */
    regs.h.dl = 79;    /* Lower right corner col */
    regs.h.bh = 0x07;  /* White on black */
    int86(0x10, &regs, &regs);
}

void gotoxy(int x, int y) {
    union REGS regs;
    regs.h.ah = 0x02;
    regs.h.bh = 0x00;
    regs.h.dh = y;
    regs.h.dl = x;
    int86(0x10, &regs, &regs);
}

void drawBox() {
    int i;
    gotoxy(0, 0);
    putch(218);
    for (i = 1; i < SCREEN_WIDTH - 1; i++) putch(196);
    putch(191);

    for (i = 1; i < SCREEN_HEIGHT - 1; i++) {
        gotoxy(0, i);
        putch(179);
        gotoxy(SCREEN_WIDTH - 1, i);
        putch(179);
    }

    gotoxy(0, SCREEN_HEIGHT - 1);
    putch(192);
    for (i = 1; i < SCREEN_WIDTH - 1; i++) putch(196);
    putch(217);
}

void printWrapped(int x, int y, const char* text, int width) {
    int curX = x, curY = y;
    int len = strlen(text);
    int i = 0;
    int j;

    while (i < len) {
        j = 0;
        while (j < width && i + j < len && text[i + j] != '\n') {
            j++;
        }

        if (i + j < len && text[i + j] != '\n') {
            while (j > 0 && text[i + j - 1] != ' ') {
                j--;
            }
        }

        gotoxy(curX, curY);
        printf("%.*s", j, &text[i]);
        i += j;
        if (text[i] == '\n' || text[i] == ' ') i++;
        curY++;
    }
}

void drawCompass() {
    int compassX = SCREEN_WIDTH - 10;
    int compassY = SCREEN_HEIGHT - 8;

    gotoxy(compassX, compassY);
    printf("   N   ");
    gotoxy(compassX, compassY + 1);
    printf("   |   ");
    gotoxy(compassX, compassY + 2);
    printf("W--+--E");
    gotoxy(compassX, compassY + 3);
    printf("   |   ");
    gotoxy(compassX, compassY + 4);
    printf("   S   ");

    /* Highlight available directions */
    if (rooms[currentRoom].north != -1) {
        gotoxy(compassX + 3, compassY);
        printf("\033[1;33mN\033[0m");
    }
    if (rooms[currentRoom].south != -1) {
        gotoxy(compassX + 3, compassY + 4);
        printf("\033[1;33mS\033[0m");
    }
    if (rooms[currentRoom].east != -1) {
        gotoxy(compassX + 6, compassY + 2);
        printf("\033[1;33mE\033[0m");
    }
    if (rooms[currentRoom].west != -1) {
        gotoxy(compassX, compassY + 2);
        printf("\033[1;33mW\033[0m");
    }
}

void initializeMap() {
    int i, j;
    for (i = 0; i < MAP_HEIGHT; i++) {
        for (j = 0; j < MAP_WIDTH; j++) {
            map[i][j] = ' ';
        }
    }
}

void updateMap() {
    int i;

    /* Initialize the map to spaces */
    initializeMap();

    /* Draw rooms */
    for (i = 0; i < 4; i++) {
        int x = roomPositions[i][0];
        int y = roomPositions[i][1];
        if (i == currentRoom) {
            map[y][x] = 'X';
        } else {
            map[y][x] = 'O';
        }
    }

    /* Draw connections */
    for (i = 0; i < 4; i++) {
        int x1 = roomPositions[i][0];
        int y1 = roomPositions[i][1];

        if (rooms[i].north != -1) {
            int northRoom = rooms[i].north;
            int x2 = roomPositions[northRoom][0];
            int y2 = roomPositions[northRoom][1];

            int y;
            for (y = y2 + 1; y < y1; y++) {
                map[y][x1] = '|';
            }
        }
        if (rooms[i].south != -1) {
            int southRoom = rooms[i].south;
            int x2 = roomPositions[southRoom][0];
            int y2 = roomPositions[southRoom][1];

            int y;
            for (y = y1 + 1; y < y2; y++) {
                map[y][x1] = '|';
            }
        }
        if (rooms[i].east != -1) {
            int eastRoom = rooms[i].east;
            int x2 = roomPositions[eastRoom][0];
            int y2 = roomPositions[eastRoom][1];

            int x;
            for (x = x1 + 1; x < x2; x++) {
                map[y1][x] = '-';
            }
        }
        if (rooms[i].west != -1) {
            int westRoom = rooms[i].west;
            int x2 = roomPositions[westRoom][0];
            int y2 = roomPositions[westRoom][1];

            int x;
            for (x = x2 + 1; x < x1; x++) {
                map[y1][x] = '-';
            }
        }
    }
}

void displayRoom() {
    int mapX, mapY;
    int i, j;

    clearScreen();
    drawBox();

    printWrapped(2, 1, rooms[currentRoom].description, SCREEN_WIDTH - 4);

    gotoxy(2, 3);
    printf("Exits:");
    if (rooms[currentRoom].north != -1) printf(" NORTH");
    if (rooms[currentRoom].south != -1) printf(" SOUTH");
    if (rooms[currentRoom].east != -1) printf(" EAST");
    if (rooms[currentRoom].west != -1) printf(" WEST");

    gotoxy(2, 5);
    printf("Items in room: %s", rooms[currentRoom].item);

    gotoxy(2, 7);
    printf("Inventory: %s", inventory);

    updateMap();
    mapX = SCREEN_WIDTH - MAP_WIDTH - 2;
    mapY = 2;

    for (i = 0; i < MAP_HEIGHT; i++) {
        gotoxy(mapX, mapY + i);
        for (j = 0; j < MAP_WIDTH; j++) {
            putch(map[i][j]);
        }
    }

    drawCompass();

    // Display objective
    gotoxy(2, 9);
    printf("Objective: Bring the key and book to the study to win!");
}

void takeItem() {
    if (strlen(rooms[currentRoom].item) > 0) {
        if (strlen(inventory) > 0) {
            strcat(inventory, ", ");
        }
        strcat(inventory, rooms[currentRoom].item);
        strcpy(rooms[currentRoom].item, "");
        printf("\nYou took the %s.", rooms[currentRoom].item);
    } else {
        printf("\nThere's nothing to take here.");
    }
}

void checkWinCondition() {
    if (currentRoom == 3 && strstr(inventory, "key") && strstr(inventory, "book")) {
        gameWon = 1;
        printf("\nCongratulations! You've brought both the key and the book to the study.");
        printf("\nYou've won the game!");
    }
}

int main() {
    char input[MAX_INPUT];

    clrscr();
    printf("Welcome to the DOS Text Adventure!\n");
    printf("Navigate by typing 'north', 'south', 'east', or 'west'.\n");
    printf("Type 'take' to pick up items, 'inventory' to check your items.\n");
    printf("Bring the key and book to the study to win!\n");
    printf("Type 'quit' to exit the game.\n\n");
    printf("Press any key to start...");
    getch();

    initializeMap();

    while (!gameWon) {
        displayRoom();
        gotoxy(2, SCREEN_HEIGHT - 2);
        printf("What do you want to do? ");
        gets(input);

        if (strcmp(input, "quit") == 0) {
            printf("Thanks for playing! Goodbye.\n");
            break;
        }
        else if (strcmp(input, "north") == 0) {
            if (rooms[currentRoom].north != -1)
                currentRoom = rooms[currentRoom].north;
            else
                printf("You can't go that way.\n");
        }
        else if (strcmp(input, "south") == 0) {
            if (rooms[currentRoom].south != -1)
                currentRoom = rooms[currentRoom].south;
            else
                printf("You can't go that way.\n");
        }
        else if (strcmp(input, "east") == 0) {
            if (rooms[currentRoom].east != -1)
                currentRoom = rooms[currentRoom].east;
            else
                printf("You can't go that way.\n");
        }
        else if (strcmp(input, "west") == 0) {
            if (rooms[currentRoom].west != -1)
                currentRoom = rooms[currentRoom].west;
            else
                printf("You can't go that way.\n");
        }
        else if (strcmp(input, "take") == 0) {
            takeItem();
        }
        else if (strcmp(input, "inventory") == 0) {
            printf("\nYou are carrying: %s", inventory);
        }
        else {
            printf("I don't understand that command.\n");
        }

        checkWinCondition();

        if (!gameWon) {
            printf("\nPress any key to continue...");
            getch();
        }
    }

    if (gameWon) {
        printf("\nPress any key to exit...");
        getch();
    }

    return 0;
}