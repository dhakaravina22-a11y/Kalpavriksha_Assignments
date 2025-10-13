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
    MEMORY_ALLOCATION_FAILED,
} ErrorCode;

struct students_data{
        unsigned short roll_number;
        char name[100];
        unsigned short subject1_marks;
        unsigned short subject2_marks;
        unsigned short subject3_marks;
};
ErrorCode validate_user_input(struct students_data student_array[], int count,
                              struct students_data new_student) {

    unsigned short roll = new_student.roll_number;

    // Roll number must be between 1 to 100
    if (roll == 0 || roll > 100) {
        return INVALID_ROLL_NUMBER;
    }

    // Check if roll number already exists
    for (int i = 0; i < count; i++) {
        if (student_array[i].roll_number == roll)
            return ROLL_NUMBER_EXISTS;
    }

    // Only letters and space allowed in name
    for (int i = 0; new_student.name[i] != '\0'; i++) {
        if (!((new_student.name[i] >= 'A' && new_student.name[i] <= 'Z') || 
              (new_student.name[i] >= 'a' && new_student.name[i] <= 'z') || 
              new_student.name[i] == ' ')) {
            return INVALID_NAME;
        }
    }

    // Marks must be between 0 and 100
    if (new_student.subject1_marks > 100 || new_student.subject2_marks > 100 || new_student.subject3_marks > 100) {
        return INVALID_MARKS;
    }

    return SUCCESS;
}

//function to calculate total marks 
int total_marks(const struct students_data *student)
{
    return student->subject1_marks + student->subject2_marks + student->subject3_marks;
}


//function to calculate average marks
float average_marks(const struct students_data *student){
    return (total_marks(student)/300.0)*100;
}
//function to calculate grade
char grade(const struct students_data *student){
    float avg = average_marks(student);
    if(avg>=85) return 'A';
    else if(avg>=70) return 'B';
    else if(avg>=50) return 'C';
    else if(avg>=35) return 'D';
    else return 'F';
    
}
//function to display stars on the based of grade they got
void student_performance(const struct students_data *student){
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
    

ErrorCode main(){

    unsigned short number_of_students;
    char line[256];  // buffer to hold input line
    //make sure number of students is between 1 to 100
    while(1){
        printf("How many students are there ?");
        fgets(line, sizeof(line), stdin);
        sscanf(line, "%hu", &number_of_students);
        if(number_of_students>100){
            printf("Invalid input. Please enter a number between 1 and 100.\n");
            continue;
        }
        else{
            break;
        
        }
    }

    struct students_data *student_array = malloc(number_of_students * sizeof(struct students_data));
    if (student_array == NULL) {
       printf("Memory allocation failed! Exiting program.\n");
       return MEMORY_ALLOCATION_FAILED;  
    }



    for (int i = 0; i < number_of_students; i++) {
        struct students_data temp_student;
        ErrorCode err;

        while (1) { 
        printf("\nEnter student %d details:\n", i + 1);
        printf("[Roll number] [Name] [Marks1] [Marks2] [Marks3]: ");
        fgets(line, sizeof(line), stdin);
        int parsed = sscanf(line, "%hu %s %hu %hu %hu",
                        &temp_student.roll_number,
                        temp_student.name,
                        &temp_student.subject1_marks,
                        &temp_student.subject2_marks,
                        &temp_student.subject3_marks);
        if (parsed != 5) {
            printf("Invalid input format! Please re-enter.\n");
            continue;
        }

        // Validate the input
        err = validate_user_input(student_array, i, temp_student);
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
       student_array[i] = temp_student;
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
               total_marks(&student), average_marks(&student), grade(&student));
        float avg = average_marks(&student);
        if (avg < 35) {
           continue;
        }
        printf("Performance: ");
        student_performance(&student);
    }
    printf("\n\n List of Roll Numbers: ");
    for(int i=0;i<number_of_students;i++){
        struct students_data student = student_array[i];
        printf("%hu ", student.roll_number);

    }
    free(student_array);
    return SUCCESS;

}