//
// Created by Martin on 06.04.2021.
//

#ifndef MY_MCSP_ELF_H
#define MY_MCSP_ELF_H

#include "cstdint"
#include "limits.h"
#include "../general/Table.h"

//#include "../general/Util.h"
//#define USE_64_BIT_POINTER

#ifdef USE_64_BIT_POINTER
typedef int64_t elf_pointer;
#else
typedef int32_t elf_pointer;
#endif

class Elf : public Table{
public:
    Elf(string& name, vector<string>& col_names) : Table(name, col_names) {

    }
    static const int32_t ELF_TYPE_IS_PRE_ORDER_ORDER_ELF = 0x0;
    static const int32_t ELF_TYPE_IS_LEVEL_ORDER_ELF = 0x1;
    static const int32_t ELF_TYPE_IS_LEVEL_ORDER_ELF_SEPERATED = 0x2;
#ifdef USE_64_BIT_POINTER
    static const elf_pointer 	LAST_ENTRY_MASK	 = LLONG_MIN;//LLONG_MIN
    static const elf_pointer 	RECOVER_MASK	 = LLONG_MAX;//LLONG_MAX
#else
    static const int32_t 	LAST_ENTRY_MASK	 = INT_MIN;//0b10000000000000000000000000000000
    static const int32_t 	RECOVER_MASK	 = INT_MAX;//0b01111111111111111111111111111111
#endif
    static const elf_pointer 	NODE_LENGTH_MASK            = INT_MIN;//0b10000000000000000000000000000000
    static const elf_pointer 	RECOVER_NODE_LENGTH_MASK	= INT_MAX;//0b01111111111111111111111111111111
    static const elf_pointer 	EMPTY_ROOT_NODE  = INT_MIN;
    static const int32_t 	DIM_ELEMENT_SIZE = 2;
    static const int32_t 	FIRST_DIM = 0;
    static const elf_pointer 	NOT_FOUND = -1;
    static const int64_t 	HEADER_OFFSET = 2*4;
    static const int64_t 	MAX_POINTER = INT_MAX;

    static const bool SAVE_MODE  = false;
    static const bool DEBUG_MODE = false;

};


#endif //MY_MCSP_ELF_H
