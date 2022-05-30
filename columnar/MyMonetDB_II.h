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
    static const bool USE_BUILD_IN_COPY = true;

    Synopsis& select(Table& t, vector<int>& column_indexes, vector<vector<int>>& predicates, vector<double>& selectivities){
        int size = column_indexes.size();
        if(size == 1){//nothing to sort
            return select_internal(t,column_indexes,predicates,selectivities);
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
        //cout << Util::to_string(column_indexes) << " sel=" << Util::to_string(selectivities) << " after: ";
        //cout << Util::to_string(_column_indexes) << " sel=" << Util::to_string(_selectivities) << endl;
        return select_internal(t,_column_indexes,_predicates,_selectivities);
    }

    Synopsis& select_internal(Table& t, vector<int>& column_indexes, vector<vector<int>>& predicates, vector<double>& selectivities){
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
        intermediate_result.clear();
        const vector<int>& sorted_column = t.sorted_columns.at(col_index);
        const vector<int>& tids = t.sorted_columns_original_tids.at(col_index);//we need to use the original tids for the result found in here

        const auto low = lower_bound (sorted_column.begin(), sorted_column.end(), lower);
        const auto up  = upper_bound (sorted_column.begin(), sorted_column.end(), upper);
        //std::cout << "lower_bound at position " << (low- sorted_column.begin()) << endl;
        //std::cout << "upper_bound at position " << (up - sorted_column.begin()) << endl;

        //We copy from tid vector, not from the sorted column itself. So, the iterators (low,up) cannot be used for copying directly.
        const auto from = low - sorted_column.begin();
        const auto to = up - sorted_column.begin();
        if(USE_BUILD_IN_COPY){
            intermediate_result.copy(tids.begin()+from, tids.begin()+to);
        }else{
            for(int pos=0;pos<from;pos++){
                intermediate_result.add(tids.at(pos));
            }
        }

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

    void clear() override
    {
        intermediate_result = Synopsis();
    }
};


#endif //MY_MCSP_MYMONETDB_II_H
