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

/* Private functions */

/* Analyzer */
static struct SYM ** analyze(const char *input);       // Statistical analysis of the input file
static int compare(const void *p1, const void *p2);

/* Generator */
//static int generate();      
//static int code();          
//static int pack();          

/* Error reporting function */
void unix_error(char *msg);

void compress(const char *input, const char *output){
    struct SYM **psym = analyze(input);
}

/* Analyzer */
static struct SYM ** analyze(const char *input){
    struct SYM **symbol_table = (struct SYM **) malloc(256 * sizeof(struct SYM *));     // Array of pointers to all ASCII symbols
    unsigned int symbol_counter = 0;                                    // Counter of found symbols
    unsigned char ch[1];                                                // Read buffer
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
            symbol_counter++;
        } else {
            /* Calculate frequency */
            symbol_table[*ch]->freq++;
        }
    }

    /* Close the file */
    fclose(file_ptr);

    struct SYM **occurence_table = (struct SYM **) malloc(symbol_counter * sizeof(struct SYM *));       // Array of pointers of size symbol_counter

    /* Fill in occurence table and deallocate memory for empty pointers */
    for(int i = 0, j = 0; i < 256 && j != symbol_counter; i++) {
        if(symbol_table[i] != NULL) {
            occurence_table[j++] = symbol_table[i];
        } else {
            /* Deallocate memory */
            free(symbol_table[i]);
        }
    }

    /* Sort in ascending order */
    qsort(occurence_table, symbol_counter, sizeof(struct SYM *), compare);

    for(int i = 0; i < symbol_counter; i++) {
        printf("%c %d %s\n", occurence_table[i]->ch, occurence_table[i]->freq, occurence_table[i]->code);
    }

    return occurence_table;
}

static int compare(const void *p1, const void *p2) {
    const struct SYM *sym1 = *(const struct SYM **) p1;
    const struct SYM *sym2 = *(const struct SYM **) p2;
    return sym1->freq - sym2->freq;
}

void unix_error(char *msg){
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}