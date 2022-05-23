#pragma once

#include "libs/hot/single-threaded/include/hot/singlethreaded/HOTSingleThreadedPayload.hpp"

class HOT {
    using HotSingleThreadedPayload = typename hot::singlethreaded::HOTSingleThreadedPayload<decltype(value_tid::value), decltype(value_tid::tid)>;
    const int column;//This is the column number relevant for doing the projection, i.e., for ordering.

public:
    HOT(const int _column, ColTable &t)
            : column(_column), col_data() {
        for (auto i = 0; i < t.columns[column].size(); ++i) {
            col_data.insert(t.columns[column][i], i);
        }
    }

    HotSingleThreadedPayload col_data;
};

class HOTTable : public ColTable {
public:
    std::vector<HOT> columns;

    HOTTable(ColTable &t) : ColTable(t.my_name, t.column_names, t.columns) {
        for (int c = 0; c < t.num_dim; c++) {
            columns.emplace_back(c, t);//avoid temporary construction and subsequent push back
        }
        cout << endl;
    }
};

class HOTDBMS : public DatabaseSystem {
    Synopsis intermediate_result;

public:
    HOTDBMS() = default;

    Synopsis &select(Table &t, vector<int> &column_indexes, vector<vector<int>> &predicates,
                     vector<double> &selectivities) override {
        auto &table = dynamic_cast<HOTTable &>(t);
        intermediate_result.clear();
        intermediate_result.ensure_capacity(table.size());

        int size = column_indexes.size();
        if (size == 1) {//nothing to sort
            return select_1(table, column_indexes.at(0), predicates.at(0).at(LOWER), predicates.at(0).at(UPPER));
        }
        vector<Selecticity> to_sort;
        for (int i = 0; i < size; i++) {
            Selecticity temp(column_indexes.at(i), predicates.at(i).at(LOWER), predicates.at(i).at(UPPER),
                             selectivities.at(i));
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
        for (int i = 0; i < size; i++) {
            _column_indexes.at(i) = to_sort.at(i).column_indexes;
            _predicates.at(i).at(LOWER) = to_sort.at(i).lower;
            _predicates.at(i).at(UPPER) = to_sort.at(i).upper;
            _selectivities.at(i) = to_sort.at(i).selectivity;
        }

//cout << Util::to_string(column_indexes) << " sel=" << Util::to_string(selectivities) << " after: ";
//cout << Util::to_string(_column_indexes) << " sel=" << Util::to_string(_selectivities) << endl;
        return select_mcsp(table, _column_indexes, _predicates, _selectivities);
    }

    static void materialize(const int from, const int to, Synopsis &result_tids, const vector<int> &org_tids) {
        result_tids.add(org_tids.begin() + from, org_tids.begin() + to);
        if (LOG_COST) { write_cost += to - from; }
    }

    void
    select_mcsp(HOT &p, vector<int> &column_indexes, vector<vector<int>> &predicates,
                vector<double> &selectivities,
                const long long int run_start, const long long int run_stop, const int column_index,
                Synopsis &result_tids
    ) const {
        //TODO
        /*const int column_num = column_indexes.at(column_index);
        auto lower = p.col_data.lower_bound(LOWER);
        auto upper = p.col_data.lower_bound(UPPER);

        const int last_selected_level = column_indexes.at(column_indexes.size() - 1);
        const bool is_final_column = column_num == last_selected_level;
        int offset_where_run_starts = -1;

        auto offset = run_start;
        while (offset < run_stop) {
            //(1) find start of result run in this column
            while (offset < run_stop) {
                const int value = raw_column.at(offset);
                if (Util::isIn(value, lower, upper)) {
                    offset_where_run_starts = offset;
                    offset++;
                    break;
                }
                offset++;
            }

            //(2) find stop of result run in this column
            while (offset < run_stop) {
                const int value = raw_column[offset];
                if (!Util::isIn(value, lower, upper)) {//found end of run in this column
                    if (is_final_column) {
                        materialize(offset_where_run_starts, offset, result_tids, p.org_tids);
                    } else {
                        select_mcsp(p, column_indexes, predicates, selectivities, offset_where_run_starts, offset,
                                    column_index + 1, result_tids);
                    }
                    offset++;
                    offset_where_run_starts = -1;
                    break;
                }
                offset++;
            }
        }

        //last run: We do not see its end.
        if (offset_where_run_starts != -1) {
            if (is_final_column) {
                materialize(offset_where_run_starts, offset, result_tids, p.org_tids);
            } else {
                select_mcsp(p, column_indexes, predicates, selectivities, offset_where_run_starts, offset,
                            column_index + 1, result_tids);
            }
        }

        if (LOG_COST) {
            read_cost += (run_stop - run_start);
        }*/
    }

    Synopsis &select_mcsp(HOTTable &t, vector<int> &column_indexes, vector<vector<int>> &predicates,
                          vector<double> &selectivities) {

        //TODO
        //inlined select_1() method
        /*const int col_index = column_indexes.at(0);
        const int lower = predicates.at(0).at(LOWER);
        const int upper = predicates.at(0).at(UPPER);

        HOT &p = t.columns.at(col_index);
        //Projection& p = t.first_proj;
        const vector<int> &sorted_column = p.col_data.at(col_index);
        const vector<int> tids;

        const auto low = p.col_data.lower_bound(lower);
        const auto up = p.col_data.lower_bound(upper);

        if (LOG_COST) {
            read_cost += 2 * log2(t.size());
        }

        int column_index = 1;
        select_mcsp(p, column_indexes, predicates, selectivities, low, up, column_index, intermediate_result);
*/
        return intermediate_result;
    }

    Synopsis &select_1(HOTTable &t, const int col_index, const int lower, const int upper) {
        HOT &p = t.columns.at(col_index);
        //Projection& p = t.first_proj;
        vector<int> tids;

        auto low = p.col_data.lower_bound( lower);
        auto up = p.col_data.upper_bound(upper);

        //std::cout << "lower_bound at position " << (low- sorted_column.begin()) << endl;
        //std::cout << "upper_bound at position " << (up - sorted_column.begin()) << endl;

        //We copy from tid vector, not from the sorted column itself. So, the iterators (low,up) cannot be used for copying directly.

        int i = 0;
        for (auto iter = low; iter != up; ++iter)
        {
            auto &vals = (*iter);
            std::cout << i++ << endl;
            //tids.insert(tids.end(), vals.begin(), vals.end());
            tids.push_back(3);
        }
        //intermediate_result.move(std::move(tids));

        if (LOG_COST) {
            read_cost += 2 * log2(t.size());
            write_cost += intermediate_result.size();
        }
        return intermediate_result;
    }

    Table *get_TPC_H_lineitem(double scale) override {
        Table *t;

        ColTable col_t(scale); // create only locally, s.t. it gets destroyed after leaving the method
        auto *table = new HOTTable(col_t);
        t = dynamic_cast<Table *>(table);
        return t;
    }

    string name() override {
        return "HOT DBMS";
    }
};

