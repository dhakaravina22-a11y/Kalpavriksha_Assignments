#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>

#define NAME_LEN 50
#define MAX_PRODUCTS 100
#define LINEBUFFER 256

typedef enum {
    STATUS_SUCCESS = 0,
    STATUS_FAILURE = -1,
    STATUS_NOT_FOUND = 1,
    STATUS_INVALID_INPUT = 2
} StatusCode;

// functions to validate use input 
static void trim_newline(char *s) {
    size_t n = strlen(s);
    if (n > 0 && s[n - 1] == '\n') s[n - 1] = '\0';
}

int is_name_valid(const char *s) {
    for (int i = 0; s[i]; i++) {
        if (isalpha((unsigned char)s[i]))
            return 1;
    }
    return 0;
}

unsigned short read_ushort_range(const char *prompt, unsigned short minv, unsigned short maxv) {
    char line[LINEBUFFER];
    long val;
    char *endp;
    while (1) {
        printf("%s", prompt);
        if (!fgets(line, sizeof(line), stdin)) { clearerr(stdin); continue; }
        trim_newline(line);
        errno = 0;
        val = strtol(line, &endp, 10);
        if (endp == line || errno == ERANGE) {
            printf("Invalid number. Try again.\n");
            continue;
        }
        while (isspace((unsigned char)*endp)) endp++;
        if (*endp != '\0') {
            printf("Trailing characters ignored. Try again.\n");
            continue;
        }
        if (val < minv || val > maxv) {
            printf("Value must be between %hu and %hu.\n", minv, maxv);
            continue;
        }
        return (unsigned short)val;
    }
}

unsigned int read_uint_range(const char *prompt, unsigned int minv, unsigned int maxv) {
    char line[LINEBUFFER];
    unsigned long val;
    char *endp;
    while (1) {
        printf("%s", prompt);
        if (!fgets(line, sizeof(line), stdin)) { clearerr(stdin); continue; }
        trim_newline(line);
        errno = 0;
        val = strtoul(line, &endp, 10);
        if (endp == line || errno == ERANGE) {
            printf("Invalid number. Try again.\n");
            continue;
        }
        while (isspace((unsigned char)*endp)) endp++;
        if (*endp != '\0') {
            printf("Trailing characters ignored. Try again.\n");
            continue;
        }
        if (val < minv || val > maxv) {
            printf("Value must be between %u and %u.\n", minv, maxv);
            continue;
        }
        return (unsigned int)val;
    }
}

float read_float_range(const char *prompt, float minv, float maxv) {
    char line[LINEBUFFER];
    float val;
    char *endp;
    while (1) {
        printf("%s", prompt);
        if (!fgets(line, sizeof(line), stdin)) { clearerr(stdin); continue; }
        trim_newline(line);
        errno = 0;
        val = strtof(line, &endp);
        if (endp == line || errno == ERANGE) {
            printf("Invalid number. Try again.\n");
            continue;
        }
        while (isspace((unsigned char)*endp)) endp++;
        if (*endp != '\0') {
            printf("Trailing characters ignored. Try again.\n");
            continue;
        }
        if (val < minv || val > maxv) {
            printf("Value must be between %.2f and %.2f.\n", minv, maxv);
            continue;
        }
        return val;
    }
}

void read_string(const char *prompt, char *out, size_t maxlen) {
    char line[LINEBUFFER];
    while (1) {
        printf("%s", prompt);
        if (!fgets(line, sizeof(line), stdin)) { clearerr(stdin); continue; }
        trim_newline(line);

        char *ptr = line;
        while (*ptr && isspace((unsigned char)*ptr)) ptr++;

        if (*ptr == '\0') {
            printf("Input cannot be empty. Try again.\n");
            continue;
        }
        if (strlen(ptr) > maxlen) {
            printf("Input too long (max %zu chars). Try again.\n", maxlen);
            continue;
        }
        strcpy(out, ptr);
        return;
    }
}

//add new product
StatusCode addNewProduct(unsigned short **id, char (**name)[NAME_LEN],
                         float **price, unsigned int **quantity, unsigned int *totalProducts) {
    if (*totalProducts >= MAX_PRODUCTS) {
        printf("Cannot add more than %d products.\n", MAX_PRODUCTS);
        return STATUS_FAILURE;
    }

    unsigned int newTotal = *totalProducts + 1;
    *id = realloc(*id, newTotal * sizeof(unsigned short));
    *name = realloc(*name, newTotal * sizeof(**name));
    *price = realloc(*price, newTotal * sizeof(float));
    *quantity = realloc(*quantity, newTotal * sizeof(unsigned int));

    if (!*id || !*name || !*price || !*quantity) {
        fprintf(stderr, "Memory reallocation failed.\n");
        return STATUS_FAILURE;
    }

    printf("\nEnter details for new product:\n");

    unsigned short tmpId;
    while (1) {
        tmpId = read_ushort_range("Enter Product ID: ", 1, 10000);
        bool duplicate = false;
        for (unsigned int i = 0; i < *totalProducts; i++) {
            if ((*id)[i] == tmpId) {
                printf("Product ID %hu already exists. Try again.\n", tmpId);
                duplicate = true;
                break;
            }
        }
        if (!duplicate) break;
    }
    (*id)[newTotal - 1] = tmpId;

    while (1) {
        read_string("Enter Product Name: ", (*name)[newTotal - 1], NAME_LEN - 1);
        if (is_name_valid((*name)[newTotal - 1])) break;
        printf("Invalid name: must contain letters.\n");
    }

    (*price)[newTotal - 1] = read_float_range("Enter Product Price: ", 0.0f, 100000.0f);
    (*quantity)[newTotal - 1] = read_uint_range("Enter Product Quantity: ", 0, 1000000);

    *totalProducts = newTotal;
    printf("Product added successfully!\n");
    return STATUS_SUCCESS;
}

//print all products
void printAllProducts(unsigned short *id, char (*name)[NAME_LEN],
                      float *price, unsigned int *quantity, unsigned int totalProducts) {
    if (totalProducts == 0) {
        printf("No products to show.\n");
        return;
    }
    printf("\n========= PRODUCT LIST =========\n");
    for (unsigned int i = 0; i < totalProducts; i++) {
        printf("ID:%hu | Name:%s | Price:%.2f | Quantity:%u\n",
               id[i], name[i], price[i], quantity[i]);
    }
}

// update product quantity - will take id from user and will take new quanity 
StatusCode updateProductQuantity(unsigned short *id, unsigned int *quantity,
                                 unsigned int totalProducts, unsigned short productId, unsigned int newQuantity) {
    for (unsigned int i = 0; i < totalProducts; i++) {
        if (id[i] == productId) {
            quantity[i] = newQuantity;
            printf("Quantity updated successfully!\n");
            return STATUS_SUCCESS;
        }
    }
    printf("Product with ID %hu not found.\n", productId);
    return STATUS_NOT_FOUND;
}

StatusCode searchByProductId(unsigned short *id, char (*name)[NAME_LEN],
                             float *price, unsigned int *quantity,
                             unsigned int totalProducts, unsigned short searchId) {
    for (unsigned int i = 0; i < totalProducts; i++) {
        if (id[i] == searchId) {
            printf("Product Found: ID:%hu | Name:%s | Price:%.2f | Quantity:%u\n",
                   id[i], name[i], price[i], quantity[i]);
            return STATUS_SUCCESS;
        }
    }
    printf("Product with ID %hu not found.\n", searchId);
    return STATUS_NOT_FOUND;
}

StatusCode searchByProductName(unsigned short *id, char (*name)[NAME_LEN],
                               float *price, unsigned int *quantity,
                               unsigned int totalProducts, const char *partialName) {
    bool found = false;
    printf("\nProducts Found:\n");
    for (unsigned int i = 0; i < totalProducts; i++) {
        if (strstr(name[i], partialName)) {
            printf("Product ID:%hu | Name:%s | Price:%.2f | Quantity:%u\n",
                   id[i], name[i], price[i], quantity[i]);
            found = true;
        }
    }
    if (!found) {
        printf("No products found containing \"%s\".\n", partialName);
        return STATUS_NOT_FOUND;
    }
    return STATUS_SUCCESS;
}

StatusCode searchByPriceRange(unsigned short *id, char (*name)[NAME_LEN],
                              float *price, unsigned int *quantity,
                              unsigned int totalProducts, float minPrice, float maxPrice) {
    bool found = false;
    printf("\nProducts in price range %.2f - %.2f:\n", minPrice, maxPrice);
    for (unsigned int i = 0; i < totalProducts; i++) {
        if (price[i] >= minPrice && price[i] <= maxPrice) {
            printf("Product ID:%hu | Name:%s | Price:%.2f | Quantity:%u\n",
                   id[i], name[i], price[i], quantity[i]);
            found = true;
        }
    }
    if (!found) {
        printf("No products found in that range.\n");
        return STATUS_NOT_FOUND;
    }
    return STATUS_SUCCESS;
}


StatusCode deleteProduct(unsigned short **id, char (**name)[NAME_LEN],
                         float **price, unsigned int **quantity,
                         unsigned int *totalProducts, unsigned short productId) {
    int idx = -1;
    for (unsigned int i = 0; i < *totalProducts; i++) {
        if ((*id)[i] == productId) { idx = i; break; }
    }
    if (idx == -1) {
        printf("Product with ID %hu not found.\n", productId);
        return STATUS_NOT_FOUND;
    }

    for (unsigned int i = idx; i < *totalProducts - 1; i++) {
        (*id)[i] = (*id)[i + 1];
        strcpy((*name)[i], (*name)[i + 1]);
        (*price)[i] = (*price)[i + 1];
        (*quantity)[i] = (*quantity)[i + 1];
    }

    (*totalProducts)--;
    *id = realloc(*id, (*totalProducts) * sizeof(unsigned short));
    *name = realloc(*name, (*totalProducts) * sizeof(**name));
    *price = realloc(*price, (*totalProducts) * sizeof(float));
    *quantity = realloc(*quantity, (*totalProducts) * sizeof(unsigned int));

    printf("Product with ID %hu deleted successfully!\n", productId);
    return STATUS_SUCCESS;
}

// main function

int main(void) {
    printf("===== Dynamic Inventory Management System =====\n");

    unsigned int initialQuantity = read_uint_range("Enter the initial number of products: ", 1, 100);
    unsigned int totalProducts = initialQuantity;

    unsigned short *id = calloc(initialQuantity, sizeof(unsigned short));
    char (*name)[NAME_LEN] = calloc(initialQuantity, sizeof(*name));
    float *price = calloc(initialQuantity, sizeof(float));
    unsigned int *quantity = calloc(initialQuantity, sizeof(unsigned int));

    if (!id || !name || !price || !quantity) {
        fprintf(stderr, "Memory allocation failed.\n");
        free(id); free(name); free(price); free(quantity);
        return EXIT_FAILURE;
    }

    for (unsigned int i = 0; i < initialQuantity; i++) {
        printf("\nEnter details for product %u\n", i + 1);

        while (1) {
            id[i] = read_ushort_range("Enter Product ID: ", 1, 10000);
            bool duplicate = false;
            for (unsigned int j = 0; j < i; j++) {
                if (id[j] == id[i]) { duplicate = true; break; }
            }
            if (!duplicate) break;
            printf("Product ID already exists. Try again.\n");
        }

        while (1) {
            read_string("Enter Product Name: ", name[i], NAME_LEN - 1);
            if (is_name_valid(name[i])) break;
            printf("Invalid name: must contain at least one alphabetic letter.\n");
        }

        price[i] = read_float_range("Enter Product Price: ", 0.0f, 100000.0f);
        quantity[i] = read_uint_range("Enter Product Quantity: ", 0, 1000000);
    }

    while (true) {
        printf("\n========= INVENTORY MENU =========\n"
               "1. Add New Product\n"
               "2. View All Products\n"
               "3. Update Quantity\n"
               "4. Search Product by ID\n"
               "5. Search Product by Name\n"
               "6. Search Product by Price Range\n"
               "7. Delete Product\n"
               "8. Exit\n");

        unsigned short choice = read_ushort_range("Enter your choice: ", 1, 8);
        if (choice == 8) {
            printf("Exiting program and freeing memory...\n");
            break;
        }

        switch (choice) {
            case 1:
                addNewProduct(&id, &name, &price, &quantity, &totalProducts);
                break;
            case 2:
                printAllProducts(id, name, price, quantity, totalProducts);
                break;
            case 3: {
                unsigned short pid = read_ushort_range("Enter Product ID: ", 1, 10000);
                unsigned int newQ = read_uint_range("Enter New Quantity: ", 0, 1000000);
                updateProductQuantity(id, quantity, totalProducts, pid, newQ);
                break;
            }
            case 4: {
                unsigned short pid = read_ushort_range("Enter Product ID to search: ", 1, 10000);                searchByProductId(id, name, price, quantity, totalProducts, pid);
                break;
            }

            case 5: {
                char partial[NAME_LEN];
                read_string("Enter name to search (partial allowed): ", partial, NAME_LEN - 1);
                searchByProductName(id, name, price, quantity, totalProducts, partial);
                break;
            }
            case 6: {
                float minP = read_float_range("Enter minimum price: ", 0.0f, 100000.0f);
                float maxP = read_float_range("Enter maximum price: ", 0.0f, 100000.0f);
                if (minP > maxP) { float tmp = minP; minP = maxP; maxP = tmp; }
                searchByPriceRange(id, name, price, quantity, totalProducts, minP, maxP);
                break;
            }

            case 7: {
                unsigned short pid = read_ushort_range("Enter Product ID to delete: ", 1, 10000);
                deleteProduct(&id, &name, &price, &quantity, &totalProducts, pid);
                break;
            }
        }
    }

    free(id);
    free(name);
    free(price);
    free(quantity);

    return 0;
}
