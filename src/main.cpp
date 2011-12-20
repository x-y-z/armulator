#include <cstdlib>
#include <iostream>
#include <cstring>
#include "Thumb.h"
#include "error.h"
#include "ARM.h"

// TODO (Birdman#1#): the default stacktop is at 0x200000, it limits the heap size of 1MB, if more heap spaces is needed, increase the stacktop address,  make the stacktop as a parameter to override the default 0x200000
#pragma align(1)
char file_name[100] = {0};


/*!
	entry point of the emulator, pass the parameters into the Thumb program through this function. Start the emulator.
	\param param_1 first parameter to be passed
	\param param_2 second parameter to be passed
	\return The result of the running program
 */
int main(int argc,char* argv[])
{
    //char main_param[100] = {0};
    // format the parameter, seperate the parameter by 0x20
    //sprintf(main_param, "%d\040%d\040", param_1, param_2);

	if (argc != 2)
	{
		std::cout<<"Use: \"ARMulator [file name]\" to run!"<<std::endl;
		return EXIT_FAILURE;
	}
	
	strcpy(file_name, argv[1]);
	
    CPU *arm = new ARM;

    try
    {
        arm->InitMMU();
    }
    catch(Error &e)
    {
        std::cout<<"\nError:"<<e.error_name<<std::endl;
        return EXIT_FAILURE;
    }


    while(1)
    {
        try
        {
            arm->fetch();
            arm->exec();
        }
        catch(Error &e)
        {
            arm->DeinitMMU();
            std::cout<<"\nError:"<<e.error_name<<std::endl;
            break;
        }
        catch(UnexpectInst &e)
        {
            std::cout<<"\nUnexpect Instr:"<<e.error_name<<std::endl;
            break;
        }
        catch(UndefineInst &e)
        {
            std::cout<<"\nUndefine Instr:"<<e.error_name<<std::endl;
            break;
        }
        catch(SwitchMode &e)
        {
            Thumb *tmp = new Thumb;
            tmp->CopyCPU(arm);
            delete arm;
            arm = tmp;
            //arm->getArg(main_param, strlen(main_param));
        }
        catch(ProgramEnd &e)
        {
            std::cout<<"\nThe Program Ended\n";
            break;
        }
    }

    arm->DeinitMMU();
    delete arm;


    return EXIT_SUCCESS;
}
