#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define BLOCK_SIZE 512
#define NUM_BLOCKS 1024
#define MAX_NAME 50
#define LINE_BUF 7000

typedef struct FreeBlock {
    int index;
    struct FreeBlock *next;
    struct FreeBlock *prev;
} FreeBlock;

typedef struct FileNode {
    char name[MAX_NAME + 1];// +1 for string terminator '\0'
    bool isDirectory;
    struct FileNode *parent;
    struct FileNode *child;
    struct FileNode *next;
    struct FileNode *prev;

    int *blockPointers; 
    int blockCount;
    int blockCap;
    int size;
} FileNode;

typedef struct FileSystem {
    unsigned char virtualDisk[NUM_BLOCKS][BLOCK_SIZE];
    FreeBlock *freeHead;
    FreeBlock *freeTail;
    int freeCount;

    FileNode *root;
    FileNode *cwd;
} FileSystem;

static FileSystem file;


static void safe_name_copy(char *dst, const char *src) {
    if (!dst) return;
    if (!src) { dst[0] = '\0'; return; }
    strncpy(dst, src, MAX_NAME);
    dst[MAX_NAME] = '\0';
}

static void addFreeBlockTail(int idx) {
    FreeBlock *node = (FreeBlock*)malloc(sizeof(FreeBlock));
    if (!node) {
        fprintf(stderr, "malloc failed in addFreeBlockTail\n");
        exit(EXIT_FAILURE);
    }
    node->index = idx;
    node->next = NULL;
    node->prev = file.freeTail;
    if (!file.freeHead) file.freeHead = node;
    else file.freeTail->next = node;
    file.freeTail = node;
    file.freeCount++;
}

int allocateBlockIndex() {
    if (!file.freeHead) return -1;
    FreeBlock *b = file.freeHead;
    int idx = b->index;
    file.freeHead = b->next;
    if (file.freeHead) file.freeHead->prev = NULL;
    else file.freeTail = NULL;
    free(b);
    file.freeCount--;
    memset(file.virtualDisk[idx], 0, BLOCK_SIZE);
    return idx;
}

void freeBlockIndex(int index) {
    if (index < 0 || index >= NUM_BLOCKS) return;
    addFreeBlockTail(index);
}

int countFreeBlocks() {
    return file.freeCount;
}


static void ensureBlockCapacity(FileNode *n, int need) {
    if (!n) return;
    if (n->blockCap >= need) return;
    int cap = (n->blockCap == 0) ? 4 : n->blockCap;
    while (cap < need) cap *= 2;
    int *tmp = (int*)realloc(n->blockPointers, sizeof(int) * cap);
    if (!tmp) {
        fprintf(stderr, "realloc failed\n");
        exit(EXIT_FAILURE);
    }
    n->blockPointers = tmp;
    
    for (int i = n->blockCap; i < cap; ++i) n->blockPointers[i] = -1;
    n->blockCap = cap;
}

FileNode* createNode(const char *name, int isDir) {
    FileNode *n = (FileNode*)malloc(sizeof(FileNode));
    if (!n) {
        fprintf(stderr, "malloc failed in createNode\n");
        return NULL;
    }
    safe_name_copy(n->name, name ? name : "");
    n->isDirectory = (isDir ? true : false);
    n->parent = NULL;
    n->child = NULL;
    n->next = n->prev = n;
    n->blockPointers = NULL;
    n->blockCount = 0;
    n->blockCap = 0;
    n->size = 0;
    return n;
}


static FileNode* findChild(FileNode *dir, const char *name) {
    if (!dir || !dir->child || !name) return NULL;
    FileNode *start = dir->child;
    FileNode *t = start;
    do {
        if (strcmp(t->name, name) == 0) return t;
        t = t->next;
    } while (t != start);
    return NULL;
}


static void insertChild(FileNode *dir, FileNode *node) {
    if (!dir || !node) return;
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
    if (!node || !node->parent) return;
    FileNode *parent = node->parent;
    if (!parent->child) return;

    FileNode *start = parent->child;

    
    if (start == node && node->next == node) {
        parent->child = NULL;
        node->next = node->prev = node;
        node->parent = NULL;
        return;
    }

    FileNode *t = start;
    do {
        if (t == node) {
            t->prev->next = t->next;
            t->next->prev = t->prev;
            if (parent->child == node) parent->child = node->next;
            node->next = node->prev = node;
            node->parent = NULL;
            return;
        }
        t = t->next;
    } while (t != start);
}

void freeFileBlocks(FileNode *f) {
    if (!f || f->isDirectory) return;
    for (int i = 0; i < f->blockCount; ++i) {
        int idx = f->blockPointers[i];
        if (idx >= 0 && idx < NUM_BLOCKS) {
            memset(file.virtualDisk[idx], 0, BLOCK_SIZE);
            freeBlockIndex(idx);
        }
        f->blockPointers[i] = -1;
    }
    f->blockCount = 0;
    f->size = 0;
}

void destroyNode(FileNode *node) {
    if (!node) return;
    if (!node->isDirectory) {
        freeFileBlocks(node);
        free(node->blockPointers);
    } 
    free(node);
}

static void freeDirectoryTree(FileNode *node) {
    if (!node) return;
    FileNode *child = node->child;
    if (child) {
        FileNode *t = child;
        do {
            FileNode *next = t->next;
            freeDirectoryTree(t);
            t = next;
        } while (t != child);
    }
    if (!node->isDirectory) {
        for (int i = 0; i < node->blockCount; ++i) {
            int idx = node->blockPointers[i];
            if (idx >= 0 && idx < NUM_BLOCKS) {
                memset(file.virtualDisk[idx], 0, BLOCK_SIZE);
                freeBlockIndex(idx);
            }
        }
        free(node->blockPointers);
    }
    free(node);
}


void initFS() {
    file.freeHead = file.freeTail = NULL;
    file.freeCount = 0;
    for (int i = 0; i < NUM_BLOCKS; ++i) addFreeBlockTail(i);

    file.root = createNode("/", 1);
    if (!file.root) {
        fprintf(stderr, "Failed to allocate root\n");
        exit(EXIT_FAILURE);
    }
    file.root->parent = NULL;
    file.root->child = NULL;
    file.root->next = file.root->prev = file.root;
    file.cwd = file.root;

    for (int i = 0; i < NUM_BLOCKS; ++i) memset(file.virtualDisk[i], 0, BLOCK_SIZE);
}

static void cleanupFS() {
    FreeBlock *fb = file.freeHead;
    while (fb) {
        FreeBlock *n = fb->next;
        free(fb);
        fb = n;
    }
    file.freeHead = file.freeTail = NULL;
    file.freeCount = 0;

    
    if (file.root) {
        FileNode *child = file.root->child;
        if (child) {
            FileNode *t = child;
            do {
                FileNode *next = t->next;
                freeDirectoryTree(t);
                t = next;
            } while (t != child);
        }
        
        free(file.root->blockPointers);
        free(file.root);
        file.root = NULL;
        file.cwd = NULL;
    }
}



void cmd_mkdir(const char *name) {
    if (!name || name[0] == '\0') { printf("mkdir: missing name\n"); return; }
    if (strlen(name) > MAX_NAME) { printf("mkdir: name too long\n"); return; }
    if (findChild(file.cwd, name)) { printf("Name already exists in current directory.\n"); return; }
    FileNode *dir = createNode(name, 1);
    if (!dir) { printf("mkdir: allocation failed\n"); return; }
    insertChild(file.cwd, dir);
    printf("Directory '%s' created successfully.\n", name);
}

void cmd_create(const char *name) {
    if (!name || name[0] == '\0') { printf("create: missing name\n"); return; }
    if (strlen(name) > MAX_NAME) { printf("create: name too long\n"); return; }
    if (findChild(file.cwd, name)) { printf("Name already exists in current directory.\n"); return; }
    FileNode *f = createNode(name, 0);
    if (!f) { printf("create: allocation failed\n"); return; }
    insertChild(file.cwd, f);
    printf("File '%s' created successfully.\n", name);
}

void cmd_ls() {
    if (!file.cwd->child) { printf("(empty)\n"); return; }
    FileNode *start = file.cwd->child;
    FileNode *t = start;
    do {
        printf("%s%s\n", t->name, t->isDirectory ? "/" : "");
        t = t->next;
    } while (t != start);
}

static void printPathRecursive(FileNode *n) {
    if (!n || n == file.root) return;
    printPathRecursive(n->parent);
    printf("/%s", n->name);
}

void cmd_cd(const char *name) {
    if (!name || name[0] == '\0') { printf("cd: missing name\n"); return; }
    if (strcmp(name, "..") == 0) {
    if (file.cwd->parent) file.cwd = file.cwd->parent;

    printf("Moved to ");
    if (file.cwd == file.root) printf("/");
    else printPathRecursive(file.cwd);
    printf("\n");
    return;
    }
    FileNode *target = findChild(file.cwd, name);
    if (!target || !target->isDirectory) {
        printf("Directory not found.\n");
        return;
    }
    file.cwd = target;
    printf("Moved to ");
    if (file.cwd == file.root) printf("/");
    else printPathRecursive(file.cwd);
    printf("\n");
}

void cmd_pwd() {
    if (file.cwd == file.root) {
        printf("/\n");
        return;
    }
    printPathRecursive(file.cwd);
    printf("\n");
}
void cmd_write(const char *filename, const char *content) {
    if (!filename || filename[0] == '\0') { printf("write: missing filename\n"); return; }
    FileNode *fnode = findChild(file.cwd, filename);
    if (!fnode || fnode->isDirectory) { printf("File not found.\n"); return; }

    freeFileBlocks(fnode);

    size_t contentLen = content ? strlen(content) : 0;
    size_t totalBytes = contentLen; 
    int neededBlocks = (int)((totalBytes + BLOCK_SIZE - 1) / BLOCK_SIZE);
    if (totalBytes == 0) neededBlocks = 0;

    if (neededBlocks > NUM_BLOCKS) {
        printf("File too large for single file limit.\n");
        return;
    }
    if (neededBlocks > countFreeBlocks()) {
        printf("Disk full. Not enough free blocks.\n");
        return;
    }

    
    ensureBlockCapacity(fnode, neededBlocks);

    int written = 0;
    for (int b = 0; b < neededBlocks; ++b) {
        int idx = allocateBlockIndex();
        if (idx < 0) {
            printf("Unexpected: no free block.\n");
            
            for (int i = 0; i < fnode->blockCount; ++i) {
                int bi = fnode->blockPointers[i];
                if (bi >= 0 && bi < NUM_BLOCKS) {
                    memset(file.virtualDisk[bi], 0, BLOCK_SIZE);
                    freeBlockIndex(bi);
                }
            }
            fnode->blockCount = 0;
            fnode->size = 0;
            return;
        }
        fnode->blockPointers[fnode->blockCount++] = idx;

        int copyLen = BLOCK_SIZE;
        if ((size_t)(contentLen - written) < (size_t)copyLen) copyLen = (int)(contentLen - written);
        if (copyLen > 0) {
            memcpy(file.virtualDisk[idx], content + written, copyLen);
            if (copyLen < BLOCK_SIZE) memset(file.virtualDisk[idx] + copyLen, 0, BLOCK_SIZE - copyLen);
        } else {
            
            memset(file.virtualDisk[idx], 0, BLOCK_SIZE);
        }
        written += copyLen;
    }
    fnode->size = (int)contentLen;
    printf("Data written successfully (size=%zu bytes).\n", contentLen);
}

//read
void cmd_read(const char *filename) {
    if (!filename || filename[0] == '\0') { printf("read: missing filename\n"); return; }
    FileNode *fnode = findChild(file.cwd, filename);
    if (!fnode || fnode->isDirectory) { printf("File not found.\n"); return; }
    if (fnode->blockCount == 0 || fnode->size == 0) { printf("(empty)\n"); return; }

    int remaining = fnode->size;
    for (int i = 0; i < fnode->blockCount; ++i) {
        int idx = fnode->blockPointers[i];
        int toPrint = remaining < BLOCK_SIZE ? remaining : BLOCK_SIZE;
        if (toPrint > 0) fwrite(file.virtualDisk[idx], 1, toPrint, stdout);
        remaining -= toPrint;
    }
    printf("\n");
}
//delete a file
void cmd_delete(const char *filename) {
    if (!filename || filename[0] == '\0') { printf("delete: missing filename\n"); return; }
    FileNode *fnode = findChild(file.cwd, filename);
    if (!fnode) { printf("File not found.\n"); return; }
    if (fnode->isDirectory) { printf("Target is a directory. Use rmdir to remove directories.\n"); return; }

    freeFileBlocks(fnode);
    free(fnode->blockPointers);
    removeChildFromParent(fnode);
    destroyNode(fnode);
    printf("File deleted successfully.\n");
}
//remove directory
void cmd_rmdir(const char *dirname) {
    if (!dirname || dirname[0] == '\0') { printf("rmdir: missing name\n"); return; }
    FileNode *d = findChild(file.cwd, dirname);
    if (!d) { printf("Directory not found.\n"); return; }
    if (!d->isDirectory) { printf("Not a directory.\n"); return; }
    if (d->child) { printf("Directory not empty. Remove files first.\n"); return; }

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
    size_t len = strlen(s);
    size_t i = 0;
    while (i < len && isspace((unsigned char)s[i])) i++;
    if (i > 0) memmove(s, s + i, len - i + 1);
    len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) { s[len - 1] = '\0'; len--; }
}


void handle_line(char *input) {
    if (!input) return;
    char line[LINE_BUF];
    
    strncpy(line, input, LINE_BUF - 1);
    line[LINE_BUF - 1] = '\0';
    trim(line);
    if (strlen(line) == 0) return;

    
    char cmd[64] = "";
    int pos = 0;
    while (line[pos] && !isspace((unsigned char)line[pos]) && pos < (int)(sizeof(cmd) - 1)) {
        cmd[pos] = line[pos];
        pos++;
    }
    cmd[pos] = '\0';

    
    char *rest = line + pos;
    while (*rest && isspace((unsigned char)*rest)) rest++;

    if (strcmp(cmd, "mkdir") == 0) {
        cmd_mkdir(rest);
    } else if (strcmp(cmd, "create") == 0) {
        cmd_create(rest);
    } else if (strcmp(cmd, "ls") == 0) {
        cmd_ls();
    } else if (strcmp(cmd, "cd") == 0) {
        if (strcmp(rest, "..") == 0 || strcmp(rest, "") == 0) cmd_cd("..");
        else cmd_cd(rest);
    } else if (strcmp(cmd, "pwd") == 0) {
        cmd_pwd();
    } else if (strcmp(cmd, "write") == 0) {
        
        char fname[256] = "";
        char content[LINE_BUF];
        content[0] = '\0';

        
        int i = 0;
        while (*rest && !isspace((unsigned char)*rest) && *rest != '"' && i < (int)sizeof(fname)-1) {
            fname[i++] = *rest++;
        }
        fname[i] = '\0';

        while (*rest && isspace((unsigned char)*rest)) rest++;

        if (fname[0] == '\0') {
            printf("write: missing filename\n");
            return;
        }

        if (*rest == '"') {
            rest++;
            char *endq = strchr(rest, '"');
            if (!endq) {
                printf("write: missing closing quote\n");
                return;
            }
            size_t copyLen = (size_t)(endq - rest);
            if (copyLen >= sizeof(content)) copyLen = sizeof(content)-1;
            memcpy(content, rest, copyLen);
            content[copyLen] = '\0';
        } else {
            
            strncpy(content, rest, sizeof(content)-1);
            content[sizeof(content)-1] = '\0';
        }

        cmd_write(fname, content);
    } else if (strcmp(cmd, "read") == 0) {
        cmd_read(rest);
    } else if (strcmp(cmd, "delete") == 0) {
        cmd_delete(rest);
    } else if (strcmp(cmd, "rmdir") == 0) {
        cmd_rmdir(rest);
    } else if (strcmp(cmd, "df") == 0) {
        cmd_df();
    } else if (strcmp(cmd, "exit") == 0) {
        printf("Memory released. Exiting program...\n");
        cleanupFS();
        exit(0);
    } else {
        printf("Unknown command: %s\n", cmd);
    }
}

int main(void) {
    initFS();
    printf("Compact VFS - ready. Type 'exit' to quit.\n");

    char line[LINE_BUF];

    while (1) {
        if (file.cwd == file.root) printf("/ > ");
        else printf("%s > ", file.cwd->name);

        if (!fgets(line, sizeof(line), stdin)) break;
        
        line[strcspn(line, "\n")] = '\0';
        handle_line(line);
    }
    cleanupFS();
    return 0;
}
