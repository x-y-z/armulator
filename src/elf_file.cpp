#include "elf_file.h"
#include <cstring>
#include "error.h"




/**
  * Initialize the pointer to NULL, zero the code_offset value.
  */
 elf_file::elf_file()
{
    pro_header = NULL;
    sec_header = NULL;
    code_offset = 0;
}

/**
  * Deinitialize the modular, delete the program header array, section header array
  */
 elf_file::~elf_file()
{
    if (pro_header != NULL)
    {
        if (elf_header.e_phnum == 1)
            delete pro_header;
        else
            delete []pro_header;
    }

    if (sec_header != NULL)
    {
        if (elf_header.e_shnum == 1)
            delete sec_header;
        else
            delete []sec_header;
    }

}

/**
  * Find the .ARM section, get the file offset of .ARM section(The section name can be modified).
  * @param ifile The input file stream, it should be shared object file itself
  * @return The file offset of .ARM section
  */
int elf_file::getCodeOffset(std::ifstream &ifile)
{
    Elf32_Ehdr self_header;
    Elf32_Shdr *self_section;
    char *self_sec_name;
    int snum;


    //if (ifile.eof())
    ifile.clear();

    ifile.seekg(std::ios::beg);
    ifile.read(reinterpret_cast<char *>(&self_header), sizeof(Elf32_Ehdr));

    if (self_header.e_shoff != 0)
    {
        ifile.seekg(self_header.e_shoff);
        snum = self_header.e_shnum;

        if (snum > 1)
            self_section = new Elf32_Shdr[snum];
        else if(snum == 1)
            self_section = new Elf32_Shdr;

        for (int i = 0; i < snum; i++)
            ifile.read(reinterpret_cast<char *>(self_section + i), sizeof(Elf32_Shdr));
    }
    else
    {
        Error e;
        throw e;
    }

    int name_len = self_section[self_header.e_shstrndx].sh_size;

    self_sec_name = new char [name_len];

    ifile.seekg(self_section[self_header.e_shstrndx].sh_offset);
    ifile.read(self_sec_name, name_len);


    for (int i = 0; i < snum; i++)
        if (strcmp(&self_sec_name[self_section[i].sh_name], ".ARM") == 0)
        {
            code_offset = self_section[i].sh_offset;
            break;
        }

    return code_offset;

}



/**
  * Get the Thumb code ELF structure from the current dynamic link library.
  * @param ifile The file stream of the current shared object file
  */
void elf_file::get_info(std::ifstream &ifile)
{
    ifile.seekg(code_offset);
    ifile.read(reinterpret_cast<char *>(&elf_header), sizeof(elf_header));

    if (elf_header.e_phoff != 0)
    {
        ifile.seekg(elf_header.e_phoff + code_offset);
        int pnum = elf_header.e_phnum;

        if (pnum > 1)
            pro_header = new Elf32_phdr[pnum];
        else if(pnum == 1)
            pro_header = new Elf32_phdr;

        for (int i = 0; i < pnum; i++)
            ifile.read(reinterpret_cast<char *>(pro_header + i), sizeof(Elf32_phdr));
    }

    if (elf_header.e_shoff != 0)
    {
        ifile.seekg(elf_header.e_shoff + code_offset);
        int snum = elf_header.e_shnum;

        if (snum > 1)
            sec_header = new Elf32_Shdr[snum];
        else if(snum == 1)
            sec_header = new Elf32_Shdr;

        for (int i = 0; i < snum; i++)
            ifile.read(reinterpret_cast<char *>(sec_header + i), sizeof(Elf32_Shdr));
    }

    sec_name_len = sec_header[elf_header.e_shstrndx].sh_size;

    sec_name = new char [sec_name_len];

    ifile.seekg(sec_header[elf_header.e_shstrndx].sh_offset + code_offset);
    ifile.read(sec_name, sec_name_len);

    return;
}

/**
  * Set up the memory layout of MMU modular 
  * @param aMMU The reference of a MMU modular
  */
void elf_file::setup_MMU(MMU &aMMU)
{
    for (int i = 0; i < elf_header.e_shnum; i++)
    {
        /*
        if (strcmp(&sec_name[sec_header[i].sh_name], ".init") == 0)
        {
            aMMU.setTextSeg(sec_header[i].sh_offset, sec_header[i].sh_size);
            aMMU.setTextVMA(FileOff2VMA(sec_header[i].sh_offset));
        }

        if (strcmp(&sec_name[sec_header[i].sh_name], ".fini") == 0)
        {
            aMMU.setTextSeg(sec_header[i].sh_offset, sec_header[i].sh_size);
            aMMU.setTextVMA(FileOff2VMA(sec_header[i].sh_offset));
        }

        if (strcmp(&sec_name[sec_header[i].sh_name], ".text") == 0)
        {
            aMMU.setTextSeg(sec_header[i].sh_offset, sec_header[i].sh_size);
            aMMU.setTextVMA(FileOff2VMA(sec_header[i].sh_offset));
        }

        if (strcmp(&sec_name[sec_header[i].sh_name], ".rodata") == 0)
        {
            aMMU.setRodataSeg(sec_header[i].sh_offset, sec_header[i].sh_size);
            aMMU.setRodataVMA(FileOff2VMA(sec_header[i].sh_offset));
        }

        if (strcmp(&sec_name[sec_header[i].sh_name], ".data") == 0)
        {
            aMMU.setDataSeg(sec_header[i].sh_offset, sec_header[i].sh_size);
            aMMU.setDataVMA(FileOff2VMA(sec_header[i].sh_offset));
        }*/

        if (sec_header[i].sh_flags == (SHF_ALLOC | SHF_EXECINSTR))
        {
            aMMU.setTextSeg(sec_header[i].sh_offset, sec_header[i].sh_size);
            aMMU.setTextVMA(FileOff2VMA(sec_header[i].sh_offset));
        }

        if ((sec_header[i].sh_flags == (SHF_ALLOC | SHF_WRITE))
        && (strcmp(&sec_name[sec_header[i].sh_name], ".bss") != 0))
        {
            aMMU.setDataSeg(sec_header[i].sh_offset, sec_header[i].sh_size);
            aMMU.setDataVMA(FileOff2VMA(sec_header[i].sh_offset));
        }

        //if (strcmp(&sec_name[sec_header[i].sh_name], ".rodata") == 0)
        if (sec_header[i].sh_flags == (SHF_ALLOC))
        {
            aMMU.setRodataSeg(sec_header[i].sh_offset, sec_header[i].sh_size);
            aMMU.setRodataVMA(FileOff2VMA(sec_header[i].sh_offset));
        }

        if (strcmp(&sec_name[sec_header[i].sh_name], ".bss") == 0)
        {
            aMMU.setBssSeg(sec_header[i].sh_offset, sec_header[i].sh_size);
            aMMU.setBssVMA(FileOff2VMA(sec_header[i].sh_offset));
        }

        //if (strcmp(&sec_name[sec_header[i].sh_name], ".stack") == 0)
        //{
        //    aMMU.setStackSeg(sec_header[i].sh_offset);
        //    aMMU.setStackVMA(sec_header[i].sh_addr);
        //}
    }
    aMMU.setStackSeg(0x200000);
    aMMU.setStackVMA(0x200000);
}


/**
  * Transform file offset to virtual address, according to the section information.
  * @param FileOff The offset in file
  * @return The virtual address in memory
  */
int elf_file::FileOff2VMA(int FileOff)
{
    for (int i = 0; i < elf_header.e_phnum; i++)
    {
        if (FileOff >= pro_header[i].p_offset && FileOff < (pro_header[i].p_offset + pro_header[i].p_memsz))
            return pro_header[i].p_vaddr + (FileOff - pro_header[i].p_offset);
    }

    return -1;
}

/**
  * Give out the entry point of the Thumb code
  * @return The entry point of the Thumb code
  */
int elf_file::getEntryPoint()
{
    return elf_header.e_entry;
}




