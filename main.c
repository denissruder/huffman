#include "compressor.h"

int main(const int argc, const char *argv[]) {
    if(argc == 3) {
        compress(argv[1], argv[2]);
    }

    return 0;
}