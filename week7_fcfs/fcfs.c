#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>

#define SIZE 100
#define HASH_SIZE 11

typedef enum ProcessState {
    READY,
    RUNNING,
    WAITING,
    TERMINATED,
    KILLED
} ProcessState;

typedef struct ProcessControlBlock {
    char name[SIZE+1];
    int burstTime;
    int remBurstTime;
    int ioStartTime;
    int ioDuration;
    int ioRemaining;
    int cpuExecuted;
    ProcessState state; 
    int turnAroundTime;
    int waitingTime;
    bool ioStartedThisTick;
} ProcessControlBlock;

typedef struct HashNode {
    int key;
    ProcessControlBlock* value;
    struct HashNode* next;
} HashNode;

typedef struct HashMap {
    HashNode* buckets[HASH_SIZE];
} HashMap;

typedef struct Node {
    int processId;
    struct Node* next;
} Node;

typedef struct Queue {
    Node* front;
    Node* rear;
} Queue;

typedef struct KillEvent {
    int processId;
    int time;
} KillEvent;

HashMap* createHashMap() {
    HashMap* temp = malloc(sizeof(HashMap));
    if (!temp){
        printf("Memory Allocation Failed!\n");
        exit(1);
    }

    for (int index = 0; index < HASH_SIZE; index++){
        temp->buckets[index] = NULL;
    }

    return temp;
}

HashNode* createHashNode(int key, ProcessControlBlock* pcb) {
    HashNode* newNode = malloc(sizeof(HashNode));
    if (!newNode){
        printf("Memory Allocation Failed!\n");
        exit(1);
    }

    newNode->key = key;
    newNode->value = pcb;
    newNode->next = NULL;

    return newNode;
}

int hash(int key) {
    return (key % HASH_SIZE + HASH_SIZE) % HASH_SIZE;
}

void insertIntoHashMap(HashMap* hashMap, int key, ProcessControlBlock* pcb) {
    if (!hashMap || !pcb){
        return;
    }

    int index = hash(key);

    HashNode* newNode = createHashNode(key, pcb);
    newNode->next = hashMap->buckets[index];
    hashMap->buckets[index] = newNode;
}

ProcessControlBlock* getFromHashMap(HashMap* hashMap, int key) {
    if (!hashMap){
        return NULL;
    }

    int index = hash(key);
    HashNode* temp = hashMap->buckets[index];
    while(temp){
        if (temp->key == key){
            return temp->value;
        }
        temp = temp->next;
    }

    return NULL;
}

void freeHashMap(HashMap* hashMap) {
    if (!hashMap){
        return;
    }

    for (int index = 0; index < HASH_SIZE; index++){
        HashNode* temp = hashMap->buckets[index];
        while (temp){
            HashNode* toDelete = temp;
            temp = temp->next;
            free(toDelete->value);
            free(toDelete);
        }
    }
    free(hashMap);
}


Queue* createQueue() {
    Queue* queue = malloc(sizeof(Queue));
    if (!queue){
        printf("Memory Allocation Failed!\n");
        exit(1);
    }
    queue->front = queue->rear = NULL;
    return queue;
}

Node* createQueueNode(int processId) {
    Node* newNode = malloc(sizeof(Node));
    if (!newNode){
        return NULL;
    }

    newNode->processId = processId;
    newNode->next = NULL;

    return newNode;
}

void enqueue(Queue* queue, int processId) {
    if (!queue){
        return;
    }

    Node* newNode = createQueueNode(processId);
    if (!newNode) {
        printf("ERROR: Failed to allocate queue node\n");
        return;  
    }
    if (!queue->front){
        queue->front = queue->rear = newNode;
        return;
    }

    queue->rear->next = newNode;
    queue->rear = newNode;
}

int dequeue(Queue* queue) {
    if (!queue || !queue->front){
        printf("Empty Queue\n");
        return -1;
    }

    Node* toDelete  = queue->front;
    int processId = toDelete->processId;

    if (queue->front == queue->rear){
        queue->rear = NULL;
    }
    queue->front = queue->front->next;
    free(toDelete);

    return processId;
}

bool removeFromQueue(Queue* queue, int processId) {
    if (!queue || !queue->front){
        return false;
    }

    Node* curr = queue->front;
    Node* prev = NULL;

    while (curr) {
        if (curr->processId == processId){
            if (!prev){
                queue->front = curr->next;
            }else{
                prev->next = curr->next;
            }

            if (queue->rear == curr){
                queue->rear = prev;
            }

            free(curr);
            return true;
        }
        prev = curr;
        curr = curr->next;
    }

    return false;
}


bool isQueueEmpty(Queue* queue) {
    return !queue || !queue->front;
}

void freeQueue(Queue* queue) {
    if (!queue){
        return;
    }

    while (queue->front){
        dequeue(queue);
    }

    free(queue);
}

KillEvent* createKillEvents(int killCount) {
    KillEvent* kills = malloc(killCount * sizeof(KillEvent));
    if (!kills){
        printf("Memory Allocation Failed\n");
        exit(1);
    }

    for (int index = 0; index < killCount; index++){
        kills[index].processId = -1;
        kills[index].time = -1;
    }

    return kills;
}

void moveToWaiting(ProcessControlBlock* pcb, int processId, Queue* waitingQueue) {
    pcb->state = WAITING;
    pcb->ioRemaining = pcb->ioDuration;
    pcb->ioStartedThisTick = true;
    enqueue(waitingQueue, processId);
}

void moveToTerminated(ProcessControlBlock* pcb, int processId, int clock, Queue* terminatedQueue) {
    pcb->turnAroundTime = clock;
    pcb->waitingTime = pcb->turnAroundTime - pcb->burstTime;
    pcb->state = TERMINATED;
    enqueue(terminatedQueue, processId);
}

void killProcess(ProcessControlBlock* pcb, int processId, int clock, Queue* terminatedQueue) {
    pcb->turnAroundTime = -1;
    pcb->waitingTime = -1;
    pcb->state = KILLED;
    enqueue(terminatedQueue, processId);
}

void updateWaitingProcesses(Queue* waitingQueue, Queue* readyQueue, HashMap* hashMap) {
    Node* curr = waitingQueue->front;
    Node* prev = NULL;

    while (curr){
        int processId = curr->processId;
        ProcessControlBlock* pcb = getFromHashMap(hashMap, processId);

        if (!pcb){
            prev = curr;
            curr = curr->next;
            continue;
        }

        if (pcb->ioStartedThisTick){
            pcb->ioStartedThisTick = false;
            prev = curr;
            curr = curr->next;
            continue;
        }

        pcb->ioRemaining--;

        if (pcb->ioRemaining <= 0){
            Node* toRemove = curr;

            if (!prev){
                waitingQueue->front = curr->next;
            }else{
                prev->next = curr->next;
            }

            if (waitingQueue->rear == curr){
                waitingQueue->rear = prev;
            }

            curr = curr->next;
            free(toRemove);

            pcb->state = READY;
            enqueue(readyQueue, processId);
        }else{
            prev = curr;
            curr = curr->next;
        }
    }
}

void applyKillEvents(KillEvent* kills, int killCount, int clock, HashMap* hashMap, Queue* readyQueue, Queue* waitingQueue, Queue* terminatedQueue,
int* runningProcessId,  int* terminatedCount) {
    for (int index = 0; index < killCount; index++){
        if (kills[index].time != clock){
            continue;
        }

        int processId = kills[index].processId;
        ProcessControlBlock* pcb = getFromHashMap(hashMap, processId);
        if (!pcb || pcb->state == TERMINATED || pcb->state == KILLED) {
            continue;
        }

        if (*runningProcessId == processId){
            killProcess(pcb, processId, clock, terminatedQueue);
            (*terminatedCount)++;
            *runningProcessId = -1;
            continue;
        }

        if (removeFromQueue(readyQueue, processId) || removeFromQueue(waitingQueue, processId)){
            killProcess(pcb, processId, clock, terminatedQueue);
            (*terminatedCount)++;
        }

    }
}

void printResults(HashMap* hashMap) {
    printf("\n%-8s%-15s%-10s%-10s%-15s%-12s%-10s\n","PID", "Name", "CPU", "IO", "Turnaround", "Waiting", "Status");
    printf("--------------------------------------------------------------------------------\n");
    
    for (int index = 0; index < HASH_SIZE; index++) {
        HashNode* node = hashMap->buckets[index];
        while (node) 
        {
            ProcessControlBlock* pcb = node->value;
            const char* status = (pcb->state == KILLED) ? "KILLED" : "OK";
            
            printf("%-8d%-15s%-10d%-10d%-15d%-12d%-10s\n",
                   node->key, pcb->name, pcb->burstTime, pcb->ioDuration, pcb->turnAroundTime, pcb->waitingTime, status);
            
            node = node->next;
        }
    }
}

void fcfsScheduler(HashMap* hashMap, Queue* readyQueue, int totalProcesses, KillEvent* kills, int killCount) {
    Queue* waitingQueue = createQueue();
    Queue* terminatedQueue = createQueue();

    int clock = 0;
    int runningProcessId = -1;
    int terminatedProcesses = 0;

    while (terminatedProcesses < totalProcesses){
        applyKillEvents(kills, killCount, clock, hashMap, readyQueue, waitingQueue, terminatedQueue, &runningProcessId, &terminatedProcesses);

        if (runningProcessId == -1 && !isQueueEmpty(readyQueue)){
            runningProcessId = dequeue(readyQueue);
            ProcessControlBlock* pcb = getFromHashMap(hashMap, runningProcessId);
            if (pcb){
                pcb->state = RUNNING;
            }
        }

        if (runningProcessId != -1){
            ProcessControlBlock* pcb = getFromHashMap(hashMap, runningProcessId);

            if (pcb){
                pcb->cpuExecuted++;
                pcb->remBurstTime--;

                if (pcb->ioStartTime >= 0 && pcb->cpuExecuted == pcb->ioStartTime && pcb->remBurstTime > 0){
                    moveToWaiting(pcb, runningProcessId, waitingQueue);
                    runningProcessId = -1;
                }else if (pcb->remBurstTime <= 0){
                    moveToTerminated(pcb, runningProcessId, clock + 1, terminatedQueue);
                    terminatedProcesses++;
                    runningProcessId = -1;
                }
            }
        }

        updateWaitingProcesses(waitingQueue, readyQueue, hashMap);

        clock++;
    }

    printResults(hashMap);

    freeQueue(waitingQueue);
    freeQueue(terminatedQueue);
}

ProcessControlBlock* createProcess() {
    ProcessControlBlock* pcb = malloc(sizeof(ProcessControlBlock));
    if (!pcb){
        printf("Memory Allocation Failed!\n");
        exit(1);
    }

    pcb->cpuExecuted = 0;
    pcb->waitingTime = 0;
    pcb->turnAroundTime = 0;
    pcb->ioRemaining = 0;
    pcb->ioStartedThisTick = false;
    pcb->state = READY;

    return pcb;
}

void readProcessInput(HashMap* hashMap, Queue* readyQueue, int* totalProcesses) {

    printf("Enter number of processes: "); 
    scanf("%d", totalProcesses);

    printf("name  pid  burst  ioStart  ioDuration\n");
    printf("(Use '-' for no I/O)\n\n");

    for (int i = 0; i < *totalProcesses; i++) {

        char name[SIZE];
        char ioStartStr[20], ioDurStr[20];
        int pid, burst;

        scanf("%s %d %d %s %s", name, &pid, &burst, ioStartStr, ioDurStr);

        ProcessControlBlock* pcb = createProcess();
        strcpy(pcb->name, name);

        pcb->burstTime = burst;
        pcb->remBurstTime = burst;

        
        if (strcmp(ioStartStr, "-") == 0 || strcmp(ioDurStr, "-") == 0) {
            pcb->ioStartTime = -1;   
            pcb->ioDuration = 0;
        } else {
            pcb->ioStartTime = atoi(ioStartStr);
            pcb->ioDuration = atoi(ioDurStr);

            if (pcb->ioStartTime < 0 || pcb->ioDuration <= 0) {
                pcb->ioStartTime = -1;
                pcb->ioDuration = 0;
            }
        }

        insertIntoHashMap(hashMap, pid, pcb);
        enqueue(readyQueue, pid);
    }
}


void readKillEvents(KillEvent** kills, int* killCount) {

    printf("\nEnter number of KILL events: ");
    scanf("%d", killCount);

    if (*killCount <= 0) {
        return;
    }

    *kills = createKillEvents(*killCount);
    printf("pid time\n\n");

    for (int i = 0; i < *killCount; i++) {
        scanf("%d %d", &(*kills)[i].processId, &(*kills)[i].time);
    }
}

int main() {
    HashMap* hashMap = createHashMap();
    Queue* readyQueue = createQueue();

    int totalProcesses = 0;
    readProcessInput(hashMap, readyQueue, &totalProcesses);

    KillEvent* kills = NULL;
    int killCount = 0;
    readKillEvents(&kills, &killCount);

    fcfsScheduler(hashMap, readyQueue, totalProcesses, kills, killCount);

    if (kills)
    {
        free(kills);
    }

    freeQueue(readyQueue);
    freeHashMap(hashMap);
}