//
// Created by b1074672 on 25.08.2021.
//

#ifndef MY_MCSP_ELF_DBMS_LVL_RANGES_H
#define MY_MCSP_ELF_DBMS_LVL_RANGES_H


class Elf_Dbms_Lvl_Ranges : public Elf_Dbms_Lvl_Cutoffs{
public:

    Synopsis& select(Table& t, vector<int>& column_indexes, vector<vector<int>>& predicates, vector<double>& selectivities){
        Elf_Table_Lvl_Cutoffs& elf = dynamic_cast<Elf_Table_Lvl_Cutoffs &>(t);//the table
        result_buffer.clear();
        if(column_indexes.size()==1){
            select_1(elf, column_indexes.at(0), predicates.at(0).at(LOWER), predicates.at(0).at(UPPER));
        }else{
            //return select_mcsp(elf, column_numbers, predicates);
        }
        return result_buffer;
    }

    Table* get_TPC_H_lineitem(double scale){
        Table* t;
        Elf_Table_Lvl_Cutoffs* table;

        ColTable col_t(scale); // create only locally, s.t. it gets destroyed after leaving the method
        table = Elf_builder_separate::build_with_cuttoffs(col_t);

        //std::cout << table->out() << std::endl;
        t =  dynamic_cast<Table *>(table);
        return t;
    }

    string name(){
        return "Elf_Dbms_Lvl_Ranges";
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
    void select_1(const Elf_Table_Lvl_Cutoffs& elf, const int level, const int lower, const int upper) {

        if(level == FIRST_DIM) {//for the first dim everything is special
            elf.select_1_first_level(lower, upper, result_buffer);
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

        int stop_level = elf.level_stop(level);
        int start_level= elf.level_start(level);

        elf.scan_mono_column_cutoff_ranges(start_level, stop_level, level, result_buffer, lower, upper);
    }
};


#endif //MY_MCSP_ELF_DBMS_LVL_RANGES_H
