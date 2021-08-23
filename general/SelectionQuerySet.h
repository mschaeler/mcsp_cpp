//
// Created by Martin on 22.07.2021.
//

#ifndef MY_MCSP_SELECTIONQUERYSET_H
#define MY_MCSP_SELECTIONQUERYSET_H

#include <vector>
#include "Util.h"
#include "SelectionQuery.h"
#include "Random.h"
#include "SCSP.h"
#include "MCSP.h"
#include <random>
#include <algorithm>    // std::sort

/**
 * This is a set containing multiple selection queries. Independent whether they are Mono or multicolumn selections.
 */
class SelectionQuerySet {
public:
    static const bool UNIFORM_COLUMN_PROBABILIY = false;
    double PROB_INCREASE_PER_COLUMN;

    vector<MCSP> myQueries;
    vector<int> cardinalities;

    /**
	 * Constructor for single predicate queries
	 * @param column
	 * @param selectivity
	 * @param num_queries
	 */
    SelectionQuerySet(int column, double selectivity, double scale, int num_queries, Random& _rand) :
            cardinalities(Util::cardinalitiesTPCH(scale))
            //, myQueries(num_queries)
    {
        //rand.seed(123);
        for(int q=0;q<num_queries;q++) {
            //myQueries.at(q) = SCSP(column, selectivity, cardinalities, rand);
        }
    }

    /**
      * Constructor for multi predicate queries
      * @param selectivity
      * @param scale
      * @param num_queries
      * @param max_num_cols
      */
    SelectionQuerySet(vector<double> base_selectivity, double p_values, double scale, int num_queries, int max_num_cols, Random& _rand) :
            cardinalities(Util::cardinalitiesTPCH(scale))
            , PROB_INCREASE_PER_COLUMN(p_values)
            //, myQueries(num_queries)
    {
        for(int q=0;q<num_queries;q++) {
            myQueries.push_back(MCSP(max_num_cols, 15, base_selectivity, PROB_INCREASE_PER_COLUMN, cardinalities, _rand));
            //myQueries.at(q) = new MCSP(max_num_cols, 15, base_selectivity, PROB_INCREASE_PER_COLUMN, cardinalities, rand);
        }
    }

    static void statisticsQuerySet(vector<SelectionQuerySet>& query_sets) {
        int num_queries = 0;
        vector<int> column_frequencies (15, 0);
        double counter = 0;
        double sum_selectivity = 0;
        vector<int> predicate_count_frequencies (5, 0);

        for(SelectionQuerySet set : query_sets) {
            num_queries += set.myQueries.size();
            for(auto& q : set.myQueries) {
                auto num_predicates = q.getColumns().size();
                predicate_count_frequencies.at(num_predicates)+=1;
                for(int c : q.getColumns()) {
                    column_frequencies.at(c)+=1;
                    //todo selectivity
                }
            }

            for(auto& q : set.myQueries) {
                auto num_predicates = q.getSelectivities().size();
                for(double s : q.getSelectivities()) {
                    sum_selectivity += s;
                    counter++;
                }
            }

        }

        double avg_selectivty = sum_selectivity / counter;
        cout << "Statistics for all sets: num_queries=\t" << to_string(num_queries);
        cout << "\tcolumn_frequencies=\t" << Util::to_string(column_frequencies);
        cout << "\tpredicate_count_frequencies=\t" << Util::to_string(predicate_count_frequencies);
        cout << "\tavg_col_selectivty=\t" << to_string(avg_selectivty);
        cout << endl;
    }
};



#endif //MY_MCSP_SELECTIONQUERYSET_H
