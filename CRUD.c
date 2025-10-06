#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#define FILE_NAME "users.txt"

struct User {
    unsigned int id;
    char name[100];
    unsigned short age;
};

void trimNewline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
}

// Helper function to get valid integer input from user using fgets
int getIntegerInput(const char *prompt) {
    char buffer[100];
    int value;
    while (1) {
        printf("%s", prompt);
        if (fgets(buffer, sizeof(buffer), stdin)) {
            buffer[strcspn(buffer, "\n")] = '\0'; // remove newline

            char *endptr;
            value = strtol(buffer, &endptr, 10);

            if (endptr != buffer && *endptr == '\0') {
                return value;
            } else {
                printf("Please enter valid number. Please try again.\n");
            }
        } else {
            printf("Error reading input. Try again.\n");
            clearerr(stdin);
        }
    }
}

// Helper function to get string input containing only alphabets
void getStringInput(const char *prompt, char *buffer, size_t size) {
    int valid;
    do {
        valid = 1; // first lets assume its valid
        printf("%s", prompt);

        if (!fgets(buffer, size, stdin)) {
            printf("Error reading input.\n");
            clearerr(stdin);
            continue;
        }

        buffer[strcspn(buffer, "\n")] = '\0'; 

        // Empty check
        if (strlen(buffer) == 0) {
            printf("Input cannot be empty...write your name.\n");
            valid = 0;
            continue;
        }

        // Check if every character is alphabetic or space
        for (size_t i = 0; i < strlen(buffer); i++) {
            if (!isalpha((unsigned char)buffer[i]) && !isspace((unsigned char)buffer[i])) {
                printf("Invalid Name. Only alphabets and spaces are allowed.\n");
                valid = 0;
                break;
            }
        }

    } while (!valid);
}


//create new user 
void createUser() {
    FILE *fp = fopen(FILE_NAME, "a+");
    if (!fp) {
        printf("Error opening file\n");
        return;
    }
    struct User user;
    user.id = getIntegerInput("Enter your ID: ");
    getStringInput("Enter your Name: ", user.name, sizeof(user.name));
    user.age = getIntegerInput("Enter your Age: ");
    
    struct User temp;
    rewind(fp);
    while (fscanf(fp, "%d,%99[^,],%d\n", &temp.id, temp.name, &temp.age) == 3) {
        if (temp.id == user.id) {
            printf("User with ID %d already exist\n", user.id);
            fclose(fp);
            return;
        }
    }

    fprintf(fp, "%d,%s,%d\n", user.id, user.name, user.age);
    fclose(fp);
    printf("User added successfully\n");
}

// read all data from users.txt and print them to console

void readUsers() {
    FILE *fp = fopen(FILE_NAME, "r");
    if (!fp) {
        printf("No user found.\n");
        return;
    }

    struct User user;
    printf("\n----User Records ---\n");
    while (fscanf(fp, "%d,%99[^,],%d\n", &user.id, user.name, &user.age) == 3) {
        printf("ID: %d, Name: %s, Age: %d\n", user.id, user.name, user.age);
    }
    fclose(fp);
}


void updateUser() {
    FILE *fp = fopen(FILE_NAME, "r");
    if (!fp) {
        printf("No users found.\n");
        return;
    }

    struct User *users = NULL;
    int count = 0;

    // first...read all users into memory dynamically
    struct User temp;
    while (fscanf(fp, "%d,%99[^,],%hu\n", &temp.id, temp.name, &temp.age) == 3) {
        struct User *newBlock = realloc(users, (count + 1) * sizeof(struct User));
        if (!newBlock) {
            printf("Memory allocation failed.\n");
            free(users);
            fclose(fp);
            return;
        }
        users = newBlock;
        users[count++] = temp;
    }
    fclose(fp);

    if (count == 0) {
        printf("No records found.\n");
        free(users);
        return;
    }

    // Second - ask user for ID to update
    int id = getIntegerInput("Enter ID to update: ");
    int found = 0;

    // Third - modify record in memory struct memory
    for (int i = 0; i < count; i++) {
        if (users[i].id == id) {
            found = 1;
            getStringInput("Enter new name: ", users[i].name, sizeof(users[i].name));
            users[i].age = getIntegerInput("Enter new age: ");
            break;
        }
    }

    if (!found) {
        printf("No user found with ID %d\n", id);
        free(users);
        return;
    }

    // 4- Rewrite the file into users.txt  with updated records
    fp = fopen(FILE_NAME, "w");
    if (!fp) {
        printf("Error rewriting file.\n");
        free(users);
        return;
    }

    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d,%s,%hu\n", users[i].id, users[i].name, users[i].age);
    }
    fclose(fp);
    free(users);

    printf("User updated successfully.\n");
}


void deleteUser() {
    FILE *fp = fopen(FILE_NAME, "r");
    if (!fp) {
        printf("No users found.\n");
        return;
    }

    struct User *users = NULL;
    int count = 0;


    struct User temp;
    while (fscanf(fp, "%d,%99[^,],%hu\n", &temp.id, temp.name, &temp.age) == 3) {
        struct User *newBlock = realloc(users, (count + 1) * sizeof(struct User));
        if (!newBlock) {
            printf("Memory allocation failed.\n");
            free(users);
            fclose(fp);
            return;
        }
        users = newBlock;
        users[count++] = temp;
    }
    fclose(fp);

    if (count == 0) {
        printf("No users to delete.\n");
        free(users);
        return;
    }

    int id = getIntegerInput("Enter ID to delete: ");
    int found = 0;

    for (int i = 0; i < count; i++) {
        if (users[i].id == id) {
            found = 1;
            for (int j = i; j < count - 1; j++) {
                users[j] = users[j + 1];
            }
            count--;
            break;
        }
    }

    if (!found) {
        printf("No user found with ID %d\n", id);
        free(users);
        return;
    }

    fp = fopen(FILE_NAME, "w");
    if (!fp) {
        printf("Error rewriting file.\n");
        free(users);
        return;
    }

    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d,%s,%hu\n", users[i].id, users[i].name, users[i].age);
    }

    fclose(fp);
    free(users);

    printf("User deleted successfully.\n");
}

int main() {
    int choice;
    while (1) {
        printf("\n -------User Management System-----\n");
        printf("1-Create User\n");
        printf("2-Read Users\n");
        printf("3-Update User\n");
        printf("4-Delete User\n");
        printf("5-Exit\n");
        choice = getIntegerInput("Enter your choice 1,2,3,4,5 : ");

        switch (choice) {
            case 1:
                createUser();
                break;
            case 2:
                readUsers();
                break;
            case 3:
                updateUser();
                break;
            case 4:
                deleteUser();
                break;
            case 5:
                printf("exit from code \n");
                exit(0);
            default:
                printf("write valid choice\n");
        }
    }
    return 0;
}

