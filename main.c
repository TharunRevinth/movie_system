#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define DATA_PATH "/data/"
#else
#include <unistd.h>
#define DATA_PATH "./"
#define EMSCRIPTEN_KEEPALIVE
#endif

#define MAX_SHOWS 200
#define MAX_BOOKINGS 500
#define ROWS 5
#define COLS 10

typedef struct {
    int show_id;
    char movie_name[64];
    char timing[32];
    float price;
    char img_url[256];
    char region[32]; // Added region field
    int seats[ROWS][COLS];
} Show;

typedef struct {
    char booking_id[16];
    char customer_name[64];
    int show_id;
    int num_seats;
    int seat_rows[20];
    int seat_cols[20];
    float total_amount;
} Booking;

Show shows[MAX_SHOWS];
int num_shows = 0;
Booking bookings[MAX_BOOKINGS];
int num_bookings = 0;

void saveData();
void loadData();

// --- WASM Sync Functions ---

void EMSCRIPTEN_KEEPALIVE clearShows() {
    num_shows = 0;
    memset(shows, 0, sizeof(shows));
}

void EMSCRIPTEN_KEEPALIVE addOrUpdateShow(int id, char* name, char* timing, float price, char* img, char* region) {
    int idx = -1;
    for (int i = 0; i < num_shows; i++) {
        if (shows[i].show_id == id) {
            idx = i;
            break;
        }
    }

    if (idx == -1) {
        if (num_shows >= MAX_SHOWS) return;
        idx = num_shows++;
        memset(shows[idx].seats, 0, sizeof(shows[idx].seats));
    }

    shows[idx].show_id = id;
    strncpy(shows[idx].movie_name, name, 63);
    strcpy(shows[idx].timing, timing);
    shows[idx].price = price;
    strncpy(shows[idx].img_url, img, 255);
    strncpy(shows[idx].region, region, 31);
}

void EMSCRIPTEN_KEEPALIVE resetSystem() {
    num_shows = 0;
    num_bookings = 0;
    memset(shows, 0, sizeof(shows));
    memset(bookings, 0, sizeof(bookings));
    saveData();
    printf("MSG|System Reset Success\n");
    fflush(stdout);
}

void EMSCRIPTEN_KEEPALIVE markSeat(int show_id, int r, int c, int state) {
    for (int i = 0; i < num_shows; i++) {
        if (shows[i].show_id == show_id) {
            if (r >= 0 && r < ROWS && c >= 0 && c < COLS) {
                shows[i].seats[r][c] = state;
            }
            return;
        }
    }
}

void EMSCRIPTEN_KEEPALIVE saveData() {
#ifndef __EMSCRIPTEN__
    char path1[128], path2[128];
    sprintf(path1, "%sshows_v4.dat", DATA_PATH); 
    sprintf(path2, "%sbookings_v4.dat", DATA_PATH);

    FILE *f1 = fopen(path1, "wb");
    if (f1) {
        fwrite(&num_shows, sizeof(int), 1, f1);
        fwrite(shows, sizeof(Show), num_shows, f1);
        fclose(f1);
    }
    FILE *f2 = fopen(path2, "wb");
    if (f2) {
        fwrite(&num_bookings, sizeof(int), 1, f2);
        fwrite(bookings, sizeof(Booking), num_bookings, f2);
        fclose(f2);
    }
#endif
}

void EMSCRIPTEN_KEEPALIVE loadData() {
    num_shows = 0;
    num_bookings = 0;
    memset(shows, 0, sizeof(shows));
    memset(bookings, 0, sizeof(bookings));

#ifndef __EMSCRIPTEN__
    char path1[128], path2[128];
    sprintf(path1, "%sshows_v4.dat", DATA_PATH);
    sprintf(path2, "%sbookings_v4.dat", DATA_PATH);
    
    FILE *f1 = fopen(path1, "rb");
    if (f1) {
        int loaded_num = 0;
        if (fread(&loaded_num, sizeof(int), 1, f1) == 1) {
            if (loaded_num >= 0 && loaded_num <= MAX_SHOWS) {
                num_shows = loaded_num;
                fread(shows, sizeof(Show), num_shows, f1);
            }
        }
        fclose(f1);
    }
    
    FILE *f2 = fopen(path2, "rb");
    if (f2) {
        int loaded_bookings = 0;
        if (fread(&loaded_bookings, sizeof(int), 1, f2) == 1) {
            if (loaded_bookings >= 0 && loaded_bookings <= MAX_BOOKINGS) {
                num_bookings = loaded_bookings;
                fread(bookings, sizeof(Booking), num_bookings, f2);
            }
        }
        fclose(f2);
    }
#endif
}

void EMSCRIPTEN_KEEPALIVE getShows() {
    for (int i = 0; i < num_shows; i++) {
        printf("SHOW|%d|%s|%s|%.2f|%s|%s\n", shows[i].show_id, shows[i].movie_name, shows[i].timing, shows[i].price, shows[i].img_url, shows[i].region);
    }
    fflush(stdout);
}

void EMSCRIPTEN_KEEPALIVE getSeatLayout(int show_id) {
    int idx = -1;
    for (int i = 0; i < num_shows; i++) if (shows[i].show_id == show_id) idx = i;
    if (idx == -1) return;
    for (int r = 0; r < ROWS; r++) { for (int c = 0; c < COLS; c++) printf("SEAT|%d|%d|%d\n", r, c, shows[idx].seats[r][c]); }
    printf("SEAT_DONE|%d\n", show_id); fflush(stdout);
}

void EMSCRIPTEN_KEEPALIVE bookTickets(int show_id, char* cust_name, int num_seats, char* seat_list, char* b_id) {
    int idx = -1;
    for (int i = 0; i < num_shows; i++) if (shows[i].show_id == show_id) idx = i;
    if (idx == -1 || num_bookings >= MAX_BOOKINGS) return;

    int r_list[20], c_list[20], parsed = 0;
    char *copy = strdup(seat_list), *token = strtok(copy, "-");
    while (token != NULL && parsed < num_seats) {
        int r, c; if (sscanf(token, "%d,%d", &r, &c) == 2) { r_list[parsed] = r; c_list[parsed] = c; parsed++; }
        token = strtok(NULL, "-");
    }
    free(copy);

    strncpy(bookings[num_bookings].booking_id, b_id, 15);
    strncpy(bookings[num_bookings].customer_name, cust_name, 63);
    bookings[num_bookings].show_id = show_id;
    bookings[num_bookings].num_seats = parsed;
    bookings[num_bookings].total_amount = shows[idx].price * parsed;
    for (int i = 0; i < parsed; i++) {
        bookings[num_bookings].seat_rows[i] = r_list[i];
        bookings[num_bookings].seat_cols[i] = c_list[i];
        shows[idx].seats[r_list[i]][c_list[i]] = 1;
    }
    num_bookings++; saveData();
    printf("SUCCESS_BOOKING|%s|%.2f|%s\n", b_id, shows[idx].price * parsed, cust_name); fflush(stdout);
}

void EMSCRIPTEN_KEEPALIVE viewBooking(char* booking_id) {
    for (int i = 0; i < num_bookings; i++) {
        if (strcmp(bookings[i].booking_id, booking_id) == 0) {
            int s_idx = -1;
            for (int j = 0; j < num_shows; j++) if (shows[j].show_id == bookings[i].show_id) s_idx = j;
            if (s_idx != -1) {
                printf("RECEIPT|%s|%s|%s|%s|%d|%.2f|", bookings[i].booking_id, bookings[i].customer_name, shows[s_idx].movie_name, shows[s_idx].timing, bookings[i].num_seats, bookings[i].total_amount);
                for (int k = 0; k < bookings[i].num_seats; k++) printf("R%d C%d%s", bookings[i].seat_rows[k]+1, bookings[i].seat_cols[k]+1, (k==bookings[i].num_seats-1)?"":", ");
                printf("\n"); fflush(stdout); return;
            }
        }
    }
    printf("MSG|Error: ID not found.\n"); fflush(stdout);
}

void EMSCRIPTEN_KEEPALIVE cancelBooking(char* booking_id) {
    int found = -1;
    for (int i = 0; i < num_bookings; i++) if (strcmp(bookings[i].booking_id, booking_id) == 0) found = i;
    if (found == -1) { printf("MSG|Error: ID not found.\n"); fflush(stdout); return; }

    int s_idx = -1;
    for (int j = 0; j < num_shows; j++) if (shows[j].show_id == bookings[found].show_id) s_idx = j;
    if (s_idx != -1) {
        for (int k = 0; k < bookings[found].num_seats; k++) shows[s_idx].seats[bookings[found].seat_rows[k]][bookings[found].seat_cols[k]] = 0;
    }
    for (int i = found; i < num_bookings - 1; i++) bookings[i] = bookings[i+1];
    num_bookings--; saveData();
    printf("MSG|Success: Booking %s cancelled.\n", booking_id); fflush(stdout);
}

void EMSCRIPTEN_KEEPALIVE getOccupancyReport() {
    for (int i = 0; i < num_shows; i++) {
        int total = ROWS * COLS, booked = 0;
        for (int r = 0; r < ROWS; r++) { for (int c = 0; c < COLS; c++) if (shows[i].seats[r][c] == 1) booked++; }
        printf("REPORT|%d|%s|%s|%d|%d|%d|%.2f\n", shows[i].show_id, shows[i].movie_name, shows[i].timing, total, booked, total - booked, ((float)booked / total) * 100.0f);
    }
    fflush(stdout);
}

void EMSCRIPTEN_KEEPALIVE getTicketQRData(char* booking_id) {
    for (int i = 0; i < num_bookings; i++) {
        if (strcmp(bookings[i].booking_id, booking_id) == 0) {
            printf("QR_DATA|CINEBOOKING-%s-%s\n", bookings[i].booking_id, bookings[i].customer_name);
            fflush(stdout);
            return;
        }
    }
    printf("QR_DATA|INVALID\n");
    fflush(stdout);
}

// --- Terminal CLI ---
#ifndef __EMSCRIPTEN__
void displayMenu() {
    printf("\n--- VIP CINEBOOKING ADMIN TERMINAL ---\n1. View Shows\n2. Book (Override)\n3. View Ticket\n4. Cancel Booking\n5. Occupancy Report\n6. Exit\nChoice: ");
}

int authenticateAdmin() {
    char username[64];
    char password[64];
    printf("\n--- CINEBOOKING ADMIN LOGIN ---\n");
    printf("Username: ");
    scanf("%s", username);
    printf("Password: ");
    scanf("%s", password);
    
    if (strcmp(username, "admin") == 0 && strcmp(password, "admin123") == 0) {
        printf("Login successful. Welcome, Admin.\n");
        return 1;
    } else {
        printf("Access denied.\n");
        return 0;
    }
}

int main() {
    loadData();
    
    int authAttempts = 0;
    while (authAttempts < 3) {
        if (authenticateAdmin()) {
            break;
        }
        authAttempts++;
    }
    if (authAttempts >= 3) {
        printf("Too many failed attempts. Terminating.\n");
        return 1;
    }

    int choice, seats, r, c, s_id;
    char name[64], seat_str[256], b_id[16];
    while(1) {
        displayMenu();
        if (scanf("%d", &choice) != 1) break;
        if (choice == 6) return 0;
        switch(choice) {
            case 1: getShows(); break;
            case 2:
                printf("Show ID: "); scanf("%d", &s_id);
                printf("Name: "); scanf("%s", name);
                printf("Seats: "); scanf("%d", &seats);
                seat_str[0] = '\0';
                for(int i=0; i<seats; i++) {
                    printf("Row Col (0-4 0-9): "); scanf("%d %d", &r, &c);
                    char tmp[16]; sprintf(tmp, "%d,%d%s", r, c, (i==seats-1)?"":"-");
                    strcat(seat_str, tmp);
                }
                char rand_id[16]; sprintf(rand_id, "%X", rand() % 1000000);
                bookTickets(s_id, name, seats, seat_str, rand_id);
                break;
            case 3: printf("Ticket ID: "); scanf("%s", b_id); viewBooking(b_id); break;
            case 4: printf("Ticket ID: "); scanf("%s", b_id); cancelBooking(b_id); break;
            case 5: getOccupancyReport(); break;
            default: printf("Invalid choice.\n");
        }
    }
    return 0;
}
#else
int main() { return 0; }
#endif
