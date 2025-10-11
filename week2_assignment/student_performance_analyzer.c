#include<stdio.h>
#include<string.h>
#include<stdlib.h>


#define MAX_STUDENTS 100

typedef enum {
    SUCCESS =0,
    INVALID_NAME,
    INVALID_MARKS,
    INVALID_ROLL_NUMBER,
    ROLL_NUMBER_EXISTS,
} ErrorCode;
struct students_data{
        unsigned short roll_number;
        char name[100];
        unsigned short Subject1_marks;
        unsigned short Subject2_marks;
        unsigned short Subject3_marks;
};
ErrorCode validate_user_input(struct students_data student_array[], int count,
                          char *roll_str, char *name,
                         unsigned short marks1, unsigned short marks2, unsigned short marks3) {
    // Check if roll number is numeric
    for (int i = 0; roll_str[i] != '\0'; i++) {
        if (roll_str[i] < '0' || roll_str[i] > '9') {
            return INVALID_ROLL_NUMBER;
        }
    }
    unsigned short roll = (unsigned short)atoi(roll_str);
    //roll number must be between 1 to 100
    if(roll<0|| roll>100||roll==0){
        return INVALID_ROLL_NUMBER;
    }


    //Check if roll number alreary exists
    for (int i = 0; i < count; i++) {
        if (student_array[i].roll_number == roll)
            return ROLL_NUMBER_EXISTS;
    }

    //only letters and space allowed
    for (int i = 0; name[i] != '\0'; i++) {
        if (!((name[i] >= 'A' && name[i] <= 'Z') || 
              (name[i] >= 'a' && name[i] <= 'z') || 
              name[i] == ' ')) {
            return INVALID_NAME;
        }
    }

    //marks must b between 1 to 100
    if (marks1 > 100 || marks2 > 100 || marks3 > 100 ||
    marks1 < 0 || marks2 < 0 || marks3 < 0){
        return INVALID_MARKS;
    }
    return SUCCESS;
}
//function to calculate total marks 
int total_marks(struct students_data student){
    return student.Subject1_marks + student.Subject2_marks + student.Subject3_marks;
}
//function to calculate average marks
float average_marks(struct students_data student){
    return (total_marks(student)/300.0)*100;
}
//function to calculate grade
char grade(struct students_data student){
    float avg = average_marks(student);
    if(avg>=85) return 'A';
    else if(avg>=70) return 'B';
    else if(avg>=50) return 'C';
    else if(avg>=35) return 'D';
    else if(avg<35) return 'F';
    else return 'X'; //error case;
}
//function to display stars on the based of grade they got
void student_performance(struct students_data student){
    char grade_obtained = grade(student);
    int stars =0;
    switch (grade_obtained) {
        case 'A': stars = 5; break;
        case 'B': stars = 4; break;
        case 'C': stars = 3; break;
        case 'D': stars = 2; break;
        default: stars = 0;
    }
    for(int i=0;i<stars;i++){
        printf("*");
    }
    printf("\n");
}
    

int main(){
    unsigned short number_of_students;
    char line[256];  // buffer to hold input line
    //make sure number of students is between 1 to 100
    while(1){
        printf("How many students are there ?");
        fgets(line, sizeof(line), stdin);
        sscanf(line, "%hu", &number_of_students);
        if(number_of_students<0 || number_of_students>100){
            printf("Invalid input. Please enter a number between 1 and 100.\n");
            continue;
        }
        else{
            break;
        
        }
    }

    struct students_data *student_array = malloc(number_of_students * sizeof(struct students_data));


    for (int i = 0; i < number_of_students; i++) {
        char roll_str[20];
        char temp_name[100];
        unsigned short temp_marks1, temp_marks2, temp_marks3;
        ErrorCode err;

        while (1) { 
        printf("\nEnter student %d details:\n", i + 1);
        printf("[Roll number] [Name] [Marks1] [Marks2] [Marks3]: ");
        fgets(line, sizeof(line), stdin);
        sscanf(line, "%s %s %hu %hu %hu",roll_str, temp_name, &temp_marks1, &temp_marks2, &temp_marks3);
        
        // Validate the input
        err = validate_user_input(student_array, i,roll_str, temp_name, temp_marks1, temp_marks2, temp_marks3);
        if (err != SUCCESS) {
            switch (err) {
                case INVALID_ROLL_NUMBER:
                    printf("Roll number must be integer and between 1 to 100.\n");
                    break;
                case ROLL_NUMBER_EXISTS:
                    printf("Error - Roll number already exists!\n");
                    break;
                case INVALID_NAME:
                    printf("Error - Name must contain only alphabets and spaces.\n");
                    break;
                case INVALID_MARKS:
                    printf("Error -  Marks must be between 0 and 100.\n");
                    break;
                default:
                    printf("Unknown error.\n");
            }
            continue; // re-enter data
        }

        //If validation passes, store data and break loop
        unsigned short roll = (unsigned short)atoi(roll_str);
        student_array[i].roll_number = roll;
        strcpy(student_array[i].name, temp_name);
        student_array[i].Subject1_marks = temp_marks1;
        student_array[i].Subject2_marks = temp_marks2;
        student_array[i].Subject3_marks = temp_marks3;
        break;
    }

    }
    //sorting the student data based on roll number
    for(int i=0;i<number_of_students-1;i++){
        for(int j=0;j<number_of_students-i-1;j++){
            if(student_array[j].roll_number > student_array[j+1].roll_number){
                struct students_data temp = student_array[j];
                student_array[j] = student_array[j+1];
                student_array[j+1] = temp;
            }
        }
    }
    // display all students data and it is sorted based on roll number
    printf("\n\n=== Student Report ===\n");
    for (int i = 0; i < number_of_students; i++) {
        struct students_data student = student_array[i];
        printf("\nRoll No: %hu \nName: %s\n", student.roll_number, student.name);
        printf("Total Marks: %d \nAverage: %.2f \nGrade: %c\n",
               total_marks(student), average_marks(student), grade(student));
        printf("Performance: ");
        student_performance(student);
    }
    printf("\n\n List of Roll Numbers: ");
    for(int i=0;i<number_of_students;i++){
        struct students_data student = student_array[i];
        printf("%hu ", student.roll_number);

    }
    return 0;
}