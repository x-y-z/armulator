/*! \file swi_semihost.cpp
	\brief The implementation of software interrupt handler
 */
#include "swi_semihost.h"
#include <iostream>
#include <time.h>
#include "error.h"
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <errno.h>
#include <unistd.h>

/*! \def O_BINARY
	\brief Define it when O_BINARY not defined
 */
#ifndef O_BINARY
#define O_BINARY 0
#endif


/*! \var fopen_mode
	\brief 12 types of open file mode
 */
static int fopen_mode[]=
{
  O_RDONLY,		/* "r"   */
  O_RDONLY + O_BINARY,	/* "rb"  */
  O_RDWR,		/* "r+"  */
  O_RDWR + O_BINARY,		/* "r+b" */
  O_WRONLY + O_CREAT + O_TRUNC,	/* "w"   */
  O_WRONLY + O_BINARY + O_CREAT + O_TRUNC,	/* "wb"  */
  O_RDWR + O_CREAT + O_TRUNC,	/* "w+"  */
  O_RDWR + O_BINARY + O_CREAT + O_TRUNC,	/* "w+b" */
  O_WRONLY + O_APPEND + O_CREAT,	/* "a"   */
  O_WRONLY + O_BINARY + O_APPEND + O_CREAT,	/* "ab"  */
  O_RDWR + O_APPEND + O_CREAT,	/* "a+"  */
  O_RDWR + O_BINARY + O_APPEND + O_CREAT	/* "a+b" */
};

/*! \var previous_errno
	\brief Record of last operation error number, not to be used
 */
static int previous_errno = 0;

/**
  * Nothing to be done.
  */
 swi_semihost::swi_semihost()
{
    //fopen_mode[12][4]={"r","rb","r+","r+b","w","wb","w+","w+b","a","ab","a+","a+b"};
}

/**
  * Nothing to be done.
  */
 swi_semihost::~swi_semihost()
{

}

/**
  * Get the heap starting address, heap size, stact high address, stack size information
  * @param heap_addr The heap starting address
  * @param heap_limit The heap size
  * @param stack_addr The stack high address
  * @param stack_limit The stack size
  */
void swi_semihost::getHeapInfo(int heap_addr, int heap_limit, int stack_addr, int stack_limit)
{
    hs_addr = heap_addr;
    hs_sz = heap_limit;
    ss_addr = stack_addr;
    ss_sz = stack_limit;
}

/**
  * Get the MMU modular pointer, in order to get parameter from r1
  * @param mmu The pointer to MMU modular
  */
void swi_semihost::getMMU(MMU *mmu)
{
    my_mmu = mmu;
}


/**
  * Get the pointer to parameter array, namely the general purpose registers
  * @param para The pointer to parameter array
  */
void swi_semihost::get_para(const GP_Reg *para)
{
    swi_type = para[0];
    parameter = const_cast<GP_Reg *>(para);
}


/**
  * Choose right swi handler according to the swi_type
  */
void swi_semihost::swi_handler()
{
    switch (swi_type)
    {
    	case SYS_OPEN:
            sys_open();
    		break;
        case SYS_CLOSE:
            sys_close();
            break;
        case SYS_WRITEC:
            sys_writec();
            break;
        case SYS_WRITE0:
            sys_write0();
        	break;
        case SYS_WRITE:
            sys_write();
        	break;
        case SYS_READ:
            sys_read();
        	break;
        case SYS_READC:
            sys_readc();
        	break;
        case SYS_ISERROR:
            sys_iserror();
        	break;
        case SYS_ISTTY:
            sys_istty();
        	break;
        case SYS_SEEK:
            sys_seek();
        	break;
        case SYS_FLEN:
            sys_flen();
        	break;
        case SYS_TMPNAM:
            sys_tmpnam();
        	break;
        case SYS_REMOVE:
            sys_remove();
        	break;
        case SYS_RENAME:
            sys_rename();
        	break;
        case SYS_CLOCK:
            sys_clock();
        	break;
        case SYS_TIME:
            sys_time();
        	break;
        case SYS_SYSTEM:
            sys_system();
        	break;
        case SYS_ERRNO:
            sys_errno();
        	break;
        case SYS_GET_CMDLINE:
            sys_get_cmdline();
        	break;
        case SYS_HEAPINFO:
            sys_heapinfo();
        	break;
        case SYS_KILL:
            sys_kill();
            break;
        case SYS_ELAPSED:
            sys_elapsed();
        	break;
        case SYS_TICKFREQ:
            sys_tickfreq();
        	break;
    	default:
    		break;
    }

}

/**
  *
  */
void swi_semihost::sys_kill()
{
    ProgramEnd e;
    throw e;
}

/**
  *
  */
void swi_semihost::sys_elapsed()
{

}

/**
  *
  */
void swi_semihost::sys_heapinfo()
{
    int address;

    address = my_mmu->get_word(parameter[1]);

    my_mmu->set_word(address, hs_addr);
    my_mmu->set_word(address + 4, hs_sz);
    my_mmu->set_word(address + 8, ss_addr);
    my_mmu->set_word(address + 12, ss_sz);
}

/**
  *
  */
void swi_semihost::sys_get_cmdline()
{
    char cmdline[arg_len + 1];
    int cmd_pointer = my_mmu->get_word(parameter[1]);

    memset(cmdline, 0, arg_len + 1);
    memcpy(cmdline, argv, arg_len);

    for (int i = 0; i < strlen(cmdline); i++)
        my_mmu->set_byte(cmd_pointer + i, cmdline[i]);

    my_mmu->set_byte(cmd_pointer + strlen(cmdline), 0);

    //UnexpectInst e;
    //e.error_name = "cmd";
    //throw e;

    parameter[0] = 0;
}

/**
  *
  */
void swi_semihost::sys_errno()
{
    parameter[0] = previous_errno;
}

/**
  *
  */
void swi_semihost::sys_system()
{
    int cmd_pointer = my_mmu->get_word(parameter[1]);
    int len = my_mmu->get_word(parameter[1] + 4);
    char cmd[len + 1];

    for (int i = 0; i < len; i++)
        cmd[i] = my_mmu->get_byte(cmd_pointer + i);
    cmd[len] = 0;

    parameter[0] = system(cmd);
}

/**
  *
  */
void swi_semihost::sys_time()
{
    parameter[0] = time(NULL);
    //get_errno();
    previous_errno = errno;
}

/**
  *
  */
void swi_semihost::sys_clock()
{
    parameter[0] =
#ifdef CLOCKS_PER_SEC
    (CLOCKS_PER_SEC >= 100)
    ? (clock () / (CLOCKS_PER_SEC / 100))
    : ((clock () * 100) / CLOCKS_PER_SEC);
#else
    /* Presume unix... clock() returns microseconds.  */
    (clock () / 10000);
#endif

    //get_errno();
    previous_errno = errno;
}

/**
  *
  */
void swi_semihost::sys_rename()
{
    int file_pre = my_mmu->get_word(parameter[1]);
    int pre_len = my_mmu->get_word(parameter[1] + 4);
    int file_re = my_mmu->get_word(parameter[1] + 8);
    int re_len = my_mmu->get_word(parameter[1] + 12);

    char pre_name[pre_len + 1];
    char re_name[re_len + 1];

    for (int i = 0; i < pre_len; i++)
        pre_name[i] = my_mmu->get_byte(file_pre + i);
    pre_name[pre_len] = 0;

    for (int i = 0; i < re_len; i++)
        re_name[i] = my_mmu->get_byte(file_re + i);
    re_name[re_len] = 0;

    parameter[0] = rename(pre_name, re_name);
}

/**
  *
  */
void swi_semihost::sys_remove()
{
    int file_pointer = my_mmu->get_word(parameter[1]);
    int length = my_mmu->get_word(parameter[1] + 4);

    char *file_name = new char[length + 1];

    for (int i = 0; i < length; i++)
        file_name[i] = my_mmu->get_byte(file_pointer + i);
    file_name[length] = 0;

    parameter[0] = remove(file_name);
}

/**
  *
  */
void swi_semihost::sys_tmpnam()
{
//    int buffer_pointer = my_mmu->get_word(parameter[1]);
//    FILE *afile = reinterpret_cast<FILE *>(my_mmu->get_word(parameter[1] + 4));
//    int len = my_mmu->get_word(parameter[1] + 8);
//    char buffer[len];

    //strncpy(buffer, afile->_tmpfname, strlen(afile->_tmpfname));

    //for (int i = 0; i < strlen(afile->_tmpfname); i++)
    //    my_mmu->set_byte(buffer_pointer + i, buffer[i]);

    parameter[0] = 0;

}

/**
  *
  */
void swi_semihost::sys_flen()
{
    int handler = my_mmu->get_word(parameter[1]);

    if (handler == 0 || handler  > 64)
    {
        //errno
        parameter[0] = -1;
        return;
    }

    int addr = lseek(handler, 0, 1);//SEEK_CUR
    parameter[0] = lseek(handler, 0, 2);//SEEK_END
    lseek(handler, addr, 0);//SEEK_SET

    //get_errno();
    previous_errno = errno;

}

/**
  *
  */
void swi_semihost::sys_seek()
{
    int handler = my_mmu->get_word(parameter[1]);
    long pos = my_mmu->get_word(parameter[1] + 4);

    parameter[0] = -1 >= lseek(handler, pos, 0);//SEEK_SET

    //get_errno();
    previous_errno = errno;
}

/**
  *
  */
void swi_semihost::sys_istty()
{
    WORD isatty =  my_mmu->get_word(parameter[1]);

    if (isatty > 2 && isatty <= 64)
    {
        parameter[0] = 0;
    }
    else
    {
        parameter[0] = isatty;
    }
    //parameter[0] = -1;
    previous_errno = errno;
}

/**
  *
  */
void swi_semihost::sys_iserror()
{
    parameter[0] = 0;
    //get_errno();
    previous_errno = errno;
}

/**
  *
  */
void swi_semihost::sys_readc()
{
    char msg;
    std::cin>>msg;

    parameter[0] = msg;
}

/**
  *
  */
void swi_semihost::sys_read()
{
    int handler = my_mmu->get_word(parameter[1]);
    int file_pointer = my_mmu->get_word(parameter[1] +4);
    uint32_t len = my_mmu->get_word(parameter[1] + 8);

    char *data = new char[len+1];

    int res = read(handler, data, len);

    if (res > 0)
        for (int i = 0; i < res; i++)
        {
            my_mmu->set_byte(file_pointer + i, data[i]);
        }

    parameter[0] = res == -1 ? -1 : len - res;

    delete []data;
    //get_errno();
    previous_errno = errno;
}

/**
  *
  */
void swi_semihost::sys_write()
{
    int handler = my_mmu->get_word(parameter[1]);
    int file_pointer = my_mmu->get_word(parameter[1] +4);
    uint32_t len = my_mmu->get_word(parameter[1] + 8);

    char *data = new char[len+1];

    for (int i = 0; i < len; i++)
        data[i] = my_mmu->get_byte(file_pointer + i);

    data[len] = 0;

    int res = write(handler, data, len);

    parameter[0] = res == -1 ? -1 : len - res;

    delete []data;

    //get_errno();
    previous_errno = errno;
}

/**
  *
  */
void swi_semihost::sys_write0()
{
    char msg;
    int i = 0;
    while ((msg = my_mmu->get_byte(parameter[1] + i)) != 0)
        std::cout<<msg;
}

/**
  *
  */
void swi_semihost::sys_writec()
{
    char msg = my_mmu->get_byte(parameter[1]);
    std::cout<<msg;
}

/**
  *
  */
void swi_semihost::sys_close()
{
    int handler = my_mmu->get_word(parameter[1]);

    if (handler != 1)
        close(handler);
    //get_errno();
    previous_errno = errno;
}

/**
  *
  */
void swi_semihost::sys_open()
{
    int file_pointer = my_mmu->get_word(parameter[1]);
    int mode = my_mmu->get_word(parameter[1] + 4);
    int length = my_mmu->get_word(parameter[1] + 8);

    char *file_name = new char[length + 1];

    for (int i = 0; i < length; i++)
        file_name[i] = my_mmu->get_byte(file_pointer + i);
    file_name[length] = 0;

    if (strcmp(file_name, ":tt") == 0)
    {
        if (mode >= 0 && mode <= 3)
            parameter[0] = 0;//stdin
        else
            parameter[0] = 1;//stdout
    }
    else
    {
        parameter[0] = open(file_name,fopen_mode[mode]);
        //get_errno();
    }

    delete []file_name;
    previous_errno = errno;
}

/**
  *
  */
void swi_semihost::sys_tickfreq()
{

}

void swi_semihost::getArg(char *arg, int len)
{
    memset(argv, 0, 100);
    
    for (int i = 0; i < len; i++)
        argv[i] = arg[i];

    arg_len = len;
}

