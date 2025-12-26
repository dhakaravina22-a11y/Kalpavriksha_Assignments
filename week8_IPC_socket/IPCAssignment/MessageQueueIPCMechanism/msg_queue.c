#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MAX_SIZE 100
#define MSG_KEY 1234
#define MSG_TYPE_TO_CHILD 1
#define MSG_TYPE_TO_PARENT 2

typedef struct{
    long msgType;
    int size;
    int data[MAX_SIZE];
} Message;

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

int createMessageQueue(){
    key_t key = ftok(".", MSG_KEY);
    int msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1){
        printf("Message queue creation failed.\n");
        return -1;
    }
    return msgid;
}

void sendMessage(int msgid, Message *msg){
    msgsnd(msgid, msg, sizeof(Message) - sizeof(long), 0);
}

void receiveMessage(int msgid, Message *msg, long msgType){
    msgrcv(msgid, msg, sizeof(Message) - sizeof(long), msgType, 0);
}

void prepareMessage(Message *msg, int *arr, int size, long msgType){
    msg->msgType = msgType;
    msg->size = size;
    for (int i = 0; i < size; i++){
        msg->data[i] = arr[i];
    }
}

void childProcess(int msgid){
    printf("\n--- Process 2 - Child ---\n");

    Message msg;
    receiveMessage(msgid, &msg, MSG_TYPE_TO_CHILD);

    printf("Child process sorting the array...\n");
    sortArray(msg.data, msg.size);

    msg.msgType = MSG_TYPE_TO_PARENT;
    sendMessage(msgid, &msg);
}

void cleanupMessageQueue(int msgid){
    msgctl(msgid, IPC_RMID, NULL);
}

void initiateMessageQueueIPC(){
    int array[MAX_SIZE];
    int size;

    printf("\n============================================\n");
    printf("Message Queue IPC Mechanism");
    printf("\n============================================\n");

    size = readInputArray(array);
    if (size == -1)
        return;

    int msgid = createMessageQueue();
    if (msgid == -1)
        return;

    printf("\n--- Process 1 - Parent ---\n");
    printf("Array before sorting: ");
    displayArray(array, size);

    Message msg;
    prepareMessage(&msg, array, size, MSG_TYPE_TO_CHILD);
    sendMessage(msgid, &msg);

    pid_t pid = fork();

    if (pid < 0){
        printf("Fork failed.\n");
        cleanupMessageQueue(msgid);
        return;
    }else if (pid == 0){
        childProcess(msgid);
        exit(0);
    }else{
        wait(NULL);

        printf("\n--- Process 1 - Parent ---\n");
        receiveMessage(msgid, &msg, MSG_TYPE_TO_PARENT);
        printf("Array after sorting: ");
        displayArray(msg.data, msg.size);

        cleanupMessageQueue(msgid);
    }

    printf("\n============================================\n");
    printf("Message Queue IPC Completed Successfully");
    printf("\n============================================\n");
}

int main(){
    initiateMessageQueueIPC();
    return 0;
}