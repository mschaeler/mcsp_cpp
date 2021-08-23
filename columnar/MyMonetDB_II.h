//
// Created by Martin on 30.07.2021.
//

#ifndef MY_MCSP_MYMONETDB_II_H
#define MY_MCSP_MYMONETDB_II_H

#include <algorithm>
#include "Selecticity.h"

class MyMonetDB_II : public MyMonetDB {
public:
    Synopsis& select(Table& t, vector<int>& column_indexes, vector<vector<int>>& predicates, vector<double>& selectivities){
        int size = column_indexes.size();
        vector<Selecticity> to_sort;
        for(int i=0;i<size;i++){
            Selecticity temp(column_indexes.at(i), predicates.at(i).at(LOWER), predicates.at(i).at(UPPER), selectivities.at(i));
            to_sort.push_back(temp);
        }
        //Selecticity::out(to_sort);
        //sort
        std::sort(to_sort.begin(), to_sort.end());
        //Selecticity::out(to_sort);
        // copy because otherwise we alter the data of query itself as it is passed by reference
        vector<int> _column_indexes(size);
        vector<vector<int>> _predicates(size, vector<int>(2));
        vector<double> _selectivities(size);
        for(int i=0;i<size;i++){
            _column_indexes.at(i)  	    = to_sort.at(i).column_indexes;
            _predicates.at(i).at(LOWER) = to_sort.at(i).lower;
            _predicates.at(i).at(UPPER) = to_sort.at(i).upper;
            _selectivities.at(i) 	    = to_sort.at(i).selectivity;
        }

        return  MyMonetDB::select(t, _column_indexes, _predicates, _selectivities);
        /*
        ColTable& table = dynamic_cast<ColTable &>(t);
        if(column_indexes.size()==1){
            return mono_column_select(table, column_indexes[0], predicates[0][LOWER], predicates[0][UPPER]);
        } else {
            return multi_column_select(table, column_indexes, predicates);
        }
        */
    }
    string name(){
        return "MyMonetDB II";
    }
};


#endif //MY_MCSP_MYMONETDB_II_H
