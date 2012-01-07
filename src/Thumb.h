/*! \file Thumb.h
	\brief Define Thumb status information.

	About the number of registers, alias for registers, and other MACROs
 */
#ifndef __THUMB_H__
#define __THUMB_H__

/*!
	\addtogroup instruction
 */
/*@{*/

#include <limits.h>
#include "CPU.h"
#include "assert.h"
#include "MMU.h"
#include "swi_semihost.h"



/*! \def GPR_num
	\brief The total number of registers.
 */
#define GPR_num     16


/*! \def rPC
	\brief Alias for PC register.
 */

/*! \def rLR
	\brief Alias for LR register.
 */

/*! \def rSP
	\brief Alias for SP register.
 */
#define rPC         r[15]
#define rLR         r[14]
#define rSP         r[13]


#define MASK_1BIT   0x1
#define MASK_2BIT   0x3
#define MASK_3BIT   0x7
#define MASK_4BIT   0xf
#define MASK_5BIT   0x1f
#define MASK_6BIT   0x3f
#define MASK_7BIT   0x7f
#define MASK_8BIT   0xff





/*! \class Thumb
	\brief Thumb instruction decode class.

	Decode all the Thumb instructions, except breakpoint, and mode switch related instructions.
 */
class Thumb: public CPU
{
public:
	//!A constructor
	Thumb();
	//!A destructor
	~Thumb();

	//members
private:
	//! The general purpose registers array.
	GP_Reg r[GPR_num];//only user mode will appear in this CPU
	//! The current process status register.
	EFLAG cpsr; //spsr will not be used in this CPU
	//! The current instruction to be executed.
	T_INSTR cur_instr;
	//! The pointer to MMU module.
	MMU *my_mmu;
	//! The SWI component.
    swi_semihost swi;

	//implement
public:
    //inherit
	//! Fetch a new instruction from MMU module.
    virtual void fetch();
	//! Execute current instruction.
    virtual STATUS exec();
	//! Get register value by its name(future use, not implemented).
    virtual GP_Reg get_reg_by_name(const char *reg_name);
	//! Get register value by its index(future use, not implemented).
    virtual GP_Reg get_reg_by_code(int reg_code);

    //about MMU
	//! Initialize the MMU module.
    void InitMMU(){};
    void InitMMU(char *file_name);
	//! Deinitialize the MMU module.
    void DeinitMMU();

	//! Initialize the SWI component.
    void InitSWI();
	//! Get the MMU pointer(Mode switch use).
    MMU *get_mmu();

	//! Copy another CPU's pointer(Mode switch use).
    void CopyCPU(CPU *a_cpu);
	//! Get all kinds of register and MMU pointer.
    void getRegs(GP_Reg reg[], EFLAG &flags, MMU* &mmu);

	//! Get argument for SWI component.
    void getArg(char *arg, int len);


private:
    //inherit
	//! Set the current process status register.
    virtual void set_eflag(EFLAG p_eflag);
	//! Get the value of CPSR.
    virtual EFLAG get_eflag();
	//EFLAG related
private://change to private after test
	//! Get the negative bit value.
	/*!
		\return The value of negative bit(1:neg, 0:pos)
	*/
    inline int getNeg(){ return (cpsr>>31) & MASK_1BIT; };
	//! Get the zero bit value
	/*!
		\return The value of zero bit(1:reg is zero, 0:not zero)
	*/
    inline int getZero(){ return (cpsr>>30) & MASK_1BIT; };
	//! Get the carry bit value
	/*!
		\return The value of carry bit(1:last operation has carry, 0:no carry)
	*/
    inline int getCarry(){ return (cpsr>>29) & MASK_1BIT; };
	//! Get the overflow bit value
	/*! 
		\return The value of overflow bit(1:last operation has overflow, 0:no overflow)
	*/
    inline int getOverflow(){ return (cpsr>>28) & MASK_1BIT; };

	//! Set negative bit
    inline void setNeg(){cpsr |= 1<<31;};
	//! Set Zero bit
    inline void setZero(){cpsr |= 1<<30;};
	//! Set Carry bit
    inline void setCarry(){cpsr |= 1<<29;};
	//! Set Overflow bit
    inline void setOverflow(){cpsr |= 1<<28;};

	//! Clear Negative bit
    inline void clrNeg(){cpsr &= ~(1<<31);};
	//! Clear Zero bit
    inline void clrZero(){cpsr &= ~(1<<30);};
	//! Clear Carry bit
    inline void clrCarry(){cpsr &= ~(1<<29);};
	//! Clear Overflow bit
    inline void clrOverflow(){cpsr &= ~(1<<28);};

	//! Get the number of 1 in a binary number
	/*!
		Get the number of 1 in a binary number.
		\param bits The input number which need to be caculated the number of 1
		\return The number of 1 in the input number
     */
    inline int num_of_bits_set(int bits){ int cnt = 0; while (bits>0){bits &= bits -1; cnt++;} return cnt;};

	//! Exetension of signed number
	/*!
		Extend a signed short bit size number to a signed long bit size number.
		\param num a signed number to be extended
		\param num_bit the number of bit of the to be extended number
		\return The result of extension
	*/
    inline int SignExtend(int num, int num_bit)
    {
        int mask = 0;
        for (int i = 0; i < num_bit; i++)
        {
            mask = mask<<1 | 1;
        }

        if ((num>>(num_bit - 1)) & MASK_1BIT == 1)
        {
            num |= ~mask;
        }
        else
        {
            num &= mask;
        }

        return num;
    };

	//! Determine the carry of an add operation 
	/*!
		It determines the add operation, negate num_b to determine substraction. 
		\param num_a first operand
		\param num_b second operand
		\return The carry result
	*/
    inline int CarryFrom(unsigned int num_a, unsigned int num_b)
    {
        if ((unsigned long long)(num_a) + num_b > UINT_MAX)
            return 1;
        else
            return 0;
    };

	//! Determine the overflow of an add operation
	/*!
		It is used as CarryFrom, negate num_b to determine substraction.
		\param num_a first operand
		\param num_b second operand
		\return The overflow result
	*/
    inline int OverflowFrom(int num_a, int num_b)
    {
        int c = num_a + num_b;
        if (num_b == 0x80000000)
            return 1;

        if ((num_a > 0 && num_b > 0 && c < 0) || (num_a < 0 && num_b <0 && c >0))
            return 1;
        else
            return 0;
    };

	//! Determine whether the condition is the same as CPRS
    int ConditionPassed(unsigned char cond);

//instruction exection implement
private:
    //000
	//! Add or sub with register or immediate number
    void add_sub_reg_or_imm(const T_INSTR instruction);
	//! Shift a number by immediate number
    void shift_by_imm(const T_INSTR instruction);
    //001
	//! Add, sub, mov, cmp with immediate number
    void add_sub_mov_cmp_imm(const T_INSTR instruction);
    //010
	//! Load or store with register offset
    void ld_str_reg_offset(const T_INSTR instruction);
	//! Load from a literal pool
    void ld_from_pool(const T_INSTR instruction);
	//! branch or exechange instruction set
    void br_or_exec_is(const T_INSTR instruction);
	//! Special data processing
    void spec_data_proc(const T_INSTR instruction);
	//! Data-processing register
    void data_proc_reg(const T_INSTR instruction);
    //011
	//! Load/store word/byte immediate offset
    void ld_str_word_byte_imm(const T_INSTR instruction);
    //100
	//! Load/store to/from stack
    void ld_str_stack(const T_INSTR instruction);
	//! Load/store halfword immediate offset
    void ld_str_halfw_imm(const T_INSTR instruction);
    //101
	//! Miscellaneous instruction
    void misc(const T_INSTR instruction);
	//! Add to SP or PC
    void add_to_sp_or_pc(const T_INSTR instruction);
    //110
	//! Load/store multiple
    void ld_str_multiple(const T_INSTR instruction);
	//! Conditional branch
    void con_br(const T_INSTR instruction);
    //111
	//! Unconditional branch
    void uncon_br(const T_INSTR instruction);
	//! BLX suffix
    void blx_suffix(const T_INSTR instruction);
	//! BL/BLX prefix
    void bl_blx_prefix(const T_INSTR instruction);
	//! BL suffix
    void bl_suffix(const T_INSTR instruction);

};


/*@}*/
#endif// __THUMB_H__
