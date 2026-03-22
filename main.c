#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DATA_PATH "./"
#define MAX_SHOWS 200
#define MAX_BOOKINGS 500
#define ROWS 5
#define COLS 10

// ANSI Colors
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"
#define BOLD  "\x1B[1m"

typedef struct {
    int show_id;
    char movie_name[64];
    char timing[32];
    float price;
    char img_url[256];
    char region[32];
    int seats[ROWS][COLS]; // Runtime cache
} Show;

typedef struct {
    char booking_id[16];
    char customer_name[64];
    int show_id;
    int num_seats;
    int seat_rows[20];
    int seat_cols[20];
    float total_amount;
    char user_email[64];
    char status[16];
    char show_date[16]; // YYYY-MM-DD
} Booking;

Show shows[MAX_SHOWS];
int num_shows = 0;
Booking bookings[MAX_BOOKINGS];
int num_bookings = 0;

// Forward declarations
void saveData();
void loadData();

// --- Core Logic ---

void markSeatsFromBookings() {
    // Reset all seats to available
    for(int s=0; s<num_shows; s++)
        for(int r=0; r<ROWS; r++)
            for(int c=0; c<COLS; c++)
                shows[s].seats[r][c] = 0;

    // Repopulate from booking file
    for(int b=0; b<num_bookings; b++) {
        for(int s=0; s<num_shows; s++) {
            if(bookings[b].show_id == shows[s].show_id) {
                // We only mark seats if the date logic matches (handled in reports)
                // For local simulation, we mark them generally
            }
        }
    }
}

void loadData() {
    system("node sync.js pull"); 
    FILE *f1 = fopen(DATA_PATH "shows_v4.dat", "rb");
    if (f1) { if (fread(&num_shows, sizeof(int), 1, f1)) fread(shows, sizeof(Show), num_shows, f1); fclose(f1); }
    FILE *f2 = fopen(DATA_PATH "bookings_v4.dat", "rb");
    if (f2) { if (fread(&num_bookings, sizeof(int), 1, f2)) fread(bookings, sizeof(Booking), num_bookings, f2); fclose(f2); }
}

void saveData() {
    FILE *f1 = fopen(DATA_PATH "shows_v4.dat", "wb");
    if (f1) { fwrite(&num_shows, sizeof(int), 1, f1); fwrite(shows, sizeof(Show), num_shows, f1); fclose(f1); }
    FILE *f2 = fopen(DATA_PATH "bookings_v4.dat", "wb");
    if (f2) { fwrite(&num_bookings, sizeof(int), 1, f2); fwrite(bookings, sizeof(Booking), num_bookings, f2); fclose(f2); }
}

// --- Features ---

void displayOccupancyReport() {
    printf("\n" BOLD YEL "DATE-WISE OCCUPANCY REPORT" RESET "\n");
    printf("--------------------------------------------------------------------------------\n");
    printf("%-12s | %-15s | %-10s | %-6s | %-6s | %-6s | %-5s\n", "DATE", "MOVIE", "TIME", "TOTAL", "BOOKED", "AVAIL", "LOAD%");
    printf("--------------------------------------------------------------------------------\n");

    // Strategy: Find every unique (show_id, show_date) pair in bookings
    typedef struct {
        int sid;
        char date[16];
    } UniqueShowDate;
    UniqueShowDate pairs[MAX_BOOKINGS];
    int pair_count = 0;

    for(int b=0; b<num_bookings; b++) {
        int exists = 0;
        for(int p=0; p<pair_count; p++) {
            if(pairs[p].sid == bookings[b].show_id && strcmp(pairs[p].date, bookings[b].show_date) == 0) {
                exists = 1; break;
            }
        }
        if(!exists) {
            pairs[pair_count].sid = bookings[b].show_id;
            strcpy(pairs[pair_count].date, bookings[b].show_date);
            pair_count++;
        }
    }

    // Now display stats for each unique pair
    for(int p=0; p<pair_count; p++) {
        int s_idx = -1;
        for(int s=0; s<num_shows; s++) if(shows[s].show_id == pairs[p].sid) s_idx = s;
        if(s_idx == -1) continue;

        int booked_seats = 0;
        for(int b=0; b<num_bookings; b++) {
            if(bookings[b].show_id == pairs[p].sid && strcmp(bookings[b].show_date, pairs[p].date) == 0) {
                booked_seats += bookings[b].num_seats;
            }
        }

        int total = ROWS * COLS;
        float perc = ((float)booked_seats / total) * 100.0;
        printf("%-12s | %-15.15s | %-10s | %-6d | %-6d | %-6d | %.1f%%\n", 
               pairs[p].date, shows[s_idx].movie_name, shows[s_idx].timing, total, booked_seats, total-booked_seats, perc);
    }

    // Also list shows with 0 bookings for "Today"
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char today[16]; strftime(today, sizeof(today), "%Y-%m-%d", tm);

    for(int s=0; s<num_shows; s++) {
        int has_booking = 0;
        for(int p=0; p<pair_count; p++) if(pairs[p].sid == shows[s].show_id) has_booking = 1;
        if(!has_booking) {
            printf("%-12s | %-15.15s | %-10s | %-6d | %-6d | %-6d | 0.0%%\n", 
                   today, shows[s].movie_name, shows[s].timing, 50, 0, 50);
        }
    }
}

void bookTicket() {
    int sid; char date[16], name[64];
    printf("\n" CYN "ENTER BOOKING DETAILS" RESET "\n");
    printf(" Show ID: "); scanf("%d", &sid);
    printf(" Date (YYYY-MM-DD): "); scanf("%s", date);
    printf(" Customer Name: "); scanf("%s", name);

    int s_idx = -1;
    for(int i=0; i<num_shows; i++) if(shows[i].show_id == sid) s_idx = i;
    if(s_idx == -1) { printf(RED "Invalid Show ID!\n" RESET); return; }

    int count; printf(" Number of Seats: "); scanf("%d", &count);
    
    // Check available space
    int current_booked = 0;
    for(int b=0; b<num_bookings; b++) 
        if(bookings[b].show_id == sid && strcmp(bookings[b].show_date, date) == 0) current_booked += bookings[b].num_seats;
    
    if(current_booked + count > 50) { printf(RED "Not enough seats available!\n" RESET); return; }

    // Generate Booking
    if(num_bookings >= MAX_BOOKINGS) return;
    Booking *nb = &bookings[num_bookings];
    sprintf(nb->booking_id, "%X", (unsigned int)time(NULL) % 1000000);
    strcpy(nb->customer_name, name);
    nb->show_id = sid;
    nb->num_seats = count;
    strcpy(nb->show_date, date);
    strcpy(nb->status, "confirmed");
    nb->total_amount = shows[s_idx].price * count;
    
    num_bookings++;
    saveData();

    printf("\n" GRN "--- BOOKING SUCCESSFUL ---" RESET "\n");
    printf(" ID: #%s | Total: ₹%.2f\n", nb->booking_id, nb->total_amount);
}

void viewTicket() {
    char id[16]; printf("\n Enter Booking ID: "); scanf("%s", id);
    for(int i=0; i<num_bookings; i++) {
        if(strcmp(bookings[i].booking_id, id) == 0) {
            printf("\n" BOLD MAG "RECEIPT FOR #%s" RESET "\n", id);
            printf(" Customer: %s\n", bookings[i].customer_name);
            printf(" Date: %s\n", bookings[i].show_date);
            printf(" Seats: %d\n", bookings[i].num_seats);
            printf(" Total: ₹%.2f\n", bookings[i].total_amount);
            return;
        }
    }
    printf(RED "Ticket not found.\n" RESET);
}

int main() {
    loadData();
    int choice;
    while(1) {
        printf("\n" BOLD CYN "--- CINEBOOKING TERMINAL ---" RESET "\n");
        printf(" 1. View Shows\n 2. Book Tickets\n 3. View Receipt\n 4. Occupancy Report\n 0. Exit\n Choice > ");
        if(scanf("%d", &choice) != 1) break;
        if(choice == 0) break;
        switch(choice) {
            case 1: for(int i=0; i<num_shows; i++) printf(" [%d] %-15s | %s | ₹%.2f\n", shows[i].show_id, shows[i].movie_name, shows[i].timing, shows[i].price); break;
            case 2: bookTicket(); break;
            case 3: viewTicket(); break;
            case 4: displayOccupancyReport(); break;
        }
    }
    return 0;
}
