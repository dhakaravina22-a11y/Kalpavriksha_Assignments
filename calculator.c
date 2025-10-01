#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAX_LEN 1000

//calculator program using stacks 
typedef struct {
    int data[MAX_LEN]; // created stack for integers
    int top;
} IntStack;


typedef struct {
    char data[MAX_LEN];
    int top; // created stack.. for characters
} CharStack;


void pushInt(IntStack *s, int val) {
    if (s->top == MAX_LEN - 1) {
        printf("stack overflow.\n");
        exit(1);
    }
    s->data[++(s->top)] = val;
}

int popInt(IntStack *s) {
    if (s->top == -1) {
        printf("stack underflow.\n");
        exit(1);
    }
    return s->data[(s->top)--];
}

void pushChar(CharStack *s, char c) {
    if (s->top == MAX_LEN - 1) {
        printf("stack overflow.\n");
        exit(1);
    }
    s->data[++(s->top)] = c;
}

char popChar(CharStack *s) {
    if (s->top == -1) {
        printf("stack underflow.\n");
        exit(1);
    }
    return s->data[(s->top)--];
}

char peekChar(CharStack *s) {
    if (s->top == -1) return '\0';
    return s->data[s->top];
}

//precedence
int precedenceCheck(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    return 0;
}


int applyOp(int a, int b, char op) {
    if (op == '+') return a + b;
    if (op == '-') return a - b;
    if (op == '*') return a * b;
    if (op == '/') {
        if (b == 0) {
            printf("error - division by zero.\n");
            exit(1);
        }
        return a / b;
    }
    return 0;
}


int evaluate(const char *expr) {
    IntStack values = {.top = -1};
    CharStack ops = {.top = -1};
    int i = 0, len = strlen(expr);

    while (i < len) {
        if (isspace(expr[i])) {
            i++;
            continue;
        }

        if (isdigit(expr[i])) {
            int val = 0;
            while (i < len && isdigit(expr[i])) {
                val = val * 10 + (expr[i] - '0');
                i++;
            }
            pushInt(&values, val);
            continue;
        }

        if (expr[i] == '+' || expr[i] == '-' || expr[i] == '*' || expr[i] == '/') {
            while (ops.top != -1 && precedenceCheck(peekChar(&ops)) >= precedenceCheck(expr[i])) {
                int b = popInt(&values);
                int a = popInt(&values);
                char op = popChar(&ops);
                pushInt(&values, applyOp(a, b, op));
            }
            pushChar(&ops, expr[i]);
        } else {
            printf("error - invalid expression.\n");
            exit(1);
        }
        i++;
    }

    while (ops.top != -1) {
        int b = popInt(&values);
        int a = popInt(&values);
        char op = popChar(&ops);
        pushInt(&values, applyOp(a, b, op));
    }

    return popInt(&values);
}

int main() {
    char input[MAX_LEN];

    printf("enter the expression: ");
    if (!fgets(input, MAX_LEN, stdin)) {
        printf("error - invalid input.\n");
        return 1;
    }


    input[strcspn(input, "\n")] = '\0'; 

    int result = evaluate(input);
    printf("%d\n", result);
    return 0;
}
