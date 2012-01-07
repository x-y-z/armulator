/*! \file ARM.h
	\brief Define the ARM status information.

	About the number of register, alias for register, and other MACROs
*/

/*! \def GPR_num
	\brief The total number of registers
*/

/*! \def rPC
	\brief Alias for PC register
*/

/*! \def rLR
	\brief Alias for LR register
*/

/*! \def rSP
	\brief Alias for SP register
*/

/*! \def LSL_DATA
	\brief The LSL shifting type
*/

/*! \def LSR_DATA
	\brief The LSR shifting type
*/

/*! \def ASR_DATA
	\brief The ASR shifting type
*/

/*! \def ROR_DATA
	\brief The ROR shifting type
*/

#ifndef __ARM_H__
#define __ARM_H__

/*!
	\defgroup instruction ARM/Thumb instructions decode module
 */
/*@{*/

#include "CPU.h"
#include "MMU.h"
#include "swi_semihost.h"
#include <limits.h>

#define GPR_num     16

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


#define LSL_DATA    0x0
#define LSR_DATA    0x1
#define ASR_DATA    0x2
#define ROR_DATA    0x3



#define IMM32           1
#define IMM_SH          1<<2
#define REG_SH          1<<3
#define IMM_OFF         1<<4
#define REG_OFF         1<<5
#define SCALE_OFF       1<<6
#define MISC_IMM_OFF    1<<7
#define MISC_REG_OFF    1<<8


/*! \class ARM
    \brief ARM instruction decode class.

    Decode part of the ARM instructions
*/
class ARM: public CPU
{
public:
    //!A constructor
    ARM();
    //!A destructor
    ~ARM();

private:
	//! The general purpose registers array
    GP_Reg r[GPR_num];//only user mode will appear in this CPU
	//! The current process status register
    EFLAG cpsr; //spsr will not be used in this CPU
	//! The current instruction to be executed
    A_INSTR cur_instr;
	//! The pointer to MMU module 
    MMU *my_mmu;
	//! The SWI component
    swi_semihost swi;

public:
    //inherit
	//! Fetch a new instruction from MMU module
    virtual void fetch();
	//! Execute current instruction
    virtual STATUS exec();
	//! Get register value by its name(future use, not implemented)
    virtual GP_Reg get_reg_by_name(const char *reg_name);
	//! Get register value by its index(future use, not implemented)
    virtual GP_Reg get_reg_by_code(int reg_code);

    //about MMU
    void InitMMU(){};
	//! Initialize the MMU module
    void InitMMU(char *file_name);
	//! Deintialize the MMU module
    void DeinitMMU();

	//! Initialize the SWI component
    void InitSWI();
	//! Get the MMU pointer(Mode switch use)
    MMU *get_mmu();

	//! Copy another CPU's pointer(Mode switch use)
    void CopyCPU(CPU *a_cpu);
	//! Get all kinds of register and MMU pointer
    void getRegs(GP_Reg reg[], EFLAG &flags, MMU* &mmu);
	//! Get arg for SWI component
    void getArg(char *arg, int len);

private:
    //inherit
	//! Set the current process status register
    virtual void set_eflag(EFLAG p_eflag);
	//! Get the value of CPSR
    virtual EFLAG get_eflag();

private:
	//! Get the negative bit value
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
        if ((num_a > 0 && num_b > 0 && c < 0) || (num_a < 0 && num_b <0 && c >0))
            return 1;
        else
            return 0;
    };

	//! Another CarryFrom
	/*!
		CarryFrom with carry.
		\param num_a first operand
		\param num_b second operand
		\param carry the carry bit
		\return The carry result
	*/
    inline int CarryFrom(unsigned int num_a, unsigned int num_b, int carry)
    {
        if (carry == 0)
        {
            return CarryFrom(num_a, num_b);//C flag
        }
        else// carry == 1
        {
            if (CarryFrom(num_a, num_b+1)
             || CarryFrom(num_a+1, num_b)
             || CarryFrom(num_a, num_b))// in case one of them is 0xffffffff, or both are
                return 1;
            else
                return 0;
        }

    };

	//! Another OverflowFrom
	/*! 
		OverflowFrom with carry
		\param num_a first operand
		\param num_b second operand
		\param carry the carry bit
		\return the overflow result
	*/
    inline int OverflowFrom(int num_a, int num_b, int carry)
    {
        if (getCarry() == 0)
        {
            return OverflowFrom(num_a, num_b);//V flag
        }
        else// carry == 1
        {
            if (OverflowFrom(num_a, num_b+1)
             || OverflowFrom(num_a+1, num_b)
             || OverflowFrom(num_a, num_b))
                return 1;
            else
                return 0;
        }

    };

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

	//! Determine whether the condition is the same as CPRS
    int ConditionPassed(unsigned char cond);

private:
    //void data_proc_imm(A_INSTR instruction);
	//! Data process decode
    void data_proc(A_INSTR instruction);
	//! Misc instruction decode
    void misc_instr(A_INSTR instruction);
	//! Multiply instruction decode
    void multiplies(A_INSTR instruction);
	//! Extra load or store instruction decode
    void extra_ld_str(A_INSTR instruction);
	//! Move immediate number into status register
    void mov_imm_to_status_reg(A_INSTR instruction);
	//! Load or store immediate offset
    void ld_str_imm_off(A_INSTR instruction);
	//! Load or store register offset
    void ld_str_reg_off(A_INSTR instruction);
	//! Media instruction decode
    void media_instr(A_INSTR instruction);
	//! Load or store all or subset of general purpose registers
    void ld_str_multiple(A_INSTR instruction);
	//! Branch with link instruction decode
    void branch_or_with_link(A_INSTR instruction);
	//! SWI handle function
    void swi_handler(A_INSTR instruction);

private:
	//! Shifter for addressing mode
    int shifter_operand(uint32_t shifter_operand, uint16_t type, int &carry_out);
};

/*@}*/
#endif // __ARM_H__

