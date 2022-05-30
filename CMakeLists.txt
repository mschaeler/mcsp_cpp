cmake_minimum_required(VERSION 3.17)
project(my_mcsp)

set(CMAKE_CXX_STANDARD 20)


add_executable(my_mcsp main.cpp columnar/ColTable.h general/Synopsis.h columnar/MyMonetDB.h elf/Elf_builder_separate.h elf/Elf.h elf/Elf_table_lvl_seperate.h benchmark/SelectionQuerySet.h benchmark/SelectionQuery.cpp benchmark/SelectionQuery.h general/Random.h general/SCSP.h general/MCSP.h benchmark/SelectionTests.h columnar/MyMonetDB_II.h columnar/Selecticity.h columnar/MyHyper.h general/Config.h elf/Elf_Dbms_Lvl.h elf/Elf_Table_Lvl_Cutoffs.h elf/Elf_Dbms_Lvl_Cutoffs.h elf/Elf_Dbms_Lvl_Ranges.h elf_cutoff_external/Elf_Table_Cutoff_External.h elf_cutoff_external/Elf_Dbms_Lvl_Cutoffs_External.h elf_cutoff_external/Elf_Dbms_Lvl_Ranges_External.h columnar/SortedProjection.h hot/HOT.hpp)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -mavx -mavx2 -O3" )
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_RELEASE} -march=native -mavx -mavx2 -O0 -g" )

add_subdirectory(hot)