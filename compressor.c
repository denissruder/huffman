/* Standard libraries*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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

/* Coder */
static void code(const char *input, const char *output, struct SYM **generated_table);   

/* Packer */    
static void pack(const char *input, const char *temp, const char *output, struct SYM **generated_table);  
   

/* Utility functions */
static struct SYM ** build_tree(struct SYM *occurence_table[], int N);
static unsigned char pack_byte(unsigned char buf[]);     
static void generate_codes(struct SYM *root);
static int compare(const void *p1, const void *p2);
static const char *get_filename_ext(const char *filename);
static int ilog10c(unsigned x);
static void unix_error(char *msg);
static void strip_ext(char *fname);

/* Public functions*/
void compress(const char *input, const char *output) {
    struct SYM **occurence_table = analyze(input);
    struct SYM **generated_table = generate(occurence_table);
    code(input, "temp.txt", generated_table);
    pack(input, "temp.txt", output, generated_table);
}

void decompress(const char *input) {
    unsigned char signature[3];     // Signature
    unsigned char size_buf[3]; // Number of unique symbols buf
    unsigned char ch[1];
    unsigned long ch_size = 0;
    unsigned char curr_ch;
    unsigned tail = 0;
    unsigned long file_size = 0;
    unsigned char *extension;
    FILE *file_ptr;

    /* Check for Errors while opening a file */
    if ((file_ptr = fopen(input, "r")) == NULL) {    
        unix_error("Error opening file");
    }

    /* Read Signature and check it */
    fread(&signature, 1, 2, file_ptr);
    signature[2] = '\0';

    if (strcmp(signature, "DR") != 0) {
        unix_error("Wrong signature, this file wasn't compressed by this compressor.");
    }

    /* Read Number of unique symbols */
    fread(&size_buf, 1, 3, file_ptr);
    size = atoi(size_buf);
    
    /* Read Symbol code and Frequency */
    struct SYM **occurence_table = (struct SYM **) malloc(size * sizeof(struct SYM *));
    for(int i = 0; i < size; i++) {
        /* Read Symbol */
        fread(&ch, 1, 1, file_ptr);
        occurence_table[i] = (struct SYM *) malloc(sizeof(struct SYM));
        occurence_table[i]->ch = *ch;;
        *occurence_table[i]->code = '\0';
        occurence_table[i]->left = NULL;
        occurence_table[i]->right = NULL;

        /* Read frequency size in bytes */
        fread(&ch, 1, 1, file_ptr);
        ch_size = atoi(ch);

        /* Read frequency */        
        unsigned char freq_string[ch_size];
        fread(&freq_string, 1, ch_size, file_ptr);
        occurence_table[i]->freq = atoi(freq_string);
    }

    /* Read tail */
    fread(&ch, 1, 1, file_ptr);
    tail = *ch - '0';

    /* Read Input file size */
    fread(&ch, 1, 1, file_ptr);
    int fsize = *ch - '0';
    unsigned char fsize_buf[fsize];
    fread(&fsize_buf, 1, fsize, file_ptr);
    file_size = atoi(fsize_buf);

    /* Read Input file extension */
    fread(&ch, 1, 1, file_ptr);
    unsigned ext_size = *ch - '0';
    unsigned char ext[ext_size + 1];
    fread(&ext, 1, ext_size, file_ptr);
    ext[ext_size] = '\0';

    qsort(occurence_table, size, sizeof(struct SYM *), compare);
    struct SYM **generated_table = build_tree(occurence_table, size);
    generate_codes(generated_table[0]);
                                    
    FILE *temp_file_ptr;
    FILE *output_file_ptr;
    
    // READ from left to right and concatenate to string
    // char *input_no_ext = input;
    // strip_ext(input_no_ext);
    // int input_no_ext_length = strlen(input_no_ext);
    // char output[input_no_ext_length + ext_size + 2];

    // for(int i = 0; i < input_no_ext_length; i++) {
    //     output[i] = input_no_ext[i];
    // }
    // output[input_no_ext_length] = '.';
    // for(int i = input_no_ext_length + 1; i < (input_no_ext_length + ext_size + 1) ; i++) {
    //     output[i] = ext[i-(input_no_ext_length + 1)];
    // }
    // output[input_no_ext_length + ext_size + 1] = '\0';
    // printf("%s\n", output);

    if ((temp_file_ptr = fopen("temp.txt", "wb+")) == NULL) {    
        unix_error("Error opening file");
    }

    union CODE code;
    while(fread(&ch, 1, 1, file_ptr) != 0) {
        code.ch = *ch;
        fprintf(temp_file_ptr, "%d", code.byte.b1);     
        fprintf(temp_file_ptr, "%d", code.byte.b2);     
        fprintf(temp_file_ptr, "%d", code.byte.b3);     
        fprintf(temp_file_ptr, "%d", code.byte.b4);     
        fprintf(temp_file_ptr, "%d", code.byte.b5);     
        fprintf(temp_file_ptr, "%d", code.byte.b6);     
        fprintf(temp_file_ptr, "%d", code.byte.b7);     
        fprintf(temp_file_ptr, "%d", code.byte.b8);     
    }
    
    int temp_file_size = ftell(temp_file_ptr);
    rewind(temp_file_ptr);

    int temp_file_size_no_tail = temp_file_size - tail;

    if ((output_file_ptr = fopen("decoded.txt", "wb")) == NULL) {    
        unix_error("Error opening file");
    }

    unsigned char c;
    char str[256];
    int str_pointer = 0;
    
    for(int i = 0; i < temp_file_size_no_tail; i++) {
        c = getc(temp_file_ptr);
        str[str_pointer++] = c;
        str[str_pointer] = '\0';
        for(int j = 0; j < size; j++) {
            if(generated_table[j]->ch != '\0') {
                if(strcmp(generated_table[j]->code, str) == 0) {
                    fprintf(output_file_ptr, "%c", generated_table[j]->ch); 
                    str_pointer = 0;
                    str[0] = '\0';
                    break;
                }
            }
        }
    }

    int output_file_size = ftell(output_file_ptr);

    fclose(temp_file_ptr);
    fclose(output_file_ptr);
    fclose(file_ptr);

    /* Check file sizes */
    if(output_file_size != file_size) {
        unix_error("Decoded file size is different.");
    }
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

    return occurence_table;
}

/* Code generator */
static struct SYM ** generate(struct SYM **occurence_table) {
    struct SYM **generated_table = build_tree(occurence_table, size);
    generate_codes(generated_table[0]);

    return generated_table;
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

    fprintf(output_file_ptr, "%s", signature);                                      // (0s) + Signature

    if (occurence_table_size < 10) {
        fprintf(output_file_ptr, "%s", "00"); 
    } else if (occurence_table_size >= 10 && occurence_table_size < 100) {
        fprintf(output_file_ptr, "%s", "0"); 
    }

    fprintf(output_file_ptr, "%d", occurence_table_size);                           // Number of unique symbols

    for(int i = 0; i < size; i++) {
        if (generated_table[i]->ch != '\0') {
            fprintf(output_file_ptr, "%c", generated_table[i]->ch);                 // Symbol code
            fprintf(output_file_ptr, "%d", ilog10c(generated_table[i]->freq)+1);    // Frequency integer size in bytes
            fprintf(output_file_ptr, "%d", generated_table[i]->freq);               // Frequency
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
    fprintf(output_file_ptr, "%d", ilog10c(input_file_size) +1);       // Input file integer size
    fprintf(output_file_ptr, "%d", input_file_size);                   // Input file size in bytes
    const char *extension = get_filename_ext(input);
    fprintf(output_file_ptr, "%ld", strlen(extension)); 
    fprintf(output_file_ptr, "%s", extension);                         // Input file extension

    unsigned char ch;
    while(fread(&buf, 1, 8, temp_file_ptr) != 0) {
        ch = pack_byte(buf);
        fprintf(output_file_ptr, "%c", ch);
    }

    for(int i = 0; i < size; i++) {
        free(generated_table[i]);
    }

    fclose(input_file_ptr);
    fclose(temp_file_ptr);
    fclose(output_file_ptr);
}

/* Utility functions */
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
static int compare(const void *p1, const void *p2) {
    const struct SYM *sym1 = *(const struct SYM **) p1;
    const struct SYM *sym2 = *(const struct SYM **) p2;
    return sym2->freq - sym1->freq;
}

static const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        return "";
    }
    return dot + 1;
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

static int ilog10c(unsigned x) {
   if (x > 99)
      if (x < 1000000)
         if (x < 10000)
            return 3 + ((int)(x - 1000) >> 31);
         else
            return 5 + ((int)(x - 100000) >> 31);
      else
         if (x < 100000000)
            return 7 + ((int)(x - 10000000) >> 31);
         else
            return 9 + ((int)((x-1000000000)&~x) >> 31);
   else
      if (x > 9)
            return 1;
      else
            return ((int)(x - 1) >> 31);
}

static void strip_ext(char *fname) {
    char *end = fname + strlen(fname);

    while(end > fname && *end != '.') {
        --end;
    }

    if (end > fname) {
        *end = '\0';
    }
}

static void unix_error(char *msg) {
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}