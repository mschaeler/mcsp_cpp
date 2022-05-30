//
// Created by Martin on 23.03.2021.
//

#ifndef MCSP_DATABASESYSTEM_H
#define MCSP_DATABASESYSTEM_H

#include <memory>
#include "Table.h"
#include "Synopsis.h"

extern int64_t read_cost; // declaration (not definition)
extern int64_t write_cost; // declaration (not definition)

class DatabaseSystem {
public:
    /**** Begin Cost Logging stuff ***********/
    static const bool LOG_COST = Config::LOG_COST;

    static const int LOWER = 0;
    static const int UPPER = 1;

    virtual Synopsis& select(Table& t, vector<int>& column_indexes, vector<vector<int>>& predicates, vector<double>& selectivities) =0 ;
    /**
     * For backwards compatibility.
     * @param t
     * @param column_indexes
     * @param predicates
     * @return
     */
    Synopsis& select(Table& t, vector<int>& column_indexes, vector<vector<int>>& predicates){
        vector<double> dummy_selectivities(column_indexes.size(),-1);
        return select(t, column_indexes, predicates, dummy_selectivities);
    }
    virtual string name() = 0;
    virtual  std::unique_ptr<Table> get_TPC_H_lineitem(double scale) = 0;

    virtual void clear() {
    }

};


#endif //MCSP_DATABASESYSTEM_H
