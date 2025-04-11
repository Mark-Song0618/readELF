#include "parseElf.hh"

bool checkParam(const int argc) {
    if (argc != 2) {
        printf("Please Name a file.\n");
        return false;
    }
    return true;
}

int main (const int argc, const char** argv) {
    if (!checkParam(argc)) {
        return -1;
    }

    ELF::ELFParser parser;
    if (!parser.checkFile(argv[1])) {
        parser.reportErr();
    } else {
        parser.parseFile(argv[1]);
    }
    return 0;
}
