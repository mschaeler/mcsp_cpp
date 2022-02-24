//
// Created by Martin on 24.02.2022.
//

#ifndef MY_MCSP_SORTEDPROJECTION_H
#define MY_MCSP_SORTEDPROJECTION_H

#include "ColTable.h"

class Projection {

public:
    const int column;//This is the column number relevant for doing the projection, i.e., for ordering.
    vector<vector<int>> projected_columns;
    vector<int> org_tids;

    Projection(const int _column, ColTable& t)
            : column(_column),
              projected_columns(t.num_dim, vector<int>(t.size())),
              org_tids(t.size())
    {
        cout << "P"<<column<<"("; cout.flush();
        const vector<int>& org_data = t.columns.at(column);
        const int num_tuples = t.size();
        vector<value_tid> to_sort(num_tuples);

        //vector<value_tid> to_sort;
        for(int tid=0;tid<num_tuples;tid++) {
            to_sort.at(tid).tid   = tid;
            to_sort.at(tid).value = org_data.at(tid);
        }

        std::sort(to_sort.begin(),to_sort.end());

        //copy tids
        cout <<"tids) ";cout.flush();
        for(int i=0;i<num_tuples;i++) {
            value_tid v_t = to_sort.at(i);
            int tid = v_t.tid;
            org_tids.at(i) = tid;
        }

        //copy data
        for(int c=0;c<t.num_dim;c++){
            //cout <<","<< c; cout.flush();
            const vector<int>& org_column = t.columns.at(c);
            vector<int>& projected_column = projected_columns.at(c);

            for(int i=0;i<num_tuples;i++) {
                int tid = org_tids.at(i);
                int value_c = org_column.at(tid);
                projected_column.at(i) = value_c;
                /*if(i>0) {
                    if (value_c < projected_column.at(i)){
                        cout << "error" << endl;
                    }
                }*/
            }
        }
        //cout << ") ";
        //check projection
        /*for(int tid=1;tid<t.size();tid++){
            int before_val = projected_columns.at(0).at(tid-1);
            int val = projected_columns.at(0).at(tid);
            //check column
            if(val<before_val){
                cout << "error @tid=" << tid << endl;
            }
        }*/
    }
};

class Sorted_Projection_Table : public ColTable{
public:
    vector<Projection> projections;
    //Projection first_proj;

    Sorted_Projection_Table(ColTable& t)
    : ColTable(t.my_name, t.column_names, t.columns)
    //, first_proj(0,t)
    {
        //check projection
        /*for(int tid=1;tid<t.size();tid++){
            int before_val = first_proj.projected_columns.at(0).at(tid-1);
            int val = first_proj.projected_columns.at(0).at(tid);
            //check column
            if(val<before_val){
                cout << "error @tid=" << tid << endl;
            }
        }*/

        for(int c=0;c<t.num_dim;c++){
            projections.push_back(Projection(c,t));
            cout <<"c="<<c<<" vec size="<<projections.size();
        }
        cout<<endl;
    }
};

class SortedProjectionDBMS : public DatabaseSystem{
    static const bool USE_BUILD_IN_COPY = true;
    Synopsis intermediate_result;

public:

    SortedProjectionDBMS() : intermediate_result(){

    }

    Synopsis& select(Table& t, vector<int>& column_indexes, vector<vector<int>>& predicates, vector<double>& selectivities){
        Sorted_Projection_Table& table = dynamic_cast<Sorted_Projection_Table &>(t);

        int size = column_indexes.size();
        if(size == 1){//nothing to sort
            return mono_column_select(table,column_indexes.at(0), predicates.at(0).at(LOWER),predicates.at(0).at(UPPER));
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
        return mcsp_select(table,_column_indexes,_predicates,_selectivities);
    }

    Synopsis& mcsp_select(Sorted_Projection_Table& t, vector<int>& column_indexes, vector<vector<int>>& predicates, vector<double>& selectivities){
        return intermediate_result;
    }

    Synopsis& mono_column_select(Sorted_Projection_Table& t, const int col_index, const int lower, const int upper){
        intermediate_result.clear();
        Projection& p = t.projections.at(col_index);
        //Projection& p = t.first_proj;
        const vector<int>& sorted_column = p.projected_columns.at(col_index);
        const vector<int>& tids = p.org_tids;//we need to use the original tids for the result found in here

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

        ColTable col_t(scale); // create only locally, s.t. it gets destroyed after leaving the method
        Sorted_Projection_Table* table = new Sorted_Projection_Table(col_t);
        t =  dynamic_cast<Table*>(table);
        return t;
    }

    string name(){
        return "Sorted Projection DBMS";
    }
};

#endif //MY_MCSP_SORTEDPROJECTION_H