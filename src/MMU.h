/*! \file MMU.h
	\brief The memory management unit module.

	Set up the emulator's memory layout, define the ranges of code segment, data segment, heap, stack, etc. Memory access, including read and write operation.
 */
#ifndef __MMU_H__
#define __MMU_H__

/*!
	\defgroup mmu Memory management module
 */
/*@{*/

#include "arch.h"
#include "elf_file.h"

/*! \def SEGTYPE
	\brief new type for differentiate segments
 */

/*! \def TEXTSEG
	\brief Code segment
 */

/*! \def DATASEG
	\brief Data segment
 */

/*! \def RODATASEG
	\brief Read only data segment
 */

/*! \def BSSSEG
	\brief Zero initialization data segment
 */

/*! \def STACKSEG
	\brief Stack segment
 */

/*! \def HEAPSEG
	\brief Heap segment
 */

/*! \def HEADERSEG
	\brief future use
 */
#define SEGTYPE     int
#define TEXTSEG     1
#define DATASEG     1<<1
#define RODATASEG   1<<2
#define BSSSEG      1<<3
#define STACKSEG    1<<4
#define HEAPSEG     1<<5
#define HEADERSEG   1<<6

/*! \def GET_BIT(x,bit)
	\brief Get a certain bit in x
 */

/*! \def SET_BIT(x,bit)
	\brief Set a certain bit in x
 */
#define GET_BIT(x,bit)  ((x>>bit)& 1)
#define SET_BIT(x,bit)  (x|(1<<bit))

/*! \def STACK_SZ
	\brief programer define the size of stack
 */
#define STACK_SZ    0x2000

class elf_file;//predeclaration

/*! \class MMU
	\brief The memory management unit class

	It contains the memory layout of the emulator, has the ranges of various segments, owns the memory access right, including read and write.
 */
class MMU
{
public:
	//! A constructor
    MMU();
	//! A destructor
    ~MMU();

private:
	//! The file handler of the Thumb code file
    std::ifstream ifile;
	//! The pointer to ELF interpretion modualr, which will be deleted after use
    elf_file *my_elf;

private:
	/*! \var _text
		\brief Code segment file offset
	 */
	
	/*! \var _text_sz
		\brief The size of code segment
     */

	/*! \var _text_VMA
		\brief The virtual address of code segment
	 */
    int _text, _text_sz, _text_VMA;


	/*! \var _data
		\brief Data segment file offset
	 */
	
	/*! \var _data_sz
		\brief The size of data segment
     */

	/*! \var _data_VMA
		\brief The virtual address of data segment
	 */
    int _data, _data_sz, _data_VMA;


	/*! \var _rodata
		\brief Read only segment file offset
	 */
	
	/*! \var _rodata_sz
		\brief The size of read only segment
     */

	/*! \var _rodata_VMA
		\brief The virtual address of read only segment
	 */
    int _rodata, _rodata_sz, _rodata_VMA;


	/*! \var _bss
		\brief Zero initialization segment file offset
	 */
	
	/*! \var _bss_sz
		\brief The size of zero initialization segment
     */

	/*! \var _bss_VMA
		\brief The virtual address of zero initialization segment
	 */
    int _bss, _bss_sz, _bss_VMA;


	/*! \var _ss
		\brief Stack segment file offset
	 */
	
	/*! \var _ss_VMA
		\brief The virtual address of stack segment
	 */
    int32_t _ss, _ss_VMA;
	//! The virtual address of heap segment
    int _heap_VMA;
	//! The array of stack content
    int stack[STACK_SZ];
	//! The pointer to bss segment in memory
    BYTE *bss;

	//! The pointer to heap segment in memroy
    BYTE *heap;
	//! The pointer to data segment in memory
    BYTE *data_seg;
	//! The pointer to text segment in memory
	BYTE *code;
	//! The pointer to read only data segment in memroy
	BYTE *rodata;

	//! The entry point of the Thumb code(virtual address)
    int entry_point;
	//! The file offset of Thumb code in the shared object file
    int code_infile_off;

private:
	//! Transform virtual address to file offset of Thumb code file
    int VMA2FileOff(int VMAddr);
	//! Determine which segment the virtual address resides
    SEGTYPE VMA2Seg(int VMAddr);

public:
	//! Set code segment file range
    void setTextSeg(int start, int size);
	//! Set data segment file range
    void setDataSeg(int start, int size);
	//! Set read only data file range
    void setRodataSeg(int start, int size);
	//! Set bss segment file range
    void setBssSeg(int start, int size);
	//! Set stack segment file range
    void setStackSeg(int high_addr);
	//! Set heap low virtual address
    void setHeapSeg(int low_addr);

	//! Set code segment virtual address
    void setTextVMA(int VMA_start);
	//! Set data segment virtual address
    void setDataVMA(int VMA_start);
	//! Set read only segment virtual address
    void setRodataVMA(int VMA_start);
	//! Set bss segment virtual address
    void setBssVMA(int VMA_start);
	//! Set stack segment virtual address
    void setStackVMA(int VMA_high);


	//! Give out the Thumb instruction
    T_INSTR getInstr(int address);
	//! Give out the ARM instruction
    A_INSTR getInstr32(int address);
//get method
	//! Output byte data by address
    BYTE get_byte(int address);
	//! Output halfword data by address
    HALFWORD get_halfword(int address);
	//! Output word data by address
    WORD get_word(int address);

//set method
	//! Input byte data by address
    void set_byte(int address, BYTE data);
	//! Input halfword data by address
    void set_halfword(int address, HALFWORD data);
	//! Input word data by address
    void set_word(int address, WORD data);

	//! Push operation for stack
	/*! \deprecated replaced by set_word()
     */
    void push_stack(WORD data, SP arm_sp);
	//! Pop operation for stack
    /*! \deprecated replaced by get_word()
	 */
    WORD pop_stack(SP arm_sp);

//setPC & SP
	//! Give out the starting PC
    int getEntry();
	//! Give out the high address of stack
    int getStackTop();
	//! Give out the size of stack
    int getStackSz();

	//! Give out the address of heap
    int getHeapTop();
	//! Give out the size of heap
    int getHeapSz();

};


/*@}*/
#endif // __MMU_H__

