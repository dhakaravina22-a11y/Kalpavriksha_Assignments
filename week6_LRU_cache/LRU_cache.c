#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VALUE_LEN 100
#define HASH_SIZE 10007   

typedef struct Node {
    int key;
    char value[VALUE_LEN + 1];
    struct Node *prev, *next;
} Node;

typedef struct {
    int capacity;
    int size;
    Node *head, *tail;
    Node **hash;   
} LRUCache;


int hashKey(int key) {
    return key % HASH_SIZE;
}


Node* createNode(int key, const char *value) {
    Node *n = (Node*)malloc(sizeof(Node));
    n->key = key;
    strcpy(n->value, value);
    n->prev = n->next = NULL;
    return n;
}


void addToFront(LRUCache *obj, Node *n) {
    n->next = obj->head->next;
    n->prev = obj->head;
    obj->head->next->prev = n;
    obj->head->next = n;
}


void removeNode(Node *n) {
    n->prev->next = n->next;
    n->next->prev = n->prev;
}


LRUCache* createCache(int capacity) {
    LRUCache *obj = (LRUCache*)malloc(sizeof(LRUCache));
    obj->capacity = capacity;
    obj->size = 0;

    obj->hash = (Node**)calloc(HASH_SIZE, sizeof(Node*));

    obj->head = createNode(-1, "");
    obj->tail = createNode(-1, "");

    obj->head->next = obj->tail;
    obj->tail->prev = obj->head;

    return obj;
}


char* getValue(LRUCache *obj, int key) {
    int h = hashKey(key);

    Node *n = obj->hash[h];

    if (!n || n->key != key)     
        return NULL;

    removeNode(n);
    addToFront(obj, n);

    return n->value;
}

void putValue(LRUCache *obj, int key, const char *value) {
    int h = hashKey(key);

    Node *n = obj->hash[h];

   
    if (n && n->key == key) {
        strcpy(n->value, value);
        removeNode(n);
        addToFront(obj, n);
        return;
    }

    
    Node *newNode = createNode(key, value);
    addToFront(obj, newNode);
    obj->hash[h] = newNode;
    obj->size++;

    
    if (obj->size > obj->capacity) {
        Node *lru = obj->tail->prev;

        int oldH = hashKey(lru->key);
        obj->hash[oldH] = NULL;   

        removeNode(lru);
        free(lru);

        obj->size--;
    }
}


void freeCache(LRUCache *obj) {
    Node *curr = obj->head;
    while (curr) {
        Node *next = curr->next;
        free(curr);
        curr = next;
    }

    free(obj->hash);
    free(obj);
}

int main() {
    char command[50];
    LRUCache *cache = NULL;

    while (1) {
        scanf("%s", command);

        if (strcmp(command, "createCache") == 0) {
            int cap;
            scanf("%d", &cap);
            cache = createCache(cap);
        }

        else if (strcmp(command, "put") == 0) {
            int key;
            char value[VALUE_LEN];
            scanf("%d %s", &key, value);
            putValue(cache, key, value);
        }

        else if (strcmp(command, "get") == 0) {
            int key;
            scanf("%d", &key);

            char *result = getValue(cache, key);
            if (result)
                printf("%s\n", result);
            else
                printf("NULL\n");
        }

        else if (strcmp(command, "exit") == 0) {
            if (cache) freeCache(cache);
            break;
        }
    }

    return 0;
}
