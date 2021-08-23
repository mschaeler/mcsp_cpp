//
// Created by Martin on 25.03.2021.
//

#ifndef MY_MCSP_COLTABLE_2_H
#define MY_MCSP_COLTABLE_2_H


class ColTable_2 : public Table{
public:
    const int num_dim;
    const int my_size;
    const vector<int>& columns;

    RowTable_2(string& name, vector<string>& col_names, vector<int>& all_data) :
    Table(name, col_names), tuples(all_data), num_dim(col_names.size()), my_size(all_data.size() / col_names.size()) {
        //Nothing to do
    }

    ColTable(string& name, vector<string>& col_names,  vector<int>& all_data) :
    Table(name, col_names), columns(all_data), num_dim(col_names.size()), my_size(all_data.size() / col_names.size()){
        for (int i=0; i<col_data.size(); i++){
            vector<int> to_copy = col_data[i];
            vector<int> copy (to_copy);//should physically copy the vector
            columns[i] = copy;
        }
    }

    int size() {return my_size;}

    string out(){
        std::stringstream ss;
        ss << Table::out();

        for(int tid=0;tid<5;tid++){
            ss << endl;
            for(int i = 0; i < columns.size(); ++i)
            {
                if(i != 0)
                    ss << ",";
                ss << columns[i][tid];
            }
        }
        return ss.str();
    }
};


#endif //MY_MCSP_COLTABLE_2_H
