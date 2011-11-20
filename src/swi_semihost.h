/*! \file swi_semihost.h
	\brief Handle the software interrupt

	In ARM status, use SWI 0x123456 to handle, In Thumb status, use SWI 0xAB to handle. Using the semihosting technology. r0 represents the type of interruption handling program, r1 passes the parameter.
 */
#ifndef __SWI_SEMIHOST_H__
#define __SWI_SEMIHOST_H__


/*!
	\defgroup swi Software interruption handler module
 */
/*@{*/

#include "arch.h"
#include "MMU.h"

#define SYS_OPEN        0x01    //!< open a file on the host
#define SYS_CLOSE       0x02    //!< close a file on the host
#define SYS_WRITEC      0x03    //!< write a char to debug channel
#define SYS_WRITE0      0x04    //!< write a string to debug channel
#define SYS_WRITE       0x05    //!< write to a file on the host
#define SYS_READ        0x06    //!< read the content of a file into a buffer
#define SYS_READC       0x07    //!< read a byte from the debug channel
#define SYS_ISERROR     0x08    //!< determine if a return code is an error
#define SYS_ISTTY       0x09    //!< chech whether a file is connected to an interactive device
#define SYS_SEEK        0x0a    //!< seek to a position in a file
#define SYS_FLEN        0x0c    //!< return the length of a file
#define SYS_TMPNAM      0x0d    //!< return a temporary name for a file
#define SYS_REMOVE      0x0e    //!< remove a file from the host
#define SYS_RENAME      0x0f    //!< rename a file on the host
#define SYS_CLOCK       0x10    //!< number of centiseconds since support code started
#define SYS_TIME        0x11    //!< number of seconds since Jan 1, 1970
#define SYS_SYSTEM      0x12    //!< pass a command to the host command-line interpreter
#define SYS_ERRNO       0x13    //!< get the value of the C library errno variable
#define SYS_GET_CMDLINE 0x15    //!< get the command-line used to call the executable
#define SYS_HEAPINFO    0x16    //!< get the system heap parameters
#define SYS_KILL        0x18    //!< kill the current process
#define SYS_ELAPSED     0x30    //!< get the number of target ticks since support code started
#define SYS_TICKFREQ    0x31    //!< define a tick frequency


/*! \class swi_semihost
	\brief Handle the software interrupt

	In ARM status, use SWI 0x123456 to handle, In Thumb status, use SWI 0xAB to handle. Using the semihosting technology. r0 represents the type of interruption handling program, r1 passes the parameter.
 */
class swi_semihost
{
public:
    //!A constructor
    swi_semihost();
    //!A destructor
    ~swi_semihost();

private:
	//! The type of software interrupt handler
    int swi_type;
	//! The pointer to parameter array, namely array general purpos register
    GP_Reg *parameter;

	/*! \var hs_addr
		\brief Heap starting address
	 */

	/*! \var hs_sz
		\brief The size of the heap
	 */

	/*! \var ss_addr
		\brief The high address of stack
	 */

	/*! \var ss_sz
 		\brief The size of the stack
	 */
    int hs_addr, hs_sz, ss_addr, ss_sz;
	//! The pointer to MMU module
    MMU *my_mmu;

	//! The string of arguments
    char argv[100];
	//! The length of arguments string
    int arg_len;

public:
	//! Get the parameter
    void get_para(const GP_Reg *para);
	//! Choose the corresponding handler program
    void swi_handler();

	//! Get the pointer of MMU module
    void getMMU(MMU *mmu);
	//! Get the address information of heap and stack
    void getHeapInfo(int heap_addr, int heap_limit, int stack_addr, int stack_limit);
	//! Get the arguments for running program inside the emulator
    void getArg(char *arg, int len);

private:
	//! open a file on the host
    void sys_open();
	//! close a file on the host
    void sys_close();
	//! write a char to debug channel
    void sys_writec();
	//! write a string to debug channel
    void sys_write0();
	//! write to a file on the host
    void sys_write();
	//! read the content of a file into a buffer
    void sys_read();
	//! read a byte from the debug channel
    void sys_readc();
	//! determine if a return code is an error
    void sys_iserror();
	//! chech whether a file is connected to an interactive device
    void sys_istty();
	//! seek to a position in a file
    void sys_seek();
	//! return the length of a file
    void sys_flen();
	//! return a temporary name for a file
    void sys_tmpnam();
	//! remove a file from the host
    void sys_remove();
	//! rename a file on the host
    void sys_rename();
	//! number of centiseconds since support code started
    void sys_clock();
	//! number of seconds since Jan 1, 1970
    void sys_time();
	//! pass a command to the host command-line interpreter
    void sys_system();
	//! get the value of the C library errno variable
    void sys_errno();
	//! get the command-line used to call the executable
    void sys_get_cmdline();
	//! get the system heap parameters
    void sys_heapinfo();
	//! kill the current process
    void sys_kill();
	//! get the number of target ticks since support code started
    void sys_elapsed();
	//! define a tick frequency
    void sys_tickfreq();


};

/*@}*/
#endif // __SWI_SEMIHOST_H__

