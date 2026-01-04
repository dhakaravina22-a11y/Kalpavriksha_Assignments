#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_SIZE 100
#define FILENAME "file.txt"

int getValidInteger(){
    int number;
    while (1){
        if (scanf("%d", &number) != 1){
            printf("Invalid input. Please enter again: ");
            while (getchar() != '\n');
        }else{
            if (getchar() != '\n'){
                printf("Invalid input. Please enter again: ");
                while (getchar() != '\n');
            }else{
                return number;
            }
        }
    }
}

int readInputArray(int *arr){
    int size;
    printf("Enter number of elements: ");
    size = getValidInteger();
    if (size <= 0 || size > MAX_SIZE){
        printf("Invalid size. Please enter between 1 and %d\n", MAX_SIZE);
        return -1;
    }
    printf("Enter %d elements:\n", size);
    for (int i = 0; i < size; i++){
        arr[i] = getValidInteger();
    }
    return size;
}

void displayArray(int *arr, int size){
    for (int i = 0; i < size; i++){
        printf("%d ", arr[i]);
    }
    printf("\n");
}

void writeArrayToFile(int *arr, int size)
{
    FILE *file = fopen(FILENAME, "w");
    if (!file)
    {
        printf("Error opening file.\n");
        return;
    }
    fprintf(file, "%d\n", size);
    for (int i = 0; i < size; i++)
    {
        fprintf(file, "%d ", arr[i]);
    }
    fclose(file);
}

int readArrayFromFile(int *arr){
    FILE *file = fopen(FILENAME, "r");
    if (!file){
        printf("Error opening file.\n");
        return 0;
    }
    int size;
    if (fscanf(file, "%d", &size) != 1) {
        fprintf(stderr, "Error: Failed to read size from file\n");
        fclose(file);
        return -1;
    }

    for (int i = 0; i < size; i++){
        if (fscanf(file, "%d", &arr[i]) != 1) {
        fprintf(stderr, "Error: Failed to read element %d from file\n", i);
        fclose(file);
        return -1;  
    }

    }
    fclose(file);
    return size;
}

void sortArray(int *arr, int size){
    for (int i = 0; i < size - 1; i++){
        for (int j = 0; j < size - i - 1; j++){
            if (arr[j] > arr[j + 1]){
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

void parentProcess(int *array, int size){
    printf("\n--- Process 1 - Parent ---\n");
    printf("Array before sorting: ");
    displayArray(array, size);
    writeArrayToFile(array, size);
    printf("Data written to file: %s\n", FILENAME);
}

void childProcess() {
    printf("\n--- Process 2 - Child ---\n");

    int childArray[MAX_SIZE];
    int childSize = readArrayFromFile(childArray);

    if (childSize <= 0) {
        fprintf(stderr, "Error: Failed to read array from file\n");
        return;   
    }

    printf("Array read from file: ");
    displayArray(childArray, childSize);

    sortArray(childArray, childSize);

    printf("Array after sorting: ");
    displayArray(childArray, childSize);
}


void initiateFileIPC(){
    int array[MAX_SIZE];
    int size;

    printf("File-Based IPC Mechanism\n");
    size = readInputArray(array);
    if (size == -1){
        return;
    }

    parentProcess(array, size);

    pid_t pid = fork();

    if (pid < 0){
        printf("Fork failed!..\n");
        return;
    }else if (pid == 0){
        childProcess();
        exit(0);
    }else{
        wait(NULL);
        printf("\n--- Process 1 - Parent ---\n");
        int sortedArray[MAX_SIZE];
        int sortedSize = readArrayFromFile(sortedArray);
        printf("Array after sorting: ");
        displayArray(sortedArray, sortedSize);
        
        //Delete temporary file
        if (remove(FILENAME) != 0) {
            perror("Failed to delete temporary file");
        } else {
            printf("Temporary file deleted successfully\n");
        }
    
    }
    printf("File-Based IPC Completed Successfully");
    
}
int main(){
    initiateFileIPC();
    return 0;
}