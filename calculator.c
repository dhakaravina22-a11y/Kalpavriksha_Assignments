#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAX_EXPRESSION_LEN 1000
#define MAX_STACK_SIZE (MAX_EXPRESSION_LEN / 2 + 1)

typedef enum {
    SUCCESS = 0,
    STACK_OVERFLOW,
    STACK_UNDERFLOW,
    INVALID_EXPRESSION,
    EMPTY_EXPRESSION,
    EXPRESSION_TOO_LONG,
} ErrorCode;


typedef struct {
    long long data[MAX_STACK_SIZE]; // created stack for numbers 
    int top;
} NumberStack;


typedef struct {
    char data[MAX_STACK_SIZE];
    int top; // created stack.. for operators
} OperatorStack;


ErrorCode pushNumber(NumberStack *numbers, long long val) {
    if (numbers->top == MAX_STACK_SIZE - 1) {
        return STACK_OVERFLOW;  // return error instead of exit
    }
    numbers->data[++(numbers->top)] = val;
    return SUCCESS;
}

ErrorCode popNumber(NumberStack *numbers, long long *val) {
    if (numbers->top == -1) {
        return STACK_UNDERFLOW;
    }
    *val = numbers->data[(numbers->top)--];
    return SUCCESS;
}


ErrorCode pushOperator(OperatorStack *ops, char c) {
    if (ops->top == MAX_STACK_SIZE - 1) {
        return STACK_OVERFLOW;
    }
    ops->data[++(ops->top)] = c;
    return SUCCESS;
}


ErrorCode popOperator(OperatorStack *ops, char *op) {
    if (ops->top == -1) {
        return STACK_UNDERFLOW;
    }
    *op = ops->data[(ops->top)--];
    return SUCCESS;
}


char peekLastOperator(OperatorStack *numbers) {
    if (numbers->top == -1) return '\0';
    return numbers->data[numbers->top];
}

//precedence
int precedenceCheck(char op) {
    if (op == '+' || op == '-') return 1;
    return 2;
}

ErrorCode validateExpression(const char *expr) {
    int len = strlen(expr);

    if (len == 0) return EMPTY_EXPRESSION;                 // Empty
    if (len > MAX_EXPRESSION_LEN) return EXPRESSION_TOO_LONG; // Too long

    // Cannot start or end with operator(except + and - because +5-2 is valid expression)
    if ((strchr("*/", expr[0])) || strchr("+-*/", expr[len - 1])) {
        return INVALID_EXPRESSION;
    }
    

    // Check for consecutive operators
    for (int i = 1; i < len; i++) {
        if (strchr("+-*/", expr[i]) && strchr("+-*/", expr[i - 1])) {
        // allow only  leading + or -5, +5
        if (i == 0 && (expr[i] == '+' || expr[i] == '-')) {
            if (i + 1 < len && isdigit(expr[i + 1])) {
                continue; // valid leading unary
            }
        }
        // Otherwise invalid  **, //, */ , ++, 2*-3 
        return INVALID_EXPRESSION;
        }
    }


    return SUCCESS;
}



ErrorCode applyOperation(long long a, long long b, char op, long long *res) {
    if (op == '+') *res = a + b;
    else if (op == '-') *res = a - b;
    else if (op == '*') *res = a * b;
    else if (op == '/') {
        if (b == 0) return INVALID_EXPRESSION;
        *res = a / b;
    } else return INVALID_EXPRESSION;

    return SUCCESS;
}



ErrorCode evaluate(const char *expr, long long *result) {
    NumberStack values = {.top = -1};
    OperatorStack ops = {.top = -1};
    int i = 0, len = strlen(expr);

    while (i < len) {
        if (isspace(expr[i])) { i++; continue; }

        // Case 1: number
        if (isdigit(expr[i])) {
            long long val = 0;
            while (i < len && isdigit(expr[i])) {
                val = val * 10 + (expr[i] - '0');
                i++;
            }
            ErrorCode err = pushNumber(&values, val);
            if (err != SUCCESS) return err;
            continue;
        }

        // Case 2: unary + or -
        if ((expr[i] == '+' || expr[i] == '-') && (i == 0 || strchr("+-*/", expr[i - 1]))) {
            char sign = expr[i];
            i++;
            if (i < len && isdigit(expr[i])) {
                long long val = 0;
                while (i < len && isdigit(expr[i])) {
                    val = val * 10 + (expr[i] - '0');
                    i++;
                }
                if (sign == '-') val = -val;
                ErrorCode err = pushNumber(&values, val);
                if (err != SUCCESS) return err;
                continue;
            } else {
                return INVALID_EXPRESSION;
            }
        }

        // Case 3: operator
        if (strchr("+-*/", expr[i])) {
            char currentOp = expr[i];
            while (ops.top != -1 && precedenceCheck(peekLastOperator(&ops)) >= precedenceCheck(currentOp)) {
                long long a, b, res;
                char op;
                ErrorCode err;
                err = popNumber(&values, &b); if (err) return err;
                err = popNumber(&values, &a); if (err) return err;
                err = popOperator(&ops, &op); if (err) return err;
                err = applyOperation(a, b, op, &res); if (err) return err;
                err = pushNumber(&values, res); if (err) return err;
            }
            ErrorCode err = pushOperator(&ops, currentOp);
            if (err != SUCCESS) return err;
            i++;
            continue;
        }

        // Case 4: invalid char
        return INVALID_EXPRESSION;
    }

    
    while (ops.top != -1) {
        long long a, b, res; char op;
        ErrorCode err;
        err = popNumber(&values, &b); if (err) return err;
        err = popNumber(&values, &a); if (err) return err;
        err = popOperator(&ops, &op); if (err) return err;
        err = applyOperation(a, b, op, &res); if (err) return err;
        err = pushNumber(&values, res); if (err) return err;
    }

    return popNumber(&values, result);
}


int main() {
    char input[MAX_EXPRESSION_LEN];
    printf("enter the expression: ");

    if (!fgets(input, sizeof(input), stdin)) {
        printf("error - invalid input\n");
        return 1;
    }

    //Checking if input is too long 
    if (strchr(input, '\n') == NULL) {
        
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF);

        printf("Error-Expression too long...Max allowed length is %d.\n", MAX_EXPRESSION_LEN - 1);
        return INVALID_EXPRESSION;
    }

    input[strcspn(input, "\n")] = '\0'; 
    //validate 
    ErrorCode valid = validateExpression(input);
    if (valid != SUCCESS) {
    if (valid == EMPTY_EXPRESSION) {
        printf("Error: Empty Expression!\n");
    } else if (valid == EXPRESSION_TOO_LONG) {
        printf("Error: Expression Too Long!\n");
    } else {
        printf("Error: Invalid Expression!\n");
    }
    return valid;
    }


    long long result;
    ErrorCode err = evaluate(input, &result);

    if (err == SUCCESS) {
        printf("%lld\n", result);
    } else if (err == STACK_OVERFLOW) {
        printf("Error: Stack Overflow!\n");
    } else if (err == STACK_UNDERFLOW) {
        printf("Error: Stack Underflow!\n");
    } else if (err == INVALID_EXPRESSION) {
        printf("Error: Invalid Expression!\n");
    }

    return err;
}
