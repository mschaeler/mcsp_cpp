//
// Created by Martin on 22.07.2021.
//

#ifndef MY_MCSP_SELECTIONQUERY_H
#define MY_MCSP_SELECTIONQUERY_H

#include <vector>
#include "../general/Util.h"
#include <random>

/**
 * This is an abstract class representing a single query: Basically the idea is as follows:
 * getPredicate() returns a 2d-array. The first dimension is the column (e.g., getPredicate()[1]).
 * The second dimension is the lower and upper selection bound of that column.
 */
class SelectionQuery {
public:
    virtual vector<vector<int>>& getPredicate() = 0;
    virtual vector<int>& getColumns() = 0;
    virtual vector<double>& getSelectivities() = 0;

    string toString(){
        vector<vector<int>> predicates = getPredicate();
        vector<int> columns = getColumns();

        string ret = "SelectionQuery(" + std::to_string(columns.size())+")";
        for(int c=0;c<columns.size();c++) {
            //ret+= "\t"+columns[c] +" between "+Util::to_string(predicates[c]);
            ret +="\t";
            ret +=std::to_string(columns[c]);
            ret +=" between ";
            ret +=Util::to_string(predicates[c]);
        }
        return ret;
    }
};


#endif //MY_MCSP_SELECTIONQUERY_H
