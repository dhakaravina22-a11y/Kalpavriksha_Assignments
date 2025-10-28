#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>

#define NAME_LEN 50
#define MAX_PRODUCTS 100
#define LINEBUFFER 256

typedef struct {
    unsigned short id;
    char name[NAME_LEN];
    float price;
    unsigned int quantity;
} Product;


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
            printf("Invalid input. Try again.\n");
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
            printf("Invalid input. Try again.\n");
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
            printf("Invalid input. Try again.\n");
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


void addNewProduct(Product **products, unsigned int *totalProducts) {
    if (*totalProducts >= MAX_PRODUCTS) {
        printf("Cannot add more than %d products.\n", MAX_PRODUCTS);
        return;
    }

    Product newProduct;
    printf("\nEnter details for new product:\n");

    while (1) {
        newProduct.id = read_ushort_range("Enter Product ID: ", 1, 10000);
        bool duplicate = false;
        for (unsigned int i = 0; i < *totalProducts; i++) {
            if ((*products)[i].id == newProduct.id) {
                printf("Product ID already exists. Try again.\n");
                duplicate = true;
                break;
            }
        }
        if (!duplicate) break;
    }

    while (1) {
        read_string("Enter Product Name: ", newProduct.name, NAME_LEN - 1);
        if (is_name_valid(newProduct.name)) break;
        printf("Invalid name: must contain letters.\n");
    }

    newProduct.price = read_float_range("Enter Product Price: ", 0.0f, 100000.0f);
    newProduct.quantity = read_uint_range("Enter Product Quantity: ", 0, 1000000);

    Product *temp = realloc(*products, (*totalProducts + 1) * sizeof(Product));
    if (!temp) {
        printf("Memory reallocation failed.\n");
        return;
    }

    *products = temp;
    (*products)[*totalProducts] = newProduct;
    (*totalProducts)++;

    printf("Product added successfully!\n");
}

void printAllProducts(Product *products, unsigned int totalProducts) {
    if (totalProducts == 0) {
        printf("No products to show.\n");
        return;
    }
    printf("\n========= PRODUCT LIST =========\n");
    for (unsigned int i = 0; i < totalProducts; i++) {
        printf("ID:%hu | Name:%s | Price:%.2f | Quantity:%u\n",
               products[i].id, products[i].name, products[i].price, products[i].quantity);
    }
}

void updateProductQuantity(Product *products, unsigned int totalProducts,
                           unsigned short productId, unsigned int newQuantity) {
    for (unsigned int i = 0; i < totalProducts; i++) {
        if (products[i].id == productId) {
            products[i].quantity = newQuantity;
            printf("Quantity updated successfully!\n");
            return;
        }
    }
    printf("Product with ID %hu not found.\n", productId);
}

void searchByProductId(Product *products, unsigned int totalProducts, unsigned short searchId) {
    for (unsigned int i = 0; i < totalProducts; i++) {
        if (products[i].id == searchId) {
            printf("Product Found: ID:%hu | Name:%s | Price:%.2f | Quantity:%u\n",
                   products[i].id, products[i].name, products[i].price, products[i].quantity);
            return;
        }
    }
    printf("Product with ID %hu not found.\n", searchId);
}

void searchByProductName(Product *products, unsigned int totalProducts, const char *partialName) {
    bool found = false;
    printf("\nProducts Found:\n");
    for (unsigned int i = 0; i < totalProducts; i++) {
        if (strstr(products[i].name, partialName)) {
            printf("ID:%hu | Name:%s | Price:%.2f | Quantity:%u\n",
                   products[i].id, products[i].name, products[i].price, products[i].quantity);
            found = true;
        }
    }
    if (!found)
        printf("No products found containing \"%s\".\n", partialName);
}

void searchByPriceRange(Product *products, unsigned int totalProducts, float minPrice, float maxPrice) {
    bool found = false;
    printf("\nProducts in price range %.2f - %.2f:\n", minPrice, maxPrice);
    for (unsigned int i = 0; i < totalProducts; i++) {
        if (products[i].price >= minPrice && products[i].price <= maxPrice) {
            printf("ID:%hu | Name:%s | Price:%.2f | Quantity:%u\n",
                   products[i].id, products[i].name, products[i].price, products[i].quantity);
            found = true;
        }
    }
    if (!found)
        printf("No products found in that range.\n");
}

void deleteProduct(Product **products, unsigned int *totalProducts, unsigned short productId) {
    int idx = -1;
    for (unsigned int i = 0; i < *totalProducts; i++) {
        if ((*products)[i].id == productId) { idx = i; break; }
    }
    if (idx == -1) {
        printf("Product with ID %hu not found.\n", productId);
        return;
    }

    for (unsigned int i = idx; i < *totalProducts - 1; i++) {
        (*products)[i] = (*products)[i + 1];
    }

    Product *temp = realloc(*products, (*totalProducts - 1) * sizeof(Product));
    if (!temp && *totalProducts > 1) {
        printf("Memory reallocation failed.\n");
        return;
    }

    *products = temp;
    (*totalProducts)--;

    printf("Product with ID %hu deleted successfully!\n", productId);
}

// Main function
int main() {
    printf("===== Dynamic Inventory Management System=====\n");

    unsigned int totalProducts = read_uint_range("Enter the initial number of products: ", 1, 100);
    Product *products = calloc(totalProducts, sizeof(Product));
    if (!products) {
        fprintf(stderr, "Memory allocation failed.\n");
        return EXIT_FAILURE;
    }

    for (unsigned int i = 0; i < totalProducts; i++) {
        printf("\nEnter details for product %u\n", i + 1);

        while (1) {
            products[i].id = read_ushort_range("Enter Product ID: ", 1, 10000);
            bool duplicate = false;
            for (unsigned int j = 0; j < i; j++) {
                if (products[j].id == products[i].id) { duplicate = true; break; }
            }
            if (!duplicate) break;
            printf("Product ID already exists. Try again.\n");
        }

        while (1) {
            read_string("Enter Product Name: ", products[i].name, NAME_LEN - 1);
            if (is_name_valid(products[i].name)) break;
            printf("Invalid name: must contain letters.\n");
        }

        products[i].price = read_float_range("Enter Product Price: ", 0.0f, 100000.0f);
        products[i].quantity = read_uint_range("Enter Product Quantity: ", 0, 1000000);
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
            printf("Exiting program..\n");
            break;
        }

        switch (choice) {
            case 1:
                addNewProduct(&products, &totalProducts);
                break;
            case 2:
                printAllProducts(products, totalProducts);
                break;
            case 3: {
                unsigned short pid = read_ushort_range("Enter Product ID: ", 1, 10000);
                unsigned int newQ = read_uint_range("Enter New Quantity: ", 0, 1000000);
                updateProductQuantity(products, totalProducts, pid, newQ);
                break;
            }
            case 4: {
                unsigned short pid = read_ushort_range("Enter Product ID to search: ", 1, 10000);
                searchByProductId(products, totalProducts, pid);
                break;
            }
            case 5: {
                char partial[NAME_LEN];
                read_string("Enter name to search (partial allowed): ", partial, NAME_LEN - 1);
                searchByProductName(products, totalProducts, partial);
                break;
            }
            case 6: {
                float minP = read_float_range("Enter minimum price: ", 0.0f, 100000.0f);
                float maxP = read_float_range("Enter maximum price: ", 0.0f, 100000.0f);
                if (minP > maxP) { float tmp = minP; minP = maxP; maxP = tmp; }
                searchByPriceRange(products, totalProducts, minP, maxP);
                break;
            }
            case 7: {
                unsigned short pid = read_ushort_range("Enter Product ID to delete: ", 1, 10000);
                deleteProduct(&products, &totalProducts, pid);
                break;
            }
        }
    }

    free(products);
    return 0;
}
