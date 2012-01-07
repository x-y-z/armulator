/*! \file CPU.h
	\brief General CPU interface.

	The abstract basic class for all CPU-like class, provide basic interface for inherited classes.
 */
#ifndef __CPU_H__
#define __CPU_H__

#include "arch.h"
#include "MMU.h"

/*! \class CPU
	\brief General CPU interface.

	Abstract class, provide interface for inherited class.
  */
class CPU
{
public:
	//! A constructor
    CPU(){};
	//! Virtual destructor
    virtual ~CPU(){};
	//implement
public:
	//! Get instruction from memory, increase PC
    virtual void fetch() = 0;
	//! Execute the current instruction
    virtual STATUS exec() = 0;
	//! Get the register value by its name
	/*!
		\param reg_name Register's name
		\return the register value
	 */
    virtual GP_Reg get_reg_by_name(const char *reg_name) = 0;
	//! Get the register value by its index
	/*!
		\param reg_code the index of register list
		\return the register value
	 */
    virtual GP_Reg get_reg_by_code(int reg_code) = 0;
	//! Initialize the MMU module
    virtual void InitMMU() = 0;
	//! Initialize the MMU module with elf file
    virtual void InitMMU(char *file_name) = 0;
	//! Deinitialize the MMU module
    virtual void DeinitMMU() = 0;
	//! Copy CPU content from another cpu
    virtual void CopyCPU(CPU *a_cpu) = 0;
	//! Get register's value, process status, and MMU module
    virtual void getRegs(GP_Reg reg[], EFLAG &flags, MMU* &MMU) = 0;
	//! Get argument which is for running program in emulator
    virtual void getArg(char *arg, int len) = 0;

private:
	//! Set process status register(future use)
    virtual void set_eflag(EFLAG p_eflag) = 0;
	//! Get process status register(future use)
    virtual EFLAG get_eflag() = 0;
};

#endif// __CPU_H__
