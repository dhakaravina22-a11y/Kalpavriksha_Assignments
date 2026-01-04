#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_SIZE 100
#define SHM_KEY 5678

typedef struct{
    int size;
    int data[MAX_SIZE];
    int ready;
} SharedData;

int getValidInteger(){
    int number;
    while (1){
        if (scanf("%d", &number) != 1){
            printf("Invalid input. Please enter again: ");
            while (getchar() != '\n');
        }else{
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

SharedData *createAndAttachSharedMemory(int *shmid)
{
    key_t key = ftok(".", SHM_KEY);
    *shmid = shmget(key, sizeof(SharedData), 0666 | IPC_CREAT);
    if (*shmid == -1)
    {
        printf("Shared memory creation failed.\n");
        return NULL;
    }
    SharedData *sharedData = (SharedData *)shmat(*shmid, NULL, 0);
    if (sharedData == (void *)-1)
    {
        printf("Shared memory attachment failed.\n");
        return NULL;
    }
    return sharedData;
}

void writeToSharedMemory(SharedData *sharedData, int *arr, int size)
{
    sharedData->size = size;
    for (int i = 0; i < size; i++)
    {
        sharedData->data[i] = arr[i];
    }
    sharedData->ready = 1;
}

void waitForDataReady(SharedData *sharedData, int expectedState)
{
    while (sharedData->ready != expectedState)
    {
        usleep(1000);
    }
}

void childProcess(SharedData *sharedData)
{
    printf("\n--- Process 2 - Child ---\n");

    waitForDataReady(sharedData, 1);

    printf("Child process sorting the array...\n");
    sortArray(sharedData->data, sharedData->size);

    sharedData->ready = 2;
}

void cleanupSharedMemory(SharedData *sharedData, int shmid){
    if (shmdt(sharedData) == -1) {
        perror("shmdt failed");
    }

    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID failed");
    }
}

void initiateSharedMemoryIPC()
{
    int array[MAX_SIZE];
    int size;
    int shmid;

    printf("\n============================================\n");
    printf("Shared Memory IPC Mechanism");
    printf("\n============================================\n");

    size = readInputArray(array);
    if (size == -1)
        return;

    SharedData *sharedData = createAndAttachSharedMemory(&shmid);
    if (sharedData == NULL)
        return;

    printf("\n--- Process 1 - Parent ---\n");
    printf("Array before sorting: ");
    displayArray(array, size);

    writeToSharedMemory(sharedData, array, size);

    pid_t pid = fork();

    if (pid < 0)
    {
        printf("Fork failed.\n");
        cleanupSharedMemory(sharedData, shmid);
        return;
    }
    else if (pid == 0)
    {
        childProcess(sharedData);
        shmdt(sharedData);
        exit(0);
    }
    else
    {
        wait(NULL);

        printf("\n--- Process 1 - Parent ---\n");
        waitForDataReady(sharedData, 2);
        printf("Array after sorting: ");
        displayArray(sharedData->data, sharedData->size);

        cleanupSharedMemory(sharedData, shmid);
    }

    printf("\n============================================\n");
    printf("Shared Memory IPC Completed Successfully");
    printf("\n============================================\n");
}

int main()
{
    initiateSharedMemoryIPC();
    return 0;
}