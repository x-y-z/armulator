/*! \file arch.h
    \brief Define the architecture information.

    According to the emulator platform.
    Set the bit size of General Purpose Registers, Instruction.
    Define BYTE HALFWORD WORD DWORD  
 */

/*! \def __ARM__
    \brief Enable the ARMulator.

    This MACRO makes data structures follow the ARM way
*/

/*! \def STATUS
	\brief normal return value
*/

/*! \def GP_Reg
    \brief The bit size of General Purpose Register
*/

/*! \def EFLAG
	\brief The bit size of flag register
*/

/*! \def PC
	\brief Specify the size of PC(program counter) 
*/

/*! \def SP
	\brief Specify the size of SP(stack pointer)
*/

/*! \def LR
	\brief Specify the size of LR(link return)
*/

/*! \def T_INSTR
	\brief Specify the size of Thumb instruction
*/

/*! \def A_INSTR
	\brief Specify the size of ARM instruction
*/

/*! \typedef BYTE
	\brief Define BYTE
*/

/*! \typedef HALFWORD
	\brief Define HALFWORD
*/

/*! \typedef WORD
	\brief Define WORD
*/

/*! \typedef DWORD
	\brief Define DWORD
*/

#ifndef __ARCH_H__
#define __ARCH_H__

#include <stdint.h>

#define STATUS int


#define __ARM__

#ifdef __ARM__


#define GP_Reg	int32_t
#define EFLAG	uint32_t
#define PC	int32_t
#define SP	int32_t
#define LR	int32_t
#define T_INSTR uint16_t
#define A_INSTR uint32_t

typedef uint8_t BYTE;
typedef uint16_t HALFWORD;
typedef uint32_t WORD;
typedef uint64_t DWORD;


#endif

#ifdef __I32__

//
#endif

#ifdef __I64__

//future use

#endif

#endif//__ARCH_H__
