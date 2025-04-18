#pragma once

#include <elf.h>
#include <string>
#include <stdio.h>

namespace ELF {

#define OPT_HEADER 0
#define OPT_SECTAB 1
#define OPT_SYMTAB 2

// todo: option support: -h -s .etc
class ELFParser {
public:
    bool setOptions(const int& argc, const char* argv[]);

    bool checkFile(const char* filePath);

    void parseFile(const int& argc, const char* argv[]);

    void reportErr();

private:
    class ELF32TypeTraits {
    public:
        using HeaderType = Elf32_Ehdr;
        using SecEntryType = Elf32_Shdr;
        using SymType = Elf32_Sym;
    };
    class ELF64TypeTraits {
    public:
        using HeaderType = Elf64_Ehdr;
        using SecEntryType = Elf64_Shdr;
        using SymType = Elf64_Sym;
    };

private:
    // caller duety to free 
    template<typename ELFTypeTraits> 
    typename ELFTypeTraits::HeaderType* readELFHeader();
    template<typename ELFTypeTraits> 
    typename ELFTypeTraits::SecEntryType* readSecTable(const typename ELFTypeTraits::HeaderType* header);
    template<typename ELFTypeTraits> 
    typename ELFTypeTraits::SymType* readSymTab(const typename ELFTypeTraits::HeaderType* header, unsigned& cnt, bool dynamic);
    template<typename ELFTypeTraits> 
    char* readStrTab(const typename ELFTypeTraits::HeaderType* header);
    template<typename ELFTypeTraits> 
    char* readSecStrTab(const typename ELFTypeTraits::HeaderType* header);

    template<typename ELFTypeTraits> 
    void parseInternal();
    template<typename ELFTypeTraits> 
    void parseIdent(const typename ELFTypeTraits::HeaderType* header);
    template<typename ELFTypeTraits> 
    void parseType(const typename ELFTypeTraits::HeaderType* header);
    template<typename ELFTypeTraits> 
    void parseMachine(const typename ELFTypeTraits::HeaderType* header);
    template<typename ELFTypeTraits> 
    void parseSymTab(const typename ELFTypeTraits::HeaderType* header);
    template<typename ELFTypeTraits> 
    void parseSymbols(const typename ELFTypeTraits::HeaderType* header, typename ELFTypeTraits::SymType* first, unsigned cnt);
    template<typename ELFTypeTraits> 
    void parseSections(const typename ELFTypeTraits::HeaderType* header);
    template<typename ELFTypeTraits>
    void printSecInfo(typename ELFTypeTraits::SecEntryType* secEntry, const char* strTab);

    const char* symTypeStr(char type);

    void usage();
private:
    FILE*                   _f;
    std::string             _err;
    bool                    _elf64;
    unsigned char           _opts = 0x01;
};

template<typename ELFTypeTraits> 
typename ELFTypeTraits::HeaderType* 
ELFParser::readELFHeader()
{
    unsigned headerSize = sizeof(typename ELFTypeTraits::HeaderType);
    typename ELFTypeTraits::HeaderType *header = (typename ELFTypeTraits::HeaderType*)malloc(headerSize);
    fseek(_f, 0, SEEK_SET);
    int readBytes = fread(header, 1, headerSize, _f);
    if (headerSize> readBytes) {
        free(header);
        header = nullptr;
    }
    return header;
}

template<typename ELFTypeTraits> 
typename ELFTypeTraits::SecEntryType* 
ELFParser::readSecTable(const typename ELFTypeTraits::HeaderType* header)
{
    unsigned secTabSize = header->e_shentsize * header->e_shnum;
    void* secTab = malloc(secTabSize);
    fseek(_f, header->e_shoff, SEEK_SET);
    unsigned readBytes = fread(secTab, 1, secTabSize, _f);
    if (readBytes < secTabSize) {
        free(secTab);
        secTab = nullptr;
    }
    return (typename ELFTypeTraits::SecEntryType*)secTab;
}

template<typename ELFTypeTraits> 
char* 
ELFParser::readSecStrTab(const typename ELFTypeTraits::HeaderType* header)
{
    typename ELFTypeTraits::SecEntryType* secTab = readSecTable<ELFTypeTraits>(header);
    if (!secTab) {
        return nullptr;
    }

    typename ELFTypeTraits::SecEntryType* secStrEntry = secTab + header->e_shstrndx;
    unsigned strTabSize = secStrEntry->sh_size;
    unsigned strTabOff  = secStrEntry->sh_offset;
    void* strTab = malloc(strTabSize);
    fseek(_f, strTabOff, SEEK_SET);
    unsigned readBytes = fread(strTab, 1, strTabSize, _f);
    if (strTabSize > readBytes) {
        free(strTab);
        return nullptr;
    }
    return (char*)strTab;
}

template<typename ELFTypeTraits> 
char* 
ELFParser::readStrTab(const typename ELFTypeTraits::HeaderType* header)
{
    typename ELFTypeTraits::SecEntryType* secTab = readSecTable<ELFTypeTraits>(header);
    if (!secTab) {
        return nullptr;
    }
    
    char* strTab = nullptr;
    typename ELFTypeTraits::SecEntryType* curr = nullptr;
    for (unsigned idx = 0; idx != header->e_shnum; ++idx) {
        curr = secTab + idx;     
        if (curr->sh_type == SHT_STRTAB) {
            unsigned readBytes = 0;
            strTab = (char*)malloc(curr->sh_size);
            fseek(_f, curr->sh_offset, SEEK_SET);
            readBytes = fread(strTab, 1, curr->sh_size, _f);
            if (readBytes < curr->sh_size) {
                free(strTab);
                strTab = nullptr;
            }
            break;
        }
    }
    return strTab; 
}

template<typename ELFTypeTraits> 
typename ELFTypeTraits::SymType* 
ELFParser::readSymTab(const typename ELFTypeTraits::HeaderType* header, unsigned& cnt, bool dynamic) {
    typename ELFTypeTraits::SecEntryType* secTab = readSecTable<ELFTypeTraits>(header);  
    typename ELFTypeTraits::SecEntryType* symTabHeader = nullptr;
    for (unsigned idx = 0; idx != header->e_shnum; ++idx) {
        if (dynamic && (secTab+idx)->sh_type == SHT_DYNSYM) {
            symTabHeader = secTab + idx;
            break;
        } else if (!dynamic && (secTab+idx)->sh_type == SHT_SYMTAB) {
            symTabHeader = secTab + idx;
            break;
        }
    }

    typename ELFTypeTraits::SymType* symTab = nullptr;
    if (symTabHeader) {
        cnt = symTabHeader->sh_size / symTabHeader->sh_entsize;
        symTab = (typename ELFTypeTraits::SymType*)malloc(symTabHeader->sh_size);
        fseek(_f, symTabHeader->sh_offset, SEEK_SET);
        unsigned readBytes = fread(symTab, 1, symTabHeader->sh_size, _f); 
        if (readBytes < symTabHeader->sh_size) {
            free((void*)symTab);
            symTab = nullptr;
        }
    }
    free(secTab);
    return symTab;
}

template<typename ELFTypeTraits> 
void
ELFParser::parseIdent(const typename ELFTypeTraits::HeaderType* header)
{
    const unsigned char* buf = header->e_ident;
    
    if (buf[EI_DATA] == ELFDATA2LSB) {
        printf("DataLayout: 2's complement, Little Endian.\n");
    } else if (buf[EI_DATA] == ELFDATA2MSB) {
        printf("DataLayout: 2's complement, Big Endian.\n");
    }

    printf("ELF version: %d .\n", buf[EI_VERSION]);

    if (buf[EI_OSABI] == ELFOSABI_SYSV) {
        printf("ABI: System V.\n");
    } else if (buf[EI_OSABI] == ELFOSABI_GNU) {
        printf("ABI: GNU.\n");
    } else {
        printf("ABI: Other.\n");
    }

    printf("Padding Bytes: %d .\n", buf[EI_PAD]);
}

template<typename ELFTypeTraits> 
void
ELFParser::parseType(const typename ELFTypeTraits::HeaderType* header)
{
    if (header->e_type == ET_REL) {
        printf("ELF filetype: Relocatable.\n");
    } else if (header->e_type == ET_EXEC) {
        printf("ELF filetype: Executable.\n");
    } else if (header->e_type == ET_DYN) {
        printf("ELF filetype: Shared Object.\n");
    } else {
        printf("ELF filetype: Other.\n");
    } 
}

template<typename ELFTypeTraits> 
void
ELFParser::parseMachine(const typename ELFTypeTraits::HeaderType* header)
{
    if (header->e_machine == EM_SPARC) {
        printf("Machine: Sparc.\n");
    } else if (header->e_machine == EM_MIPS) {
        printf("Machine: MIPS.\n");
    } else if (header->e_machine == EM_ARM) {
        printf("Machine: ARM.\n");
    } else if (header->e_machine == EM_X86_64) {
        printf("Machine: X86_64.\n");
    } else {
        printf("Machine: Other.\n");
    }
}

template<typename ELFTypeTraits> 
void
ELFParser::printSecInfo(typename ELFTypeTraits::SecEntryType* secEntry, const char* strTab)
{
    printf("Section: %s, VMA: %ld, Offset: %ld, Size: %ld, Align: %ld\n", 
            strTab + secEntry->sh_name,
            secEntry->sh_addr,
            secEntry->sh_offset,
            secEntry->sh_size,
            secEntry->sh_addralign);
}

template<typename ELFTypeTraits> 
void
ELFParser::parseSymbols(const typename ELFTypeTraits::HeaderType* header, typename ELFTypeTraits::SymType* first, unsigned cnt)
{
    char* strTab = readStrTab<ELFTypeTraits>(header); 
    typename ELFTypeTraits::SymType* curr = nullptr;
    unsigned char type = 0;   
    for (unsigned idx = 0; idx != cnt; ++idx) {
        curr = first + idx;
        type = ELF32_ST_TYPE(curr->st_info); // ELF32_ST_TYPE/ELF64_ST_TYPE is the same
        printf("Symbol name: %s, type: %s, defined in section: %d, addr/offset :%ld .\n",
                strTab+curr->st_name,
                symTypeStr(type),
                curr->st_shndx,
                curr->st_value);
    } 
}

template<typename ELFTypeTraits> 
void
ELFParser::parseSymTab(const typename ELFTypeTraits::HeaderType* header)
{
    typename ELFTypeTraits::SecEntryType* secTab = readSecTable<ELFTypeTraits>(header); 
    if (!secTab) {
        return;
    }
    char* strTab = readStrTab<ELFTypeTraits>(header); 
    if (!strTab) {
        free(secTab);
        return;
    }

    unsigned symCnt = 0;
    typename ELFTypeTraits::SymType* symTab= readSymTab<ELFTypeTraits>(header, symCnt, false); 
    if (symTab) {
        printf("Symbol Table: \n");
        parseSymbols<ELFTypeTraits>(header, symTab, symCnt);        
        free(symTab);
    }
    
    symTab = readSymTab<ELFTypeTraits>(header, symCnt, true); 
    if (symTab) {
        printf("Dynamic Symbol Table: \n");
        parseSymbols<ELFTypeTraits>(header, symTab, symCnt);        
        free(symTab);
    }
    free(strTab);
    free(secTab);
}

template<typename ELFTypeTraits> 
void
ELFParser::parseSections(const typename ELFTypeTraits::HeaderType* header)
{
    typename ELFTypeTraits::SecEntryType* secTab = readSecTable<ELFTypeTraits>(header);
    if (!secTab) {
        return;
    }
    char* strTab = readSecStrTab<ELFTypeTraits>(header);
    if (!strTab) {
        free(secTab);
        return;
    }

    for (unsigned idx = 0; idx != header->e_shnum; ++idx) {
        printSecInfo<ELFTypeTraits>((typename ELFTypeTraits::SecEntryType*)secTab + idx, (char*)strTab);
    }
    free(secTab);
    free(strTab);
}

template <typename ELFTypeTraits>
void ELFParser::parseInternal()
{
    typename ELFTypeTraits::HeaderType* header = readELFHeader<ELFTypeTraits>(); 
    if (!header) {
        return;
    }
    if ((0x01 << OPT_HEADER) & _opts) {
        parseIdent<ELFTypeTraits>(header);
        parseType<ELFTypeTraits>(header);
        parseMachine<ELFTypeTraits>(header);
    }
    if ((0x01 << OPT_SECTAB) & _opts) {
        parseSections<ELFTypeTraits>(header);
    }
    if ((0x01 << OPT_SYMTAB) & _opts) {
        parseSymTab<ELFTypeTraits>(header);
    }
    // todo: report more
    free(header);
}

}
