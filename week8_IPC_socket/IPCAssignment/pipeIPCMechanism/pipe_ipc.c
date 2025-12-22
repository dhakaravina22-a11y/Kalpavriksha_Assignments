#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_SIZE 100

int getValidInteger()
{
    int number;
    while (1)
    {
        if (scanf("%d", &number) != 1)
        {
            printf("Invalid input. Please enter again: ");
            while (getchar() != '\n');
        }
        else
        {
            if (getchar() != '\n')
            {
                printf("Invalid input. Please enter again: ");
                while (getchar() != '\n');
            }
            else
            {
                return number;
            }
        }
    }
}

int readInputArray(int *arr)
{
    int size;
    printf("Enter number of elements: ");
    size = getValidInteger();
    if (size <= 0 || size > MAX_SIZE)
    {
        printf("Invalid size. Please enter between 1 and %d\n", MAX_SIZE);
        return -1;
    }
    printf("Enter %d elements:\n", size);
    for (int i = 0; i < size; i++)
    {
        arr[i] = getValidInteger();
    }
    return size;
}

void displayArray(int *arr, int size)
{
    for (int i = 0; i < size; i++)
    {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

void sortArray(int *arr, int size)
{
    for (int i = 0; i < size - 1; i++)
    {
        for (int j = 0; j < size - i - 1; j++)
        {
            if (arr[j] > arr[j + 1])
            {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

void sendDataThroughPipe(int writeFd, int *arr, int size)
{
    write(writeFd, &size, sizeof(int));
    write(writeFd, arr, size * sizeof(int));
}

int receiveDataThroughPipe(int readFd, int *arr)
{
    int size;
    read(readFd, &size, sizeof(int));
    read(readFd, arr, size * sizeof(int));
    return size;
}

void parentProcess(int *array, int size, int *pipe1, int *pipe2)
{
    printf("\n--- Process 1 - Parent ---\n");
    printf("Array before sorting: ");
    displayArray(array, size);
    close(pipe1[0]);
    close(pipe2[1]);
    sendDataThroughPipe(pipe1[1], array, size);
    close(pipe1[1]);
}

void childProcess(int *pipe1, int *pipe2)
{
    printf("\n--- Process 2 - Child ---\n");

    close(pipe1[1]);
    close(pipe2[0]);
    int childArray[MAX_SIZE];
    int childSize = receiveDataThroughPipe(pipe1[0], childArray);
    close(pipe1[0]);

    printf("Child process sorting the array...\n");
    sortArray(childArray, childSize);
    sendDataThroughPipe(pipe2[1], childArray, childSize);
    close(pipe2[1]);
}

int createPipes(int *pipe1, int *pipe2)
{
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1)
    {
        printf("Pipe creation failed.\n");
        return 0;
    }
    return 1;
}

void initiatePipeIPC()
{
    int array[MAX_SIZE];
    int size;
    int pipe1[2], pipe2[2];

    printf("\n============================================\n");
    printf("Pipe-Based IPC Mechanism");
    printf("\n============================================\n");
    size = readInputArray(array);
    if (size == -1)
        return;

    if (!createPipes(pipe1, pipe2))
        return;

    pid_t pid = fork();

    if (pid < 0)
    {
        printf("Fork failed.\n");
        return;
    }
    else if (pid == 0)
    {
        childProcess(pipe1, pipe2);
        exit(0);
    }
    else
    {
        parentProcess(array, size, pipe1, pipe2);
        wait(NULL);
        printf("\n--- Process 1 - Parent ---\n");
        int sortedArray[MAX_SIZE];
        int sortedSize = receiveDataThroughPipe(pipe2[0], sortedArray);
        close(pipe2[0]);
        printf("Array after sorting: ");
        displayArray(sortedArray, sortedSize);
    }

    printf("\n============================================\n");
    printf("Pipe-Based IPC Completed Successfully");
    printf("\n============================================\n");
}

int main()
{
    initiatePipeIPC();
    return 0;
}