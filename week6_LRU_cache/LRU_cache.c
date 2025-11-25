#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VALUE_LEN 100
#define HASH_SIZE 1009

typedef struct Node {
    int key;
    char value[VALUE_LEN+1];
    struct Node *prev, *next;
} Node;

typedef struct HashNode {
    int key;
    Node *node;
    struct HashNode *next;
} HashNode;

typedef struct {
    int capacity;
    int size;
    Node *head, *tail;       
    HashNode **hash;
} LRUCache;

int hashKey(int key) {
    int h = key % HASH_SIZE;

    if (h < 0) {
        h += HASH_SIZE;
    }

    return h;
}


Node* createNode(int key, const char *value) {
    Node *n = (Node*)malloc(sizeof(Node));
    if (!n) {
        fprintf(stderr, "Memory allocation failed for Node\n");
        return NULL;
    }
    n->key = key;
    strcpy(n->value, value);
    n->prev = n->next = NULL;
    return n;
}

void addToFront(LRUCache *obj, Node *n) {

    n->prev = NULL;
    n->next = obj->head;

    if (obj->head){
        obj->head->prev = n;
    }

    obj->head = n;
    if (obj->tail == NULL){
        obj->tail = n;
    }
}
void removeNode(LRUCache *obj, Node *n) {
    if (n->prev){
        n->prev->next = n->next;
    }else{
        obj->head = n->next;
    }

    if (n->next){
        n->next->prev = n->prev;
    }else{
        obj->tail = n->prev;
    }

    n->next = n->prev = NULL;
}

HashNode* hashFind(HashNode *list, int key) {
    while (list) {
        if (list->key == key) return list;
        list = list->next;
    }
    return NULL;
}

void hashInsert(LRUCache *obj, int key, Node *node) {
    int h = hashKey(key);
    HashNode *hn = (HashNode*)malloc(sizeof(HashNode));
    if (!hn) {
        fprintf(stderr, "Memory allocation failed for HashNode\n");
        return;
    }

    hn->key = key;
    hn->node = node;
    hn->next = obj->hash[h];
    obj->hash[h] = hn;
}

void hashRemove(LRUCache *obj, int key) {
    int h = hashKey(key);
    HashNode *curr = obj->hash[h], *prev = NULL;

    while (curr) {
        if (curr->key == key) {
            if (prev) prev->next = curr->next;
            else obj->hash[h] = curr->next;
            free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}
LRUCache* createCache(int capacity) {
    LRUCache *obj = (LRUCache*)malloc(sizeof(LRUCache));
    if (!obj) {
        fprintf(stderr, "Memory allocation failed for LRUCache\n");
        return NULL;
    }
    obj->capacity = capacity;
    obj->size = 0;
    obj->head = NULL;   
    obj->tail = NULL;

    obj->hash = (HashNode**)calloc(HASH_SIZE, sizeof(HashNode*));
    if (!obj->hash) {
        fprintf(stderr, "Calloc failed for hash table\n");
        free(obj);
        return NULL;
    }

    return obj;
}

char* getValue(LRUCache *obj, int key) {
    HashNode *hn = hashFind(obj->hash[hashKey(key)], key);
    if (!hn){
        return NULL;
    }

    Node *n = hn->node;
    removeNode(obj, n);
    addToFront(obj, n);

    return n->value;
}

void putValue(LRUCache *obj, int key, const char *value) {
    HashNode *hn = hashFind(obj->hash[hashKey(key)], key);

    if (hn) {
        Node *n = hn->node;
        strcpy(n->value, value);
        removeNode(obj,n);
        addToFront(obj, n);
        return;
    }

    Node *newNode = createNode(key, value);
    addToFront(obj, newNode);
    hashInsert(obj, key, newNode);
    obj->size++;

    if (obj->size > obj->capacity) {
        Node *lru = obj->tail;
        hashRemove(obj, lru->key);
        removeNode(obj,lru);
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

    for (int i = 0; i < HASH_SIZE; i++) {
        HashNode *hn = obj->hash[i];
        while (hn) {
            HashNode *next = hn->next;
            free(hn);
            hn = next;
        }
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
