#include "ARM.h"
#include "error.h"
#include "Thumb.h"

// TODO (Birdman#1#): A3-37,Move imm to status reg


/**
  * Initialize the general purpose registers, clear CPSR and MMU pointer.
  */
 ARM::ARM()
{
    //initialize the GPR
    for (int i = 0; i < GPR_num; i++)
	r[i] = 0;

    cpsr = 0;

    my_mmu = NULL;
}

/** 
  * Nothing to do. If the program end with ARM status, MMU can be deleted here.
  */
 ARM::~ARM()
{

}


/**
  * get the register value according to its index. 
  */
GP_Reg ARM::get_reg_by_code(int reg_code)
{
    return 0;
}

/** 
  * get the register value according to its name.
  */
GP_Reg ARM::get_reg_by_name(const char *reg_name)
{
    return 0;
}

/** 
  * Get a 32-bit instruction from MMU modular, and increase PC by 4.
  */
void ARM::fetch()
{
    cur_instr = my_mmu->getInstr32(rPC);
    rPC += 4;
}
/** 
  * Execute part of the 32-bit ARM instruction, the instructions are classified by most significant 3 bit.
  * \exception UndefineInst For undefined instructions
  * \exception UnexpectInst For instructions can not be handled
  */
STATUS ARM::exec()
{
    char cond = (cur_instr>>28) & MASK_4BIT;
    char attempt_code = (cur_instr>>25) & MASK_3BIT;

    //unconditional instruction

    switch (attempt_code)
    {
    	case 0://000
    	{
    	    //int opcode = (cur_instr>>21) & MASK_4BIT;
    	    int bit4 = (cur_instr>>4) & MASK_1BIT;
    	    int S = (cur_instr>>20) & MASK_1BIT;

    	    if (bit4 == 0)//data processing imm shift, misc instruction
    	    {
                if (S == 0 && (((cur_instr>>23) & MASK_2BIT) == 0x2))//tst, teq, cmp, cmn, these are 10xx,but S is always 1
                {
                    //misc instruction
                    misc_instr(cur_instr);
                }
                else
                {
                    //data processing imm shift
                    data_proc(cur_instr);
                }
            }
            else//bit4 == 1, data processing reg shift, misc instruction, multiplies, extra ld/str
            {
                if ((S == 0) && (((cur_instr>>23) & MASK_2BIT) == 2) && (((cur_instr>>7) & MASK_1BIT) == 0))
                {
                    //misc instruction
                    misc_instr(cur_instr);
                }
                else if (((cur_instr>>7) & MASK_1BIT) == 1)
                {
                    int sec_code = (cur_instr>>5) & MASK_2BIT;
                    int M = (cur_instr>>24) & MASK_1BIT;

                    if (M == 0 && sec_code == 0)
                    {
                        multiplies(cur_instr);
                    //multiplies
                    }
                    else
                    {
                        extra_ld_str(cur_instr);
                    //extra ld/str
                    }
                }
                else
                {
                    //data processing reg shift
                    data_proc(cur_instr);
                }
            }

    		break;
    	}
        case 1://001
        {
            int S = (cur_instr>>20) & MASK_1BIT;

            if (S == 0 && (((cur_instr>>23) & MASK_2BIT) == 0x2))//tst, teq, cmp, cmn always with S == 1
            {
                if (((cur_instr>>21) & MASK_1BIT) == 1)
                {
                    //mov imm to status reg
                    mov_imm_to_status_reg(cur_instr);
                }
                //else undefine
            }
            else
            {
                //data process imm
                data_proc(cur_instr);
            }
            break;
        }
        case 2://010
        {
            //ld/str imm offset
            ld_str_imm_off(cur_instr);
        	break;
        }
        case 3://011
        {
            int bit4 = (cur_instr>>4) & MASK_1BIT;

            if (bit4 == 0)
            {
                //ld/str reg offset
                ld_str_reg_off(cur_instr);
            }
            else
            {
                if (((cur_instr>>20) & MASK_5BIT) == 0x1f && (((cur_instr>>4) & MASK_4BIT) == 0xf))
                {
                    //architurally undefine
                }
                else
                {
                    //media instruction
                    media_instr(cur_instr);
                }
            }
        	break;
        }
        case 4://100
        {
            //ld/st multiple
            ld_str_multiple(cur_instr);
        	break;
        }
        case 5://101
        {
            //branch and branch with link
            branch_or_with_link(cur_instr);
        	break;
        }
        case 6://110
        {
            //coprocessor ld/str and double reg transfer
        	break;
        }
        case 7://111
        {
            int bit4 = (cur_instr>>4) & MASK_1BIT;
            int bit24 = (cur_instr>>24) & MASK_1BIT;

            if (bit24 == 0)
            {
                if (bit4 == 0)
                {
                    //coprocessor data processing
                }
                else
                {
                    //coprocessor reg transfer
                }
            }
            else
            {
                //swi
                swi_handler(cur_instr);
            }
        	break;
        }
    	default:
            assert(0);
    		break;
    }

    return 0;
}

/** 
  * A bucket shifter emulator, solving the ARM address mode.
  * @param shifter_operand the operand for shifter
  * @param type the type of shifting, 32bit immediate number, immediate shifter, etc.
  * @param carry_out whether there is a carry coming out
  * @return The result of the bucket shifter
  */
int ARM::shifter_operand(uint32_t shifter_operand, uint16_t type, int &carry_out)
{
    int res;

    switch (type)
    {
    	case IMM32:
    	{
    	    int rotate_imm = ((shifter_operand>>8) & MASK_4BIT) * 2;
    	    int imm_8 = shifter_operand & MASK_8BIT;

            res = (imm_8 >> rotate_imm) | (imm_8 << (32 - rotate_imm));

            if (rotate_imm == 0)
                carry_out = getCarry();
            else
                carry_out = (res>>31) & MASK_1BIT;

            return res;
    		break;
    	}
        case IMM_SH://bit4 == 0
        {
            int Rm = (shifter_operand) & MASK_4BIT;
            int shift = (shifter_operand>>5) & MASK_2BIT;
            int shift_imm = (shifter_operand>>7) & MASK_5BIT;

            if (Rm == 15)
                r[Rm] += 4;

            switch (shift)
            {
            	case LSL_DATA:
            	{
            	    if (shift_imm == 0)
            	    {
            	        res = r[Rm];
            	        carry_out = getCarry();
                    }
                    else
                    {
                        res = r[Rm]<<shift_imm;
                        carry_out = (r[Rm]>>(32 - shift_imm)) & MASK_1BIT;
                    }
            		break;
            	}
            	case LSR_DATA:
            	{
            	    if (shift_imm == 0)//shift by 32
            	    {
            	        res = 0;
            	        carry_out = (r[Rm]>>31) & MASK_1BIT;
                    }
                    else
                    {
                        res = (unsigned)r[Rm]>>shift_imm;
                        carry_out = (r[Rm]>>(shift_imm - 1)) & MASK_1BIT;
                    }
            		break;
            	}
            	case ASR_DATA:
            	{
            	    if (shift_imm == 0)
            	    {
            	        if (((r[Rm]>>31) & MASK_1BIT) == 0)
            	        {
            	            res = 0;
            	            carry_out = 0;//(r[Rm]>>31) & MASK_1BIT
                        }
                        else
                        {
                            res = 0xffffffff;
                            carry_out = 1;//(r[Rm]>>31) & MASK_1BIT
                        }
                    }
                    else
                    {
                        res = r[Rm] >> shift_imm;
                        carry_out = (r[Rm]>>(shift_imm - 1)) & MASK_1BIT;
                    }
            		break;
            	}
            	case ROR_DATA:
            	{
            	    if (shift_imm == 0)
            	    {
            	        res = (getCarry()<<31) | ((unsigned)r[Rm]>>1);
            	        carry_out = r[Rm] & MASK_1BIT;
                    }
                    else
                    {
                        res = ((unsigned)r[Rm]>>shift_imm) | (r[Rm]<<(32 - shift_imm));
                        carry_out = (r[Rm]>>(shift_imm - 1)) & MASK_1BIT;
                    }
            		break;
            	}
            	default:
                    assert(0);
            		break;
            }

            if (Rm == 15)
                r[Rm] -= 4;

            return res;
        	break;
        }
        case REG_SH://bit4 == 1, bit7 == 0
        {
            int Rm = (shifter_operand) & MASK_4BIT;
            int shift = (shifter_operand>>5) & MASK_2BIT;
            int Rs = (shifter_operand>>8) & MASK_4BIT;

            switch (shift)
            {
            	case LSL_DATA:
            	{
            	    if ((r[Rs] & MASK_8BIT) == 0)
            	    {
            	        res = r[Rm];
            	        carry_out = getCarry();
                    }
                    else if(r[Rs] & MASK_8BIT < 32)
                    {
                        res = r[Rm]<<(r[Rs] & MASK_8BIT);
                        carry_out = (r[Rm]>>(32 - r[Rs] & MASK_8BIT)) & MASK_1BIT;
                    }
                    else if((r[Rs] & MASK_8BIT) == 32)
                    {
                        res =0;
                        carry_out = r[Rm] & MASK_1BIT;
                    }
                    else//r[Rs] & MASK_8BIT > 32
                    {
                        res = 0;
                        carry_out = 0;
                    }
            		break;
            	}
                case LSR_DATA:
                {
                    if ((r[Rs] & MASK_8BIT) == 0)
            	    {
            	        res = r[Rm];
            	        carry_out = getCarry();
                    }
                    else if(r[Rs] & MASK_8BIT < 32)
                    {
                        res = (unsigned)r[Rm]>>(r[Rs] & MASK_8BIT);
                        carry_out = (r[Rm]>>(r[Rs] & MASK_8BIT) - 1) & MASK_1BIT;
                    }
                    else if((r[Rs] & MASK_8BIT) == 32)
                    {
                        res =0;
                        carry_out = (r[Rm]>>31) & MASK_1BIT;
                    }
                    else//r[Rs] & MASK_7BIT > 32
                    {
                        res = 0;
                        carry_out = 0;
                    }
                	break;
                }
                case ASR_DATA:
                {
                    if ((r[Rs] & MASK_8BIT) == 0)
            	    {
            	        res = r[Rm];
            	        carry_out = getCarry();
                    }
                    else if(r[Rs] & MASK_8BIT < 32)
                    {
                        res = r[Rm]>>(r[Rs] & MASK_8BIT);
                        carry_out = (r[Rm]>>(32 - r[Rs] & MASK_8BIT)) & MASK_1BIT;
                    }
                    else if(r[Rs] & MASK_8BIT >= 32)
                    {
                        if (((r[Rm]>>31) & MASK_1BIT) == 0)
                        {
                            res = 0;
                            carry_out = 0;//(r[Rm]>>31) & MASK_1BIT
                        }
                        else
                        {
                            res = 0xffffffff;
                            carry_out = 1;//(r[Rm]>>31) & MASK_1BIT
                        }
                    }
                	break;
                }
                case ROR_DATA:
                {
                    if ((r[Rs] & MASK_8BIT) == 0)
                    {
                        res = r[Rm];
                        carry_out = getCarry();
                    }
                    else if((r[Rs] & MASK_5BIT) == 0)
                    {
                        res = r[Rm];
                        carry_out = r[Rm]>>31 & MASK_1BIT;
                    }
                    else//r[Rs] & MASK_5BIT > 0
                    {
                        res = (unsigned)r[Rm]>>(r[Rs] & MASK_5BIT) | r[Rm]<<(32 - r[Rs] & MASK_5BIT);
                        carry_out = (r[Rm]>>(r[Rs] & MASK_5BIT -1)) & MASK_1BIT;
                    }
                	break;
                }
            	default:
                    assert(0);
            		break;
            }
            return res;
        	break;
        }
        case IMM_OFF:
        {
            int P = (shifter_operand>>24) & MASK_1BIT;
            int U = (shifter_operand>>23) & MASK_1BIT;
            int W = (shifter_operand>>21) & MASK_1BIT;
            int Rn = (shifter_operand>>16) & MASK_4BIT;
            int offset_12 = (shifter_operand) & 0xfff;

            if (Rn == 15)
                r[Rn] +=4;

            if (P == 1 && W == 0)
            {
                if (U == 1)
                    res = r[Rn] + offset_12;
                else//U == 0
                    res = r[Rn] - offset_12;
            }

            if (P == 1 && W == 1)
            {
                if (U == 1)
                    res = r[Rn] + offset_12;
                else//U == 0
                    res = r[Rn] - offset_12;

                r[Rn] = res;
            }

            if (P == 0)//W ==1 is for unprevilige access
            {
                res = r[Rn];
                if (U == 1)
                    r[Rn] = r[Rn] + offset_12;
                else//U == 0
                    r[Rn] = r[Rn] - offset_12;
            }

            if (Rn == 15)
                r[Rn] -= 4;

            return res;
        	break;
        }
        case REG_OFF:
        {
            int P = (shifter_operand>>24) & MASK_1BIT;
            int U = (shifter_operand>>23) & MASK_1BIT;
            int W = (shifter_operand>>21) & MASK_1BIT;
            int Rn = (shifter_operand>>16) & MASK_4BIT;
            int Rm = (shifter_operand) & MASK_4BIT;

            if (Rn == 15)
                r[Rn] += 4;

            if (P == 1 && W == 0)
            {
                if (U == 1)
                    res = r[Rn] + r[Rm];
                else
                    res = r[Rn] - r[Rm];
            }

            if (P == 1 && W == 1)
            {
                if (U == 1)
                    res = r[Rn] + r[Rm];
                else
                    res = r[Rn] - r[Rm];

                r[Rn] =res;
            }

            if (P == 0)//W ==1 is for unprevilige access
            {
                res = r[Rn];
                if (U == 1)
                    r[Rn] = r[Rn] + r[Rm];
                else
                    r[Rn] = r[Rn] - r[Rm];
            }

            if (Rn == 15)
                r[Rn] -= 4;

            return res;
        	break;
        }
        case SCALE_OFF:
        {
            int P = (shifter_operand>>24) & MASK_1BIT;
            int U = (shifter_operand>>23) & MASK_1BIT;
            int W = (shifter_operand>>21) & MASK_1BIT;
            int Rn = (shifter_operand>>16) & MASK_4BIT;
            int Rm = (shifter_operand) & MASK_4BIT;
            int shift_imm = (shifter_operand>>7) & MASK_5BIT;
            int shift = (shifter_operand>>5) & MASK_2BIT;
            int index;

            if (Rn == 15)
                r[Rn] += 4;

            switch (shift)
            {
            	case LSL_DATA:
            	{
            	    index = r[Rm]<<shift_imm;
            		break;
            	}
                case LSR_DATA:
                {
                    if (shift_imm == 0)
                        index = 0;
                    else
                        index = (unsigned)r[Rm]>>shift_imm;
                	break;
                }
                case ASR_DATA:
                {
                    if (shift_imm == 0)
                        if (((r[Rm]>>31) & MASK_1BIT) == 1)
                            index = 0xffffffff;
                        else
                            index = 0;
                    else
                        index = r[Rm]>>shift_imm;
                	break;
                }
                case ROR_DATA:
                {
                    if (shift_imm == 0)
                        index = (getCarry()<<31) | ((unsigned)r[Rm]>>1);
                    else
                        index = (unsigned)r[Rm]>>shift_imm;
                	break;
                }
            	default:
                    assert(0);
            		break;
            }



            if (P == 1 && W == 0)
            {
                if (U == 1)
                    res = r[Rn] + index;
                else
                    res = r[Rn] - index;
            }

            if (P == 1 && W == 1)
            {
                if (U == 1)
                    res = r[Rn] + index;
                else
                    res = r[Rn] - index;

                r[Rn] = res;
            }

            if (P == 0)//W ==1 is for unprevilige access
            {
                res = r[Rn];

                if (U == 1)
                    r[Rn] = r[Rn] + index;
                else
                    r[Rn] = r[Rn] - index;
            }

            if (Rn == 15)
                r[Rn] -= 4;

            return res;
        	break;
        }
        case MISC_IMM_OFF://bit22=1
        {
            int P = (shifter_operand>>24) & MASK_1BIT;
            int U = (shifter_operand>>23) & MASK_1BIT;
            int W = (shifter_operand>>21) & MASK_1BIT;
            int immedH = (shifter_operand>>8) & MASK_4BIT;
            int immedL = (shifter_operand) & MASK_4BIT;
            int offset_8 = (immedH<<4) | immedL;
            int Rn = (shifter_operand>>16) & MASK_4BIT;

            if (Rn == 15)
                r[Rn] += 4;

            if (P == 1 && W == 0)
            {
                if (U == 1)
                    res = r[Rn] + offset_8;
                else
                    res = r[Rn] - offset_8;
            }

            if (P == 1 && W == 1)
            {
                if (U == 1)
                    res = r[Rn] + offset_8;
                else
                    res = r[Rn] - offset_8;

                r[Rn] = res;
            }

            if (P == 0)
            {
                res = r[Rn];
                if (U == 1)
                    r[Rn] = r[Rn] + offset_8;
                else
                    r[Rn] = r[Rn] - offset_8;
            }

            if (Rn == 15)
                r[Rn] -= 4;

            return res;
        	break;
        }
        case MISC_REG_OFF://bit22=0
        {
            int P = (shifter_operand>>24) & MASK_1BIT;
            int U = (shifter_operand>>23) & MASK_1BIT;
            int W = (shifter_operand>>21) & MASK_1BIT;
            int Rn = (shifter_operand>>16) & MASK_4BIT;
            int Rm = (shifter_operand) & MASK_4BIT;

            if (Rn == 15)
                r[Rn] += 4;

            if (P == 1 && W == 0)
            {
                if (U == 1)
                    res = r[Rn] + r[Rm];
                else
                    res = r[Rn] - r[Rm];
            }

            if (P == 1 && W == 1)
            {
                if (U == 1)
                    res = r[Rn] + r[Rm];
                else
                    res = r[Rn] - r[Rm];

                r[Rn] = res;
            }

            if (P == 0)
            {
                res = r[Rn];

                if (U == 1)
                    r[Rn] = r[Rn] + r[Rm];
                else
                    r[Rn] = r[Rn] - r[Rm];
            }

            if (Rn == 15)
                r[Rn] -= 4;

            return res;
        	break;
        }
    	default:
            assert(0);
            //throw a error
            return -1;
    		break;
    }

}

/** 
  * The matching pattern is 000/001 opcode S (shift_operand).
  * @param instruction The instructio to be executed.
  * @exception UnexpectInst For instructions can not be handled
  */
void ARM::data_proc(A_INSTR instruction)
{
    int I = (instruction>>25) & MASK_1BIT;
    int S = (instruction>>20) & MASK_1BIT;
    int opcode = (instruction>>21) & MASK_4BIT;
    int Rn = (instruction>>16) & MASK_4BIT;
    int Rd = (instruction>>12) & MASK_4BIT;
    int bit4 = (instruction>>4) & MASK_1BIT;
    int shifter_carry_out;
    int operand;

    if (Rn == 15)
        r[Rn] += 4;

    if (I == 1)//IMM32
    {
        operand = shifter_operand(instruction, IMM32, shifter_carry_out);
    }
    else if(bit4 == 0)//IMM_SH
    {
        operand = shifter_operand(instruction, IMM_SH, shifter_carry_out);
    }
    else if(((instruction>>7) & MASK_1BIT) == 0)//REG_SH
    {
        operand = shifter_operand(instruction, REG_SH, shifter_carry_out);
    }

    switch (opcode)
    {
    	case 0://and
    	{
    	    r[Rd] = r[Rn] & operand;
            if (S == 1 && Rd == 15)
            {
                //to be dealed
            }
            else if(S == 1)
            {
                (r[Rd]>>31)&MASK_1BIT?setNeg():clrNeg();
                r[Rd]?clrZero():setZero();
                shifter_carry_out?setCarry():clrCarry();
                //V unaffected
            }
                    break;
    	}
    	case 1://eor
    	{
    	    r[Rd] = r[Rn] ^ operand;
    	    if (S == 1 && Rd == 15)
            {
                //to be dealed
            }
            else if(S == 1)
            {
                (r[Rd]>>31)&MASK_1BIT?setNeg():clrNeg();
                r[Rd]?clrZero():setZero();
                shifter_carry_out?setCarry():clrCarry();
                //V unaffected
            }
    		break;
    	}
    	case 2://sub
    	{
    	    r[Rd] = r[Rn] - operand;
    	    if (S == 1 && Rd == 15)
            {
                //to be dealed
            }
            else if(S == 1)
            {
                (r[Rd]>>31)&MASK_1BIT?setNeg():clrNeg();
                r[Rd]?clrZero():setZero();
                ((r[Rn] - operand) >= 0)?setCarry():clrCarry();//C flag
                OverflowFrom(r[Rn], -operand)?setOverflow():clrOverflow();//V flag
            }
    		break;
    	}
    	case 3://rsb
    	{
    	    r[Rd] = operand - r[Rn];
    	    if (S == 1 && Rd == 15)
            {
                //to be dealed
            }
            else if(S == 1)
            {
                (r[Rd]>>31)&MASK_1BIT?setNeg():clrNeg();
                r[Rd]?clrZero():setZero();
                ((operand - r[Rn]) >= 0)?setCarry():clrCarry();//C flag
                OverflowFrom(operand, -r[Rn])?setOverflow():clrOverflow();//V flag
            }
    		break;
    	}
    	case 4://add
    	{
    	    r[Rd] = r[Rn] + operand;
    	    if (S == 1 && Rd == 15)
            {
                //to be dealed
            }
            else if(S == 1)
            {
                (r[Rd]>>31)&MASK_1BIT?setNeg():clrNeg();
                r[Rd]?clrZero():setZero();
                CarryFrom(r[Rn], operand)?setCarry():clrCarry();//C flag
                OverflowFrom(r[Rn], operand)?setOverflow():clrOverflow();//V flag
            }
    		break;
    	}
    	case 5://adc
    	{
    	    r[Rd] = r[Rn] + operand + getCarry();
    	    if (S == 1 && Rd == 15)
            {
                //to be dealed
            }
            else if(S == 1)
            {
                ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
                r[Rd]?clrZero():setZero();//Z flag
                CarryFrom(r[Rn], operand, getCarry());
                OverflowFrom(r[Rn], operand, getCarry());
            }
    		break;
    	}
    	case 6://sbc
    	{
    	    r[Rd] = r[Rn] - operand - getCarry()?0:1;
    	    if (S == 1 && Rd == 15)
            {
                //to be dealed
            }
            else if(S == 1)
            {
                ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
                r[Rd]?clrZero():setZero();//Z flag
                if (getCarry() == 1)
                {
                    CarryFrom(r[Rn], -operand)?setCarry():clrCarry();//C flag
                    OverflowFrom(r[Rn], -operand)?setOverflow():clrOverflow();//V flag
                }
                else// carry == 0
                {
                    if (r[Rn] - operand - 1 > 0)// in case one of them is 0xffffffff, or both are
                        setCarry();
                    else
                        clrCarry();

                    if (OverflowFrom(r[Rn], -operand - 1))
                        setOverflow();
                    else
                        clrOverflow();
                }
            }
    		break;
    	}
    	case 7://rsc
    	{
    	    r[Rd] = operand - r[Rn] - !getCarry();

    	    if (S == 1 && Rd == 15)
            {
                //to be dealed
            }
            else if(S == 1)
            {
                ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
                r[Rd]?clrZero():setZero();//Z flag
                if (getCarry() == 1)
                {
                    CarryFrom(operand, -r[Rn])?setCarry():clrCarry();//C flag
                    OverflowFrom(operand, -r[Rn])?setOverflow():clrOverflow();//V flag
                }
                else// carry == 0
                {
                    if (operand - r[Rn] - 1 > 0)// in case one of them is 0xffffffff, or both are
                        setCarry();
                    else
                        clrCarry();

                    if (OverflowFrom(operand, -r[Rn] - 1))
                        setOverflow();
                    else
                        clrOverflow();
                }
            }
    		break;
    	}
    	case 8://tst
    	{
    	    int alu_out = r[Rn] & operand;
    	    if (S == 1 && Rd == 15)
            {
                //to be dealed
                UnexpectInst e;
                throw e;
            }
            else if(S == 1)
            {
                (alu_out>>31)&MASK_1BIT?setNeg():clrNeg();
                alu_out?clrZero():setZero();
                shifter_carry_out?setCarry():clrCarry();
                //V unaffected
            }
    		break;
    	}
    	case 9://teq
    	{
    	    int alu_out = r[Rn] ^ operand;
    	    if (S == 1 && Rd == 15)
            {
                //to be dealed
                UnexpectInst e;
                throw e;
            }
            else if(S == 1)
            {
                (alu_out>>31)&MASK_1BIT?setNeg():clrNeg();
                alu_out?clrZero():setZero();
                shifter_carry_out?setCarry():clrCarry();
                //V unaffected
            }
    		break;
    	}
    	case 10://cmp
    	{
    	    int alu_out = r[Rn] - operand;
    	    if (S == 1 && Rd == 15)
            {
                //to be dealed
                UnexpectInst e;
                throw e;
            }
            else if(S == 1)
            {
                (alu_out>>31)&MASK_1BIT?setNeg():clrNeg();
                alu_out?clrZero():setZero();
                ((r[Rn] - operand) >= 0)?setCarry():clrCarry();//C flag
                OverflowFrom(r[Rn], -operand)?setOverflow():clrOverflow();//V flag
            }
    		break;
    	}
    	case 11://cmn
    	{
    	    int alu_out = r[Rn] + operand;
    	    if (S == 1 && Rd == 15)
            {
                //to be dealed
                UnexpectInst e;
                throw e;
            }
            else if(S == 1)
            {
                (alu_out>>31)&MASK_1BIT?setNeg():clrNeg();
                alu_out?clrZero():setZero();
                CarryFrom(r[Rn], operand)?setCarry():clrCarry();//C flag
                OverflowFrom(r[Rn], operand)?setOverflow():clrOverflow();//V flag
            }
    		break;
    	}
    	case 12://orr
    	{
    	    r[Rd] =  r[Rn] | operand;
    	    if (S == 1 && Rd == 15)
    	    {
    	        //to be dealed
            }
            else if(S == 1)
            {
                (r[Rd]>>31)&MASK_1BIT?setNeg():clrNeg();
                r[Rd]?clrZero():setZero();
                shifter_carry_out?setCarry():clrCarry();
                //V unaffected
            }
    		break;
    	}
    	case 13://mov
    	{
    	    r[Rd] = operand;
    	    if (S == 1 && Rd == 15)
    	    {
    	        //to be dealed
            }
            else if(S == 1)
            {
                (r[Rd]>>31)&MASK_1BIT?setNeg():clrNeg();
                r[Rd]?clrZero():setZero();
                shifter_carry_out?setCarry():clrCarry();
                //V unaffected
            }
    		break;
    	}
    	case 14://bic
    	{
    	    r[Rd] = r[Rn] & (~operand);
    	    if (S == 1 && Rd == 15)
    	    {
    	        //to be dealed
            }
            else if(S == 1)
            {
                (r[Rd]>>31)&MASK_1BIT?setNeg():clrNeg();
                r[Rd]?clrZero():setZero();
                shifter_carry_out?setCarry():clrCarry();
                //V unaffected
            }
    		break;
    	}
    	case 15://mvn
    	{
    	    r[Rd] = ~operand;
    	    if (S == 1 && Rd == 15)
    	    {
    	        //to be dealed
            }
            else if(S == 1)
            {
                (r[Rd]>>31)&MASK_1BIT?setNeg():clrNeg();
                r[Rd]?clrZero():setZero();
                shifter_carry_out?setCarry():clrCarry();
                //V unaffected
            }
    		break;
    	}
    	default:
            assert(0);
    		break;
    }

    if (Rn == 15)
        r[Rn] -= 4;
}

/** 
  * The matching pattern is 000 10xx 0.
  * @param instruction The instruction to be executed
  * @exception UnexpectInst For instruction can not be handled
  */
void ARM::misc_instr(A_INSTR instruction)
{
    char sec_code = (instruction>>4) & MASK_4BIT;

    switch (sec_code)
    {
    	case 0://mrs,msr
    	{
    	    int rs0_sr1 = (instruction>>21) & MASK_1BIT;
    	    int R = (instruction>>22) & MASK_1BIT;
    	    int Rd = (instruction>>12) & MASK_4BIT;

    	    if (rs0_sr1 == 0)//mrs
    	    {
    	        if (R == 1)
    	        {
    	            //to be dealed
                }
    	        else
    	        {
    	            r[Rd] = cpsr;
                }
            }
            else//msr,A4-76
            {
                UnexpectInst e;
                throw e;
            }
    		break;
    	}
    	case 1://bx,count leading zeros
    	{
    	    int opcode = (instruction>>21) & MASK_2BIT;

    	    if (opcode == 1)//bx
    	    {
    	        int Rm = (instruction) & MASK_4BIT;
// TODO (Birdman#1#): switch mode, T bit = Rm[0] A4-20

                if (r[Rm] & MASK_1BIT == 1)
                {
                    SwitchMode e;
                    throw e;
                }
    	        rPC = r[Rm] & 0xfffffffe;
            }
            else if(opcode == 3)//count leading zeros
            {
                int Rm = (instruction) & MASK_4BIT;
                int Rd = (instruction>>12) & MASK_4BIT;

                if (r[Rm] == 0)
                    r[Rd] = 32;
                else
                {
                    int count = 0;
                    while(!((r[Rm]>>(31-count)) & MASK_1BIT));
                    r[Rd] = 31 - count;
                }
            }
    		break;
    	}
    	case 2://bxj
    	{
    	    UnexpectInst e;
    	    throw e;
    		break;
    	}
    	case 3://blx
    	{
    	    int Rm = (instruction) & MASK_4BIT;

    	    rLR = rPC + 4;
// TODO (Birdman#1#): switch mode, T bit = Rm[0]
            if (r[Rm] & MASK_1BIT == 1)
            {
                SwitchMode e;
                throw e;
            }
    	    rPC = r[Rm] & 0xfffffffe;
            break;
    	}
    	case 5://qadd,qdadd,qsub,qdsub
    	{
            int opcode = (instruction>>21) & MASK_2BIT;

            UnexpectInst e;
            throw e;
    		break;
    	}
    	case 7://bkpt
    	{
    	    UnexpectInst e;
    		break;
    	}
    	case 8:case 10:case 12:case 14://smla,smlaw, smulw, smlal, smul
    	{
    	    int opcode = (instruction>>21) & MASK_2BIT;
    	    int x = (instruction>>5) & MASK_1BIT;
    	    int y = (instruction>>6) & MASK_1BIT;

            UnexpectInst e;
            throw e;
// TODO (Birdman#1#): back to finish it

    	    switch (opcode)
    	    {
    	    	case 0://smla
    	    	{
                    int operand1, operand2;
                    int Rd;
                    if (x == 0)
                        //operand1 = SignExtend(
    	    		break;
    	    	}
    	    	case 1://smlaw(bit5=0),smulw(bit5=1)
    	    	{
    	    		break;
    	    	}
                case 2://smlal
                {
                	break;
                }
                case 3://smul
                {
                	break;
                }
    	    	default:
                    assert(0);
    	    		break;
    	    }

    		break;
    	}
    	default:
            assert(0);
    		break;
    }

}

/** 
  * The matching pattern is 000  1(bit7)   1(bit4).
  * @param instruction The instruction to be executed
  * @exception UnexpectInst For instruction can not be handled
  */
void ARM::multiplies(A_INSTR instruction)
{
    UnexpectInst e;
    throw e;
}

/**
  * The matching pattern is 000 1(bit7)   1(bit4).
  * @param instruction The instruction to be executed
  */
void ARM::extra_ld_str(A_INSTR instruction)
{
    int sec_code = (instruction>>5) & MASK_2BIT;

    switch (sec_code)
    {
    	case 0://swp, swpb, ldrex, strex,
    	{

    		break;
    	}
    	case 1://ldrh, strh,
    	{
    		break;
    	}
        case 2:case 3://A3-38
        {
        	break;
        }
    	default:
            assert(0);
    		break;
    }

}

/** 
  * The matching pattern is 001 10 R 10.
  * @param instruction The instruction to be executed
  */
void ARM::mov_imm_to_status_reg(A_INSTR instruction)
{

}

/**
  * The matching pattern is 010.
  * @param instruction The instruction to be executed
  */
void ARM::ld_str_imm_off(A_INSTR instruction)
{
    int L = (instruction>>20) & MASK_1BIT;
    int B = (instruction>>22) & MASK_1BIT;
    int shifter_carry_out;
    int operand = shifter_operand(instruction, IMM_OFF, shifter_carry_out);
    int Rd = (instruction>>12) & MASK_4BIT;


    if (L == 1 && B == 0)//LDR
    {
        int data = my_mmu->get_word(operand);
        if (Rd == 15)
        {
            rPC = data & 0xfffffffc;
        }
        else
            r[Rd] = data;
    }

    if (L == 1 && B == 1)//LDRB
    {
        r[Rd] = my_mmu->get_byte(operand);
    }

    if (L == 0 && B == 0)//str
    {
        my_mmu->set_word(operand, r[Rd]);
    }

    if (L == 0 && B == 1)//strb
    {
        BYTE tmp = r[Rd] & MASK_8BIT;
        my_mmu->set_byte(operand, tmp);
    }

}

/**
  * The matching pattern is 011 0(bit4).
  * @param instruction The instruction to be executed
  */
void ARM::ld_str_reg_off(A_INSTR instruction)
{

}

/** 
  * The matching pattern is 011 1(bit4).
  * @param instruction The instruction to be executed
  */
void ARM::media_instr(A_INSTR instruction)
{

}

/**
  * The matching pattern is 100.
  * @param instruction The instruction to be executed
  */
void ARM::ld_str_multiple(A_INSTR instruction)
{

}

/**
  * The matching pattern is 101.
  * @param instruction The instruction to be executed
  */
void ARM::branch_or_with_link(A_INSTR instruction)
{

}

/**
  * The matching pattern is 1111.
  * @param instruction The instruction to be executed
  */
void ARM::swi_handler(A_INSTR instruction)
{
    int imm_24 = (instruction) & 0xffffff;

    if (imm_24 == 0x123456)
    {
        swi.get_para(r);
        swi.swi_handler();
    }
}





/**
  * According to the condition field, determine whether it matches the CPSR, if matches, return true, otherwise false.
  * @param cond The conditon field.
  * @ return Whether condition field matches the CPSR.
  */
int ARM::ConditionPassed(unsigned char cond)
{
    int res = 0;
    switch (cond)//A3-4,page112
    {
    	case 0://EQ
            if (getZero())
                res = 1;
    		break;
        case 1://NE
            if (!getZero())
                res = 1;
            break;
        case 2://CS/HS
            if (getCarry())
                res = 1;
            break;
    	case 3://CC/LO
            if (!getCarry())
                res = 1;
            break;
    	case 4://MI
            if (getNeg())
                res = 1;
            break;
    	case 5://PL
            if (!getNeg())
                res = 1;
            break;
    	case 6://VS
            if (getOverflow())
                res = 1;
            break;
    	case 7://VC
            if (!getOverflow())
                res = 1;
            break;
    	case 8://HI
            if (getCarry() && !getZero())
                res = 1;
            break;
    	case 9://LS
            if (!getCarry() && getZero())
                res = 1;
            break;
        case 10://GE
            if (getNeg() == getOverflow())
                res = 1;
            break;
    	case 11://LT
            if (getNeg() != getOverflow())
                res = 1;
            break;
    	case 12://GT
            if (getZero() == 0 && getNeg() == getOverflow())
                res = 1;
            break;
    	case 13://LE
            if (getZero() == 1 && getNeg() != getOverflow())
                res = 1;
            break;
    	case 14://AL
            res = 1;
            break;
    	default:
            assert(0);
            //error
    		break;
    }

    return res;

}



/**
  * Give out the current MMU pointer(for mode switch use).
  * @return The pointer to current MMU.
  */
MMU * ARM::get_mmu()
{
    if (my_mmu != 0)
        return my_mmu;
}

/**
  * Get a new MMU modular, get stack pointer from MMU, get program's entry pointer from MMU, initialize SWI.
  */
void ARM::InitMMU()
{
    my_mmu = new MMU;

    rSP = my_mmu->getStackTop();
    rPC = my_mmu->getEntry();

    InitSWI();
}

/**
  * Delete the MMU modular.
  */
void ARM::DeinitMMU()
{
    if (my_mmu != NULL)
        delete my_mmu;
}

/**
  * Get heap information, in order to give it to program, get MMU pointer, for getting parameter and write result.
  */
void ARM::InitSWI()
{
    swi.getHeapInfo(my_mmu->getHeapTop(), my_mmu->getHeapSz(), my_mmu->getStackTop(), my_mmu->getStackSz());
    swi.getMMU(my_mmu);
}

/**
  * Not done, never used.
  * @return The CPSR value.
  */
EFLAG ARM::get_eflag()
{

}

/**
  * Not done, never used.
  * @param p_eflag The process status to be set.
  */
void ARM::set_eflag(EFLAG p_eflag)
{

}
/**
  * Copy the general purpose register, process status register, and MMU pointer.
  * @param reg The pointer to array of general purpose registers
  * @param flags The reference of process status register
  * @param mmu The pointer to MMU modular 
  */
void ARM::getRegs(GP_Reg reg[], EFLAG &flags, MMU* &mmu)
{
    for (int i = 0; i < GPR_num; i++)
        reg[i] = r[i];

    flags = cpsr;
    mmu = my_mmu;

}

/**
  * Copy a CPU content to current one(for mode switch use).
  * @param a_cpu The pointer to another CPU
  */
void ARM::CopyCPU(CPU *a_cpu)
{
    static_cast<Thumb *>(a_cpu)->getRegs(r, cpsr, my_mmu);
    InitSWI();
}

/**
  * Not done, never used.
  * @param arg The pointer to an argument string
  * @param len The length of the string
  */
void ARM::getArg(char *arg, int len)
{
}

