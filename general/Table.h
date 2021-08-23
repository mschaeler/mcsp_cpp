//
// Created by Martin on 23.03.2021.
//

#ifndef MCSP_TABLE_H
#define MCSP_TABLE_H

#include <vector>

#include "Util.h"
#include "Config.h"
#include <string>

using namespace std;

extern int64_t read_cost; // declaration (not definition)
extern int64_t write_cost; // declaration (not definition)

class Table {

public:
    /**** Begin Cost Logging stuff ***********/
    static const bool LOG_COST = Config::LOG_COST;

    void reset_cost(){
        write_cost=0;
        read_cost=0;
    }
    /**** End Cost Logging stuff ***********/

    string my_name;
    vector<string> column_names;
    Table(string name, vector<string> col_names) : my_name(name), column_names(col_names)
    {
        //nothing else todo
    }

    virtual int size() = 0;

    string out(){
        string str;
        str.append(my_name);
        Util::append(str, column_names);

        str.append(" size=");
        str.append(to_string(size()));
        return str;
    }
};


#endif //MCSP_TABLE_H
