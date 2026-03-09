#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define DATA_PATH "/data/"
#else
#define DATA_PATH "./"
#define EMSCRIPTEN_KEEPALIVE
#endif

#define MAX_SHOWS 5
#define MAX_BOOKINGS 100
#define ROWS 5
#define COLS 10

typedef struct {
    int show_id;
    char movie_name[64];
    char timing[32];
    float price;
    int seats[ROWS][COLS];
} Show;

typedef struct {
    int booking_id;
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

// --- Core Logic Functions ---

void saveData() {
    char path1[128], path2[128];
    sprintf(path1, "%sshows.dat", DATA_PATH);
    sprintf(path2, "%sbookings.dat", DATA_PATH);

    FILE *f1 = fopen(path1, "wb");
    if (f1) { fwrite(&num_shows, sizeof(int), 1, f1); fwrite(shows, sizeof(Show), num_shows, f1); fclose(f1); }
    FILE *f2 = fopen(path2, "wb");
    if (f2) { fwrite(&num_bookings, sizeof(int), 1, f2); fwrite(bookings, sizeof(Booking), num_bookings, f2); fclose(f2); }

#ifdef __EMSCRIPTEN__
    EM_ASM(
        if (typeof FS !== 'undefined' && FS.syncfs) {
            FS.syncfs(false, function (err) {
                if (err) Module.print("MSG|Error: Web sync failed!");
                else Module.print("MSG|Success: Synced to web storage.");
            });
        }
    );
#else
    printf("\n[SUCCESS] Local data saved to disk.\n");
#endif
}

void EMSCRIPTEN_KEEPALIVE loadData() {
    char path1[128], path2[128];
    sprintf(path1, "%sshows.dat", DATA_PATH);
    sprintf(path2, "%sbookings.dat", DATA_PATH);

    num_shows = 0; num_bookings = 0;
    memset(shows, 0, sizeof(shows));
    memset(bookings, 0, sizeof(bookings));

    FILE *f1 = fopen(path1, "rb");
    if (f1) { if (fread(&num_shows, sizeof(int), 1, f1) == 1) { if (num_shows > 0) fread(shows, sizeof(Show), num_shows, f1); } fclose(f1); }
    
    if (num_shows <= 0) {
        num_shows = 3;
        shows[0].show_id = 1; strcpy(shows[0].movie_name, "Inception"); strcpy(shows[0].timing, "10:00 AM"); shows[0].price = 150.0;
        shows[1].show_id = 2; strcpy(shows[1].movie_name, "Interstellar"); strcpy(shows[1].timing, "02:00 PM"); shows[1].price = 200.0;
        shows[2].show_id = 3; strcpy(shows[2].movie_name, "The Dark Knight"); strcpy(shows[2].timing, "06:00 PM"); shows[2].price = 250.0;
        saveData();
    }

    FILE *f2 = fopen(path2, "rb");
    if (f2) { if (fread(&num_bookings, sizeof(int), 1, f2) == 1) { if (num_bookings > 0) fread(bookings, sizeof(Booking), num_bookings, f2); } fclose(f2); }
}

void EMSCRIPTEN_KEEPALIVE getShows() {
    for (int i = 0; i < num_shows; i++) printf("SHOW|%d|%s|%s|%.2f\n", shows[i].show_id, shows[i].movie_name, shows[i].timing, shows[i].price);
    fflush(stdout);
}

void EMSCRIPTEN_KEEPALIVE getSeatLayout(int show_id) {
    int idx = -1;
    for (int i = 0; i < num_shows; i++) if (shows[i].show_id == show_id) idx = i;
    if (idx == -1) return;
    for (int r = 0; r < ROWS; r++) { for (int c = 0; c < COLS; c++) printf("SEAT|%d|%d|%d\n", r, c, shows[idx].seats[r][c]); }
    printf("SEAT_DONE|%d\n", show_id); fflush(stdout);
}

void EMSCRIPTEN_KEEPALIVE bookTickets(int show_id, char* cust_name, int num_seats, char* seat_list) {
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

    for (int i = 0; i < parsed; i++) {
        if (shows[idx].seats[r_list[i]][c_list[i]] == 1) { printf("MSG|Error: Seat R%d C%d taken!\n", r_list[i]+1, c_list[i]+1); fflush(stdout); return; }
    }

    int b_id = (num_bookings > 0) ? bookings[num_bookings-1].booking_id + 1 : 1001;
    bookings[num_bookings].booking_id = b_id;
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
    printf("SUCCESS_BOOKING|%d|%.2f|%s\n", b_id, shows[idx].price * parsed, cust_name); fflush(stdout);
}

void EMSCRIPTEN_KEEPALIVE viewBooking(int booking_id) {
    for (int i = 0; i < num_bookings; i++) {
        if (bookings[i].booking_id == booking_id) {
            int s_idx = -1;
            for (int j = 0; j < num_shows; j++) if (shows[j].show_id == bookings[i].show_id) s_idx = j;
            if (s_idx != -1) {
                printf("RECEIPT|%d|%s|%s|%s|%d|%.2f|", bookings[i].booking_id, bookings[i].customer_name, shows[s_idx].movie_name, shows[s_idx].timing, bookings[i].num_seats, bookings[i].total_amount);
                for (int k = 0; k < bookings[i].num_seats; k++) printf("R%d C%d%s", bookings[i].seat_rows[k]+1, bookings[i].seat_cols[k]+1, (k==bookings[i].num_seats-1)?"":", ");
                printf("\n"); fflush(stdout); return;
            }
        }
    }
    printf("MSG|Error: ID not found.\n"); fflush(stdout);
}

void EMSCRIPTEN_KEEPALIVE cancelBooking(int booking_id) {
    int found = -1;
    for (int i = 0; i < num_bookings; i++) if (bookings[i].booking_id == booking_id) found = i;
    if (found == -1) { printf("MSG|Error: ID not found.\n"); fflush(stdout); return; }

    int s_idx = -1;
    for (int j = 0; j < num_shows; j++) if (shows[j].show_id == bookings[found].show_id) s_idx = j;
    if (s_idx != -1) {
        for (int k = 0; k < bookings[found].num_seats; k++) shows[s_idx].seats[bookings[found].seat_rows[k]][bookings[found].seat_cols[k]] = 0;
    }
    for (int i = found; i < num_bookings - 1; i++) bookings[i] = bookings[i+1];
    num_bookings--; saveData();
    printf("MSG|Success: Booking %d cancelled.\n", booking_id); fflush(stdout);
}

void EMSCRIPTEN_KEEPALIVE getOccupancyReport() {
    for (int i = 0; i < num_shows; i++) {
        int total = ROWS * COLS, booked = 0;
        for (int r = 0; r < ROWS; r++) { for (int c = 0; c < COLS; c++) if (shows[i].seats[r][c] == 1) booked++; }
        printf("REPORT|%d|%s|%s|%d|%d|%d|%.2f\n", shows[i].show_id, shows[i].movie_name, shows[i].timing, total, booked, total - booked, ((float)booked / total) * 100.0f);
    }
    fflush(stdout);
}

// --- Terminal CLI Version ---

#ifndef __EMSCRIPTEN__
void displayMenu() {
    printf("\n==== CINEBOOKING TERMINAL ====\n");
    printf("STORAGE: LOCAL DISK\n");
    printf("SYNC STATUS: [ NOT SYNCED TO WEB ]\n");
    printf("NOTE: This is a standalone version. For cloud storage, use the web version.\n");
    printf("------------------------------\n");
    printf("1. View Now Showing\n");
    printf("2. Book Tickets\n");
    printf("3. View Ticket Receipt\n");
    printf("4. Cancel Booking\n");
    printf("5. Occupancy Report\n");
    printf("6. Exit\n");
    printf("Choice: ");
}

int main() {
    loadData();
    printf("\n[SYSTEM] Local data loaded. Syncing with web: DISABLED\n");
    int choice, id, seats, r, c;
    char name[64], seat_str[256], temp[32];

    while(1) {
        displayMenu();
        if (scanf("%d", &choice) != 1) { printf("Invalid input!\n"); while(getchar() != '\n'); continue; }
        
        switch(choice) {
            case 1:
                printf("\n--- MOVIES ---\n");
                for(int i=0; i<num_shows; i++) printf("[%d] %s (%s) - Rs. %.2f\n", shows[i].show_id, shows[i].movie_name, shows[i].timing, shows[i].price);
                break;
            case 2:
                printf("Enter Movie ID: "); scanf("%d", &id);
                printf("Customer Name: "); scanf("%s", name);
                printf("Number of seats: "); scanf("%d", &seats);
                seat_str[0] = '\0';
                for(int i=0; i<seats; i++) {
                    printf("Seat %d (Row Col e.g. 1 1): ", i+1); scanf("%d %d", &r, &c);
                    sprintf(temp, "%d,%d%s", r-1, c-1, (i == seats-1) ? "" : "-");
                    strcat(seat_str, temp);
                }
                bookTickets(id, name, seats, seat_str);
                break;
            case 3:
                printf("Enter Booking ID: "); scanf("%d", &id);
                viewBooking(id);
                break;
            case 4:
                printf("Enter Booking ID to cancel: "); scanf("%d", &id);
                cancelBooking(id);
                break;
            case 5:
                printf("\n--- OCCUPANCY ---\n");
                getOccupancyReport();
                break;
            case 6:
                printf("Exiting...\n");
                return 0;
            default: printf("Invalid choice!\n");
        }
    }
    return 0;
}
#else
int main() { return 0; }
#endif
