#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_SLOTS 20
#define NUMBER_LEN 15
#define OWNER_LEN 50
#define DATA_FILE "parking.dat"
#define SECONDS_PER_HOUR 3600
#define RATE_2W 10
#define RATE_4W 20

struct Vehicle
{
    char number[NUMBER_LEN];
    char owner[OWNER_LEN];
    int type;
    int slot;
    time_t entryTime;
    struct Vehicle *next;
};

struct Vehicle *head = NULL;

/* Read a line of text safely. Returns 0 if input ends. */
int readLine(const char *prompt, char *buffer, size_t size)
{
    int ch;
    size_t len;

    printf("%s", prompt);
    if (fgets(buffer, (int)size, stdin) == NULL)
        return 0;

    len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
    {
        buffer[len - 1] = '\0';
    }
    else
    {
        while ((ch = getchar()) != '\n' && ch != EOF)
        {
            /* Discard extra characters. */
        }
    }
    return 1;
}

/* Read an integer and reject non-integer input. */
int readInt(const char *prompt, int *value)
{
    char line[100];
    char extra;
    int number;

    if (!readLine(prompt, line, sizeof(line)))
        return 0;

    if (sscanf(line, "%d %c", &number, &extra) != 1)
    {
        printf("Please enter a valid whole number.\n");
        return -1;
    }

    *value = number;
    return 1;
}

int isBlank(const char *text)
{
    while (*text != '\0')
    {
        if (!isspace((unsigned char)*text))
            return 0;
        text++;
    }
    return 1;
}

/* A vehicle number must be 1 to 14 letters or digits. */
int isValidNumber(const char *text)
{
    size_t i, len = strlen(text);

    if (len == 0 || len >= NUMBER_LEN)
        return 0;

    for (i = 0; i < len; i++)
    {
        if (!isalnum((unsigned char)text[i]))
            return 0;
    }
    return 1;
}

void toUpperCase(char *text)
{
    for (; *text != '\0'; text++)
        *text = (char)toupper((unsigned char)*text);
}

struct Vehicle *findVehicle(const char *number)
{
    struct Vehicle *temp = head;
    while (temp != NULL)
    {
        if (strcmp(temp->number, number) == 0)
            return temp;
        temp = temp->next;
    }
    return NULL;
}

int slotInUse(int slot)
{
    struct Vehicle *temp = head;
    while (temp != NULL)
    {
        if (temp->slot == slot)
            return 1;
        temp = temp->next;
    }
    return 0;
}

int countVehicles(void)
{
    int count = 0;
    struct Vehicle *temp = head;
    while (temp != NULL)
    {
        count++;
        temp = temp->next;
    }
    return count;
}

/* Lowest free slot number, or 0 if the lot is full. */
int findFreeSlot(void)
{
    int slot;
    for (slot = 1; slot <= MAX_SLOTS; slot++)
    {
        if (!slotInUse(slot))
            return slot;
    }
    return 0;
}

/* Hours are rounded up; the minimum charge is one hour. */
int computeHours(time_t entry, time_t now)
{
    double seconds = difftime(now, entry);
    long hours;

    if (seconds < 0)
        seconds = 0;

    hours = (long)(seconds / SECONDS_PER_HOUR);
    if ((double)hours * SECONDS_PER_HOUR < seconds)
        hours++;

    if (hours < 1)
        hours = 1;

    return (int)hours;
}

int rateFor(int type)
{
    return (type == 2) ? RATE_2W : RATE_4W;
}

void formatTime(time_t value, char *buffer, size_t size)
{
    struct tm *info = localtime(&value);
    if (info == NULL || strftime(buffer, size, "%d-%m-%Y %H:%M", info) == 0)
        snprintf(buffer, size, "unknown");
}

void saveVehicles(void)
{
    FILE *file = fopen(DATA_FILE, "w");
    struct Vehicle *temp = head;

    if (file == NULL)
    {
        printf("Warning: Could not save parking data.\n");
        return;
    }

    while (temp != NULL)
    {
        /* slot, type, entry time, number, owner */
        fprintf(file, "%d\n%d\n%lld\n%s\n%s\n",
                temp->slot,
                temp->type,
                (long long)temp->entryTime,
                temp->number,
                temp->owner);
        temp = temp->next;
    }
    fclose(file);
}

void loadVehicles(void)
{
    FILE *file = fopen(DATA_FILE, "r");
    struct Vehicle *newVehicle, *tail = NULL;
    int slot, type;
    long long entry;

    if (file == NULL)
        return; /* First run: no saved file yet. */

    while (1)
    {
        if (fscanf(file, "%d\n%d\n%lld\n", &slot, &type, &entry) != 3)
            break;

        newVehicle = (struct Vehicle *)malloc(sizeof(struct Vehicle));
        if (newVehicle == NULL)
        {
            printf("Warning: Not enough memory to load all vehicles.\n");
            break;
        }

        if (fgets(newVehicle->number, NUMBER_LEN, file) == NULL ||
            fgets(newVehicle->owner, OWNER_LEN, file) == NULL)
        {
            free(newVehicle);
            break;
        }

        newVehicle->number[strcspn(newVehicle->number, "\r\n")] = '\0';
        newVehicle->owner[strcspn(newVehicle->owner, "\r\n")] = '\0';
        newVehicle->slot = slot;
        newVehicle->type = type;
        newVehicle->entryTime = (time_t)entry;
        newVehicle->next = NULL;

        if (slot < 1 || slot > MAX_SLOTS ||
            (type != 2 && type != 4) ||
            entry <= 0 ||
            !isValidNumber(newVehicle->number) ||
            isBlank(newVehicle->owner) ||
            findVehicle(newVehicle->number) != NULL ||
            slotInUse(slot))
        {
            free(newVehicle);
            continue;
        }

        if (head == NULL)
            head = newVehicle;
        else
            tail->next = newVehicle;

        tail = newVehicle;
    }
    fclose(file);
}

void parkVehicle(void)
{
    struct Vehicle *newVehicle, *temp;
    char number[100], owner[100];
    int type, result, slot;

    if (countVehicles() >= MAX_SLOTS)
    {
        printf("Parking is full. No slot is available.\n");
        return;
    }

    if (!readLine("Enter Vehicle Number: ", number, sizeof(number)))
        return;

    toUpperCase(number);

    if (!isValidNumber(number))
    {
        printf("Vehicle number must be 1 to 14 letters or digits only.\n");
        return;
    }

    if (findVehicle(number) != NULL)
    {
        printf("This vehicle is already parked.\n");
        return;
    }

    if (!readLine("Enter Owner Name: ", owner, sizeof(owner)) || isBlank(owner))
    {
        printf("Owner name cannot be empty.\n");
        return;
    }

    if (strlen(owner) >= OWNER_LEN)
    {
        printf("Owner name must be less than %d characters.\n", OWNER_LEN);
        return;
    }

    result = readInt("Vehicle Type (2 = Two Wheeler, 4 = Four Wheeler): ", &type);
    if (result != 1)
        return;

    if (type != 2 && type != 4)
    {
        printf("Vehicle type must be 2 or 4.\n");
        return;
    }

    slot = findFreeSlot();
    newVehicle = (struct Vehicle *)malloc(sizeof(struct Vehicle));
    if (newVehicle == NULL)
    {
        printf("Memory allocation failed.\n");
        return;
    }

    strcpy(newVehicle->number, number);
    strcpy(newVehicle->owner, owner);
    newVehicle->type = type;
    newVehicle->slot = slot;
    newVehicle->entryTime = time(NULL);
    newVehicle->next = NULL;

    if (head == NULL)
    {
        head = newVehicle;
    }
    else
    {
        temp = head;
        while (temp->next != NULL)
            temp = temp->next;
        temp->next = newVehicle;
    }

    saveVehicles();
    printf("Vehicle parked successfully at Slot %d.\n", slot);
}

void displayVehicles(void)
{
    struct Vehicle *temp = head;
    char when[40];

    if (head == NULL)
    {
        printf("No vehicles are currently parked.\n");
        return;
    }

    printf("\n============== PARKED VEHICLES ================\n");
    printf("%-6s %-14s %-20s %-6s %-16s\n",
           "Slot", "Vehicle No", "Owner", "Type", "Entry Time");
    printf("------------------------------------------------------------\n");

    while (temp != NULL)
    {
        formatTime(temp->entryTime, when, sizeof(when));
        printf("%-6d %-14s %-20.20s %-6d %-16s\n",
               temp->slot, temp->number, temp->owner, temp->type, when);
        temp = temp->next;
    }
}

void searchVehicle(void)
{
    char number[100];
    struct Vehicle *vehicle;
    char when[40];

    if (!readLine("Enter Vehicle Number to search: ", number, sizeof(number)))
        return;

    toUpperCase(number);
    vehicle = findVehicle(number);

    if (vehicle == NULL)
    {
        printf("Vehicle not found.\n");
        return;
    }

    formatTime(vehicle->entryTime, when, sizeof(when));
    printf("\nVehicle found!\n");
    printf("Vehicle No: %s\n", vehicle->number);
    printf("Owner: %s\n", vehicle->owner);
    printf("Type: %d wheeler\n", vehicle->type);
    printf("Slot: %d\n", vehicle->slot);
    printf("Entry time: %s\n", when);
}

void removeVehicle(void)
{
    struct Vehicle *temp = head, *previous = NULL;
    char number[100], entry[40], leave[40];
    time_t now;
    int hours, rate;

    if (!readLine("Enter Vehicle Number to remove: ", number, sizeof(number)))
        return;

    toUpperCase(number);

    while (temp != NULL && strcmp(temp->number, number) != 0)
    {
        previous = temp;
        temp = temp->next;
    }

    if (temp == NULL)
    {
        printf("Vehicle not found.\n");
        return;
    }

    now = time(NULL);
    hours = computeHours(temp->entryTime, now);
    rate = rateFor(temp->type);

    formatTime(temp->entryTime, entry, sizeof(entry));
    formatTime(now, leave, sizeof(leave));

    printf("\n================= PARKING BILL =================\n");
    printf("Vehicle No  : %s\n", temp->number);
    printf("Owner       : %s\n", temp->owner);
    printf("Type        : %d wheeler\n", temp->type);
    printf("Slot        : %d\n", temp->slot);
    printf("Entry time  : %s\n", entry);
    printf("Exit time   : %s\n", leave);
    printf("Hours       : %d\n", hours);
    printf("Rate        : Rs %d per hour\n", rate);
    printf("TOTAL BILL  : Rs %d\n", hours * rate);
    printf("================================================\n");

    if (previous == NULL)
        head = temp->next;
    else
        previous->next = temp->next;

    free(temp);
    saveVehicles();
    printf("Vehicle removed. Slot is now free.\n");
}

void showSummary(void)
{
    struct Vehicle *temp = head;
    int two = 0, four = 0, used;

    while (temp != NULL)
    {
        if (temp->type == 2)
            two++;
        else
            four++;
        temp = temp->next;
    }

    used = two + four;
    printf("\n========== PARKING SUMMARY ==========\n");
    printf("Total slots   : %d\n", MAX_SLOTS);
    printf("Occupied      : %d\n", used);
    printf("Available     : %d\n", MAX_SLOTS - used);
    printf("Two wheelers  : %d\n", two);
    printf("Four wheelers : %d\n", four);
}

void freeList(void)
{
    struct Vehicle *temp;
    while (head != NULL)
    {
        temp = head;
        head = head->next;
        free(temp);
    }
}

int main(void)
{
    int choice = 0, result;

    loadVehicles();
    printf("Welcome to the Parking Management System!\n");

    do
    {
        printf("\n========== PARKING MANAGEMENT SYSTEM ==========\n");
        printf("1. Park Vehicle\n");
        printf("2. Display Parked Vehicles\n");
        printf("3. Search Vehicle\n");
        printf("4. Remove Vehicle and Generate Bill\n");
        printf("5. Parking Summary\n");
        printf("6. Exit\n");

        result = readInt("Enter your choice: ", &choice);
        if (result == 0)
            choice = 6;
        else if (result == -1)
            continue;

        switch (choice)
        {
        case 1:
            parkVehicle();
            break;
        case 2:
            displayVehicles();
            break;
        case 3:
            searchVehicle();
            break;
        case 4:
            removeVehicle();
            break;
        case 5:
            showSummary();
            break;
        case 6:
            saveVehicles();
            freeList();
            printf("Parking data saved. Goodbye!\n");
            break;
        default:
            printf("Invalid choice. Please choose 1 to 6.\n");
        }
    } while (choice != 6);

    return 0;
}