//
// Created by Martin on 23.03.2021.
//

#ifndef MCSP_ROWTABLE_H
#define MCSP_ROWTABLE_H


#include <sstream>
#include <vector>

class RowTable : public Table{

public:
    vector<vector<int>> tuples;
    RowTable(string& name, vector<string>& col_names, vector<vector<int>> data) : Table(name, col_names), tuples(data.size()){
        for (int i=0; i<data.size(); i++){
            vector<int> to_copy = data[i];
            vector<int> copy (to_copy);//should physically copy the vector
            tuples[i] = copy;
        }
    }

    RowTable(double s) : Table("lineitem", Util::get_tpch_linetime_column_names()){
        cout << "RowTable lineitem constructor("<<s<<")";
        auto begin = chrono::system_clock::now();
        int size = (int)(Util::NUM_TUPLES_S_ONE*s);
        int dim  = Util::NUM_DIM_TPCH;
        srand(Util::seed);
        vector<vector<int>> data(size);
        for(int i=0;i<size;i++) {
            vector<int> tuple = Util::getDataTPCHTuple(s);
            tuples.push_back(tuple);
            if(i%1000000==0){
                cout << i << " ";
            }
        }
        auto end = chrono::system_clock::now();
        cout << "Done in "<<chrono::duration_cast<chrono::milliseconds>(end - begin).count() << endl;
    }

    int size() {return tuples.size();}

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
