//
// Created by Martin on 24.08.2021.
//

#ifndef MY_MCSP_ELF_TABLE_LVL_CUTOFFS_H
#define MY_MCSP_ELF_TABLE_LVL_CUTOFFS_H

class CuttOffs{
/** This are all tids in elf order */
vector<int> tids_in_elf_order;
public:
    CuttOffs(){

    }
};

class Elf_Table_Lvl_Cutoffs : public Elf_table_lvl_seperate {
    CuttOffs myCutoffs;
public:
};


#endif //MY_MCSP_ELF_TABLE_LVL_CUTOFFS_H
