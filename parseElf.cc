#include "parseElf.hh"
#include <elf.h>
#include <string.h>
#include <sys/stat.h>

namespace ELF {

bool
ELFParser::setOptions(const int& argc, const char* argv[])
{
    if (argc == 2) {
        _opts = 0x01;
        if (checkFile(argv[1])) {
            _f = fopen(argv[1], "r");
            return true;
        } else {
            usage();
            return false;
        }
    }
    _opts = 0;
    for (auto idx = 1; idx < argc; ++idx) {
        if (strncmp(argv[idx], "-h", 2) == 0) {
           _opts = _opts | (0x01 << OPT_HEADER);  
        } else if (strncmp(argv[idx], "-S", 2) == 0) {
           _opts = _opts | (0x01 << OPT_SECTAB);  
        } else if (strncmp(argv[idx], "-s", 2) == 0) {
           _opts = _opts | (0x01 << OPT_SYMTAB);  
        } else {
            if (checkFile(argv[idx])) {
                _f = fopen(argv[idx], "r");
                return true; // terminate successfullly.
            } else {
                usage();
                return false;
            }
        }
    }
    usage();
    return false;
}

void 
ELFParser::usage()
{
    printf("Usage: ");
    printf("[options] filePath.\n");
    printf("\t-h : print ELF header.\n");
    printf("\t-S : print sections.\n");
    printf("\t-s : print symbols.\n");
}

bool
ELFParser::checkFile(const char* filePath)
{
    FILE* f = fopen(filePath, "r");
    if (!f)
        return false;

    // check size
    struct stat s;
    if (stat(filePath, &s) != 0) {
        return false;
    }
    if (s.st_size < 16) {
        return false;
    }
    
    char tmp[16];
    if (fread(tmp, 1, 16, f) < 16) {
        return false;
    }

    if (tmp[0] != 0x7f) {
        return false;
    }
    if (strncmp(tmp + 1, "ELF", 3)) {
        return false;
    }

    if (tmp[EI_CLASS] == ELFCLASS32) {
        _elf64 = false;
    } else if (tmp[EI_CLASS] == ELFCLASS64) {
        _elf64 = true;
    } else {
        // donot support other type
        return false;
    }

    size_t headerSize = _elf64 ? sizeof(Elf64_Ehdr) : sizeof(Elf32_Ehdr);
    if (s.st_size < headerSize) {
        return false;
    }

    fclose(f);
    return true;
}

void
ELFParser::parseFile(const int& argc, const char* argv[])
{
    if (!setOptions(argc, argv)) {
        return;
    }

    if (_elf64) {
        parseInternal<ELF64TypeTraits>();
    } else {
        parseInternal<ELF32TypeTraits>();
    }
    fclose(_f);
}

void 
ELFParser::reportErr()
{
    printf("%s", _err.c_str());
}

const char* 
ELFParser::symTypeStr(char type)
{
    if (type == STT_OBJECT) {
        return "Data Object";
    } else if (type == STT_FUNC) {
        return "Function.";
    } else if (type == STT_SECTION) {
        return "Section.";
    } else if (type == STT_FILE) {
        return "FILE.";
    } else if (type ==STT_NUM) {
        return "Number.";
    } else {
        return "Other type.";
    }
}
}
