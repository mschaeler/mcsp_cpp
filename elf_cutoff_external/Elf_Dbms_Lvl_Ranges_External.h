//
// Created by Martin on 03.09.2021.
//

#ifndef MY_MCSP_ELF_DBMS_LVL_RANGES_EXTERNAL_H
#define MY_MCSP_ELF_DBMS_LVL_RANGES_EXTERNAL_H

#include "../elf/Elf_Dbms_Lvl.h"

class Elf_Dbms_Lvl_Ranges_External : public Elf_Dbms_Lvl {
    Synopsis& select(Table& t, vector<int>& column_indexes, vector<vector<int>>& predicates, vector<double>& selectivities){
        Elf_Table_Cutoff_External& elf = dynamic_cast<Elf_Table_Cutoff_External &>(t);//the table
        result_buffer.clear();
        result_buffer.ensure_capacity(elf.tids_in_elf_order.size());

        if(column_indexes.size()==1){
            select_1(elf, column_indexes.at(0), predicates.at(0).at(LOWER), predicates.at(0).at(UPPER));
        }else{
            select_mcsp(elf, column_indexes, predicates);
        }
        return result_buffer;
    }

    Table* get_TPC_H_lineitem(double scale){
        Table* t;
        Elf_Table_Cutoff_External* table;

        ColTable col_t(scale); // create only locally, s.t. it gets destroyed after leaving the method
        table = Elf_builder_separate::build_with_external_cuttoffs(col_t);

        //std::cout << table->out() << std::endl;
        t =  dynamic_cast<Table *>(table);
        return t;
    }

    string name(){
        if(Elf_Table_Cutoff_External::USE_MEMCOPY){
            return "Elf_Dbms_Lvl_Ranges_External USE_MEMCOPY=true";
        }else{
            return "Elf_Dbms_Lvl_Ranges_External";
        }
    }
private:
    /**
     * Operator executing a selection on a single column (i.e., level)
     * SQL equivalent: Select row_id from elf where level in [lower,upper]
     * The name select_1(..) is to be conform with the other DBMS implementation.
     *
     * @param elf - the table
     * @param level - the level where we intend to select
     * @param lower - where level in [*lower*,upper]
     * @param upper - where level in [lower,*upper*]
     * @return - a tid list of all ids satisfying the predicate
     */
    void select_1(const Elf_Table_Cutoff_External& elf, const int level, const int lower, const int upper) {

        if(level == FIRST_DIM) {//for the first dim everything is special
            elf.select_1_cutoff_first_level(lower, upper, result_buffer);
            return;
        }

        /**************************************************************************************
         * (1) There may be Mono Lists starting before the level referring to column_index.
         * We need to scan them all. We need to do this first s.t. the TID order
         * follows the level order assumption
         **************************************************************************************/

        for(int l=FIRST_DIM;l<=level;l++){
            elf.scan_mono_list_level(l, lower, upper, result_buffer, level);
        }

        /**************************************************************************************
         * (2) Scan the entire level where the selection is done (i.e., column_index)
         **************************************************************************************/

        elf_pointer stop_level = elf.level_stop(level);
        elf_pointer offset     = elf.level_start(level);
        bool no_monolists_above = level<elf.first_level_with_monolists;
        if(no_monolists_above){
            elf.select_1_ranges_no_monolists_above(offset, stop_level, level, result_buffer, lower, upper);//can us better copy() function
        }else{
            elf.select_1_ranges(offset, stop_level, level, result_buffer, lower, upper);//non-density bug requirees copy function using for loop
        }
    }
    /**
     * Invokes execution of an mcsp predicate using ranges.
     * @param elf
     * @param column_indexes
     * @param predicates
     */
    void select_mcsp(const Elf_Table_Cutoff_External& elf, const vector<int>& column_indexes, const vector<vector<int>>& predicates){
        /**********************************************************************************************************************
		 * (1) - Handle the first selection predicate.
		 * This produces the first Level synopsis, similar to MonetDB like operator at a time model
         * ********************************************************************************************************************/
        int level = column_indexes.at(0);
        if(level==FIRST_DIM){
            //Due to the hash-map, this special case is simpler. And we do not need to care for mono lists before this level, since there is none.
            elf.select_mcsp_ranges_first_level(column_indexes, predicates, result_buffer);
        }else{
            // This is the common way of starting an mcsp selection
            // (1.1) check everything that became a mono list before first predicate
            elf.select_mono_lists_until_first_predicate(column_indexes, predicates, result_buffer);

            // (1.2) now scan the level itself. Important: the dim lists in the synopsis are in level columns[0]+1
            int stop_level      = elf.level_stop(level);
            int next_list_start = elf.level_start(level);
            elf.select_mcsp_ranges(next_list_start, stop_level, level, result_buffer, column_indexes, predicates, 0);
        }
    }
};


#endif //MY_MCSP_ELF_DBMS_LVL_RANGES_EXTERNAL_H
