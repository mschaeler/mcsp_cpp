//
// Created by Martin on 23.03.2021.
//

#ifndef MCSP_ROWTABLE_H
#define MCSP_ROWTABLE_H


#include <sstream>
#include <vector>

class RowTable : public Table{

public:
    const vector<vector<int>> tuples;

    RowTable(string& name, vector<string>& col_names, vector<vector<int>> data)
    : Table(name, col_names)
    , tuples(data)
    {

    }

    RowTable(double s)
    : Table("lineitem", Util::get_tpch_linetime_column_names())
    , tuples(Util::getDataTPCH(s))
    {

    }

    uint64_t size() {return tuples.size();}

    string out(){
        std::stringstream ss;
        ss << Table::out();

        for(int tid=0;tid<5;tid++){
            vector<int> vec = tuples[tid];
            ss << endl;
            for(size_t i = 0; i < vec.size(); ++i)
            {
                if(i != 0)
                    ss << ",";
                ss << vec[i];
            }
        }
        return ss.str();
    }
};


#endif //MCSP_ROWTABLE_H
