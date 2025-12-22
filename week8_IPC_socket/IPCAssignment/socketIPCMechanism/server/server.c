#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define ACCOUNT_FILE "resource/accountDB.txt"

#define WITHDRAW 1
#define DEPOSIT 2
#define BALANCE 3
#define EXIT 4

pthread_mutex_t fileMutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct
{
    int clientSocket;
    int clientNumber;
} ClientInfo;

float readBalance()
{
    FILE *file = fopen(ACCOUNT_FILE, "r");
    if (!file)
    {
        printf("Error: Cannot open account file. Creating new file with balance 10000.00\n");
        file = fopen(ACCOUNT_FILE, "w");
        if (file)
        {
            fprintf(file, "10000.00");
            fclose(file);
        }
        return 10000.00;
    }
    float balance;
    fscanf(file, "%f", &balance);
    fclose(file);
    return balance;
}

void writeBalance(float balance)
{
    FILE *file = fopen(ACCOUNT_FILE, "w");
    if (!file)
    {
        printf("Error: Cannot write to account file.\n");
        return;
    }
    fprintf(file, "%.2f", balance);
    fclose(file);
}

void handleWithdraw(int clientSocket, float amount)
{
    char response[BUFFER_SIZE];

    pthread_mutex_lock(&fileMutex);

    float currentBalance = readBalance();

    if (amount > currentBalance)
    {
        snprintf(response, BUFFER_SIZE,
                 "FAILED: Insufficient balance. Current balance: %.2f",
                 currentBalance);
    }
    else if (amount <= 0)
    {
        snprintf(response, BUFFER_SIZE, "FAILED: Invalid amount.");
    }
    else
    {
        float newBalance = currentBalance - amount;
        writeBalance(newBalance);
        snprintf(response, BUFFER_SIZE,
                 "SUCCESS: Withdrawn %.2f. New balance: %.2f",
                 amount, newBalance);
    }

    pthread_mutex_unlock(&fileMutex);

    send(clientSocket, response, strlen(response), 0);
}

void handleDeposit(int clientSocket, float amount)
{
    char response[BUFFER_SIZE];

    pthread_mutex_lock(&fileMutex);

    if (amount <= 0)
    {
        snprintf(response, BUFFER_SIZE, "FAILED: Invalid amount.");
    }
    else
    {
        float currentBalance = readBalance();
        float newBalance = currentBalance + amount;
        writeBalance(newBalance);
        snprintf(response, BUFFER_SIZE,
                 "SUCCESS: Deposited %.2f. New balance: %.2f",
                 amount, newBalance);
    }

    pthread_mutex_unlock(&fileMutex);

    send(clientSocket, response, strlen(response), 0);
}

void handleBalance(int clientSocket)
{
    char response[BUFFER_SIZE];

    pthread_mutex_lock(&fileMutex);

    float currentBalance = readBalance();
    snprintf(response, BUFFER_SIZE, "Current balance: %.2f", currentBalance);

    pthread_mutex_unlock(&fileMutex);

    send(clientSocket, response, strlen(response), 0);
}

int processClientRequest(int clientSocket, int clientNumber, char *buffer)
{
    int operation;
    float amount;

    sscanf(buffer, "%d %f", &operation, &amount);

    printf("[Thread %d] Request: Operation=%d, Amount=%.2f\n",
           clientNumber, operation, amount);

    if (operation == WITHDRAW)
    {
        handleWithdraw(clientSocket, amount);
    }
    else if (operation == DEPOSIT)
    {
        handleDeposit(clientSocket, amount);
    }
    else if (operation == BALANCE)
    {
        handleBalance(clientSocket);
    }
    else if (operation == EXIT)
    {
        printf("[Thread %d] Client requested exit.\n", clientNumber);
        return 0;
    }
    else
    {
        send(clientSocket, "Invalid operation.", 18, 0);
    }

    return 1;
}

void *handleClient(void *arg)
{
    ClientInfo *clientInfo = (ClientInfo *)arg;
    int clientSocket = clientInfo->clientSocket;
    int clientNumber = clientInfo->clientNumber;

    printf("[Thread %d] Client connected.\n", clientNumber);

    char buffer[BUFFER_SIZE];

    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);

        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);

        if (bytesReceived <= 0)
        {
            printf("[Thread %d] Client disconnected.\n", clientNumber);
            break;
        }

        if (!processClientRequest(clientSocket, clientNumber, buffer))
        {
            break;
        }
    }

    close(clientSocket);
    free(clientInfo);
    pthread_exit(NULL);
}

int createServerSocket()
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        printf("Error: Socket creation failed.\n");
        return -1;
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    return serverSocket;
}

int bindServerSocket(int serverSocket)
{
    struct sockaddr_in serverAddr;

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        printf("Error: Binding failed.\n");
        return 0;
    }

    return 1;
}

int startListening(int serverSocket)
{
    if (listen(serverSocket, 5) < 0)
    {
        printf("Error: Listen failed.\n");
        return 0;
    }

    printf("Server started on port %d\n", PORT);
    printf("Waiting for clients...\n\n");

    return 1;
}

void acceptClientConnections(int serverSocket)
{
    struct sockaddr_in clientAddr;
    socklen_t addrSize;
    int clientCounter = 0;

    while (1)
    {
        addrSize = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrSize);

        if (clientSocket < 0)
        {
            printf("Error: Accept failed.\n");
            continue;
        }

        clientCounter++;

        pthread_t thread;
        ClientInfo *clientInfo = (ClientInfo *)malloc(sizeof(ClientInfo));
        clientInfo->clientSocket = clientSocket;
        clientInfo->clientNumber = clientCounter;

        if (pthread_create(&thread, NULL, handleClient, (void *)clientInfo) != 0)
        {
            printf("Error: Thread creation failed.\n");
            close(clientSocket);
            free(clientInfo);
        }
        else
        {
            pthread_detach(thread);
        }
    }
}

void initiateServer()
{
    printf("\n============================================\n");
    printf("ATM Server - Socket IPC Mechanism");
    printf("\n============================================\n");

    int serverSocket = createServerSocket();
    if (serverSocket == -1)
        return;

    if (!bindServerSocket(serverSocket))
    {
        close(serverSocket);
        return;
    }

    if (!startListening(serverSocket))
    {
        close(serverSocket);
        return;
    }

    acceptClientConnections(serverSocket);

    close(serverSocket);
}

int main()
{
    initiateServer();
    return 0;
}