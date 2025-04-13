#include "parseElf.hh"

int main (const int argc, const char** argv) {
    ELF::ELFParser parser;
    parser.parseFile(argc, argv);
    return 0;
}
