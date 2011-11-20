/*! \file elf_file.h
	\brief ELF information extration module

	Extract the header of ELF, section info, program header info, provide program entry point, transformation between file offset and virtual memory address.
 */
  
#ifndef __ELF_FILE_H__
#define __ELF_FILE_H__


/*!
	\defgroup elf ELF file extraction module
 */
/*@{*/

#include <fstream>
#include "MMU.h"

class MMU;

/*! \typedef Elf32_Addr
	\brief The 32bit address type
 */

/*! \typedef Elf32_Half
	\brief The 32bit halfword type
 */

/*! \typedef Elf32_Off
	\brief The 32bit offset type
 */

/*! \typedef Elf32_SWord
	\brief The 32bit signed word type
 */

/*! \typedef Elf32_Word
	\brief The 32bit word type
 */

typedef uint32_t    Elf32_Addr;
typedef uint16_t    Elf32_Half;
typedef uint32_t    Elf32_Off;
typedef int32_t     Elf32_SWord;
typedef uint32_t    Elf32_Word;

/*! \def EI_NIDENT
	\brief The length of ELF Identification
 */

/*! \def SHF_WRITE
	\brief The e_flags, this section can be written
 */

/*! \def SHF_ALLOC
	\brief The e_flags, this section can be allocated in memory
 */

/*! \def SHF_EXECINSTR
	\brief The e_flags, this section can be executed
 */

#define EI_NIDENT   16

#define SHF_WRITE       0x1
#define SHF_ALLOC       0x2
#define SHF_EXECINSTR   0x4


//! The structure of ELF header
typedef struct{
    unsigned char e_ident[EI_NIDENT]; /*!< The initial bytes mark the file as an object file and provide machine-independent data with which to decode and interpret the file's content.*/
    Elf32_Half e_type; /*!< This member identifies the object file type.*/
    Elf32_Half e_machine; /*!< This member's value specifies the required architecture for an individual file.*/
    Elf32_Word e_version; /*!< This member identifies the object file version.*/
    Elf32_Addr e_entry; /*!< This member gives the virtual address to which the system first transfers control, thus starting the process.*/
    Elf32_Off  e_phoff; /*!< This member holds the program header table's file offset in bytes.*/
    Elf32_Off  e_shoff; /*!< This member holds the section header table's file offset in bytes.*/
    Elf32_Word e_flags; /*!< This member holds processor-specific flags associated with the file.*/
    Elf32_Half e_ehsize; /*!< This member holds the ELF header's size in bytes.*/
    Elf32_Half e_phentsize; /*!< This member holds the size in bytes of one entry in the file's program header table; all entries are the same size.*/
    Elf32_Half e_phnum; /*!< This member holds the number of entries in the program header table.*/
    Elf32_Half e_shentsize; /*!< This member holds a section header's size in bytes.*/
    Elf32_Half e_shnum; /*!< This member holds the number of entries in the section header table.*/
    Elf32_Half e_shstrndx; /*!< This member holds the section header table index of the entry associated with the section name string table.*/
}Elf32_Ehdr;


//! The structure of program header
typedef struct {
    Elf32_Word p_type; /*!< This member tells what kind of segment this array element describes or how to interpret the array element's information.*/
    Elf32_Off  p_offset; /*!< This member gives the offset from the beginning of the file at which the first byte of the segment resides.*/
    Elf32_Addr p_vaddr; /*!< This member gives the virtual address at which the first byte of the segment resides in memory.*/
    Elf32_Addr p_paddr; /*!< On systems for which physical addressing is relevant, this member is reserved for the segment's physical address.*/
    Elf32_Word p_filesz; /*!< This member gives the number of bytes in the file image of the segment; it may be zero.*/
    Elf32_Word p_memsz; /*!< This member gives the number of bytes in the memory image of the segment; it may be zero.*/
    Elf32_Word p_flags; /*!< This member gives flagss relevant to the segment.*/
    Elf32_Word p_align; /*!< This member gives the value to which the segments are aligned in memroy and in the file.*/
} Elf32_phdr;


//! The structure of section header
typedef struct{
    Elf32_Word sh_name; /*!< This member specifies the name of the section.*/
    Elf32_Word sh_type; /*!< This member categorizeds the section's contents and semantics.*/
    Elf32_Word sh_flags; /*!< Sections support 1-bit flags that describe miscellaneous attribues.*/
    Elf32_Addr sh_addr; /*!< If the section will appear in the memroy image of a process, this member gives the address at which the section's first byte should reside.*/
    Elf32_Off  sh_offset; /*!< This member's value gives the byte offset from the beginning of the file to the first byte in the section.*/
    Elf32_Word sh_size; /*!< This member gives the section's size in bytes.*/
    Elf32_Word sh_link; /*!< This member holds a section header table index link, whose interpretation depends on the section type.*/
    Elf32_Word sh_info; /*!< This member holds extra information, whose interpretation depends on the section type.*/
    Elf32_Word sh_addralign; /*!< The value of sh_addr must be congruent to 0, modulo the value of sh_addralign.*/
    Elf32_Word sh_entsize; /*!< This member gives the size in bytes of each entry.*/
}Elf32_Shdr;


/*! \class elf_file
	\brief The class which interprets the ELF structure.

	Extract the header of ELF, section info, program header info, provide program entry point, transformation between file offset and virtual memory address.
 */
class elf_file
{
public:
	//!A constructor
    elf_file();
	//!A destructor
    ~elf_file();
private:
	//! The ELF header container
    Elf32_Ehdr elf_header;
	//! The pointer to array of program header
    Elf32_phdr *pro_header;
	//! The pointer to array of section header
    Elf32_Shdr *sec_header;

	//! The string of section name
    char *sec_name;
	//! The length of section name string
    int sec_name_len;

	//! The file offset of Thumb code
    int code_offset;

private:
	//! Transformation from file offset to virtual address
    int FileOff2VMA(int FileOff);

public:
	//! Get ELF information from a file
    void get_info(std::ifstream &ifile);
	//! Transfer the information to MMU module
    void setup_MMU(MMU &aMMU);

	//! Give out the entry point of Thumb code
    int getEntryPoint();
	//! Give out the file offset of Thumb code
    int getCodeOffset(std::ifstream &ifile);
};


/*@}*/
#endif // __ELF_FILE_H__

