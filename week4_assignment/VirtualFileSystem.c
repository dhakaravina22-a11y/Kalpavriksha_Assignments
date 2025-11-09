#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define BLOCK_SIZE 512
#define NUM_BLOCKS 1024
#define MAX_NAME 50
#define MAX_BLOCKS_PER_FILE 100

typedef struct FreeBlock {
    int index;
    struct FreeBlock *next;
    struct FreeBlock *prev;
} FreeBlock;

typedef struct FileNode {
    char name[MAX_NAME];
    int isDirectory; 
    struct FileNode *parent;
    struct FileNode *child;
    struct FileNode *next;
    struct FileNode *prev;
    int blockPointers[MAX_BLOCKS_PER_FILE];
    int blockCount;
    int size; 
} FileNode;

char virtualDisk[NUM_BLOCKS][BLOCK_SIZE];
FreeBlock *freeListHead = NULL;
FileNode *root = NULL;
FileNode *cwd = NULL;


void initFreeBlocks() {
    freeListHead = NULL;
    for (int i = 0; i < NUM_BLOCKS; ++i) {
        //using doubly linked list for free blocks
        FreeBlock *node = (FreeBlock*)malloc(sizeof(FreeBlock));
        node->index = i;
        node->next = freeListHead;
        node->prev = NULL;
        if (freeListHead) freeListHead->prev = node;
        freeListHead = node;
    }
}


int allocateBlockIndex() {
    if (!freeListHead){
        return -1;
    }
    FreeBlock *block = freeListHead;
    int idx = block->index;
    freeListHead = block->next;
    if (freeListHead){
        freeListHead->prev = NULL;
    }
    free(block);
    return idx;
}


void freeBlockIndex(int index) {
    FreeBlock *node = (FreeBlock*)malloc(sizeof(FreeBlock));
    node->index = index;
    node->next = NULL;
    node->prev = NULL;
    if (!freeListHead) {
        freeListHead = node;
        return;
    }
    FreeBlock *temp = freeListHead;
    while (temp->next) temp = temp->next;
    temp->next = node;
    node->prev = temp;
}

// count free blocks
int countFreeBlocks() {
    int count = 0;
    FreeBlock *temp = freeListHead;
    while (temp) {
       count++;
       temp = temp->next; 
    }
    return count;
}

FileNode* createNode(const char *name, int isDir) {
    FileNode *n = (FileNode*)malloc(sizeof(FileNode));
    strncpy(n->name, name, MAX_NAME-1);
    n->name[MAX_NAME-1] = '\0';
    n->isDirectory = isDir;
    n->parent = NULL;
    n->child = NULL;
    n->next = n->prev = NULL;
    n->blockCount = 0;
    n->size = 0;
    for (int i=0;i<MAX_BLOCKS_PER_FILE;i++) n->blockPointers[i] = -1;
    return n;
}


FileNode* findChild(FileNode *dir, const char *name) {
    if (!dir || !dir->child) return NULL;
    FileNode *start = dir->child;
    FileNode *t = start;
    do {
        if (strcmp(t->name, name) == 0) return t;
        t = t->next;
    } while (t != start);
    return NULL;
}


void insertChild(FileNode *dir, FileNode *node) {
    node->parent = dir;
    node->child = NULL;
    if (!dir->child) {
        dir->child = node;
        node->next = node->prev = node;
    } else {
        FileNode *first = dir->child;
        FileNode *last = first->prev;
        last->next = node;
        node->prev = last;
        node->next = first;
        first->prev = node;
    }
}


void removeChildFromParent(FileNode *node) {
    FileNode *parent = node->parent;
    if (!parent) return;
    if (!parent->child) return;
    FileNode *start = parent->child;

    
    if (start == node && node->next == node) {
        parent->child = NULL;
        node->next = node->prev = NULL;
        node->parent = NULL;
        return;
    }

    
    FileNode *t = start;
    do {
        if (t == node) {
            t->prev->next = t->next;
            t->next->prev = t->prev;
            if (parent->child == node) parent->child = node->next;
            node->next = node->prev = NULL;
            node->parent = NULL;
            return;
        }
        t = t->next;
    } while (t != start);
}


void freeFileBlocks(FileNode *file) {
    if (!file || file->isDirectory) return;
    for (int i = 0; i < file->blockCount; ++i) {
        int idx = file->blockPointers[i];
        if (idx >= 0) freeBlockIndex(idx);
        file->blockPointers[i] = -1;
    }
    file->blockCount = 0;
    file->size = 0;
}

void destroyNode(FileNode *node) {
    if (!node) return;
    
    if (!node->isDirectory) freeFileBlocks(node);
    free(node);
}


void initFS() {
    root = createNode("/", 1);
    root->parent = NULL;
    root->child = NULL;
    cwd = root;
    initFreeBlocks();
    printf("Compact VFS - ready. Type 'exit' to quit.\n");
}


void cmd_mkdir(const char *name) {
    if (!name) { printf("mkdir: missing name\n"); return; }
    if (findChild(cwd, name)) { printf("Name already exists in current directory.\n"); return; }
    FileNode *dir = createNode(name, 1);
    insertChild(cwd, dir);
    printf("Directory '%s' created successfully.\n", name);
}


void cmd_create(const char *name) {
    if (!name) { printf("create: missing name\n"); return; }
    if (findChild(cwd, name)) { printf("Name already exists in current directory.\n"); return; }
    FileNode *file = createNode(name, 0);
    insertChild(cwd, file);
    printf("File '%s' created successfully.\n", name);
}


void cmd_ls() {
    if (!cwd->child) { printf("(empty)\n"); return; }
    FileNode *start = cwd->child;
    FileNode *t = start;
    do {
        printf("%s%s\n", t->name, t->isDirectory ? "/" : "");
        t = t->next;
    } while (t != start);
}


void cmd_cd(const char *name) {
    if (!name) { printf("cd: missing name\n"); return; }
    if (strcmp(name, "..") == 0) {
        if (cwd->parent) cwd = cwd->parent;
        printf("Moved to %s\n", cwd->name);
        return;
    }
    FileNode *target = findChild(cwd, name);
    if (!target || !target->isDirectory) {
        printf("Directory not found.\n");
        return;
    }
    cwd = target;
    printf("Moved to %s\n", cwd->name);
}


void cmd_pwd() {
    FileNode *t = cwd;
    char full[1024] = "";
    
    if (t == root) {
        printf("/\n");
        return;
    }
    
    char *parts[100];
    int pc = 0;
    while (t != NULL && t != root) {
        parts[pc++] = strdup(t->name);
        t = t->parent;
    }
    
    strcat(full, "/");
    for (int i = pc-1; i >= 0; --i) {
        strcat(full, parts[i]);
        if (i > 0) strcat(full, "/");
        free(parts[i]);
    }
    printf("%s\n", full);
}


FileNode* findFileInCwd(const char *name) {
    FileNode *n = findChild(cwd, name);
    if (!n) return NULL;
    if (n->isDirectory) return NULL;
    return n;
}


void cmd_write(const char *filename, const char *content) {
    if (!filename) { printf("write: missing filename\n"); return; }
    FileNode *file = findChild(cwd, filename);
    if (!file || file->isDirectory) { printf("File not found.\n"); return; }
    freeFileBlocks(file);

    int contentLen = (int)strlen(content);
    int neededBlocks = (contentLen + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (neededBlocks > MAX_BLOCKS_PER_FILE) {
        printf("File too large for single file limit.\n");
        return;
    }
    if (neededBlocks > countFreeBlocks()) {
        printf("Disk full. Not enough free blocks.\n");
        return;
    }

    int written = 0;
    for (int b = 0; b < neededBlocks; ++b) {
        int idx = allocateBlockIndex();
        if (idx < 0) { 
            printf("Unexpected: no free block.\n");
           
            for (int i = 0; i < file->blockCount; ++i) freeBlockIndex(file->blockPointers[i]);
            file->blockCount = 0;
            file->size = 0;
            return;
        }
        file->blockPointers[file->blockCount++] = idx;
        
        int copyLen = BLOCK_SIZE;
        if (contentLen - written < copyLen) copyLen = contentLen - written;
        if (copyLen > 0)
            memcpy(virtualDisk[idx], content + written, copyLen);
        if (copyLen < BLOCK_SIZE) virtualDisk[idx][copyLen] = '\0';
        written += copyLen;
    }
    file->size = contentLen;
    printf("Data written successfully (size=%d bytes).\n", contentLen);
}

// read
void cmd_read(const char *filename) {
    if (!filename) { 
        printf("read: missing filename\n"); 
        return; 
    }
    FileNode *file = findChild(cwd, filename);
    if (!file || file->isDirectory) { 
        printf("File not found.\n"); 
        return; 
    }
    if (file->blockCount == 0) { 
        printf("(empty)\n"); 
        return; 
    }
    int remaining = file->size;
    for (int i = 0; i < file->blockCount; ++i) {
        int idx = file->blockPointers[i];
        int toPrint;
        if (remaining < BLOCK_SIZE){
            toPrint = remaining;
        }else{
            toPrint = BLOCK_SIZE;
        }

        if (toPrint > 0) {
            fwrite(virtualDisk[idx], 1, toPrint, stdout);
        }
        remaining -= toPrint;
    }
    printf("\n");
}

// delete a file
void cmd_delete(const char *filename) {
    if (!filename) { 
        printf("delete: missing filename\n"); 
        return; 
    }
    FileNode *file = findChild(cwd, filename);
    if (!file) { 
        printf("File not found.\n"); 
        return; 
    }
    if (file->isDirectory) { 
        printf("Target is a directory. Use rmdir to remove directories.\n"); 
        return; 
    }
   
    freeFileBlocks(file);
    removeChildFromParent(file);
    destroyNode(file);
    printf("File deleted successfully.\n");
}

// remove directory 
void cmd_rmdir(const char *dirname) {
    if (!dirname) { 
        printf("rmdir: missing name\n");
         return; 
    }
    FileNode *d = findChild(cwd, dirname);
    if (!d) {
        printf("Directory not found.\n"); 
        return; 
    }
    if (!d->isDirectory) {
        printf("Not a directory.\n"); 
        return; 
    }
    if (d->child) { 
        printf("Directory not empty. Remove files first.\n"); 
        return; 
    }
    
    removeChildFromParent(d);
    destroyNode(d);
    printf("Directory removed successfully.\n");
}


void cmd_df() {
    int freeCount = countFreeBlocks();
    int used = NUM_BLOCKS - freeCount;
    double usagePercent = ((double)used / (double)NUM_BLOCKS) * 100.0;
    printf("Total Blocks: %d\n", NUM_BLOCKS);
    printf("Used Blocks: %d\n", used);
    printf("Free Blocks: %d\n", freeCount);
    printf("Disk Usage: %.2f%%\n", usagePercent);
}


void trim(char *s) {
    if (!s) return;
    int len = strlen(s);
    int i=0;
    while (isspace((unsigned char)s[i])) i++;
    if (i>0){
        memmove(s, s+i, len-i+1);
    }
    len = strlen(s);
    while (len>0 && isspace((unsigned char)s[len-1])){
        s[len-1]=0; len--; 
    }
}
void handle_line(char *input) {
    char line[2048];
    strcpy(line, input); 
    trim(line);
    if (strlen(line) == 0) return;

    
    char cmd[50];
    int pos = 0;
    while (line[pos] && !isspace((unsigned char)line[pos])) {
        cmd[pos] = line[pos];
        pos++;
    }
    cmd[pos] = '\0';

    char *rest = line + pos;
    while (*rest == ' ') rest++;

    if (strcmp(cmd, "mkdir") == 0) {
        cmd_mkdir(rest);
    }
    else if (strcmp(cmd, "create") == 0) {
        cmd_create(rest);
    }
    else if (strcmp(cmd, "ls") == 0) {
        cmd_ls();
    }
   else if (strcmp(cmd, "cd") == 0 || strcmp(cmd, "cd..") == 0) {
    if (strcmp(cmd, "cd..") == 0)
        cmd_cd("..");
    else
        cmd_cd(rest);
    }

    else if (strcmp(cmd, "pwd") == 0) {
        cmd_pwd();
    }
    else if (strcmp(cmd, "write") == 0) {
        
        char fname[100] = "";
        char content[1024] = "";

        int i = 0;
        while (*rest && !isspace((unsigned char)*rest) && *rest != '"') {
            fname[i++] = *rest++;
        }
        fname[i] = '\0';

        if (strlen(fname) == 0) {
            printf("write: missing filename\n");
            return;
        }

        while (*rest == ' ') rest++;

        if (*rest == '"') {
            rest++;
            char *endq = strchr(rest, '"');
            if (!endq) {
                printf("write: missing closing quote\n");
                return;
            }
            *endq = '\0';
            strcpy(content, rest);
        } else {
            strcpy(content, rest);
        }

        cmd_write(fname, content);
    }
    else if (strcmp(cmd, "read") == 0) {
        cmd_read(rest);
    }
    else if (strcmp(cmd, "delete") == 0) {
        cmd_delete(rest);
    }
    else if (strcmp(cmd, "rmdir") == 0) {
        cmd_rmdir(rest);
    }
    else if (strcmp(cmd, "df") == 0) {
        cmd_df();
    }
    else if (strcmp(cmd, "exit") == 0) {
        printf("Memory released. Exiting program...\n");
        exit(0);
    }
    else {
        printf("Unknown command: %s\n", cmd);
    }
}



int main() {
    initFS();

    char line[2048];
    while (1) {
        
        if (cwd == root) printf("/ > ");
        else printf("%s > ", cwd->name);
        if (!fgets(line, sizeof(line), stdin)) break;
        
        line[strcspn(line, "\n")] = 0;
        handle_line(line);
    }
    return 0;
}
