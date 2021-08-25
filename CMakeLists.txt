cmake_minimum_required(VERSION 3.17)
project(my_mcsp)

set(CMAKE_CXX_STANDARD 14)

add_executable(my_mcsp main.cpp columnar/ColTable.h general/Synopsis.h columnar/MyMonetDB.h elf/Elf_builder_separate.h elf/Elf.h elf/Elf_table_lvl_seperate.h benchmark/SelectionQuerySet.h benchmark/SelectionQuery.cpp benchmark/SelectionQuery.h general/Random.h general/SCSP.h general/MCSP.h benchmark/SelectionTests.h columnar/MyMonetDB_II.h columnar/Selecticity.h columnar/MyHyper.h general/Config.h elf/Elf_Dbms_Lvl.h elf/Elf_Table_Lvl_Cutoffs.h elf/Elf_Dbms_Lvl_Cutoffs.h)