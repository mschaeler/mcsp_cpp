//
// Created by Martin on 30.07.2021.
//

#ifndef MY_MCSP_MYMONETDB_II_H
#define MY_MCSP_MYMONETDB_II_H

#include <algorithm>
#include "Selecticity.h"
#include <math.h>       /* log2 */

class MyMonetDB_II : public MyMonetDB {
public:
    Synopsis& select(Table& t, vector<int>& column_indexes, vector<vector<int>>& predicates, vector<double>& selectivities){
        int size = column_indexes.size();
        if(size == 1){//nothing to sort
            return MyMonetDB::select(t,column_indexes,predicates,selectivities);
        }
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
    }
    string name(){
        return "MyMonetDB II";
    }
};

class MyMonetDB_Indexed : public MyMonetDB{
    Synopsis& select(Table& t, vector<int>& column_indexes, vector<vector<int>>& predicates, vector<double>& selectivities){
        ColTable_Indexed& table = dynamic_cast<ColTable_Indexed &>(t);
        //first predicate produces the first synopsis
        Synopsis& s = mono_column_select(table, column_indexes[0], predicates[0][LOWER], predicates[0][UPPER]);

        if(column_indexes.size()==1){
            return s;
        }

        ColTable& table_c = dynamic_cast<ColTable &>(t);
        for(int p=1;p<column_indexes.size();p++){//start at the second predicate
            s = MyMonetDB::select(table_c, column_indexes.at(p), predicates.at(p).at(LOWER), predicates.at(p).at(UPPER), s);
        }

        return s;
    }

    Synopsis& mono_column_select(ColTable_Indexed& t, const int col_index, const int lower, const int upper){
        //cout << "daaaaaaaaaaaaaaaa" << endl;
        intermediate_result.clear();
        const vector<int>& sorted_column = t.sorted_columns.at(col_index);

        auto low = lower_bound (sorted_column.begin(), sorted_column.end(), lower);
        auto up  = upper_bound (sorted_column.begin(), sorted_column.end(), upper);
        std::cout << "lower_bound at position " << (low- sorted_column.begin()) << endl;
        std::cout << "upper_bound at position " << (up - sorted_column.begin()) << endl;
        intermediate_result.copy(low, up);
        //copy(low,up,intermediate_result.array.begin());

        if(LOG_COST){
            read_cost  += 2*log2(t.size());
            write_cost += intermediate_result.size();
        }
        return intermediate_result;
    }

    Table* get_TPC_H_lineitem(double scale){
        Table* t;

        ColTable table = ColTable(scale);//create locally
        ColTable_Indexed* t_1 = new ColTable_Indexed(table);//create globally
        t =  dynamic_cast<Table*>(t_1);
        return t;
    }

    string name(){
        return "MyMonetDB Indexed";
    }
};


#endif //MY_MCSP_MYMONETDB_II_H
