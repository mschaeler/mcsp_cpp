//
// Created by Martin on 23.03.2021.
//

#ifndef MY_MCSP_COLTABLE_H
#define MY_MCSP_COLTABLE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <numeric> //accumulate
class ColTable : public Table{
public:
    const vector<vector<int>> columns;
    const int num_dim;

    ColTable(string& name, vector<string>& col_names, vector<vector<int>> col_data)
    : Table(name, col_names)
    , columns(col_data)
    , num_dim(col_data.size())
    {

    }

    ColTable(double s)
    : Table("lineitem", Util::get_tpch_linetime_column_names())
    , num_dim(Util::NUM_DIM_TPCH)
    //, columns(Util::NUM_DIM_TPCH, vector<int>((int)(Util::NUM_TUPLES_S_ONE*s)) )
    , columns(Util::getDataTPCHTuple_columnar(s))
    {
        /*//Util::getDataTPCHTuple_columnar(columns, s);
        if(exists(s)){
            vector<vector<int>> raw_columns = Util::read_columnar_table(get_file(s), num_dim, size());
            for(int c=0; c < num_dim; c++){
                vector<int>& raw_colum  = raw_columns.at(c);
                vector<int>& colum      = columns.at(c);
                for(int tid=0;tid<size();tid++){
                    colum.at(tid) = raw_colum.at(tid);
                }
            }
        }else{
            Util::getDataTPCHTuple_columnar(columns, s);
            if(Config::MATERIALIZE_DATA){
                materialize(*this, s);
            }
        }*/
    }

    /*ColTable(RowTable& r)
    : Table(r.my_name, r.column_names)
    , columns(r.tuples[0].size())
    {
        int size = r.size();
        for(int col=0;col<columns.size();col++){
            vector<int> empty_column(size);
            columns[col] = empty_column;
        }
        for(int tid=0;tid<size;tid++){
            vector<int> tuple = r.tuples[tid];
            for(int col=0;col<columns.size();col++){
                columns[col][tid] = tuple[col];
            }
        }
    }*/

    int size() {return columns[0].size();}

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

    void copyPoint(const int tid, vector<int>& buffer){
        if(buffer.capacity()!=num_dim){
            cout << "copyPoint() invalid buffer" << endl;
        }
        for(int col=0;col<num_dim;col++){
            buffer.at(col) = columns.at(col).at(tid);
        }
    }

    /*static bool materialize(ColTable& t, const double scale){
        if(!exists(scale)){
            //create folder
            string file = get_file(scale);
            cout << "materialize(db,double): Materializing s=" << scale << " to "+file << endl;
            Util::write_file(file, t.columns);
            return true;
        }
        return false;
    }*/

    static bool exists(double scale) {
        return false;
        string file = get_file(scale);
        ifstream f(file.c_str());
        return f.good();
    }
    /**
	 * Ensures that naming convention is respected
	 * @param scale
	 * @return
	 */
    static string get_file(double scale){
        //string p = "..//data/my_file.txt";
        string p = "..//data/"+to_string(scale)+"_columnar.tbl";
        return p;
        //return ".\\data\\" +to_string(scale)+"\\columnar";
        //return ".."+separator()+"elf_dbms_lab"+"."+separator()+"data"+separator()+scale+separator()+"columnar";
    }
    static inline char separator()
    {
#ifdef _WIN32
        return '\\';
#else
        return '/';
#endif
    }

    static bool equals(ColTable& t_1, ColTable& t_2){
        cout << "equals(ColTable&, ColTable&)" << endl;
        bool is_equal = true;//we are optimistic
        //compare name
        if(t_1.my_name.compare(t_2.my_name)!=0){
            cout << "Name differs " << t_1.my_name << " vs. " << t_2.my_name << endl;
        }

        if(t_1.num_dim!=t_2.num_dim){
            cout << "t_1.num_dim!=t_2.num_dim" << endl;
            return false;
        }
        int num_columns = t_1.num_dim;
        cout << "num_columns=" << num_columns << endl;
        for(int col=0;col<num_columns;col++){
            const vector<int>& col_t_1 = t_1.columns.at(col);
            const vector<int>& col_t_2 = t_2.columns.at(col);
            if(col_t_1.size()!=col_t_2.size()){
                cout << "col_t_1.size()!=col_t_2.size()" << endl;
                is_equal = false;
                continue;//next column without data comparison
            }
            int num_tuples = col_t_1.size();
            uint64_t sum_of_elems = std::accumulate(col_t_1.begin(), col_t_1.end(), 0);
            cout <<"col="<<col<< " num_tuples=" << num_tuples << " sum=" << sum_of_elems << endl;
            for(int tid=0;tid<num_tuples;tid++){
                int val_1 = col_t_1.at(tid);
                int val_2 = col_t_2.at(tid);
                if(val_1!=val_2){
                    cout <<"column="<<col<<" tid="<<tid<< " val_1!=val_2 " << val_1 << " vs. " << val_2 <<  endl;
                    is_equal = false;
                    break;//next column, i.e., end data compariosn at first error
                }
            }
        }
        cout << "equals(ColTable&, ColTable&) [DONE]"<< endl;
        return is_equal;
    }

};


#endif //MY_MCSP_COLTABLE_H
