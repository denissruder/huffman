/* Standard libraries*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/* Implements it's interface: */
#include "compressor.h"

/* Symbol representation */
struct SYM {
    unsigned char ch;      // ASCII-code
    int freq;              // Symbol frequency
    char code[256];        // Array for a new code
    struct SYM *left;      // left child in a tree
    struct SYM *right;     // right child in a tree
};

/* Global variable to keep track of size of the array of pointers allocated by malloc */
int size;

/* Analyzer */
static struct SYM ** analyze(const char *input);

/* Code Generator */
static struct SYM ** generate(struct SYM ** occurence_table);
static struct SYM ** build_tree(struct SYM *occurence_table[], int N);
static void generate_codes(struct SYM *root);

/* Coder */
//static int code();   

/* Packer */
//static int pack();          

/* Utility functions */
static int compare(const void *p1, const void *p2);
static void unix_error(char *msg);

/* Public functions*/
void compress(const char *input, const char *output){
    struct SYM **occurence_table = analyze(input);
    struct SYM **generated_table = generate(occurence_table);
}

/* Analyzer */
static struct SYM ** analyze(const char *input){
    struct SYM **symbol_table = (struct SYM **) malloc(256 * sizeof(struct SYM *));
    size = 0;                                                    
    unsigned char ch[1];                                                                
    FILE *file_ptr; 

    /* Check for Errors while opening a file */
    if ((file_ptr = fopen(input, "r")) == NULL){    
        unix_error("Error opening file");
    }
 
     /* Read and calculate frequency of every ASCII symbol in the file */
    while(fread(&ch, 1, 1, file_ptr) != 0){
        if(symbol_table[*ch] == NULL) {
            /* Initialize symbol */
            symbol_table[*ch] = (struct SYM *) malloc(sizeof(struct SYM));
            symbol_table[*ch]->ch = *ch;
            symbol_table[*ch]->freq = 1;
            *symbol_table[*ch]->code = '\0';
            symbol_table[*ch]->left = NULL;
            symbol_table[*ch]->right = NULL;
            size++;
        } else {
            /* Calculate frequency */
            symbol_table[*ch]->freq++;
        }
    }

    /* Close the file */
    fclose(file_ptr);

    struct SYM **occurence_table = (struct SYM **) malloc(size * sizeof(struct SYM *));

    /* Fill in occurence table and deallocate memory for empty pointers */
    for(int i = 0, j = 0; i < 256 && j != size; i++) {
        if(symbol_table[i] != NULL) {
            occurence_table[j++] = symbol_table[i];
        } else {
            /* Deallocate memory */
            free(symbol_table[i]);
        }
    }

    /* Sort in descending order */
    qsort(occurence_table, size, sizeof(struct SYM *), compare);

    // for(int i = 0; i < size; i++) {
    //     printf("%c %d %s\n", occurence_table[i]->ch, occurence_table[i]->freq, occurence_table[i]->code);
    // }

    return occurence_table;
}

/* Code generator */
static struct SYM ** generate(struct SYM ** occurence_table){
    struct SYM **generated_table = build_tree(occurence_table, size);
    generate_codes(generated_table[0]);

    // for(int i = 0; i < size; i++){
    //     if (generated_table[i]->ch != '\0') {
    //         printf("[%d]: freq: %d %c - code %s \n", i, generated_table[i]->freq, generated_table[i]->ch, generated_table[i]->code);
    //     }
    // }

    return generated_table;
}

static struct SYM ** build_tree(struct SYM **occurence_table, int N){
    struct SYM *temp = (struct SYM*) malloc(sizeof(struct SYM));
    
    /* Sum of frequencies of two last elements */
    temp->freq = occurence_table[N-2]->freq + occurence_table[N-1]->freq;
    
    /* Link temp node with two last nodes */
    temp->left=occurence_table[N-1];
    temp->right=occurence_table[N-2];
    temp->ch = '\0';
    temp->code[0] = 0;

    /* Allocate space in the table for temp */
    occurence_table = (struct SYM **) realloc(occurence_table, ++size * sizeof(struct SYM *));
    occurence_table[size-1] = temp;

    /* Sort in descending order */
    qsort(occurence_table, size, sizeof(struct SYM *), compare);

    /* Root element is set, return */
    if(N == 2) {
        return occurence_table;
    }

    return build_tree(occurence_table, N-1);
}

static void generate_codes(struct SYM *root){
    if(root->left){
        strcpy(root->left->code,root->code);
        strcat(root->left->code,"0");
        generate_codes(root->left);
    } 
    if(root->right){
        strcpy(root->right->code,root->code);
        strcat(root->right->code,"1");
        generate_codes(root->right);
    }
}

/* Utility functions */
static int compare(const void *p1, const void *p2) {
    const struct SYM *sym1 = *(const struct SYM **) p1;
    const struct SYM *sym2 = *(const struct SYM **) p2;
    return sym2->freq - sym1->freq;
}

static void unix_error(char *msg){
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}