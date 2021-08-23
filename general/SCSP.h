//
// Created by Martin on 22.07.2021.
//

#ifndef MY_MCSP_SCSP_H
#define MY_MCSP_SCSP_H


class SCSP : public SelectionQuery {
    int column;
    vector<int> column_as_vector;
    vector<vector<int>> predicate;
    vector<double> selectivity;


public:
    SCSP(int _column, double _selectivity, vector<int> cardinalities, Random& rand) :
            column(_column)
            , selectivity(1)
            , predicate(1,vector<int>(2,0))
            , column_as_vector(1)
    {
        predicate.at(0) = single_column_predicate(column, cardinalities, rand, _selectivity);
        column_as_vector.at(0) = column;
        selectivity.at(0) = _selectivity;
    }

    string toString(){
        return std::to_string(column) +" "+Util::to_string(predicate[0]);
    }

    /**
	 * Creates one single predicate on the specified columns having the desired selectivity.
	 * Note selectivity is not smaller, but may be larger, i.e., more tuple are returned if cardinality of the column is small.
	 * @param col
	 * @param cardinalities
	 * @param rand
	 * @param selectivity
	 * @return
	 */
    static vector<int> single_column_predicate(int col, vector<int> cardinalities, Random rand, double selectivity) {
        vector<int> predicates(2);

        int min_val = 0;
        int max_val = cardinalities[col];

        int num_values = max_val-min_val;
        int range = (int) ((double) num_values * selectivity);
        int max_start_value = num_values - range;

        if(selectivity>=1.0d) {
            predicates[0] = 0;
            predicates[1] = max_val;
        }else {
            int lower = rand.nextInt(max_start_value);
            int upper = lower+range;
            predicates[0] = lower;
            predicates[1] = upper;
        }

        return predicates;
    }

    vector<vector<int>>& getPredicate(){
        return predicate;
    }

    vector<int>& getColumns(){
        return column_as_vector;
    }

    vector<double>& getSelectivities(){
        return selectivity;
    }
};


#endif //MY_MCSP_SCSP_H
