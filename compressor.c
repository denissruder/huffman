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

/* Union for packing */
union CODE {
    unsigned char ch;
    struct {
        unsigned short b1:1;
        unsigned short b2:1;
        unsigned short b3:1;
        unsigned short b4:1;
        unsigned short b5:1;
        unsigned short b6:1;
        unsigned short b7:1;
        unsigned short b8:1;
    } byte;
};

/* Global variable to keep track of size of the array of pointers allocated by malloc */
int size;
int occurence_table_size;

/* Analyzer */
static struct SYM ** analyze(const char *input);

/* Code Generator */
static struct SYM ** generate(struct SYM **occurence_table);
static struct SYM ** build_tree(struct SYM *occurence_table[], int N);
static void generate_codes(struct SYM *root);

/* Coder */
static void code(const char *input, const char *output, struct SYM **generated_table);   

/* Packer */    
static void pack(const char *input, const char *temp, const char *output, struct SYM **generated_table);  
static unsigned char pack_byte(unsigned char buf[]);        

/* Utility functions */
static int compare(const void *p1, const void *p2);
static const char *get_filename_ext(const char *filename);
static void unix_error(char *msg);


/* Public functions*/
void compress(const char *input, const char *output) {
    struct SYM **occurence_table = analyze(input);
    struct SYM **generated_table = generate(occurence_table);
    code(input, "temp.txt", generated_table);
    pack(input, "temp.txt", output, generated_table);
}

/* Analyzer */
static struct SYM ** analyze(const char *input) {
    struct SYM **symbol_table = (struct SYM **) malloc(256 * sizeof(struct SYM *));
    size = 0;
    occurence_table_size = 0;                                                    
    unsigned char ch[1];                                                                
    FILE *file_ptr; 

    /* Check for Errors while opening a file */
    if ((file_ptr = fopen(input, "r")) == NULL) {    
        unix_error("Error opening file");
    }
 
     /* Read and calculate frequency of every ASCII symbol in the file */
    while(fread(&ch, 1, 1, file_ptr) != 0) {
        if (symbol_table[*ch] == NULL) {
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
    occurence_table_size = size;

    /* Fill in occurence table and deallocate memory for empty pointers */
    for(int i = 0, j = 0; i < 256 && j != size; i++) {
        if (symbol_table[i] != NULL) {
            occurence_table[j++] = symbol_table[i];
        } else {
            /* Deallocate memory */
            free(symbol_table[i]);
        }
    }

    /* Sort in descending order */
    qsort(occurence_table, size, sizeof(struct SYM *), compare);

    for(int i = 0; i < size; i++) {
        printf("%c %d %s\n", occurence_table[i]->ch, occurence_table[i]->freq, occurence_table[i]->code);
    }

    return occurence_table;
}

/* Code generator */
static struct SYM ** generate(struct SYM **occurence_table) {
    struct SYM **generated_table = build_tree(occurence_table, size);
    generate_codes(generated_table[0]);

    for(int i = 0; i < size; i++) {
        if (generated_table[i]->ch != '\0') {
            printf("[%d]: freq: %d %c - code %s \n", i, generated_table[i]->freq, generated_table[i]->ch, generated_table[i]->code);
        }
    }

    return generated_table;
}

static struct SYM ** build_tree(struct SYM **occurence_table, int N) {
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
    if (N == 2) {
        return occurence_table;
    }

    return build_tree(occurence_table, N-1);
}

static void generate_codes(struct SYM *root) {
    if (root->left) {
        strcpy(root->left->code,root->code);
        strcat(root->left->code,"0");
        generate_codes(root->left);
    } 
    if (root->right) {
        strcpy(root->right->code,root->code);
        strcat(root->right->code,"1");
        generate_codes(root->right);
    }
}

/* Coder */
static void code(const char *input, const char *output, struct SYM **generated_table){
    char *codes[256] = {0};
    unsigned char ch[1];                                                                
    FILE *input_file_ptr; 
    FILE *output_file_ptr; 

    for(int i = 0; i < size; i++) {
        if (generated_table[i]->ch != '\0') {
            codes[generated_table[i]->ch] = generated_table[i]->code;
        }
    }

    // for(int i = 0; i < 256; i++) {
    //     if(codes[i] != NULL) {
    //         printf("codes[%d]: %s\n", i, codes[i]);
    //     }
    // }

    /* Check for Errors while opening a file */
    if ((input_file_ptr = fopen(input, "rb")) == NULL 
            || (output_file_ptr = fopen(output, "wb")) == NULL) {    
        unix_error("Error opening file");
    }
 
    /* Read input file and append generated code replacing symbol */
    while(fread(&ch, 1, 1, input_file_ptr) != 0) {
        if (codes[*ch] == NULL) {
            unix_error("Error inside coder");
        }
        fprintf(output_file_ptr, "%s", codes[*ch]);
    }

    fclose(input_file_ptr);
    fclose(output_file_ptr);
}

/* Packer */
static void pack(const char *input, const char *temp, const char *output, struct SYM **generated_table) {
    /* Header fields [Signature, Number of Unique symbols, [Symbol code and frequency pairs], Tail size, [Encoded symbols]*/
    unsigned char *signature = "DR";
    unsigned char buf[8];  

    FILE *input_file_ptr; 
    FILE *temp_file_ptr; 
    FILE *output_file_ptr; 

    if ((input_file_ptr = fopen(input, "rb")) == NULL 
            || (temp_file_ptr = fopen(temp, "rb")) == NULL 
            || (output_file_ptr = fopen(output, "wb")) == NULL) {    
        unix_error("Error opening file");
    }

    fprintf(output_file_ptr, "%s", signature);                          // Signature
    printf("signature %s\n", signature);
    fprintf(output_file_ptr, "%d", occurence_table_size);               // Number of unique symbols
    printf("number of unique symbols %d\n", occurence_table_size);

    for(int i = 0; i < size; i++) {
        if (generated_table[i]->ch != '\0') {
            fprintf(output_file_ptr, "%c", generated_table[i]->ch);     // Symbol code
            printf("symbol code %c\n", generated_table[i]->ch);
            fprintf(output_file_ptr, "%d", generated_table[i]->freq);   // Frequency
            printf("frequency: %d\n", generated_table[i]->freq);
        }
    }
    /* Temp and input file sizes */
    fseek(temp_file_ptr, 0L, SEEK_END);
    int temp_file_size = ftell(temp_file_ptr);
    rewind(temp_file_ptr);
    fseek(input_file_ptr, 0L, SEEK_END);
    int input_file_size = ftell(input_file_ptr);

    int tail_size = 8 - temp_file_size % 8;
    fprintf(output_file_ptr, "%d", tail_size);                         // Tail size
    printf("temp file size %d\n", temp_file_size);
    printf("tail %d\n", tail_size);
    fprintf(output_file_ptr, "%d", input_file_size);                   // Input file size
    printf("input file size %d\n", input_file_size);
    const char *extension = get_filename_ext(input);
    fprintf(output_file_ptr, "%s", extension);                         // Input file extension
    printf("input file size %s\n", extension);

    unsigned char ch;
    while(fread(&buf, 1, 8, temp_file_ptr) != 0) {
        ch = pack_byte(buf);
        fprintf(output_file_ptr, "%c", ch);
    }

    fclose(input_file_ptr);
    fclose(temp_file_ptr);
    fclose(output_file_ptr);
}

unsigned char pack_byte(unsigned char buf[]){
    union CODE code;
    code.byte.b1=buf[0]-'0';
    code.byte.b2=buf[1]-'0';
    code.byte.b3=buf[2]-'0';
    code.byte.b4=buf[3]-'0';
    code.byte.b5=buf[4]-'0';
    code.byte.b6=buf[5]-'0';
    code.byte.b7=buf[6]-'0';
    code.byte.b8=buf[7]-'0';
    return code.ch;
}

/* Utility functions */
static int compare(const void *p1, const void *p2) {
    const struct SYM *sym1 = *(const struct SYM **) p1;
    const struct SYM *sym2 = *(const struct SYM **) p2;
    return sym2->freq - sym1->freq;
}

static const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

static void unix_error(char *msg) {
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}