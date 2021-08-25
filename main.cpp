#include <iostream>

#include "general/Table.h"
#include "rowise/RowTable.h"
#include "rowise/MyRowiseHyper.h"
#include "columnar/MyMonetDB.h"
#include "columnar/MyMonetDB_II.h"
#include "elf/Elf_builder_separate.h"
#include "elf/Elf_table_lvl_seperate.h"
#include "benchmark/SelectionQuerySet.h"
#include "benchmark/SelectionTests.h"
#include "columnar/MyHyper.h"
#include "elf/Elf_Dbms_Lvl.h"
#include "elf/Elf_Dbms_Lvl_Cutoffs.h"

void run_p_benchmark(double scale){
    DatabaseSystem* dbms;
    Table* t;
    int num_queries_per_set = 10;
    int num_query_sets = 10;
    SelectionTests experiment(scale, num_query_sets, num_queries_per_set);
    //experiment.check_mcsp(new Elf_Dbms_Lvl());


    //vector<DatabaseSystem*> all_dbms = {new MyRowiseHyper(), new MyMonetDB()};
    //vector<DatabaseSystem*> all_dbms = {new MyHyper(), new MyMonetDB(), new MyMonetDB_II(), new MyRowiseHyper()};
    //vector<DatabaseSystem*> all_dbms = {new MyHyper(), new MyMonetDB(), new Elf_Dbms_Lvl()};
    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl()};
    //vector<DatabaseSystem*> all_dbms = {new MyHyper(), new MyMonetDB()};
    vector<DatabaseSystem*> all_dbms = {new MyHyper()};

    experiment.p_benchmark(all_dbms, false);
}

int64_t read_cost  = 0;
int64_t write_cost = 0;

void test_something(double scale){
    cout << "test_something(double scale)" << scale << endl;
    ColTable t_loaded(scale);

    auto* elf = Elf_builder_separate::build_with_cuttoffs(t_loaded);

    string name = "lineitem";
    vector<string> col_names = Util::get_tpch_linetime_column_names();
    vector<vector<int>> data(Util::NUM_DIM_TPCH, vector<int>((int)(Util::NUM_TUPLES_S_ONE*scale)));
    Util::getDataTPCHTuple_columnar(data, scale);
    ColTable t_constructed(name, col_names, data);

    ColTable::equals(t_loaded, t_constructed);
    //load col table from file
    //create col table
    //compare it
}

int main() {

    vector<int> dummy;
    cout << "P-Benchmark suite! I am running on 64 bit if 4611686018427387903 == " << dummy.max_size()  << std::endl;
    cout << "SelectionQuerySet::UNIFORM_COLUMN_PROBABILIY=" << SelectionQuerySet::UNIFORM_COLUMN_PROBABILIY <<endl;//just to ensure that it is inclduded
    cout << "Config::LOG_COST=" << Config::LOG_COST << endl;
    double scale = 0.1;
    cout << "scale="<<scale<<endl;
    //test_something(0.1);

    //vector<DatabaseSystem*> all_dbms = {new MyMonetDB()};
    vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl_Cutoffs()};
    //vector<DatabaseSystem*> all_dbms = {new MyHyper(), new MyMonetDB(), new Elf_Dbms_Lvl()};
    SelectionTests::run_mono_column_benchmark(all_dbms, scale , 10, false);

    //run_p_benchmark(scale);

    std::cout << "Bye, Bye!" << std::endl;
    return 0;
}

