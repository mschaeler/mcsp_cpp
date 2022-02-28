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

    explicit Sorted_Projection_Table(ColTable& t)
    : ColTable(t.my_name, t.column_names, t.columns)
    {
        for(int c=0;c<t.num_dim;c++){
            projections.emplace_back(c,t);//avoid temporary construction and subsequent push back
        }
        cout<<endl;
    }
};

class SortedProjectionDBMS : public DatabaseSystem{
    Synopsis intermediate_result;

public:

    SortedProjectionDBMS() : intermediate_result(){

    }

    Synopsis& select(Table& t, vector<int>& column_indexes, vector<vector<int>>& predicates, vector<double>& selectivities) override {
        auto& table = dynamic_cast<Sorted_Projection_Table &>(t);
        intermediate_result.clear();
        intermediate_result.ensure_capacity(table.size());

        int size = column_indexes.size();
        if(size == 1){//nothing to sort
            return select_1(table, column_indexes.at(0), predicates.at(0).at(LOWER), predicates.at(0).at(UPPER));
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
        return select_mcsp(table, _column_indexes, _predicates, _selectivities);
    }

    static void materialize(const int from, const int to, Synopsis& result_tids, const vector<int>& org_tids) {
        result_tids.add(org_tids.begin()+from,org_tids.begin()+to);
        if(LOG_COST){write_cost+=to-from;}
    }

    void
    select_mcsp(Projection& p, vector<int>& column_indexes, vector<vector<int>>& predicates, vector<double>& selectivities,
                const long long int run_start, const long long int run_stop, const int column_index
                , Synopsis& result_tids
    ) const {
        const int column_num = column_indexes.at(column_index);
        const vector<int>& raw_column = p.projected_columns.at(column_num);
        const int lower = predicates.at(column_index).at(LOWER);
        const int upper = predicates.at(column_index).at(UPPER);

        const int last_selected_level 	  = column_indexes.at(column_indexes.size()-1);
        const bool is_final_column = column_num == last_selected_level;
        int offset_where_run_starts = -1;

        auto offset = run_start;
        while(offset<run_stop){
            //(1) find start of result run in this column
            while(offset<run_stop) {
                const int value = raw_column.at(offset);
                if(Util::isIn(value, lower, upper)) {
                    offset_where_run_starts = offset;
                    offset++;
                    break;
                }
                offset++;
            }

            //(2) find stop of result run in this column
            while(offset<run_stop) {
                const int value = raw_column[offset];
                if(!Util::isIn(value, lower, upper)) {//found end of run in this column
                    if(is_final_column) {
                        materialize(offset_where_run_starts, offset, result_tids, p.org_tids);
                    }else {
                        select_mcsp(p, column_indexes, predicates, selectivities, offset_where_run_starts, offset, column_index+1, result_tids);
                    }
                    offset++;
                    offset_where_run_starts = -1;
                    break;
                }
                offset++;
            }
        }

        //last run: We do not see its end.
        if(offset_where_run_starts!=-1) {
            if(is_final_column) {
                materialize(offset_where_run_starts, offset, result_tids, p.org_tids);
            }else {
                select_mcsp(p, column_indexes, predicates, selectivities, offset_where_run_starts, offset, column_index+1, result_tids);
            }
        }

        if(LOG_COST) {
            read_cost+=(run_stop-run_start);
        }
    }

    Synopsis& select_mcsp(Sorted_Projection_Table& t, vector<int>& column_indexes, vector<vector<int>>& predicates, vector<double>& selectivities){

        //inlined select_1() method
        const int col_index = column_indexes.at(0);
        const int lower = predicates.at(0).at(LOWER);
        const int upper = predicates.at(0).at(UPPER);

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

        if(LOG_COST){
            read_cost  += 2*log2(t.size());
        }

        int column_index = 1;
        select_mcsp(p, column_indexes, predicates, selectivities, from , to, column_index, intermediate_result);

        return intermediate_result;
    }

    Synopsis& select_1(Sorted_Projection_Table& t, const int col_index, const int lower, const int upper){
        const Projection& p = t.projections.at(col_index);
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
        intermediate_result.copy(tids.begin()+from, tids.begin()+to);

        if(LOG_COST){
            read_cost  += 2*log2(t.size());
            write_cost += intermediate_result.size();
        }
        return intermediate_result;
    }

    Table* get_TPC_H_lineitem(double scale) override{
        Table* t;

        ColTable col_t(scale); // create only locally, s.t. it gets destroyed after leaving the method
        Sorted_Projection_Table* table = new Sorted_Projection_Table(col_t);
        t =  dynamic_cast<Table*>(table);
        return t;
    }

    string name() override{
        return "Sorted Projection DBMS";
    }
};

#endif //MY_MCSP_SORTEDPROJECTION_H
