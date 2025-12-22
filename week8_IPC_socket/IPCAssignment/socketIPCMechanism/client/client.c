#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

#define WITHDRAW 1
#define DEPOSIT 2
#define BALANCE 3
#define EXIT 4

void displayMenu(){
    printf("\n============================================\n");
    printf("ATM Transaction Menu");
    printf("\n============================================\n");
    printf("1. Withdraw Amount\n");
    printf("2. Deposit Amount\n");
    printf("3. Display Balance\n");
    printf("4. Exit\n");
    printf("============================================\n");
    printf("Enter your choice: ");
}

int getValidChoice(){
    int choice;
    while(1){
        if(scanf("%d", &choice) != 1){
            printf("Invalid input. Please enter again: ");
            while(getchar() != '\n');
        } 
        else{
            if(getchar() != '\n'){
                printf("Invalid input. Please enter again: ");
                while(getchar() != '\n');
            } 
            else if(choice < 1 || choice > 4){
                printf("Invalid choice. Enter 1-4: ");
            } 
            else{
                return choice;
            }
        }
    }
}

float getValidAmount(){
    float amount;
    while(1){
        if(scanf("%f", &amount) != 1){
            printf("Invalid input. Please enter again: ");
            while(getchar() != '\n');
        } 
        else{
            if(getchar() != '\n'){
                printf("Invalid input. Please enter again: ");
                while(getchar() != '\n');
            } 
            else if(amount <= 0){
                printf("Amount must be positive. Enter again: ");
            } 
            else{
                return amount;
            }
        }
    }
}

void prepareWithdrawRequest(char* request){
    printf("Enter amount to withdraw: ");
    float amount = getValidAmount();
    snprintf(request, BUFFER_SIZE, "%d %.2f", WITHDRAW, amount);
}

void prepareDepositRequest(char* request){
    printf("Enter amount to deposit: ");
    float amount = getValidAmount();
    snprintf(request, BUFFER_SIZE, "%d %.2f", DEPOSIT, amount);
}

void prepareBalanceRequest(char* request){
    snprintf(request, BUFFER_SIZE, "%d 0", BALANCE);
}

void prepareExitRequest(char* request){
    snprintf(request, BUFFER_SIZE, "%d 0", EXIT);
}

int prepareRequest(int choice, char* request){
    if(choice == WITHDRAW){
        prepareWithdrawRequest(request);
    }
    else if(choice == DEPOSIT){
        prepareDepositRequest(request);
    }
    else if(choice == BALANCE){
        prepareBalanceRequest(request);
    }
    else if(choice == EXIT){
        prepareExitRequest(request);
        return 0;
    }
    return 1;
}

void sendRequest(int clientSocket, char* request){
    send(clientSocket, request, strlen(request), 0);
}

void receiveAndDisplayResponse(int clientSocket){
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    recv(clientSocket, buffer, BUFFER_SIZE, 0);
    
    printf("\n--- Server Response ---\n");
    printf("%s\n", buffer);
    printf("=======================\n");
}

int createClientSocket(){
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(clientSocket < 0){
        printf("Error: Socket creation failed.\n");
        return -1;
    }
    return clientSocket;
}

int connectToServer(int clientSocket){
    struct sockaddr_in serverAddr;
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if(connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0){
        printf("Error: Connection failed. Make sure server is running.\n");
        return 0;
    }
    
    printf("Connected to ATM server.\n");
    return 1;
}

void handleTransactions(int clientSocket){
    char request[BUFFER_SIZE];
    
    while(1){
        displayMenu();
        int choice = getValidChoice();
        
        if(!prepareRequest(choice, request)){
            sendRequest(clientSocket, request);
            printf("\nThank you for using our ATM service!\n");
            printf("============================================\n");
            break;
        }
        
        sendRequest(clientSocket, request);
        receiveAndDisplayResponse(clientSocket);
    }
}

void initiateClient(){
    printf("\n============================================\n");
    printf("ATM Client - Socket IPC Mechanism");
    printf("\n============================================\n");
    
    int clientSocket = createClientSocket();
    if(clientSocket == -1) return;
    
    if(!connectToServer(clientSocket)){
        close(clientSocket);
        return;
    }
    
    handleTransactions(clientSocket);
    
    close(clientSocket);
}

int main(){
    initiateClient();
    return 0;
}