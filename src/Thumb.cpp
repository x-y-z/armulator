#include "Thumb.h"
#include "error.h"
#include "ARM.h"

/**
  * Zero general purpose registers, current process status register, make MMU pointer null.
  */
Thumb::Thumb()
{
	for (int i = 0; i < GPR_num; i++)
		r[i] = 0;

	cpsr = 0;

	my_mmu = NULL;
}

/**
  * Nothing to do, the MMU modular can be deleted here.
  */
Thumb::~Thumb()
{
   // if (my_mmu != NULL)
   //     delete my_mmu;
}



/**
  * Get a 16-bit instruction from MMU modular, and increase PC by 2.
  */
void Thumb::fetch()
{
    cur_instr = my_mmu->getInstr(rPC);
    rPC += 2;
}

/**
  * Execute all kinds of 16-bit Thumb instruction, the instructions are classified by most significant 3 bit.
  * @exception UndefineInst For undefined instructions
  * @exception UnexpectInst For instructions can not be handled
  */
STATUS Thumb::exec()
{
    //if (rPC == 0x88d6)
    //    printf("");
    //debug breakpoint

    char attemp_code = cur_instr>>13, sec_code;
    switch(attemp_code)//[15:13]
    {
        case 0://000:shift by imm(00,01,10), add/sub reg(110), add/sub imm(111)
        {
            sec_code = (cur_instr>>11) & MASK_2BIT;
            if (sec_code == 3)
            {
                //add/sub reg, add/sub imm
                add_sub_reg_or_imm(cur_instr);
            }
            else
            {
                //shift by imm
                shift_by_imm(cur_instr);
            }
            break;
        }
        case 1://001:add(10)/sub(11)/mov(00)/cmp(01) imm
        {
            add_sub_mov_cmp_imm(cur_instr);
            //handle with func directly
            break;
        }
        case 2://010:data-processing reg(000), special data processing(001(00-10)), branch/exechange IS(00111), ld from literal pool(01), ld/st reg offset(1)
        {
            sec_code = (cur_instr>>12) & MASK_1BIT;
            if (sec_code == 1)
            {
                //ld/st reg offset
                ld_str_reg_offset(cur_instr);
            }
            else if (((cur_instr>>11) & MASK_2BIT) == 1)
            {
                //ld from literal pool
                ld_from_pool(cur_instr);
            }
            else if (((cur_instr>>10) & MASK_3BIT) == 1)
            {
                if (((cur_instr>>8) & MASK_2BIT) == 3)//11
                {
                    //branch/exec IS
                    br_or_exec_is(cur_instr);
                }
                else
                {
                    //special data processing
                    spec_data_proc(cur_instr);
                }
            }
            else//[12:10] = 0
            {
                //data-processing reg
                data_proc_reg(cur_instr);
            }
            break;
        }
        case 3://011:ld/st word/byte imm
        {
            //ld/st word/byte imm
            ld_str_word_byte_imm(cur_instr);
            break;
        }
        case 4://100:ld/st halfword imm,(0), ld/st to/from stack(1)
        {
            sec_code = (cur_instr>>12) & MASK_1BIT;
            if (sec_code == 1)
            {
                //ld/st to/from stack
                ld_str_stack(cur_instr);
            }
            else
            {
                //ld/st halfword imm
                ld_str_halfw_imm(cur_instr);
            }
            break;
        }
        case 5://101:Add to SP or PC(0),Misc(1)
        {
            sec_code = (cur_instr>>12) & MASK_1BIT;
            if (sec_code == 1)
            {
                //Misc
                misc(cur_instr);
            }
            else
            {
                //Add to SP or PC
                add_to_sp_or_pc(cur_instr);
            }
            break;
        }
        case 6://110:ld/st multiple(0),con branch(1), undef(1110), software int(1111)    ld/st will be detected first
        {
            sec_code = (cur_instr>>12) & MASK_1BIT;
            if (sec_code == 0)
            {
                //ld/st multiple
                ld_str_multiple(cur_instr);
            }
            else if (((cur_instr>>8) & MASK_4BIT) < 0xe)
            {
                //con branch
                con_br(cur_instr);
            }
            else if ((sec_code = (cur_instr>>8) & MASK_4BIT) == 0xe)
            {
                //error
                UndefineInst e;
                throw e;
            }
            else//swi == 0xf
            {
                int imm_8 = cur_instr & MASK_8BIT;

                //printf("Aloha SWI  0x%x\n",r[0]);
                if (imm_8 == 0xab)
                {
                    swi.get_para(r);
                    swi.swi_handler();
                }
                else
                {
                    UnexpectInst e;
                    char tmp[30];
                    sprintf(tmp,"%x : %x",rPC-2, cur_instr);
                    e.error_name = tmp;
                    throw e;
                }

            }
            //other conditions will not be considered, because they can't appear
            break;
        }
        case 7://111:uncon branch(00), BLX suffix(01)last bit=0, undef(01xxxxxxxxxx1), BL/BLX prefix(10), BL suffix(11)
        {
            sec_code = (cur_instr>>11) & MASK_2BIT;
            if (sec_code == 0)
            {
                //uncon branch
                uncon_br(cur_instr);
            }
            else if(sec_code == 1)
            {
                if ((cur_instr & MASK_1BIT) == 0)
                {
                    //BLX suffix
                    blx_suffix(cur_instr);
                }
                else
                {
                    UndefineInst e;
                    char tmp[30];
                    sprintf(tmp,"%x : %x",rPC-2, cur_instr);
                    e.error_name = tmp;
                    throw e;
                    //error
                }
            }
            else if(sec_code == 2)
            {
                //BL/BLX prefix
                bl_blx_prefix(cur_instr);
            }
            else//sec_code == 3
            {
                //BL suffix
                bl_suffix(cur_instr);
            }
            break;
        }
        default://undef
            assert(0);
            //error
            break;
    }

    return 0;
}

/**
  * get the register value according to its name. 
  */
GP_Reg Thumb::get_reg_by_name(const char *reg_name)
{
    return 0;
}

/**
  * get the register value according to its index. 
  */
GP_Reg Thumb::get_reg_by_code(int reg_code)
{
    return 0;
}


/**
  * never used
  * @param p_eflag The process status to be set.
  */
void Thumb::set_eflag(EFLAG p_eflag)
{
	cpsr = p_eflag;
}

/**
  * never used
  * @return The CPSR value.
  */
EFLAG Thumb::get_eflag()
{
	return cpsr;
}


/**
  * The matching pattern is 000110 or 000111.
  * @param instruction The instruction to be executed.
  */
void Thumb::add_sub_reg_or_imm(const T_INSTR instruction)
{
    int imm =  (instruction>>10) & MASK_1BIT;//bit 10, estimate imm or reg, imm will be used to estimate add or sub subsequently
    int Rm, Rn, Rd, imm_3;

    if (imm == 1)
    {
        //add/sub imm
        imm_3 = (instruction>>6) & MASK_3BIT;
        Rn = (instruction>>3) & MASK_3BIT;
        Rd = (instruction) & MASK_3BIT;

        if (((instruction>>9) & MASK_1BIT) == 0)//add
        {
            //execuate
            r[Rd] = r[Rn] + imm_3;
            //eflag!!!
            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
            r[Rd]?clrZero():setZero();//Z flag
            CarryFrom(r[Rn], imm_3)?setCarry():clrCarry();//C flag
            OverflowFrom(r[Rn], imm_3)?setOverflow():clrOverflow();//V flag
        }
        else//sub
        {
            //executate
            r[Rd] = r[Rn] - imm_3;
            //eflags
            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
            r[Rd]?clrZero():setZero();//Z flag
            ((unsigned)r[Rn] >= (unsigned)imm_3)?setCarry():clrCarry();//C flag
            OverflowFrom(r[Rn], -imm_3)?setOverflow():clrOverflow();//V flag
        }
    }
    else//reg add sub
    {
        //add/sub reg
        Rm = (instruction>>6) & MASK_3BIT;
        Rn = (instruction>>3) & MASK_3BIT;
        Rd = (instruction) & MASK_3BIT;

        if (((instruction>>9) & MASK_1BIT) == 0)//add
        {
            //execute
            r[Rd] = r[Rn] + r[Rm];
            //eflag
            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
            r[Rd]?clrZero():setZero();//Z flag
            CarryFrom(r[Rn], r[Rm])?setCarry():clrCarry();//C flag
            OverflowFrom(r[Rn], r[Rm])?setOverflow():clrOverflow();//V flag

        }
        else//sub
        {
            //execute
            r[Rd] = r[Rn] - r[Rm];
            //eflags
            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
            r[Rd]?clrZero():setZero();//Z flag
            ((unsigned)r[Rn] >= (unsigned)r[Rm])?setCarry():clrCarry();//C flag
            OverflowFrom(r[Rn], -r[Rm])?setOverflow():clrOverflow();//V flag

        }
    }

}

/**
  * The matching pattern is 000 opcode.
  * @param instruction The instruction to be executed.
  */
void Thumb::shift_by_imm(const T_INSTR instruction)
{
    int shift_kind = (instruction>>11) & MASK_2BIT;
    int immed_5 = (instruction>>6) & MASK_5BIT;
    int Rm = (instruction>>3) & MASK_3BIT;
    int Rd = (instruction) & MASK_3BIT;

    switch (shift_kind)
    {
    	case 0://LSL
    	{
    	    if (immed_5 == 0)
    	    {
                r[Rd] = r[Rm];
    	    }
            else
            {
                //execute
                r[Rd] = r[Rm] << immed_5;
                ((r[Rm]>>(32-immed_5)) & MASK_1BIT)?setCarry():clrCarry();
            }
            //eflags, V flag unaffected
            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
            r[Rd]?clrZero():setZero();//Z flag

            break;
    	}
        case 1://LSR
        {
            if (immed_5 == 0)
            {
                ((r[Rm]>>31) & MASK_1BIT)?setCarry():clrCarry();
                r[Rd] = 0;
            }
            else// immed_5 > 0
            {
                //execute
                r[Rd] = (unsigned)r[Rm] >> immed_5;//change r[Rm] to unsigned, in order to implment LSR
                (r[Rm]>>(immed_5 - 1) & MASK_1BIT)?setCarry():clrCarry();
            }
            //eflags V flag unaffected
            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
            r[Rd]?clrZero():setZero();//Z flag

            break;
        }
        case 2://ASR
        {
            if (immed_5 == 0)
            {
                ((r[Rm]>>31) & MASK_1BIT)?setCarry():clrCarry();
                if (((r[Rm]>>31) & MASK_1BIT) == 0)
                    r[Rd] = 0;
                else// == 1
                    r[Rd] = 0xffffffff;
            }
            else// immed_5 > 0
            {
                //execute
                r[Rd] = r[Rm] >> immed_5;
                (r[Rm]>>(immed_5 - 1) & MASK_1BIT)?setCarry():clrCarry();
            }
            //eflags V flag unaffected
            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
            r[Rd]?clrZero():setZero();//Z flag

            break;
        }
    	default:
            assert(0);
            //error
    		break;
    }

}

/**
  * The matching pattern is 001, the subroutine is add(10)/sub(11)/mov(00)/cmp(01).
  * @param instruction The instruction to be executed.
  */
void Thumb::add_sub_mov_cmp_imm(const T_INSTR instruction)
{
    int opcode = (instruction>>11) & MASK_2BIT;
    int Rd = (instruction>>8) & MASK_3BIT;
    int imm = (instruction) & MASK_8BIT;

    switch (opcode)
    {
    	case 2://add
    	{
    	    CarryFrom(r[Rd], imm)?setCarry():clrCarry();//C flag
            OverflowFrom(r[Rd], imm)?setOverflow():clrOverflow();//V flag
            //execute
            r[Rd] = r[Rd] + imm;
            //flags
            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
            r[Rd]?clrZero():setZero();//Z flag

            break;
    	}
        case 3://sub
        {
            ((unsigned)r[Rd] >= (unsigned)imm)?setCarry():clrCarry();//C flag
            OverflowFrom(r[Rd], -imm)?setOverflow():clrOverflow();//V flag
            //execute
            r[Rd] = r[Rd] - imm;
            //flags
            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
            r[Rd]?clrZero():setZero();//Z flag


            break;
        }
        case 0://mov
        {
            //execute
            r[Rd] = imm;
            //flags
            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
            r[Rd]?clrZero():setZero();//Z flag

            break;
        }
        case 1://cmp
        {
            //execute
            int alu_out = r[Rd] - imm;
            //flags
            ((alu_out>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
            alu_out?clrZero():setZero();//Z flag
            ((unsigned)r[Rd] >= (unsigned)imm)?setCarry():clrCarry();//C flag
            OverflowFrom(r[Rd], -imm)?setOverflow():clrOverflow();//V flag

            break;
        }
    	default:
            assert(0);
            //error
    		break;
    }

}

/**
  * The matching pattern is 0101.
  * @param instruction The instruction to be executed.
  * @exception UnexpectInst For instructions can not be handled
  */
void Thumb::ld_str_reg_offset(const T_INSTR instruction)
{
    int opcode = (instruction>>9) & MASK_3BIT;
    int Rm = (instruction>>6) & MASK_3BIT;
    int Rn = (instruction>>3) & MASK_3BIT;
    int Rd = (instruction) & MASK_3BIT;

    switch (opcode)
    {
    	case 4://LDR(2)
    	{
            int addr = r[Rm] + r[Rn];
            if ((addr & MASK_2BIT) == 0)
                r[Rd] = my_mmu->get_word(addr);
            else
            {
                UnexpectInst e;
                char tmp[50];
                sprintf(tmp,"%x : %x -- Load Error, No aligned\n",rPC-2, cur_instr);
                e.error_name = tmp;
                throw e;
            }
            break;
    	}
        case 6://LDRB
        {
            int addr = r[Rn] + r[Rm];
            r[Rd] = my_mmu->get_byte(addr) & 0xff;//return is unsigned, automatically zero extend

            break;
        }
        case 5://LDRH
        {
            int addr = r[Rn] + r[Rm];
            if ((addr & MASK_1BIT) == 0)
                r[Rd] = my_mmu->get_halfword(addr) & 0xffff;//zero extend
            else
            {
                UnexpectInst e;
                char tmp[50];
                sprintf(tmp,"%x : %x -- Load Error, No aligned\n",rPC-2, cur_instr);
                e.error_name = tmp;
                throw e;
            }
            break;
        }
        case 3://LDRSB
        {
            int addr = r[Rn] + r[Rm];
            r[Rd] = SignExtend(my_mmu->get_byte(addr), 8);//sign extend
            break;
        }
        case 7://LDRSH
        {
            int addr = r[Rn] + r[Rm];

            if (addr & MASK_1BIT != 0)
            {
                UnexpectInst e;
                e.error_name = "Not halfword aligned\n";
                throw e;
            }

            r[Rd] = SignExtend(my_mmu->get_halfword(addr), 16);//sign extend
        	break;
        }
        case 0://STR(2)
        {
            int addr = r[Rm] + r[Rn];
            if ((addr & MASK_2BIT) == 0)
                my_mmu->set_word(addr, r[Rd]);
            else
            {
                UnexpectInst e;
                char tmp[50];
                sprintf(tmp,"%x : %x -- Store Error, No aligned\n",rPC-2, cur_instr);
                e.error_name = tmp;
                throw e;
            }
            break;
        }
        case 2://STRB(2)
        {
            int addr = r[Rn] + r[Rm];
            BYTE tmp = r[Rd] & MASK_8BIT;

            if ((addr & MASK_2BIT) == 0)
                my_mmu->set_byte(addr, tmp);//return is unsigned, automatically zero extend
            else
            {
                UnexpectInst e;
                char tmp[50];
                sprintf(tmp,"%x : %x -- Store Error, No aligned\n",rPC-2, cur_instr);
                e.error_name.append(tmp);
                throw e;
            }
        	break;
        }
        case 1://STRH(2)
        {
            int addr = r[Rn] + r[Rm];
            HALFWORD tmp = r[Rd] & 0xffff;
            if ((addr & MASK_1BIT) == 0)
                my_mmu->set_halfword(addr, tmp);//zero extend
            else
            {
                UnexpectInst e;
                char tmp[50];
                sprintf(tmp,"%x : %x -- Load Error, No aligned\n",rPC-2, cur_instr);
                e.error_name = tmp;
                throw e;
            }
        	break;
        }
    	default:
            assert(0);
    		break;
    }


}

/**
  * The matching pattern is 01001.
  * @param instruction The instruction to be executed.
  */
void Thumb::ld_from_pool(const T_INSTR instruction)
{
    int Rd = (instruction>>8) & MASK_3BIT;
    int offset = (instruction) & MASK_8BIT;
    int address = ((rPC + 2) & 0xfffffffc) + (offset * 4);//pc + 2, due to pipeline

    r[Rd] = my_mmu->get_word(address);//memory accessA7-51

}

/**
  * The matching pattern is 01000111.
  * @param instruction The instruction to be executed.
  * @exception UnexpectInst For instructions can not be handled
  */
void Thumb::br_or_exec_is(const T_INSTR instruction)
{
    int L = (instruction>>7) & MASK_1BIT;
    int H2 = (instruction>>6) & MASK_1BIT;
    int Rm = (instruction>>3) & MASK_3BIT | (H2<<3);
    //int SBZ = (instruction) & MASK_3BIT;

    if (L == 1)//BLX
    {
        rLR = (rPC) | 1;//A7-30,rLR = (rPC + 2) | 1;
        // NOTE (Birdman#1#): change rLR= (rPC + 2) | 1 to rLR = (rPC) | 1

        rPC = r[Rm] & 0xfffffffe;
        //flags
        if ((r[Rm] & MASK_1BIT) == 0)
        {
            UnexpectInst e;
            char tmp[40];
            sprintf(tmp,"BLX Mode Switch not supported %x : %x\n",rPC-2, cur_instr);
            e.error_name = tmp;
            throw e;
        }

    }
    else//BX
    {
        rPC = r[Rm] & 0xfffffffe;
        //flags
        if ((r[Rm] & MASK_1BIT) == 0)
        {
            UnexpectInst e;
            char tmp[40];
            sprintf(tmp,"BX Mode Switch not supported %x : %x\n",rPC-2, cur_instr);
            e.error_name = tmp;
            throw e;
        }

    }

}

/**
  * The matching pattern is 010001 + opcode.
  * @param instruction The instruction to be executed.
  */
void Thumb::spec_data_proc(const T_INSTR instruction)
{
    int opcode =  (instruction>>8) & MASK_2BIT;
    int H1 = (instruction>>7) & MASK_1BIT;
    int H2 = (instruction>>6) & MASK_1BIT;
    int Rm = (instruction>>3) & MASK_3BIT | (H2<<3);
    int Rn = (instruction) & MASK_3BIT | (H1<<3);

    switch (opcode)
    {
    	case 1://cmp(3)
    	{
            int alu_out = r[Rn] - r[Rm];
            //flags
            ((alu_out>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
            alu_out?clrZero():setZero();//Z flag
            ((unsigned)r[Rn] >= (unsigned)r[Rm])?setCarry():clrCarry();//C flag
            OverflowFrom(r[Rn], -r[Rm])?setOverflow():clrOverflow();//V flag

    		break;
    	}
        case 2://cpy
        {
            r[Rn] = r[Rm];
            //no flags
            break;
        }
        case 0://add(4)
        {
            r[Rn] = r[Rn] + r[Rm];
            //no flags
            break;
        }
    	default:
            assert(0);
            //error
    		break;
    }

}

/**
  * The matching pattern is 010000 opcode.
  * @param instruction The instruction to be executed.
  */
void Thumb::data_proc_reg(const T_INSTR instruction)
{
    int opcode = (instruction>>6) & MASK_4BIT;
    int Rs = (instruction>>3) & MASK_3BIT;
    int Rd = (instruction) & MASK_3BIT;

    switch (opcode)
    {
    	case 5://adc
    	{

            if (getCarry() == 0)
            {
                CarryFrom(r[Rd], r[Rs])?setCarry():clrCarry();//C flag
                OverflowFrom(r[Rd], r[Rs])?setOverflow():clrOverflow();//V flag
            }
            else// carry == 1
            {
                if (CarryFrom(r[Rd], r[Rs]+1)
                 || CarryFrom(r[Rd]+1, r[Rs])
                 || CarryFrom(r[Rd], r[Rs]))// in case one of them is 0xffffffff, or both are
                    setCarry();
                else
                    clrCarry();

                if (OverflowFrom(r[Rd], r[Rs]+1)
                 || OverflowFrom(r[Rd]+1, r[Rs])
                 || OverflowFrom(r[Rd], r[Rs]))
                    setOverflow();
                else
                    clrOverflow();
            }
            //execute
            r[Rd] = r[Rd] + r[Rs] + getCarry();
            //flags
            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
            r[Rd]?clrZero():setZero();//Z flag

    		break;
    	}
        case 0://and, C flag & V flag unaffected
        {
            r[Rd] = r[Rd] & r[Rs];
            //flags
            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
            r[Rd]?clrZero():setZero();//Z flag

            break;
        }
        case 4://asr(2)
        {
            int shifter = r[Rs] & 0xff;
            if ( shifter < 32 && shifter != 0)
            {
                if ((r[Rd] & (1<<(shifter-1))) == 0)
                    clrCarry();
                else
                    setCarry();
                r[Rd] = r[Rd] >> shifter;
            }
            else if( shifter >= 32)
            {
                if ((r[Rd] & (1<<31)) == 0)//(r[Rd] & (1<<31))?setCarry():clrCarry();
                {
                    clrCarry();
                    r[Rd] = 0;
                }
                else
                {
                    setCarry();
                    r[Rd] = 0xffffffff;
                }

            }//shifter == 0, nothing will be done

            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();
            r[Rd]?clrZero():setZero();

            break;
        }
        case 14://bic C flag & V flag unaffected
        {
            r[Rd] = r[Rd] & ~(r[Rs]);
            //flags
            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
            r[Rd]?clrZero():setZero();//Z flag

            break;
        }
        case 11://CMN
        {
            int alu_out = r[Rd] + r[Rs];
            //flags
            ((alu_out>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
            alu_out?clrZero():setZero();//Z flag
            CarryFrom(r[Rd], r[Rs])?setCarry():clrCarry();//C flag
            OverflowFrom(r[Rd], r[Rs])?setOverflow():clrOverflow();//V flag

            break;
        }
        case 10://cmp
        {
            int alu_out = r[Rd] - r[Rs];
            //flags
            ((alu_out>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
            alu_out?clrZero():setZero();//Z flag
            ((unsigned)r[Rd] >= (unsigned)r[Rs])?setCarry():clrCarry();//C flag
            OverflowFrom(r[Rd], -r[Rs])?setOverflow():clrOverflow();//V flag

            break;
        }
        case 1://EOR
        {
            r[Rd] = r[Rd] ^ r[Rs];
            //flags
            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
            r[Rd]?clrZero():setZero();//Z flag

            break;
        }
        case 2://LSL,A7-67
        {
            int shifter = r[Rs] & 0xff;
            if ( shifter < 32 && shifter != 0)
            {
                if ((r[Rd] & (1<<(32-shifter))) == 0)
                    clrCarry();
                else
                    setCarry();
                r[Rd] = r[Rd] << shifter;
            }
            else if( shifter == 32)
            {
                if ((r[Rd] & MASK_1BIT) == 1)
                    setCarry();
                else
                    clrCarry();

                r[Rd] = 0;
            }
            else if (shifter > 32)//shifter > 32
            {
                clrCarry();
                r[Rd] = 0;
            }//else shifter == 0, unaffected

            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();
            r[Rd]?clrZero():setZero();

            break;
        }
        case 3://LSR(2)
        {
            int shifter = r[Rs] & 0xff;
            if ( shifter < 32 && shifter != 0)
            {
                if ((r[Rd] & (1<<(shifter-1))) == 0)
                    clrCarry();
                else
                    setCarry();
                r[Rd] = (unsigned)r[Rd] >> shifter;//use unsigned to implement lsr
            }
            else if( shifter == 32)
            {
                if (((r[Rd]>>31) & MASK_1BIT) == 1)
                    setCarry();
                else
                    clrCarry();

                r[Rd] = 0;
            }
            else if (shifter > 32)//shifter > 32
            {
                clrCarry();
                r[Rd] = 0;
            }//else shifter == 0, unaffected

            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();
            r[Rd]?clrZero():setZero();

            break;
        }
        case 13://MUL
        {
            long long rd = r[Rd] & 0xffffffff, rs = r[Rs] & 0xffffffff;
// NOTE (Birdman#1#): try long long, in case of data loss

            r[Rd] = (rd * rs) & 0xffffffff;
            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();
            r[Rd]?clrZero():setZero();

            break;
        }
        case 15://mvn
        {
            r[Rd] = ~r[Rs];
            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();
            r[Rd]?clrZero():setZero();
            break;
        }
        case 9://neg
        {
            r[Rd] = 0 - r[Rs];
            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();
            r[Rd]?clrZero():setZero();
            //C,V flags
            ((unsigned)0 >= (unsigned)r[Rs])?setCarry():clrCarry();//C flag
            OverflowFrom(0, -r[Rs])?setOverflow():clrOverflow();//V flag

            break;
        }
        case 12://orr
        {
            r[Rd] = r[Rd] | r[Rs];
            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
            r[Rd]?clrZero():setZero();//Z flag
            break;
        }
        case 7://ror
        {
            int shifter = r[Rs] & 0xff;
            if (shifter == 0)
            {
                ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();
                r[Rd]?clrZero():setZero();
                break;
            }

            if ((shifter & MASK_5BIT) == 0)
            {
                if (((r[Rd]>>31) & MASK_1BIT) == 1)
                    setCarry();
                else
                    clrCarry();

            }
            else
            {
                if (((r[Rd]>>((shifter & MASK_5BIT) - 1)) & MASK_1BIT) == 1)
                    setCarry();
                else
                    clrCarry();

                /*for (int i = (shifter & MASK_5BIT); i > 0; i--)
                {
                    (r[Rd] & MASK_1BIT)?r[Rd] = ((unsigned)r[Rd] >> 1) | (1<<31):r[Rd] = (unsigned)r[Rd] >> 1;
                }*/
                r[Rd] = (((r[Rd]) >> (shifter & MASK_5BIT)) | ((r[Rd]) << (32 - (shifter & MASK_5BIT))));
            }

            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();
            r[Rd]?clrZero():setZero();
            break;
        }
        case 6://sbc
        {
            if (getCarry() == 1)
            {
                ((unsigned)r[Rd] >= (unsigned)r[Rs])?setCarry():clrCarry();//C flag
                OverflowFrom(r[Rd], -r[Rs])?setOverflow():clrOverflow();//V flag
            }
            else// carry == 0
            {
                if ((unsigned)r[Rd] >= (unsigned)(r[Rs] + 1))// in case one of them is 0xffffffff, or both are
                    setCarry();
                else
                    clrCarry();

                if (OverflowFrom(r[Rd], - r[Rs] - 1))
                    setOverflow();
                else
                    clrOverflow();
            }

            r[Rd] = r[Rd] - r[Rs] - getCarry()?0:1;

            ((r[Rd]>>31) & MASK_1BIT)?setNeg():clrNeg();//N flag
            r[Rd]?clrZero():setZero();//Z flag
            break;
        }
        case 8://tst
        {
            int alu_out = r[Rd] & r[Rs];
            (alu_out & (1<<31))?setNeg():clrNeg();//N flag
            alu_out?clrZero():setZero();//Z flag

            break;
        }
    	default:
            assert(0);
    		break;
    }


}

/**
  * The matching pattern is 011 BL.
  * @param instruction The instruction to be executed.
  * @exception UnexpectInst For instructions can not be handled
  */
void Thumb::ld_str_word_byte_imm(const T_INSTR instruction)
{
    int B = (instruction>>12) & MASK_1BIT;// 1:byte, 0:word
    int L = (instruction>>11) & MASK_1BIT;// 1:load, 0:store
    int offset = (instruction>>6) & MASK_5BIT;
    int Rn = (instruction>>3) & MASK_3BIT;
    int Rd = (instruction) & MASK_3BIT;



//memory access, LDR(1), A7-47, 01101
    if (B == 0 && L == 1)//load word
    {
        int addr = r[Rn] + (offset * 4);

        if (addr & MASK_2BIT != 0)
        {
            UnexpectInst e;
            char tmp[20];
            sprintf(tmp,"%x : %x",rPC-2, cur_instr);
            e.error_name = tmp;
            throw e;
        }
        r[Rd] = my_mmu->get_word(addr);
    }

//memory access, LDRB(1), A7-55, 01111
    if (B == 1 && L == 1)//load byte
    {
        int addr = r[Rn] + offset;

        r[Rd] = my_mmu->get_byte(addr);
    }



//memory access, STR(1), A7-99, 01100
    if (B == 0 && L == 0)
    {
        int addr = r[Rn] + (offset * 4);
        if (addr & MASK_2BIT != 0)
        {
            UnexpectInst e;
            char tmp[20];
            sprintf(tmp,"%x : %x",rPC-2, cur_instr);
            e.error_name = tmp;
            throw e;
        }
        my_mmu->set_word(addr, r[Rd]);
    }


//memory access, STRB(1), A7-105, 01110
    if (B == 1 && L == 0)//store byte
    {
        int addr = r[Rn] + offset;
        BYTE temp = r[Rd] & MASK_8BIT;

        my_mmu->set_byte(addr, temp);
    }

}

/**
  * The matching pattern is 1001 L.
  * @param instruction The instruction to be executed.
  * @exception UnexpectInst For instructions can not be handled
  */
void Thumb::ld_str_stack(const T_INSTR instruction)
{
    int L = (instruction>>11) & MASK_1BIT;// 1:load, 0:store
    int Rd = (instruction>>8) & MASK_3BIT;
    int imm = (instruction) & MASK_8BIT;

// memory access, LDR(4), A7-53
    if (L == 1)
    {
        int addr = rSP + (imm * 4);
        if (addr & MASK_2BIT != 0)
        {
            UnexpectInst e;
            char tmp[20];
            sprintf(tmp,"%x : %x",rPC-2, cur_instr);
            e.error_name = tmp;
            throw e;
        }

        r[Rd] = my_mmu->get_word(addr);
    }
//(Birdman#1#): memory access, STR(3), A7-103
    if (L == 0)
    {
        int addr = rSP + (imm * 4);
        if (addr & MASK_2BIT != 0)
        {
            UnexpectInst e;
            char tmp[20];
            sprintf(tmp,"%x : %x",rPC-2, cur_instr);
            e.error_name = tmp;
            throw e;
        }

        my_mmu->set_word(addr, r[Rd]);
    }

}

/**
  * The matching pattern is 1000 L.
  * @param instruction The instruction to be executed.
  * @exception UnexpectInst For instructions can not be handled
  */
void Thumb::ld_str_halfw_imm(const T_INSTR instruction)
{
    int L = (instruction>>11) & MASK_1BIT;// 1:load, 0:store
    int imm = (instruction>>6) & MASK_5BIT;
    int Rn = (instruction>>3) & MASK_3BIT;
    int Rd = (instruction) & MASK_3BIT;

// memory access, LDRH(1), A7-57, 10001
    if (L == 1)
    {
        int addr = r[Rn] + (imm * 2);
        if (addr & MASK_1BIT != 0)
        {
            UnexpectInst e;
            char tmp[20];
            sprintf(tmp,"%x : %x",rPC-2, cur_instr);
            e.error_name = tmp;
            throw e;
        }

        r[Rd] = my_mmu->get_halfword(addr) & 0xffff;//zero extend
    }
//(Birdman#1#): memory access, STRH(1), A7-109, 10000
    if (L == 0)
    {
        int addr = r[Rn] + (imm * 2);
        if (addr & MASK_1BIT != 0)
        {
            UnexpectInst e;
            char tmp[20];
            sprintf(tmp,"%x : %x",rPC-2, cur_instr);
            e.error_name = tmp;
            throw e;
        }

        HALFWORD temp = r[Rd] & 0xffff;
        my_mmu->set_halfword(addr, temp);
    }

}

/**
  * The matching pattern is 1011, 10 instructions included.
  * @param instruction The instruction to be executed.
  * @exception UndefineInst For undefined instructions
  */
void Thumb::misc(const T_INSTR instruction)
{
    int sec_code = (instruction>>8) & MASK_4BIT;

    switch (sec_code)
    {
    	case 0://adjust stack pointer
        {
            int imm = (instruction) & MASK_7BIT;
            int opc = (instruction>>7) & MASK_1BIT;

            if (opc == 0)//add(7) page528
            {
                rSP = rSP + (imm<<2);
            }
            else//sub(4) page632
            {
                rSP = rSP - (imm<<2);
            }
    		break;
        }
        case 2://sign/zero extend
        {
            int opc = (instruction>>6) & MASK_2BIT;
            int Rm = (instruction>>3) & MASK_3BIT;
            int Rd = (instruction) & MASK_3BIT;

            if (opc == 1)//sxtb
            {
                r[Rd] = SignExtend(r[Rm], 8);
            }
            else if(opc == 0)//sxth
            {
                r[Rd] = SignExtend(r[Rm], 16);
            }
            else if(opc == 3)//UXTB
            {
                r[Rd] = r[Rm] & 0xff;
            }
            else//opc == 2,UXTH
            {
                r[Rd] = r[Rm] & 0xffff;
            }
            break;
        }
        case 4:case 5:// L=0,R=1/0 push
        {
            int R = (instruction>>8) & MASK_1BIT;
            int reg_list = (instruction) & MASK_8BIT;
            int start_addr = rSP - 4 * (R + num_of_bits_set(reg_list));
            int end_addr = rSP - 4;
            int addr = start_addr;

            for (int i = 0; i < 8; i++)
                if (((reg_list>>i) & MASK_1BIT) == 1)
                {
                    //put r[i] in memory addr ,A7-85
                    //my_mmu->push_stack(r[i], addr);
                    my_mmu->set_word(addr, r[i]);

                    addr += 4;
                }
            if (R == 1)
            {
                //put in LR, A7-85
                //my_mmu->push_stack(rLR, addr);
                my_mmu->set_word(addr, rLR);
                addr += 4;
            }
            assert(end_addr == addr - 4);
            rSP = rSP - 4 * (R + num_of_bits_set(reg_list));

            break;
        }
        case 12:case 13://L=1,R=1/0 pop
        {
            int R = (instruction>>8) & MASK_1BIT;
            int reg_list = (instruction) & MASK_8BIT;
            int start_addr = rSP;
            int end_addr = rSP + 4 * (R + num_of_bits_set(reg_list));
            int addr = start_addr;

            for (int i = 0; i < 8; i++)
                if (((reg_list>>i) & MASK_1BIT) == 1)
                {
                    //r[i] = my_mmu->pop_stack(rSP);//get reg from memory, addr's content, A7-83
                    r[i] = my_mmu->get_word(addr);
                    addr +=4;
                }
            if (R == 1)
            {
                int value = my_mmu->get_word(addr);//my_mmu->pop_stack(addr);//get addr's content, A7-83
                rPC = value & 0xfffffffe;
                if ((value & MASK_1BIT) == 0)
                {
                    UnexpectInst e;
                    char tmp[40];
                    sprintf(tmp,"Mode Switch not supported %x : %x\n",rPC-2, cur_instr);
                    e.error_name = tmp;
                    throw e;
                }
                addr += 4;
            }

            assert(end_addr == addr);
            //assert(end_addr == rSP);// check rSP == end_addr?
            rSP = end_addr;
            break;
        }
        case 6://set endianness, change processor state
        {
            UnexpectInst e;
            char tmp[40];
            sprintf(tmp,"No Endian change support %x : %x",rPC-2, cur_instr);
            e.error_name = tmp;
            throw e;
            break;
        }
        case 10://reverse bytes
        {
            int Rn = (instruction>>3) & MASK_3BIT;
            int Rd = (instruction) & MASK_3BIT;
            int opcode = (instruction>>6) & MASK_2BIT;

            if (opcode == 0)//rev
            {
                r[Rd] = (r[Rn] & 0xff)<<24;
                r[Rd] |= (r[Rn] & 0xff00)<<8;
                r[Rd] |= (r[Rn] & 0xff0000)>>8;
                r[Rd] |= (r[Rn] & 0xff000000)>>24;
            }
            else if(opcode == 1)//rev16,page606
            {
                r[Rd] = (r[Rn] & 0xff)<<8;
                r[Rd] |= (r[Rn] & 0xff00)>>8;
                r[Rd] |= (r[Rn] & 0xff0000)<<8;
                r[Rd] |= (r[Rn] & 0xff000000)>>8;
            }
            else if(opcode == 3)//revsh
            {
                r[Rd] = (r[Rn] & 0xff)<<8;
                r[Rd] |= (r[Rn] & 0xff00)>>8;
                if (((r[Rn]>>7) & MASK_1BIT) == 1)
                    r[Rd] |= 0xffff0000;
                else
                    r[Rd] &= 0xffff;
            }
            else
            {
                UndefineInst e;
                throw e;
            }
            //this caculation is tested
            break;
        }
        case 14://breakpoint, will not come
        {
            UnexpectInst e;
            char tmp[30];
            sprintf(tmp,"Breakpoint not supported,%x : %x\n",rPC-2, cur_instr);
            e.error_name = tmp;
            throw e;
            break;
        }
    	default:
    	{
            //error
            assert(0);
            UndefineInst e;
            throw e;
    		break;
    	}
    }

}

/**
  * The matching pattern is 1010 SP.
  * @param instruction The instruction to be executed.
  */
void Thumb::add_to_sp_or_pc(const T_INSTR instruction)
{
    int SP_BIT = (instruction>>11) & MASK_1BIT;
    int Rd = (instruction>>8) & MASK_3BIT;
    int imm = (instruction) & MASK_8BIT;

    if (SP_BIT == 0)//add(5)
    {
        r[Rd] = ((rPC + 2) & 0xfffffffc) + (imm * 4);
    }
    else//ADD(6)
    {
        r[Rd] = rSP + (imm << 2);
    }
}

/**
  * The matching pattern is 1100 L.
  * @param instruction The instruction to be executed.
  */
void Thumb::ld_str_multiple(const T_INSTR instruction)
{
    int L = (instruction>>11) & MASK_1BIT;
    int Rn = (instruction>>8) & MASK_3BIT;
    int reg_list = (instruction) & MASK_8BIT;
    int start_addr, end_addr, addr;

    if (L == 1)//ldmia
    {
        addr = start_addr = r[Rn];
        end_addr = r[Rn] + num_of_bits_set(reg_list) * 4 - 4;
        for (int i = 0; i < 8; i++)
            if (((reg_list>>i) & MASK_1BIT) == 1)
            {
                r[i] = my_mmu->get_word(addr); //memory access, A7-45
                addr +=4;
            }
        assert(end_addr  == addr -4);
        r[Rn] = r[Rn] + num_of_bits_set(reg_list) * 4;
    }
    else//stmia
    {
        addr = start_addr = r[Rn];
        end_addr = r[Rn] + num_of_bits_set(reg_list) * 4 - 4;
        for (int i = 0; i < 8; i++)
            if (((reg_list>>i) & MASK_1BIT) == 1)
            {
//(Birdman#1#): memory access store, A7-97
                my_mmu->set_word(addr, r[i]);

                addr +=4;
            }
        assert(end_addr  == addr -4);
        r[Rn] = r[Rn] + num_of_bits_set(reg_list) * 4;
    }
}

/**
  * The matching pattern is 1101 cond.
  * @param instruction The instruction to be executed.
  */
void Thumb::con_br(const T_INSTR instruction)
{
    unsigned char cond = (instruction>>8) & MASK_4BIT;
    int imm = (instruction) & MASK_8BIT;

    if (ConditionPassed(cond))
        rPC = (rPC + 2) + (SignExtend(imm, 8) << 1);//conditon judge, change pc, A7-19, revisit is needed

}

/**
  * The matching pattern is 11100.
  * @param instruction The instruction to be executed.
  */
void Thumb::uncon_br(const T_INSTR instruction)
{
    int imm_11 = instruction & 0x7ff;//mask bit[10:0]

    rPC = (rPC + 2) + (SignExtend(imm_11, 11) << 1);
//change PC, A7-21

}

/**
  * The matching pattern is 11101.
  * @param instruction The instruction to be executed.
  */
void Thumb::blx_suffix(const T_INSTR instruction)
{
    //int offset = instruction & 0x3ff;

     //should not come here
     printf("Switch to ARM, not supported\n");
}

/**
  * The matching pattern is 11110.
  * @param instruction The instruction to be executed.
  */
void Thumb::bl_blx_prefix(const T_INSTR instruction)
{
    int offset = instruction & 0x7ff;

    rLR = (rPC + 2) + (SignExtend(offset, 11) << 12);
}

/**
  * The matching pattern is 11111.
  * @param instruction The instruction to be executed.
  */
void Thumb::bl_suffix(const T_INSTR instruction)
{
    int offset = instruction & 0x7ff;
    int temp = rPC;

    rPC = rLR + (offset << 1);
    rLR = ( temp ) | 1;//rLR = temp | 1;
}

/**
  * According to the condition field, determine whether it matches the CPSR, if matches, return true, otherwise false.
  * @param cond The conditon field.
  * @ return Whether condition field matches the CPSR.
  */
int Thumb::ConditionPassed(unsigned char cond)
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
            if (!getCarry() || getZero())
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
            if ((getZero() == 0) && (getNeg() == getOverflow()))
                res = 1;
            break;
    	case 13://LE
            if ((getZero() == 1) || (getNeg() != getOverflow()))
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
MMU * Thumb::get_mmu()
{
    if (my_mmu != NULL)
        return my_mmu;

    return NULL;
}



/**
  * Get a new MMU modular, get stack pointer from MMU, get program's entry pointer from MMU, initialize SWI.
  */
void Thumb::InitMMU(char *file_name)
{
    my_mmu = new MMU(file_name);

    rSP = my_mmu->getStackTop();
    rPC = my_mmu->getEntry();

    InitSWI();
}

/**
  * Delete the MMU modular.
  */
void Thumb::DeinitMMU()
{
    if (my_mmu != NULL)
        delete my_mmu;

    my_mmu = NULL;
}

/**
  * Get heap information, in order to give it to program, get MMU pointer, for getting parameter and write result.
  */
void Thumb::InitSWI()
{
    swi.getHeapInfo(my_mmu->getHeapTop(), my_mmu->getHeapSz(), my_mmu->getStackTop(), my_mmu->getStackSz());
    swi.getMMU(my_mmu);
}

/**
  * Copy the general purpose register, process status register, and MMU pointer.
  * @param reg The pointer to array of general purpose registers
  * @param flags The reference of process status register
  * @param mmu The pointer to MMU modular 
  */
void Thumb::getRegs(GP_Reg reg[], EFLAG &flags, MMU* &mmu)
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
void Thumb::CopyCPU(CPU *a_cpu)
{
    static_cast<ARM *>(a_cpu)->getRegs(r, cpsr, my_mmu);
    InitSWI();
}

/**
  * Get the argument for the program running inside emulator.
  * @param arg The pointer to an argument string
  * @param len The length of the string
  */
void Thumb::getArg(char *arg, int len)
{
    swi.getArg(arg, len);
}


