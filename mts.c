#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// train object
struct train {
    //TODO add in thread_id when creating threads
    int train_no;
    char priority[5];
    char direction[5];
    float loading_time;
    float crossing_time;
    char state[20]; //? what should the length of this be 
        // waiting -> loading -> ready -> granted -> crossing -> crossed
};


int main() { 
    FILE *file = fopen("input.txt", "r");
    if (file == NULL) {
        printf("Could not open file.\n");
        return 1;
    }
    
    struct train trains[5];
    int count = 0;
    int num;
    char dir;
    float ltime;
    float ctime;
    while (fscanf(file, "%s %f %f", &dir, &ltime, &ctime) != EOF) {        
        trains[count].train_no = count;
        if (isupper(dir)) {
            strcpy(trains[count].priority, "high");
        } else {
            strcpy(trains[count].priority, "low");
        }
        if (tolower(dir) == 'e') {
            strcpy(trains[count].direction, "east");
        } else {
            strcpy(trains[count].direction, "west");
        }
        trains[count].loading_time = ltime / 10;
        trains[count].crossing_time = ctime / 10;
        strcpy(trains[count].state, "waiting");
        count++;
    }

    

    fclose(file);
    return 0;
}