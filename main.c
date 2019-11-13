#include "compressor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(const int argc, const char *argv[]) {
    if(argc == 3) {
        if (strcmp(argv[1], "-d") == 0) {
            decompress(argv[2]);
        } else {
            compress(argv[1], argv[2]);
        }
    }

    return 0; 
}