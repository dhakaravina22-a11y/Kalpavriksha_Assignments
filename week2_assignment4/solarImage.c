#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>


typedef enum {
    INPUT_VALID,
    INPUT_INVALID
} InputStatus;

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

void rotateMatrix(int n, int (*matrix)[n]) {
    // In-place rotation (90° clockwise)
    // Step 1: Transpose (swap elements across diagonal)
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            int *p1 = *matrix + i * n + j;
            int *p2 = *matrix + j * n + i;
            swap(p1, p2);
        }
    }

    // Step 2: Reverse each row (swap columns)
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n / 2; j++) {
            int *p1 = *matrix + i * n + j;
            int *p2 = *matrix + i * n + (n - j - 1);
            swap(p1, p2);
        }
    }
}

void smoothing(int n, int (*matrix)[n]) {
    int dx[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dy[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

    // First pass: store both old + new values in same cell using bit manipulation
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int *curr = *matrix + i * n + j;
            int sum = *curr & 0xFF;
            int count = 1;

            for (int k = 0; k < 8; k++) {
                int ni = i + dx[k];
                int nj = j + dy[k];
                if (ni >= 0 && ni < n && nj >= 0 && nj < n) {
                    int *neighbor = *matrix + ni * n + nj;
                    sum += *neighbor & 0xFF;
                    count++;
                }
            }
            int avg = sum / count;
            *curr |= (avg << 8);  // store new avg in upper 8 bits
        }
    }

    // Second pass: extract only the new (smoothed) values
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int *curr = *matrix + i * n + j;
            *curr >>= 8;
        }
    }
}

InputStatus readMatrixSize(unsigned short *n) {
    int result;

    printf("Enter matrix size (2 - 10): ");
    result = scanf("%hu", n);

    if (result != 1) {
        // invalid input (non-numeric)
        while (getchar() != '\n'); // clear input buffer
        return INPUT_INVALID;
    }

    if (*n < 2 || *n > 10) {
        while (getchar() != '\n'); // clear buffer
        return INPUT_INVALID;
    }

    return INPUT_VALID;
}

int main() {
   unsigned short n;
    InputStatus status;

    do {
        status = readMatrixSize(&n);
        if (status == INPUT_INVALID) {
            printf("Invalid input! Please enter an integer between 2 and 10.\n");
        }
    } while (status != INPUT_VALID);

    int matrix[n][n];
    srand(time(0));

    printf("\nOriginal Randomly Generated Matrix:\n");
    for (short unsigned i = 0; i < n; i++) {
        for (short unsigned j = 0; j < n; j++) {
            *(*matrix + i * n + j) = rand() % 256;
            printf("%3d ", *(*matrix + i * n + j));
        }
        printf("\n");
    }

    rotateMatrix(n, matrix);
    printf("\nMatrix after 90° Clockwise Rotation:\n");
    for (short unsigned i = 0; i < n; i++) {
        for (short unsigned j = 0; j < n; j++) {
            printf("%3d ", *(*matrix + i * n + j));
        }
        printf("\n");
    }

    smoothing(n, matrix);
    printf("\nMatrix after Applying 3×3 Smoothing Filter:\n");
    for (short unsigned i = 0; i < n; i++) {
        for (short unsigned j = 0; j < n; j++) {
            printf("%3d ", *(*matrix + i * n + j));
        }
        printf("\n");
    }

    return 0;
}
