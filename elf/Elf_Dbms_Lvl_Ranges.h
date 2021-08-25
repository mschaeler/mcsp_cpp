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
};


#endif //MY_MCSP_ELF_DBMS_LVL_RANGES_H
