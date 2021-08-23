//
// Created by Martin on 23.03.2021.
//

#ifndef MCSP_MYROWISEHYPER_H
#define MCSP_MYROWISEHYPER_H


#include "RowTable.h"
#include "../general/DatabaseSystem.h"
#include "../columnar/ColTable.h"

class MyRowiseHyper : public DatabaseSystem{
    Synopsis intermediate_result;

    Synopsis& mono_column_select(RowTable& t, const int col_index, const int lower, const int upper){
        intermediate_result.clear();

        const int size=t.size();
        for(int tid=0;tid<size;tid++){
            const vector<int>& tuple = t.tuples.at(tid);
            if(Util::isIn(tuple.at(col_index),lower,upper)){
                intermediate_result.add(tid);
            }
        }
        if(LOG_COST){
            read_cost += size*t.tuples.at(0).size();//num_tuples (i.e., size) * tuple size (of the first tuple)
            write_cost += intermediate_result.size();
        }
        return intermediate_result;
    }

    Synopsis& multi_column_select(RowTable& t, const vector<int>& column_indexes, const vector<vector<int>>& predicates){
        intermediate_result.clear();

        const int size=t.size();
        for(int tid=0;tid<size;tid++){
            const vector<int>& tuple = t.tuples.at(tid);
            if(Util::isIn(tuple, column_indexes, predicates)){
                intermediate_result.add(tid);
            }
        }
        if(LOG_COST){
            read_cost += size*t.tuples.at(0).size();//num_tuples (i.e., size) * tuple size (of the first tuple)
            write_cost += intermediate_result.size();
        }
        return intermediate_result;
    }

public:
    MyRowiseHyper() : intermediate_result(){
        //nothing todo
    }

    Synopsis& select(Table& t, vector<int>& column_indexes, vector<vector<int>>& predicates, vector<double>& selectivities){
        RowTable& table = dynamic_cast<RowTable &>(t);
        if(column_indexes.size()==1){
            return mono_column_select(table, column_indexes[0], predicates[0][LOWER], predicates[0][UPPER]);
        } else {
            return multi_column_select(table, column_indexes, predicates);
        }
    }

    string name(){
        return "MyRowiseHyper";
    }

    Table* get_TPC_H_lineitem(double scale){
        Table* t;
        RowTable* row_table = new RowTable(scale);
        //std::cout << row_table->out() << std::endl;
        t =  dynamic_cast<Table *>(row_table);
        return t;
    }
};


#endif //MCSP_MYROWISEHYPER_H
