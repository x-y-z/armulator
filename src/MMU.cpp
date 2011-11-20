#include "MMU.h"
#include "error.h"
#include "Thumb.h"
#include "cstring"
// TODO (Birdman#1#): add .init and .fini sections to MMU

extern char file_name[100];

/**
  * Initialize the memory layout, set up the ranges of code segment, data segment, heap, stack, etc. The information will be retrieved from ELF file.
  * @exception Error For errors which are memory-related, file-related, etc.
  */
 MMU::MMU()
{
    //char file_name[100] = {0};

    _text = 0xffffffff;
    _text_VMA = 0xffffffff;
    _text_sz = 0;

    _data = 0xffffffff;
    _data_VMA = 0xffffffff;
    _data_sz = 0;

    _rodata = 0xffffffff;
    _rodata_VMA = 0xffffffff;
    _rodata_sz = 0;

    code_infile_off = 0;

    //strcpy(file_name, "libARM.so");

    ifile.open(file_name, std::ios::binary|std::ios::in);

    if (!ifile.is_open())
    {
        Error e;
        e.error_name = "File libARM.so Not Exist";
        throw e;
    }

    my_elf = new elf_file;
    if (!my_elf)
    {
        Error e;
        e.error_name = "Elf Module initialize error!";
        throw e;
    }

    code_infile_off =  my_elf->getCodeOffset(ifile);
    my_elf->get_info(ifile);
    my_elf->setup_MMU(*this);

    bss = new BYTE[_bss_sz];
    if (!bss)
    {
        Error e;
        e.error_name = "No mem space for bss segment!";
        throw e;
    }

    entry_point = my_elf->getEntryPoint();

    delete my_elf;

    code = NULL;
    code = new BYTE[_text_sz];
    if (!code)
    {
        Error e;
        e.error_name = "No mem space for text segment!";
        throw e;
    }

    ifile.seekg(_text +code_infile_off);
    ifile.read(reinterpret_cast<char *>(code), _text_sz);

    rodata = NULL;
    rodata = new BYTE[_rodata_sz];
    if (!rodata)
    {
        Error e;
        e.error_name = "No mem space for rodata segment!";
        throw e;
    }

    ifile.seekg(_rodata +code_infile_off);
    ifile.read(reinterpret_cast<char *>(rodata), _rodata_sz);

    data_seg = NULL;
    data_seg = new BYTE[_data_sz];
    if (!data_seg)
    {
        Error e;
        e.error_name = "No mem space for data segment!";
        throw e;
    }

    ifile.seekg(_data +code_infile_off);
    ifile.read(reinterpret_cast<char *>(data_seg), _data_sz);

    heap = NULL;
    heap = new BYTE[(_ss_VMA - STACK_SZ) - (_bss_VMA + _bss_sz)];
    if (!heap)
    {
        Error e;
        e.error_name = "No mem space for heap segment!";
        throw e;
    }

    _heap_VMA = _bss_VMA + _bss_sz;
}

/**
  * Deinitialization, delete the bss segment, data segment, heap.
  */
 MMU::~MMU()
{

    if (bss != NULL)
        if (_bss_sz > 1)
            delete []bss;
        else
            delete bss;

    if (code != NULL)
        delete []code;

    if (rodata != NULL)
        delete []rodata;

    if (data_seg != NULL)
        delete []data_seg;

    if (heap != NULL)
        delete []heap;
}


/**
  * Set the maximum range of code, include .text .init .deint, etc.
  * @param start The start file offset of the section
  * @param size The size of the section
  */
void MMU::setTextSeg(int start, int size)
{
    if ((unsigned)start < (unsigned)_text)
    {
        _text = start;
    }

    _text_sz += size;
}

/**
  * Set the maximum range of data, include .data, etc.
  * @param start The start file offset of the section
  * @param size The size of the section
  */
void MMU::setDataSeg(int start, int size)
{
    if ((unsigned)start < (unsigned)_data)
    {
        _data = start;
    }
    _data_sz += size;
}

/**
  * Set the maximum range of read only data, include .rodata, etc.
  * @param start The start file offset of the section
  * @param size The size of the section
  */
void MMU::setRodataSeg(int start, int size)
{
    if ((unsigned)start < (unsigned)_rodata)
    {
        _rodata = start;
    }

    _rodata_sz += size;
}

/**
  * Set the range of bss segment
  * @param start The start file offset of the section
  * @param size The size of the section
  */
void MMU::setBssSeg(int start, int size)
{
    _bss = start;
    _bss_sz = size;
}

/**
  * Set the high address of stack
  * @param high_addr The high address of stack
  */
void MMU::setStackSeg(int high_addr)
{
    _ss = high_addr;
}

/**
  * Set the low address of heap(not used)
  * @param low_addr The low address of the heap
  */
void MMU::setHeapSeg(int low_addr)
{

}

/**
  * Set the starting virtual address of bss segment
  * @param VMA_start The starting virtual address of bss segment
  */
void MMU::setBssVMA(int VMA_start)
{
    _bss_VMA = VMA_start;
}

/**
  * Set the starting virtual address of data segment
  * @param VMA_start The starting virtual address of data segment
  */
void MMU::setDataVMA(int VMA_start)
{
    if ((unsigned)VMA_start < (unsigned)_data_VMA)
    {
        _data_VMA = VMA_start;
    }
}

/**
  * Set the starting virtual address of read only data segment
  * @param VMA_start The starting virtual address of read only data segment
  */
void MMU::setRodataVMA(int VMA_start)
{
    if ((unsigned)VMA_start < (unsigned)_rodata_VMA)
    {
        _rodata_VMA = VMA_start;
    }
}

/**
  * Set the starting virtual address of code segment
  * @param VMA_start The starting virutal address of code segment
  */
void MMU::setTextVMA(int VMA_start)
{
    if ((unsigned)VMA_start < (unsigned)_text_VMA)
    {
        _text_VMA = VMA_start;
    }
}

/**
  * Set the high virtual address of stack
  * @param VMA_high The high virtual address of stack segment
  */
void MMU::setStackVMA(int VMA_high)
{
    _ss_VMA = VMA_high;
}



/**
  * Transform virtual address to file offset
  * @param VMAddr The virtual address
  * @exception UnexpectInst For instruction can not be handled
  */
int MMU::VMA2FileOff(int VMAddr)
{
    if (VMAddr >= _text_VMA && VMAddr < (_text_VMA + _text_sz))
        return _text + (VMAddr - _text_VMA);

    if (VMAddr >= _rodata_VMA && VMAddr < (_rodata_VMA + _rodata_sz))
        return _rodata + (VMAddr - _rodata_VMA);

    if (VMAddr >= _data_VMA && VMAddr < (_data_VMA + _data_sz))
        return _data + (VMAddr - _data_VMA);

    if (VMAddr >= _bss_VMA && VMAddr < (_bss_VMA + _bss_sz))
        return _bss + (VMAddr - _bss_VMA);



    UnexpectInst e;
    char tmp[30];
    sprintf(tmp,"Out of File Range:0x%x", VMAddr);
    e.error_name = tmp;
    throw e;
}

/**
  * Find the corresponding segment, according to virtual address
  * @param VMAddr The input virtual address
  * @exception UnexpectInst For instruction can not be handled
  */
SEGTYPE MMU::VMA2Seg(int VMAddr)
{
    if (VMAddr >= _text_VMA && VMAddr < (_text_VMA + _text_sz))
        return TEXTSEG;

    if (VMAddr >= _rodata_VMA && VMAddr < (_rodata_VMA + _rodata_sz))
        return RODATASEG;

    if (VMAddr >= _data_VMA && VMAddr < (_data_VMA + _data_sz))
        return DATASEG;

    if (VMAddr >= _bss_VMA && VMAddr < (_bss_VMA + _bss_sz))
        return BSSSEG;

    if (VMAddr >= (_bss_VMA + _bss_sz) && VMAddr < (_ss_VMA - STACK_SZ))
        return HEAPSEG;

    if (VMAddr >= (_ss_VMA - STACK_SZ) && VMAddr <= _ss_VMA)
        return STACKSEG;

    UnexpectInst e;
    char tmp[30];
    sprintf(tmp,"Segment fault:0x%x", VMAddr);
    e.error_name = tmp;
    throw e;
}


/**
  * Give out a Thumb code instruction, according to the given virtual address
  * @param address The given virtual address of the desired instruction
  * @exception Error For errors which are memory-related, file-related, etc.
  */
T_INSTR MMU::getInstr(int address)
{
    T_INSTR aInstr;
    if (VMA2Seg(address) != TEXTSEG)
    {
        Error e;
        char tmp[30];
        sprintf(tmp,"Out of Code Segment:0x%x", address);
        e.error_name = tmp;
        throw e;
    }
    
    //ifile.seekg(VMA2FileOff(address) + code_infile_off);

    //ifile.read(reinterpret_cast<char *>(&aInstr), 2);

	aInstr = *reinterpret_cast<HALFWORD *>(&code[address - _text_VMA]);

    return aInstr;
}

/**
  * Give out a ARM code instruction, according to the given virtual address
  * @param address The given virtual address of the desired instruction
  * @exception Error For errors which are memory-related, file-related, etc.
  */
A_INSTR MMU::getInstr32(int address)
{
    A_INSTR aInstr;
    if (VMA2Seg(address) != TEXTSEG)
    {
        Error e;
        char tmp[30];
        sprintf(tmp,"Out of Code Segment:0x%x", address);
        e.error_name = tmp;
        throw e;
    }

    
    //ifile.seekg(VMA2FileOff(address) + code_infile_off);

    //ifile.read(reinterpret_cast<char *>(&aInstr), 4);

	aInstr = *reinterpret_cast<WORD *>(&code[address - _text_VMA]);

    return aInstr;
}




/**
  * Give out a byte data, according to the given virtual address. First classify the virtual addresses into different segments, then get the data in various ways(e.g data, bss, heap, stack are from memory, code, rodata are directly from the file)
  * @param address The given virtual address of desired data
  * @exception Error For errors which are memory-related, file-related, etc.
  * @sa get_halfword(), get_word()
  */
BYTE MMU::get_byte(int address)
{
    BYTE data;
    switch (VMA2Seg(address))
    {
    	case BSSSEG:
    	{
    	    return bss[address - _bss_VMA];
    		break;
    	}
    	case HEAPSEG:
    	{
    	    return heap[address - _heap_VMA];
    		break;
    	}
    	case DATASEG:
    	{

            return data_seg[address - _data_VMA];
    	    break;
        }
        case TEXTSEG:
	{
	    data = code[address - _text_VMA];
	    return data;
	    break;
	}
	case RODATASEG:
        {
            /*try
            {
                ifile.seekg(VMA2FileOff(address) + code_infile_off);
            }
            catch(Error &e)
            {}

            ifile.read(reinterpret_cast<char *>(&data), 1);
*/

	    data = rodata[address - _rodata_VMA];

            return data;
            break;
        }
        case STACKSEG:
        {
            data = reinterpret_cast<BYTE *>(&stack[(_ss_VMA - address)/4 -1])[(_ss_VMA - address) & MASK_2BIT];
            return data;
            //may never come to this
            break;
        }
		// we shouldn't come here, this segment is reserved
        case HEADERSEG:
        {
            //data = program_header[address];
            return data;

            break;
        }
    	default:
    		break;
    }
    //should never come here
    return -1;
}

/**
  * Give out the halfword data, according to the given virtual address
  * @param address The given virtual address of desired data
  * @sa get_byte(), get_word()
  */
HALFWORD MMU::get_halfword(int address)
{
    HALFWORD data;

    switch (VMA2Seg(address))
    {
    	case BSSSEG:
    	{
            data = *reinterpret_cast<HALFWORD *>(&bss[address - _bss_VMA]);
            return data;
    		break;
    	}
    	case HEAPSEG:
    	{
    	    data = *reinterpret_cast<HALFWORD *>(&heap[address - _heap_VMA]);
            return data;
    		break;
    	}
        case DATASEG:
        {
            data = *reinterpret_cast<HALFWORD *>(&data_seg[address - _data_VMA]);
            return data;
            break;
        }
        case TEXTSEG:
	{
	    data = *reinterpret_cast<HALFWORD *>(&code[address - _text_VMA]);
	    return data;
	    break;
	}
	case RODATASEG:
        {
            /*try
            {
                ifile.seekg(VMA2FileOff(address) + code_infile_off);
            }
            catch(Error &e)
            {}

            ifile.read(reinterpret_cast<char *>(&data), 2);
*/

	    data = *reinterpret_cast<HALFWORD *>(&rodata[address - _rodata_VMA]);

            return data;
            break;
        }
        case STACKSEG:
        {
            data = reinterpret_cast<HALFWORD *>(&stack[(_ss_VMA - address)/4 -1])[((_ss_VMA - address)>>1) & MASK_1BIT];
            return data;
            //may never come to this
            break;
        }
        case HEADERSEG:
        {
            //data = *reinterpret_cast<HALFWORD *>(&program_header[address]);
            return data;
            break;
        }
    	default:
    		break;
    }

    //should never come here
    return -1;
}

/**
  * Give out a word data, according to the given virtual address
  * @param address The given virtual address of the desired data
  * @sa get_byte(), get_halfword()
  */
WORD MMU::get_word(int address)
{
    WORD data;

    switch (VMA2Seg(address))
    {
    	case BSSSEG:
    	{
            data = *reinterpret_cast<WORD *>(&bss[address - _bss_VMA]);
            return data;

            break;
        }
        case HEAPSEG:
        {
            data = *reinterpret_cast<WORD *>(&heap[address - _heap_VMA]);
            return data;
        	break;
        }
        case DATASEG:
        {
            data = *reinterpret_cast<WORD *>(&data_seg[address - _data_VMA]);
            return data;
            break;
        }
        case TEXTSEG:
	{
	    data = *reinterpret_cast<WORD *>(&code[address - _text_VMA]);
	    return data;
	    break;
	}
	case RODATASEG:
        {
            //ifile.seekg(VMA2FileOff(address) + code_infile_off);
            //ifile.read(reinterpret_cast<char *>(&data), 4);

	    data = *reinterpret_cast<WORD *>(&rodata[address - _rodata_VMA]);

            return data;

            break;
        }
        case STACKSEG:
        {
            return stack[(_ss_VMA - address)/4 -1];
        }
        case HEADERSEG:
        {
            return data;

            break;
        }
    	default:
    		break;
    }

    //should never come here
    return -1;
}

/**
  * Set a byte data, according to the given virtual address. First classify the virtual addresses into different segments, then set the data in various ways.(e.g data, bss, heap, stack are from memory, code, rodata are directly from the file)
  * @param address The given virtual address where the data will be set
  * @param data The input data
  * @sa set_halfword(), set_word()
  */
void MMU::set_byte(int address, BYTE data)
{

    switch (VMA2Seg(address))
    {
    	case DATASEG:
    	{
            data_seg[address - _data_VMA] = data;
            break;
    	}
        case BSSSEG:
        {
            bss[address - _bss_VMA] = data;
            break;
        }
        case HEAPSEG:
        {
            heap[address - _heap_VMA] = data;
        	break;
        }
		// we shouldn't come here, it is reserved
        case HEADERSEG:
        {

            break;
        }
        case STACKSEG:
        {
            //printf("stack%d,%d",(_ss_VMA - address)/4 - 1, (_ss_VMA - address) % 4);
            reinterpret_cast<BYTE *>(&stack[(_ss_VMA - address)/4 - 1])[(_ss_VMA - address) % 4] = data;
            break;
        }
    	default:
    		break;
    }


}

/**
  * Set a halfword data, according to the given virtual address
  * @param address The given virtual address where the data will be set
  * @param data The input data 
  */
void MMU::set_halfword(int address, HALFWORD data)
{
    //address is halfword aligned

    switch (VMA2Seg(address))
    {
    	case DATASEG:
    	{
            *reinterpret_cast<HALFWORD *>(&data_seg[address - _data_VMA]) = data;
    		break;
    	}
        case BSSSEG:
        {
            *reinterpret_cast<HALFWORD *>(&bss[address - _bss_VMA]) = data;
            break;
        }
        case HEAPSEG:
        {
            *reinterpret_cast<HALFWORD *>(&heap[address - _heap_VMA]) = data;
        	break;
        }
        case HEADERSEG:
        {
            //*reinterpret_cast<HALFWORD *>(&program_header[address]) = data;
        	break;
        }
        case STACKSEG:
        {
            reinterpret_cast<HALFWORD *>(&stack[(_ss_VMA - address)/4 - 1])[((_ss_VMA - address)>>1) & MASK_1BIT] = data;
            break;
        }
    	default:
    		break;
    }


}

/**
  * Set a word data, according to the given virtual address
  * @param address The given virtual address where the data will be set
  * @param data The input data
  */
void MMU::set_word(int address, WORD data)
{
    //address is word aligned
    switch (VMA2Seg(address))
    {
    	case DATASEG:
    	{
            *reinterpret_cast<WORD *>(&data_seg[address - _data_VMA]) = data;
    	    break;
    	}
        case BSSSEG:
        {
            *reinterpret_cast<WORD *>(&bss[address - _bss_VMA]) = data;
            break;
        }
        case STACKSEG:
        {
            stack[(_ss_VMA - address)/4 - 1] = data;
            break;
        }
        case HEAPSEG:
        {
            *reinterpret_cast<WORD *>(&heap[address - _heap_VMA]) = data;
            break;
        }
        case HEADERSEG:
        {
            //*reinterpret_cast<WORD *>(&program_header[address]) = data;
            break;
        }
    	default:
    		break;
    }


}

/**
  * Push a word data into stack
  * @param data The input data
  * @param arm_sp The pointer to the stack top
  */
void MMU::push_stack(WORD data, SP arm_sp)//deprecated
{
    stack[(_ss_VMA - arm_sp)/4 -1] = data;
}

/**
  * Pop a word data from stack
  * @param arm_sp The pointer to the stack top
  * @return The data
  */
WORD MMU::pop_stack(SP arm_sp)//deprecated
{
    int data;
    data = stack[(_ss_VMA - arm_sp)/4 -1];
    return data;
}



/**
  * Give out the high address of stack
  * @return The high address of stack
  */
int MMU::getStackTop()
{
    return _ss_VMA;
}

/**
  * Give out the size of the stack
  * @return The size of the stack
  */
int MMU::getStackSz()
{
    return STACK_SZ;
}

/**
  * Give out the entry point of the Thumb code file, namely the address from which the program starts
  * @return The entry point address
  */
int MMU::getEntry()
{
    return entry_point;
}

/**
  * Give out the heap starting virtual address
  * @return The heap starting virtual address
  */
int MMU::getHeapTop()
{
    return _heap_VMA; //(_bss_VMA + _bss_sz);
}

/**
  * Give out the size of the heap, the size is uncertain, is due to the stack top address and the top address of bss segment
  * @return The size of the heap
  */
int MMU::getHeapSz()
{
    return (_ss_VMA - STACK_SZ) - _heap_VMA;
}


