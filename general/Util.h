//
// Created by Martin on 23.03.2021.
//

#ifndef MCSP_UTIL_H
#define MCSP_UTIL_H

#include <chrono>
#include <vector>
#include <string>
#include <iostream>
#include <fstream> //file writing


using namespace std;

class Util {
public:
    const static long seed = 12345678;
    /** s=1 */
    const static int NUM_TUPLES_S_ONE = 6000000;//6,000,000
    const static int SF_LINEITEM = NUM_TUPLES_S_ONE;
    const static int SF_ORDERS 		= 1500000;//1,500,000
    const static int SF_CUSTOMER	=  150000;//150,000
    const static int SF_SUPPLIER	=   10000;//10,000
    const static int SF_PART 		=  200000;//200,000
    const static int SF_PARTSUPP	=  800000;//800,000
    const static int MAX_ORDERDATE = 2600;
    const static int NUM_DIM_TPCH = 15;


    static vector<int> getDataTPCHTuple(double s){

        vector<int> tuple(NUM_DIM_TPCH);

        int l_shipdate	=0;
        int l_discount	=1;
        int l_quantity	=2;
        int l_linestatus=3;
        int l_returnflag=4;
        int l_shipinstr	=5;
        int l_shipmode	=6;
        int l_linenumber=7;
        int l_tax		=8;
        int l_commitdate=9;
        int l_receiptdate=10;
        int l_suppkey	=11;
        int l_partkey	=12;
        int l_orderkey	=13;
        int l_extendedprice=14;

        //l_orderkey
        tuple[l_orderkey] = rand()%(int)(s*SF_ORDERS);
        //l_partkey
        tuple[l_partkey] = rand()%(int)(s*SF_PART);
        //l_suppkey
        tuple[l_suppkey] = rand()%(int)(s*SF_CUSTOMER);//XXX
        //l_linenumber
        tuple[l_linenumber] = rand()%7;
        //l_quantity
        tuple[l_quantity] = rand()%50;
        //l_extendedprice
        tuple[l_extendedprice] = tuple[l_quantity]*(rand()%21000);
        //l_discount
        tuple[l_discount] = rand()%11;//in %
        //l_tax
        tuple[l_tax] = rand()%9;//in %
        //l_returnflag
        tuple[l_returnflag] = rand()%3;//enum
        //l_linestatus
        tuple[l_linestatus] = rand()%2;//enum
        //l_shipmode
        tuple[l_shipmode] = rand()%7;//enum
        //l_shipinstr
        tuple[l_shipinstr] = rand()%4;//enum
        //dates
        int o_orderdate = rand()%MAX_ORDERDATE;
        //l_shipdate
        tuple[l_shipdate] = o_orderdate+1+(rand()%121);//in %
        //l_commitdate
        tuple[l_commitdate] = o_orderdate+30+(rand()%61);//in %
        //l_receiptdate
        tuple[l_receiptdate] = tuple[l_shipdate]+1+(rand()%30);//in %

        return tuple;//by value
    }

    static void getDataTPCHTuple_lin(vector<int>& all_data, size_t offset, double s){

        size_t l_shipdate	=0;
        size_t l_discount	=1;
        size_t l_quantity	=2;
        size_t l_linestatus=3;
        size_t l_returnflag=4;
        size_t l_shipinstr	=5;
        size_t l_shipmode	=6;
        size_t l_linenumber=7;
        size_t l_tax		=8;
        size_t l_commitdate=9;
        size_t l_receiptdate=10;
        size_t l_suppkey	=11;
        size_t l_partkey	=12;
        size_t l_orderkey	=13;
        size_t l_extendedprice=14;

        //l_orderkey
        all_data[offset+l_orderkey] = rand()%(int)(s*SF_ORDERS);
        //l_partkey
        all_data[offset+l_partkey] = rand()%(int)(s*SF_PART);
        //l_suppkey
        all_data[offset+l_suppkey] = rand()%(int)(s*SF_CUSTOMER);//XXX
        //l_linenumber
        all_data[offset+l_linenumber] = rand()%7;
        //l_quantity
        all_data[offset+l_quantity] = rand()%50;
        //l_extendedprice
        all_data[offset+l_extendedprice] = all_data[offset+l_quantity]*(rand()%21000);
        //l_discount
        all_data[offset+l_discount] = rand()%11;//in %
        //l_tax
        all_data[offset+l_tax] = rand()%9;//in %
        //l_returnflag
        all_data[offset+l_returnflag] = rand()%3;//enum
        //l_linestatus
        all_data[offset+l_linestatus] = rand()%2;//enum
        //l_shipmode
        all_data[offset+l_shipmode] = rand()%7;//enum
        //l_shipinstr
        all_data[offset+l_shipinstr] = rand()%4;//enum
        //dates
        int o_orderdate = rand()%MAX_ORDERDATE;
        //l_shipdate
        all_data[offset+l_shipdate] = o_orderdate+1+(rand()%121);//in %
        //l_commitdate
        all_data[offset+l_commitdate] = o_orderdate+30+(rand()%61);//in %
        //l_receiptdate
        all_data[offset+l_receiptdate] = all_data[offset+l_shipdate]+1+(rand()%30);//in %

    }

    static vector<vector<int>> getDataTPCHTuple_columnar(double s){
        cout << "Util::getDataTPCHTuple_columnar(s="<<s<<")";
        vector<vector<int>> all_data(Util::NUM_DIM_TPCH, vector<int>((int)(Util::NUM_TUPLES_S_ONE*s)));
        getDataTPCHTuple_columnar(all_data, s);
        return all_data;//by value
    }

    static void getDataTPCHTuple_columnar(vector<vector<int>>& all_data, double s){
        size_t l_shipdate	=0;
        size_t l_discount	=1;
        size_t l_quantity	=2;
        size_t l_linestatus=3;
        size_t l_returnflag=4;
        size_t l_shipinstr	=5;
        size_t l_shipmode	=6;
        size_t l_linenumber=7;
        size_t l_tax		=8;
        size_t l_commitdate=9;
        size_t l_receiptdate=10;
        size_t l_suppkey	=11;
        size_t l_partkey	=12;
        size_t l_orderkey	=13;
        size_t l_extendedprice=14;

        cout << "Util::getDataTPCHTuple_columnar(vec<vec<int>>, "<<s<<")";

        auto begin = chrono::system_clock::now();
        uint64_t num_tuples = (int)(NUM_TUPLES_S_ONE * s);
        srand(seed);

        for(uint64_t tid=0;tid<num_tuples;tid++) {
            //l_orderkey
            all_data.at(l_orderkey).at(tid) = rand() % (int) (s * SF_ORDERS);
            //l_partkey
            all_data.at(l_partkey).at(tid) = rand() % (int) (s * SF_PART);
            //l_suppkey
            all_data.at(l_suppkey).at(tid) = rand() % (int) (s * SF_CUSTOMER);//XXX
            //l_linenumber
            all_data.at(l_linenumber).at(tid) = rand() % 7;
            //l_quantity
            all_data.at(l_quantity).at(tid) = rand() % 50;
            //l_extendedprice
            all_data.at(l_extendedprice).at(tid) = all_data.at(l_quantity).at(tid) * (rand() % 21000);
            //l_discount
            all_data.at(l_discount).at(tid) = rand() % 11;//in %
            //l_tax
            all_data.at(l_tax).at(tid) = rand() % 9;//in %
            //l_returnflag
            all_data.at(l_returnflag).at(tid) = rand() % 3;//enum
            //l_linestatus
            all_data.at(l_linestatus).at(tid) = rand() % 2;//enum
            //l_shipmode
            all_data.at(l_shipmode).at(tid) = rand() % 7;//enum
            //l_shipinstr
            all_data.at(l_shipinstr).at(tid) = rand() % 4;//enum
            //dates
            int o_orderdate = rand() % MAX_ORDERDATE;
            //l_shipdate
            all_data.at(l_shipdate).at(tid) = o_orderdate + 1 + (rand() % 121);//in %
            //l_commitdate
            all_data.at(l_commitdate).at(tid) = o_orderdate + 30 + (rand() % 61);//in %
            //l_receiptdate
            all_data.at(l_receiptdate).at(tid) = all_data.at(l_shipdate).at(tid) + 1 + (rand() % 30);//in %
        }
        auto end = chrono::system_clock::now();
        cout << " Done in "<<chrono::duration_cast<chrono::milliseconds>(end - begin).count() << " |D|=" << all_data.at(0).size() << endl;
    }

    static void getDataTPCHTuple_lin_columnar(vector<int>& all_data, size_t tid, vector<size_t> column_offsets, double s){

        size_t l_shipdate	=0;
        size_t l_discount	=1;
        size_t l_quantity	=2;
        size_t l_linestatus=3;
        size_t l_returnflag=4;
        size_t l_shipinstr	=5;
        size_t l_shipmode	=6;
        size_t l_linenumber=7;
        size_t l_tax		=8;
        size_t l_commitdate=9;
        size_t l_receiptdate=10;
        size_t l_suppkey	=11;
        size_t l_partkey	=12;
        size_t l_orderkey	=13;
        size_t l_extendedprice=14;

        //l_orderkey
        all_data[column_offsets[l_orderkey]+tid] = rand()%(int)(s*SF_ORDERS);
        //l_partkey
        all_data[column_offsets[l_partkey]+tid] = rand()%(int)(s*SF_PART);
        //l_suppkey
        all_data[column_offsets[l_suppkey]+tid] = rand()%(int)(s*SF_CUSTOMER);//XXX
        //l_linenumber
        all_data[column_offsets[l_linenumber]+tid] = rand()%7;
        //l_quantity
        all_data[column_offsets[l_quantity]+tid] = rand()%50;
        //l_extendedprice
        all_data[column_offsets[l_extendedprice]+tid] = all_data[column_offsets[l_quantity]+tid]*(rand()%21000);
        //l_discount
        all_data[column_offsets[l_discount]+tid] = rand()%11;//in %
        //l_tax
        all_data[column_offsets[l_tax]+tid] = rand()%9;//in %
        //l_returnflag
        all_data[column_offsets[l_returnflag]+tid] = rand()%3;//enum
        //l_linestatus
        all_data[column_offsets[l_linestatus]+tid] = rand()%2;//enum
        //l_shipmode
        all_data[column_offsets[l_shipmode]+tid] = rand()%7;//enum
        //l_shipinstr
        all_data[column_offsets[l_shipinstr]+tid] = rand()%4;//enum
        //dates
        int o_orderdate = rand()%MAX_ORDERDATE;
        //l_shipdate
        all_data[column_offsets[l_shipdate]+tid] = o_orderdate+1+(rand()%121);//in %
        //l_commitdate
        all_data[column_offsets[l_commitdate]+tid] = o_orderdate+30+(rand()%61);//in %
        //l_receiptdate
        all_data[column_offsets[l_receiptdate]+tid] = all_data[column_offsets[l_shipdate]+tid]+1+(rand()%30);//in %

    }
    static string to_string(vector<int64_t> vec){
        string str;
        str.append("(");
        for(int s : vec){
            str.append(std::__cxx11::to_string(s)+", ");
        }
        str.replace(str.length()-2,1,")");
        return str;
    }
    static string to_string(vector<vector<int64_t>> vec){
        string str;
        str.append("(");
        for(vector<int64_t>& v : vec){
            str.append(to_string(v)+", ");
        }
        str.replace(str.length()-2,1,")");
        return str;
    }
    static string to_string(vector<int> vec){
        string str;
        str.append("(");
        for(int s : vec){
            str.append(std::__cxx11::to_string(s)+", ");
        }
        str.replace(str.length()-2,1,")");
        return str;
    }
    static string to_string(vector<vector<int>> vec){
        string str;
        str.append("(");
        for(vector<int>& v : vec){
            str.append(to_string(v)+", ");
        }
        str.replace(str.length()-2,1,")");
        return str;
    }
    static string to_string(vector<double> vec){
        string str;
        str.append("(");
        for(int s : vec){
            str.append(std::__cxx11::to_string(s)+", ");
        }
        str.replace(str.length()-2,1,")");
        return str;
    }

    static void append(string& str, vector<string>& to_append){
        str.append("(");
        for(auto& s : to_append){
            str.append(s+", ");
        }
        str.replace(str.length()-2,1,")");
    }

    static vector<string> get_tpch_linetime_column_names(){
        vector<string> TPC_H_LINEITEM_COLUMN_NAMES = {
                "l_shipdate"
                ,"l_discount"
                ,"l_quantity"
                ,"l_linestatus"
                ,"l_returnflag"
                ,"l_shipinstr"
                ,"l_shipmode"
                ,"l_linenumber"
                ,"l_tax"
                ,"l_commitdate"
                ,"l_receiptdate"
                ,"l_suppkey"
                ,"l_partkey"
                ,"l_orderkey"
                ,"l_extendedprice"
        };
        return TPC_H_LINEITEM_COLUMN_NAMES;
    }

    static vector<vector<int>> getDataTPCH(double s){
        cout << "Util::getDataTPCH("<<s<<")";
        auto begin = chrono::system_clock::now();
        int size = (int)(NUM_TUPLES_S_ONE*s);
        int dim  = NUM_DIM_TPCH;
        srand(seed);
        vector<vector<int>> data(size);
        for(int i=0;i<size;i++) {
            data[i]=getDataTPCHTuple(s);
            if(i%1000000==0){
                cout << i << " ";
            }
        }
        auto end = chrono::system_clock::now();
        cout << "Done in "<<chrono::duration_cast<chrono::milliseconds>(end - begin).count() << endl;
        return data;
    }

    static vector<int>* getDataTPCH_rowise(double s){
        cout << "Util::getDataTPCH_rowise("<<s<<")";
        auto begin = chrono::system_clock::now();
        uint64_t num_tuples = (int)(NUM_TUPLES_S_ONE * s);
        srand(seed);

        size_t vec_size = (size_t)num_tuples*(size_t)NUM_DIM_TPCH;
        std::vector<int> v;
        std::cout << v.max_size() << " vs " << vec_size << " vs "<<(2^32)/sizeof(int) - 1 << endl;

        vector<int>* data = new vector<int>(vec_size);//allocate on heap
        for(uint64_t i=0; i < num_tuples; i++) {
            size_t offset = i*NUM_DIM_TPCH;
            getDataTPCHTuple_lin(*data,offset,s);
            if(i%1000000==0){
                cout << i << " ";
            }
        }
        auto end = chrono::system_clock::now();
        cout << "Done in "<<chrono::duration_cast<chrono::milliseconds>(end - begin).count() << endl;
        return data;
    }

    static vector<size_t> get_offsets(const size_t total_size, const size_t num_columns) {
        size_t size_per_column = total_size / num_columns;
        vector<size_t> ret(num_columns);
        for(size_t col=0;col<num_columns;col++){
            ret[col] = col*size_per_column;
        }
        return ret;
    }

    static vector<int>* getDataTPCH_columnar(double s){
        cout << "Util::getDataTPCH_columnar("<<s<<")";
        auto begin = chrono::system_clock::now();
        uint64_t num_tuples = (int)(NUM_TUPLES_S_ONE * s);
        srand(seed);

        size_t vec_size = (size_t)num_tuples*(size_t)NUM_DIM_TPCH;
        std::vector<int> v;
        std::cout << v.max_size() << " vs " << vec_size << " vs "<<(2^32)/sizeof(int) - 1 << endl;

        vector<size_t> offsets = get_offsets(vec_size, NUM_DIM_TPCH);

        vector<int>* data = new vector<int>(vec_size);//allocate on heap
        for(uint64_t i=0; i < num_tuples; i++) {
            getDataTPCHTuple_lin_columnar(*data,i,offsets,s);
            if(i%1000000==0){
                cout << i << " ";
            }
        }
        auto end = chrono::system_clock::now();
        cout << "Done in "<<chrono::duration_cast<chrono::milliseconds>(end - begin).count() << endl;
        return data;
    }


    static inline bool isIn(const int value, const int lower, const int upper){
        return (lower<=value && value<=upper);// lower <= value <= upper
    }
    static bool isIn(const int id_to_search, const vector<int>& in_list){
        for(int id : in_list){
            if(id_to_search==id){
                return true;
            }
        }
        return false;
    }
    static inline bool isIn(const vector<int>& tuple, const vector<int>& colum_indexes, const vector<vector<int>>& predictates){
        const int num_predicates = colum_indexes.size();
        for(int p=0;p<num_predicates;p++){
            const int column_index = colum_indexes[p];
            if(!isIn(tuple[column_index], predictates[p][0],predictates[p][1])){
                return false;
            }
        }
        return true;
    }
    static bool isIn(const vector<int>& tuples, const size_t offset, const vector<int>& colum_indexes, const vector<vector<int>>& predictates){
        const int num_predicates = colum_indexes.size();
        for(int p=0;p<num_predicates;p++){
            const size_t column_index = colum_indexes[p];
            int val = tuples[offset+column_index];
            if(!isIn(val, predictates[p][0],predictates[p][1])){
                return false;
            }
        }
        return true;
    }

    static const int32_t NOT_FOUND = -1;

    static vector<int> cardinalitiesTPCH(double scale){
        vector<int> cardinalities = {//default cardinalities for s=1.0
                2720
                ,11
                ,50
                ,2
                ,3
                ,4
                ,7
                ,6
                ,8
                ,2689
                ,2750
                ,SF_CUSTOMER
                ,SF_PART
                ,SF_ORDERS
                ,SF_LINEITEM
        };

        // change scale-factor-dependned values
        cardinalities[11] = (int)((double)cardinalities[11]*scale);
        cardinalities[12] = (int)((double)cardinalities[12]*scale);
        cardinalities[13] = (int)((double)cardinalities[13]*scale);
        cardinalities[14] = (int)((double)cardinalities[14]*scale);
        return cardinalities;
    }

    static bool write_file(const string& path, vector<int>& to_write){
        std::ofstream out(path, std::ios_base::binary);
        //out.write(reinterpret_cast<char*>(&size), sizeof(size));
        out.write(reinterpret_cast<char*>(to_write.data()), to_write.size()*sizeof(int));
        /*std::ofstream outFile(path, ios::out | ios::binary);

        // the important part
        char* p = &to_write[0];
        outFile.write(p, to_write.size() * sizeof(to_write));
        //for (const auto &e : to_write) outFile << e << "\n";
         */
        out.close();
        return true;
    }
    static bool write_file(const string& path, vector<uint64_t>& to_write){
        std::ofstream out(path, std::ios_base::binary);
        //out.write(reinterpret_cast<char*>(&size), sizeof(size));
        out.write(reinterpret_cast<char*>(to_write.data()), to_write.size()*sizeof(uint64_t));
        /*std::ofstream outFile(path, ios::out | ios::binary);

        // the important part
        char* p = &to_write[0];
        outFile.write(p, to_write.size() * sizeof(to_write));
        //for (const auto &e : to_write) outFile << e << "\n";
         */
        out.close();
        return true;
    }
    /**
     *
     * @param path
     * @param to_write - columnar or rowise table
     * @return
     */
    static bool write_file(string& path, vector<vector<int>>& to_write){
        cout << "write_file(string,vector<vector<int>>&) writing to " << path << " vec_size="<<to_write.size()<< " vec_size.at(0)=" << to_write.at(0).size() << " ";
        std::ofstream out(path, std::ios_base::binary);
        //out.write(reinterpret_cast<char*>(&size), sizeof(size));
        for(auto& v : to_write){
            size_t size = v.size();
            size*=sizeof(int);
            out.write(reinterpret_cast<char*>(v.data()), size);
        }
        out.close();
        cout << " [Done]" << endl;
        return true;
    }

    static vector<int> read_file(const string& path){
        // open the file:
        std::streampos fileSize;
        std::ifstream file(path, std::ios::binary);

        // get its size:
        file.seekg(0, std::ios::end);
        fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        // read the data:
        cout << "file size=" << fileSize << endl;
        std::vector<int> fileData(fileSize/sizeof(int));
        file.read((char*) &fileData[0], fileSize);
        return fileData;
    }
    static vector<uint64_t> read_file_uint64_t(const string& path){
        // open the file:
        std::streampos fileSize;
        std::ifstream file(path, std::ios::binary);

        // get its size:
        file.seekg(0, std::ios::end);
        fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        // read the data:
        cout << "file size=" << fileSize << endl;
        std::vector<uint64_t> fileData(fileSize/sizeof(uint64_t));
        file.read((char*) &fileData[0], fileSize);
        return fileData;
    }
    static vector<vector<int>> read_columnar_table(const string& path, const uint64_t num_columns, const uint64_t num_tuples){
        cout << "read_columnar_table() path=" << path << " num_columns=" << num_columns << " num_tuples=" << num_tuples;
        vector<vector<int>> table_data;
        // open the file:
        vector<int> raw_data = read_file(path);
        if(raw_data.size()!=num_columns*num_tuples){
            cout << "Error read_columnar_table() raw_data.size()!=num_columns*num_tuples" << endl;
        }
        for(uint64_t c=0;c<num_columns;c++){
            vector<int> temp_column(num_tuples);
            uint64_t start_offset = num_tuples*c;
            uint64_t stop_offset  = num_tuples*(c+1);
            for(uint64_t i=0;i<num_tuples;i++){
                temp_column.at(i) = raw_data.at(start_offset+i);
            }
            /*vector<int>::const_iterator first = raw_data.begin() + num_tuples*c;
            vector<int>::const_iterator last  = raw_data.begin() + num_tuples*(c+1);
            vector<int> temp_column(first,last);*/
            table_data.push_back(temp_column);
        }
        cout << "[Done]" << endl;
        return table_data;
    }
    static vector<vector<int>> read_rowise_table(const string& path, const int num_columns, const int num_tuples) {
        vector<vector<int>> table_data;
        // open the file:
        vector<int> raw_data = read_file(path);
        if (raw_data.size() != num_columns * num_tuples) {
            cout << "Error read_columnar_table() raw_data.size()!=num_columns*num_tuples" << endl;
        }
        for (int tid = 0; tid < num_tuples; tid++) {
            vector<int>::const_iterator first = raw_data.begin() + (tid * num_columns);
            vector<int>::const_iterator last = raw_data.begin() + ((tid + 1) * num_columns);
            vector<int> temp_column(first, last);
            table_data.push_back(temp_column);
        }
        return table_data;
    }
};

#endif //MCSP_UTIL_H
