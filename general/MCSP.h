//
// Created by Martin on 22.07.2021.
//

#ifndef MY_MCSP_MCSP_H
#define MY_MCSP_MCSP_H

#include "algorithm"

class MCSP : public SelectionQuery {
    vector<int> columns;
    vector<vector<int>> predicates;
    vector<double> selectivities;
    double prob_increase_per_column;

    /**
	*
	* @param dice - the dice
	* @param column_probs - array[0] - min dice required for first column...
	* @return
	*/
    int getColumn(double dice, vector<double> column_probs) {
        for(int c=0;c<column_probs.size()-1;c++) {
            double prob = column_probs[c+1];//not small enough to one of the next columns
            if(dice >= prob) {
                return c;
            }
        }
        //cout << "getColumn(): Warning selected last column" << endl;
        return column_probs.size()-1;
    }

    vector<double> probabilities(int columns, double prob_increase) {
        if(prob_increase<1 || prob_increase>2) {
            cout << "probabilities prob_increase=" << prob_increase << endl;
        }
        vector<double> probs(columns);
        probs[columns-1] = 1.0;
        for(int c=columns-2;c>=0;c--) {
            probs[c] = (probs[c+1]*prob_increase)+probs[columns-1];
        }
        return probs;
    }

    void diceColumns(int table_dimensionality, int num_columns) {

        //dice the columns for select
        if(table_dimensionality < num_columns) {
            cout << "MCSP(int, int) - Table does not have that many columns." << endl;
        }else if(num_columns == table_dimensionality){
            for(int c=0;c<num_columns;c++){columns.push_back(c);}//simply fill with all columns ASC
        }else{
            int i = 0;
            vector<double> column_probs = probabilities(table_dimensionality, prob_increase_per_column);
            while(i<num_columns) {
                double f = (double)rand() / RAND_MAX;
                double dice = f*column_probs[0];//time max value of the dice
                int column = getColumn(dice, column_probs);
                if(!(Util::isIn(column, columns))) {//The columns need to unique ...
                    columns.push_back(column);
                    i++;
                }
            }
            std::sort(columns.begin(),columns.end());
        }
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
    static vector<int> single_column_predicate(int col, vector<int> cardinalities, Random& rand, double selectivity) {
        vector<int> predicates(2);

        int min_val = 0;
        int max_val = cardinalities[col];

        int num_values = max_val-min_val;
        int range = (int) ((double) num_values * selectivity);
        int max_start_value = num_values - range;

        if(selectivity>=1.0) {
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

public:
    MCSP(int num_columns, int table_dimensionality, double selectivity, double prob_increase, vector<int> cardinalities, Random& rand) :
            columns(num_columns)
            , predicates(num_columns)
            , selectivities(num_columns)
    {
        prob_increase_per_column = prob_increase;
        vector<double> column_probs = probabilities(table_dimensionality, prob_increase_per_column);
        cout << "diceColumns() " << Util::to_string(column_probs) << endl;
        diceColumns(table_dimensionality, num_columns);

        //for each dice column create the predicate
        for(int i=0;i<columns.size();i++) {
            int column = columns[i];
            selectivities[i] = selectivity;
            vector<int> sinlge_predicate = single_column_predicate(column, cardinalities, rand, selectivities[i]);
            predicates.at(i) = sinlge_predicate;
        }
    }

    MCSP(int max_num_columns, int table_dimensionality, vector<double> base_selectivities, double prob_increase, vector<int> cardinalities, Random& rand)
    {
        prob_increase_per_column = prob_increase;
        bool ensure_mcsp = false;
        int num_columns;
        if(ensure_mcsp) {
            num_columns = 2+rand.nextInt(max_num_columns-2);
        }else {
            num_columns = 1+rand.nextInt(max_num_columns-1);
        }
        diceColumns(table_dimensionality, num_columns);
        //for each dice column create the predicate
        for(int i=0;i<columns.size();i++) {
            int column = columns.at(i);
            selectivities.push_back(base_selectivities[rand.nextInt(base_selectivities.size())]);
            vector<int> sinlge_predicate = single_column_predicate(column, cardinalities, rand, selectivities[i]);
            predicates.push_back(sinlge_predicate);
        }
    }

    vector<vector<int>>& getPredicate(){
        return predicates;
    }

    vector<int>& getColumns(){
        return columns;
    }

    vector<double>& getSelectivities(){
        return selectivities;
    }

};


#endif //MY_MCSP_MCSP_H
