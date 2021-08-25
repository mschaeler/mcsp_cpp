//
// Created by Martin on 13.08.2021.
//

#ifndef MY_MCSP_ELF_DBMS_LVL_H
#define MY_MCSP_ELF_DBMS_LVL_H

#include "Elf_builder_separate.h"

class Elf_Dbms_Lvl : public DatabaseSystem{
protected:
    static const elf_pointer 	LAST_ENTRY_MASK	 = Elf::LAST_ENTRY_MASK;//0b10000000000000000000000000000000
    static const elf_pointer 	RECOVER_MASK	 = Elf::RECOVER_MASK;//0b01111111111111111111111111111111
    static const elf_pointer 	EMPTY_ROOT_NODE  = Elf::EMPTY_ROOT_NODE;
    static const int32_t 	DIM_ELEMENT_SIZE = Elf::DIM_ELEMENT_SIZE;
    static const int32_t 	FIRST_DIM = Elf::FIRST_DIM;
    static const elf_pointer 	NOT_FOUND = Elf::NOT_FOUND;

    static const int NODE_HEAD_LENGTH    = Elf_table_lvl_seperate::NODE_HEAD_LENGTH;
    static const bool SAVE_MODE          = Elf::SAVE_MODE;


    Synopsis result_buffer;
private:
    Synopsis& select(Elf_table_lvl_seperate& t, const vector<int>& column_indexes, const vector<vector<int>>& predicates){
        result_buffer.clear();//Here we clear hte buffer s.t. we do not care anywhere later
        if(column_indexes.size()==1){
            select_1(t, column_indexes.at(0),predicates.at(0).at(LOWER),predicates.at(0).at(UPPER));
        }else{
            select_mcsp(t, column_indexes, predicates);
        }
        return result_buffer;
    }

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
    void select_1(Elf_table_lvl_seperate& elf, const int level, const int lower, const int upper){
        if(level == FIRST_DIM) {//for the first dim everything is special
            return elf.select_1_first_dim(lower, upper, result_buffer);
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

        while (offset<stop_level) {
            //scan each node
            offset = elf.scan_1_node(offset, result_buffer, lower, upper, level);
        }
    }

    void select_mcsp(Elf_table_lvl_seperate& elf, const vector<int>& columns, const vector<vector<int>>& predicates) {

        /**********************************************************************************************************************
         * (1) - Handle the first selection predicate.
         * This produces the first Level synopsis, similar to MonetDB like operator at a time model
         * ********************************************************************************************************************/
        if(columns.at(0)==FIRST_DIM){
            //Due to the hash-map, this special case is simpler. And we do not need to care for mono lists before this level, since there is none.
            elf.select_first_dim_preorder(columns, predicates, result_buffer);
        }else {
            // This is the common way of starting an mcsp selection
            // (1.1) check everything that became a mono list before first predicate
            elf.select_mono_lists_until_first_predicate(columns, predicates, result_buffer);

            // (1.2) now scan the level itself. Important: the dim lists in the synopsis are in level columns[0]+1
            elf.select_mcsp_level_preorder(columns, predicates, result_buffer);
        }
    }

public:
    Elf_Dbms_Lvl() : result_buffer(){

    }

    Synopsis& select(Table& t, vector<int>& column_indexes, vector<vector<int>>& predicates, vector<double>& selectivities){
        Elf_table_lvl_seperate& table = dynamic_cast<Elf_table_lvl_seperate &>(t);
        return select(table, column_indexes, predicates);
    }

    Table* get_TPC_H_lineitem(double scale){
        Table* t;
        Elf_table_lvl_seperate* table;

        if(Elf_table_lvl_seperate::exists(scale)){
            table = Elf_table_lvl_seperate::from_file(scale);
        }else{
            //We first create a col table and then build the Elf from it
            ColTable col_t(scale); // create only locally, s.t. it gets destroyed after leaving the method
            table = Elf_builder_separate::build(col_t);
        }

        if(Config::MATERIALIZE_DATA){
            Elf_table_lvl_seperate::materialize(*table, scale);
        }

        //std::cout << table->out() << std::endl;
        t =  dynamic_cast<Table *>(table);
        return t;
    }

    string name(){
        return "Elf_Dbms_Lvl";
    }
};


#endif //MY_MCSP_ELF_DBMS_LVL_H
