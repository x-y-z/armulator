/*! \file error.h
	\brief The error exception class collection.

	All exceptions from instruction execution, Mode switch, Program End.
 */
#ifndef __ERROR_H__
#define __ERROR_H__

#include <string>

/*!
	\defgroup exception Exception class
 */
/*@{*/

/*!	\exception InstructionExcept
	\brief The base exception class for unexpected instruction.

	The base class, not use in the program, has a string member, used for storing the error message.
 */
class InstructionExcept
{
public:
	//! The string stores the error information.
    std::string error_name;
};

/*!	\exception UndefineInst
	\brief The derived class from InstructionExcept, used for a undefined instruction appeared in program.

	The emulator throws out this exception, when encounters a undefined instruction(ARM or Thumb instruction).
 */
class UndefineInst: public InstructionExcept
{
};

/*!	\exception UnexpectInst
	\brief The derived class from InstructionExcept, used for a instruction can not be handled.

	Like breakpoint, mode switch in Thumb status, these instruction cannot be handled by emulator, then this exception will be thrown out.
 */
class UnexpectInst: public InstructionExcept
{
};

/*!	\exception Error
	\brief The exception for error other than instruction-related.

	Memory access violation, file cannot be opened, and so on, these errors will be dealed with Error class.
 */
class Error
{
public:
	//! The string stores the error information.
    std::string error_name;
};

/*!	\exception SwitchMode
	\brief For mode switch use.

	When the emulator switches from ARM status to Thumb status, it throws out this exception.
 */
class SwitchMode
{
public:
	//! Note for mode switch, from what status to what status.
    std::string switch_way;
};

/*!	\exception ProgramEnd
	\brief For ending the program running in the emulator.

	When SWI's 0x18 subroutine is called, this exception will be thrown out, the emulator will end as well as the program running in it.
 */
class ProgramEnd
{
};

/*@}*/
#endif // __ERROR_H__

