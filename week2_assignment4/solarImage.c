#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

typedef enum {
    INPUT_VALID,
    INPUT_INVALID
} InputStatus;


void swap(unsigned short *a, unsigned short *b) {
    unsigned short temp = *a;
    *a = *b;
    *b = temp;
}


void rotateMatrix(int n, unsigned short *matrix) {
    for (int i = 0; i < n; i++) { 
        for (int j = i + 1; j < n; j++) { 
            unsigned short *p1 = matrix + i * n + j;
            unsigned short *p2 = matrix + j * n + i;
            swap(p1, p2);
        }
    }  
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n / 2; j++) {
            unsigned short *p1 = matrix + i * n + j;
            unsigned short *p2 = matrix + i * n + (n - j - 1);
            swap(p1, p2);
        }
    }
}

void smoothing(int n, unsigned short *matrix) {
    int dx[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dy[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            unsigned short *curr = matrix + i * n + j;
            int sum = *curr & 0xFF;
            int count = 1;

            for (int k = 0; k < 8; k++) {
                int ni = i + dx[k];
                int nj = j + dy[k];
                if (ni >= 0 && ni < n && nj >= 0 && nj < n) {
                    unsigned short *neighbor = matrix + ni * n + nj;
                    sum += *neighbor & 0xFF;
                    count++;
                }
            }

            int avg = sum / count;
            *curr |= (avg << 8);
        }
    }

    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            unsigned short *curr = matrix + i * n + j;
            *curr >>= 8;
        }
    }
}



InputStatus readMatrixSize(unsigned short *n) {
    printf("Enter matrix size (2 - 10): ");
    if (scanf("%hu", n) != 1 || *n < 2 || *n > 10) {
        while (getchar() != '\n');
        return INPUT_INVALID;
    }
    return INPUT_VALID;
}


void printMatrix(int n, unsigned short *matrix, const char *message) {
    printf("\n%s\n", message);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%3hu ", *(matrix + i * n + j));
        }
        printf("\n");
    }
}

int main() {
    unsigned short n;
    InputStatus status;

    do {
        status = readMatrixSize(&n);
        if (status == INPUT_INVALID)
            printf("Invalid input! Please enter an integer between 2 and 10.\n");
    } while (status != INPUT_VALID);

    // Dynamically allocate 2D array
    unsigned short *matrix = malloc(n * n * sizeof(unsigned short));
    if (!matrix) {
        printf("Memory allocation is  failed!\n");
        return 1;
    }


    srand(time(0));

   
    for (int i = 0; i < n * n; i++){
        *(matrix + i) = rand() % 256;
    }



    printMatrix(n, matrix, "Original Randomly Generated Matrix:");
    rotateMatrix(n, matrix);
    printMatrix(n, matrix, "Matrix after 90 degree Clockwise Rotation:");
    smoothing(n, matrix);
    printMatrix(n, matrix, "Matrix after Applying 3 * 3 Smoothing Filter:");

    free(matrix);

    return 0;
}
