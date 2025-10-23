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

void rotateMatrix(int n, int **matrix) {
    // Transpose
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            swap(&matrix[i][j], &matrix[j][i]);
        }
    }
    // Reverse rows
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n / 2; j++) {
            swap(&matrix[i][j], &matrix[i][n - j - 1]);
        }
    }
}

void soothing(int n, int **matrix) {
    int dx[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dy[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

    
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
            *curr |= (avg << 8); 
        }
    }

    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int *curr = *matrix + i * n + j;
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
void printMatrix(int n, int **matrix, const char *message) {
    printf("\n%s\n", message);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%3d ", matrix[i][j]);
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
    int **matrix = malloc(n * sizeof(int*));
    for (int i = 0; i < n; i++)
        matrix[i] = malloc(n * sizeof(int));

    srand(time(0));
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i][j] = rand() % 256; 
        }
    }


    printMatrix(n, matrix, "Original Randomly Generated Matrix:");
    rotateMatrix(n, matrix);
    printMatrix(n, matrix, "Matrix after 90 degree Clockwise Rotation:");
    soothing(n, matrix);
    printMatrix(n, matrix, "Matrix after Applying 3 * 3 Soothing Filter:");

    for (int i = 0; i < n; i++)
        free(matrix[i]);
    free(matrix);

    return 0;
}
