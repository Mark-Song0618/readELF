#include <elf.h>
#include <string>
#include <vector>
#include <string.h>
#include <stdio.h>

namespace ELF {

class ELFParser {
public:
    bool checkFile(const char* filePath);

    void parseFile(const char* filePath);

    void reportErr();

private:
    template<typename HeaderType> 
    void parseInternal(const char* filePath);
    template<typename HeaderType> 
    void parseIdent(const HeaderType& header);
    template<typename HeaderType> 
    void parseType(const HeaderType& header);
    template<typename HeaderType> 
    void parseMachine(const HeaderType& header);
    template<typename HeaderType, typename SecEntryType> 
    void parseSections(const HeaderType& header, FILE* f);
    template<typename SecEntryType>
    void printSecInfo(SecEntryType* secEntry, const char* strTab);
private:
    std::string              _err;
    bool                     _elf64;
};

template<typename HeaderType> 
void
ELFParser::parseIdent(const HeaderType& header)
{
    const unsigned char* buf = header.e_ident;
    
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

template<typename HeaderType> 
void
ELFParser::parseType(const HeaderType& header)
{
    if (header.e_type == ET_REL) {
        printf("ELF filetype: Relocatable.\n");
    } else if (header.e_type == ET_EXEC) {
        printf("ELF filetype: Executable.\n");
    } else if (header.e_type == ET_DYN) {
        printf("ELF filetype: Shared Object.\n");
    } else {
        printf("ELF filetype: Other.\n");
    } 
}

template<typename HeaderType> 
void
ELFParser::parseMachine(const HeaderType& header)
{
    if (header.e_machine == EM_SPARC) {
        printf("Machine: Sparc.\n");
    } else if (header.e_machine == EM_MIPS) {
        printf("Machine: MIPS.\n");
    } else if (header.e_machine == EM_ARM) {
        printf("Machine: ARM.\n");
    } else if (header.e_machine == EM_X86_64) {
        printf("Machine: X86_64.\n");
    } else {
        printf("Machine: Other.\n");
    }
}

template<typename SecEntryType>
void
ELFParser::printSecInfo(SecEntryType* secEntry, const char* strTab)
{
    printf("Section: %s, VMA: %ld, Offset: %ld, Size: %ld, Align: %ld\n", 
            strTab + secEntry->sh_name,
            secEntry->sh_addr,
            secEntry->sh_offset,
            secEntry->sh_size,
            secEntry->sh_addralign);
}

template<typename HeaderType, typename SecEntryType> 
void
ELFParser::parseSections(const HeaderType& header, FILE* f)
{
    unsigned secTabSize = header.e_shnum * header.e_shentsize;
    void* secTab = malloc(secTabSize);
    fseek(f, header.e_shoff, SEEK_SET);
    int readBytes = fread(secTab, 1, secTabSize, f);
    if (secTabSize > readBytes) {
       return; 
    }
     
    SecEntryType* entPtr = (SecEntryType*)secTab + header.e_shstrndx;
    unsigned strTabSize = entPtr->sh_size;
    unsigned strTabOff  = entPtr->sh_offset;
    void* strTab = malloc(strTabSize);
    fseek(f, strTabOff, SEEK_SET);
    readBytes = fread(strTab, 1, strTabSize, f);
    if (strTabSize > readBytes) {
        return;
    }
   
    for (unsigned idx = 0; idx != header.e_shnum; ++idx) {
        printSecInfo((SecEntryType*)secTab + idx, (char*)strTab);
    }

    free(secTab);
    free(strTab);
}

template <typename HeaderType>
void ELFParser::parseInternal(const char* filePath)
{
    // read header to buffer
    HeaderType header;
    constexpr size_t headerSize = sizeof(HeaderType);
    FILE* f = fopen(filePath, "r");
    if (fread(&header, 1, headerSize, f) != headerSize) {
        // todo: err
        return ;
    }

    parseIdent(header);
    parseType(header);
    parseMachine(header);
    if (_elf64) {
        parseSections<HeaderType, Elf64_Shdr>(header, f);
    } else {
        parseSections<HeaderType, Elf32_Shdr>(header, f);
    }
    // todo: report more
}

}
