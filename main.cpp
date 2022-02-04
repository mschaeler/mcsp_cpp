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
#include "elf/Elf_Dbms_Lvl_Ranges.h"
#include "elf_cutoff_external/Elf_Table_Cutoff_External.h"
#include "elf_cutoff_external/Elf_Dbms_Lvl_Cutoffs_External.h"
#include "elf_cutoff_external/Elf_Dbms_Lvl_Ranges_External.h"

void run_p_benchmark(double scale, vector<DatabaseSystem*> all_dbms){
    DatabaseSystem* dbms;
    Table* t;
    int num_queries_per_set = 100;
    int num_query_sets = 20;
    SelectionTests experiment(scale, num_query_sets, num_queries_per_set);
    experiment.p_benchmark(all_dbms, false);
}

int64_t read_cost  = 0;
int64_t write_cost = 0;

void test_something(double scale){
    cout << "test_something(double scale)" << scale << endl;


    ColTable t(scale);

    Elf_Table_Cutoff_External* table = Elf_builder_separate::build_with_external_cuttoffs(t);


    /*string name = "lineitem";
    vector<string> col_names = Util::get_tpch_linetime_column_names();
    vector<vector<int>> data(Util::NUM_DIM_TPCH, vector<int>((int)(Util::NUM_TUPLES_S_ONE*scale)));
    Util::getDataTPCHTuple_columnar(data, scale);
    ColTable t_constructed(name, col_names, data);

    ColTable::equals(t, t_constructed);
    //load col table from file
    //create col table
    //compare it
     */
}

int main() {

    vector<int> dummy;
    cout << "P-Benchmark suite! I am running on 64 bit if 4611686018427387903 == " << dummy.max_size()  << std::endl;
    cout << "SelectionQuerySet::UNIFORM_COLUMN_PROBABILIY=" << SelectionQuerySet::UNIFORM_COLUMN_PROBABILIY <<endl;//just to ensure that it is inclduded
    cout << "Config::LOG_COST=" << Config::LOG_COST << " Elf pointer size= " <<sizeof(elf_pointer) << endl;
    double scale = 30;
    cout << "scale="<<scale<<endl;
    //test_something(0.1);

    //SelectionTests::check_mcsp_queries(scale, new Elf_Dbms_Lvl_Ranges_External());

    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl_Cutoffs()};
    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl_Cutoffs(), new Elf_Dbms_Lvl_Cutoffs_External(), new Elf_Dbms_Lvl_Ranges(), new Elf_Dbms_Lvl_Ranges_External(), new MyHyper(), new MyMonetDB(), new MyMonetDB_Indexed(),new MyRowiseHyper()};
    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl(), new Elf_Dbms_Lvl_Cutoffs(), new Elf_Dbms_Lvl_Cutoffs_External(), new Elf_Dbms_Lvl_Ranges(), new Elf_Dbms_Lvl_Ranges_External()};
    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl(), new Elf_Dbms_Lvl_Cutoffs_External(), new Elf_Dbms_Lvl_Ranges_External(), new MyHyper()};
    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl_Cutoffs(), new MyRowiseHyper(), new MyMonetDB()};
    //vector<DatabaseSystem*> all_dbms = {new MyHyper};
    //vector<DatabaseSystem*> all_dbms = {new MyMonetDB_Indexed()};
    //vector<DatabaseSystem*> all_dbms = {new MyHyper()};
    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl_Cutoffs_External(), new Elf_Dbms_Lvl_Ranges_External()};
    vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl(), new Elf_Dbms_Lvl_Ranges_External(), new MyMonetDB(), new MyHyper(), new MyMonetDB_Indexed(),new MyRowiseHyper() };
    //vector<DatabaseSystem*> all_dbms = {new MyMonetDB_Indexed(), new Elf_Dbms_Lvl_Ranges_External()};
    //SelectionTests::run_mono_column_benchmark(all_dbms, scale , 100, false);
    run_p_benchmark(scale, all_dbms);

    std::cout << "Bye, Bye!" << std::endl;
    return 0;
}

