//
// Created by Martin on 23.03.2021.
//

#ifndef MY_MCSP_MYMONETDB_H
#define MY_MCSP_MYMONETDB_H


#include "../general/DatabaseSystem.h"

class MyMonetDB : public DatabaseSystem{
protected:
    Synopsis intermediate_result;


    Synopsis& mono_column_select(ColTable& t, const int col_index, const int lower, const int upper){
        intermediate_result.clear();
        const vector<int>& column = t.columns.at(col_index);

        const int size=t.size();
        for(int tid=0;tid<size;tid++){
            if(Util::isIn(column.at(tid),lower,upper)){
                intermediate_result.add(tid);
            }
        }
        if(LOG_COST){
            read_cost  += size;
            write_cost += intermediate_result.size();
        }
        return intermediate_result;
    }

    Synopsis& multi_column_select(ColTable& t, const vector<int>& column_indexes, const vector<vector<int>>& predicates){

        //first predicate produces the first synopsis
        Synopsis& s = mono_column_select(t, column_indexes[0], predicates[0][LOWER], predicates[0][UPPER]);

        if(column_indexes.size()==1){
            cout << name() << "called mcsp operator for mono selection" << endl;
            return s;
        }

        for(int p=1;p<column_indexes.size();p++){//start at the second predicate
            s = select(t, column_indexes.at(p), predicates.at(p).at(LOWER), predicates.at(p).at(UPPER), s);
        }

        return s;
    }

    Synopsis& select(ColTable& t, const int col_index, const int lower, const int upper, Synopsis& s){
        const int size = s.size();
        s.clear();
        const vector<int>& column = t.columns.at(col_index);
        for(int i=0;i<size;i++){
            const int tid = s.get(i);
            if(Util::isIn(column.at(tid),lower,upper)){
                s.add(tid);
            }
        }
        if(LOG_COST){
            read_cost  += 2*size;//read tid from synopsis and column value
            write_cost += intermediate_result.size();//size of result, i.e., new synopsis
        }
        return s;
    }

public:

    MyMonetDB() : intermediate_result(){

    }

    Synopsis& select(Table& t, vector<int>& column_indexes, vector<vector<int>>& predicates, vector<double>& selectivities){
        ColTable& table = dynamic_cast<ColTable &>(t);
        if(column_indexes.size()==1){
            return mono_column_select(table, column_indexes[0], predicates[0][LOWER], predicates[0][UPPER]);
        } else {
            return multi_column_select(table, column_indexes, predicates);
        }
    }

    std::unique_ptr<Table> get_TPC_H_lineitem(double scale){
        auto table = std::make_unique<ColTable>(scale);
        return table;
    }

    string name(){
        return "MyMonetDB";
    }

    void clear() override
    {
        intermediate_result.clear();
    }
};


#endif //MY_MCSP_MYMONETDB_H
