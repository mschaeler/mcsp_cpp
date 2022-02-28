#include <iostream>

#include "general/Table.h"
#include "rowise/RowTable.h"
#include "rowise/MyRowiseHyper.h"
#include "columnar/MyMonetDB.h"
#include "columnar/MyMonetDB_II.h"
#include "columnar/SortedProjection.h"
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

void run_p_benchmark(double scale, vector<DatabaseSystem*> all_dbms, int max_num_columns){
    DatabaseSystem* dbms;
    Table* t;
    int num_queries_per_set = 100;
    int num_query_sets = 20;
    SelectionTests experiment(scale, num_query_sets, num_queries_per_set, max_num_columns);
    experiment.p_benchmark(all_dbms, false);
}

void run_p_1_benchmark(vector<DatabaseSystem*> all_dbms, int max_num_columns){
    DatabaseSystem* dbms;
    Table* t;
    int num_queries_per_set = 100;
    int num_query_sets = 20;
    double p = 1.0;
    vector<double> scales = {0.1,1,10,20,30,40,50};
    for(double scale : scales) {
        cout << "***scale=" << scale << endl;
        SelectionTests experiment(scale, num_query_sets, num_queries_per_set, max_num_columns, p);
        experiment.p_benchmark(all_dbms, false);
    }
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

int main(int argc, char* argv[]) {
    double scale = 0.01;
    int max_num_columns = 4;

    vector<int> dummy;
    cout << "P-Benchmark suite! I am running on 64 bit if 4611686018427387903 == " << dummy.max_size()  << std::endl;
    if(argc>1){
        cout << "Command line args="<< argc << ": " << endl;
        for(int i=0;i<argc;i++){
            cout << "argv[" << i << "] " << argv[i] << endl;
            if(std::string(argv[i]) == "--s"){
                cout << "Found --s at i=" << i << endl;
                if (i + 1 < argc) { // Make sure we aren't at the end of argv!
                    std::string to_parse(argv[++i]);
                    std::stringstream str(to_parse);
                    double temp;
                    str >> temp;
                    if (!str){
                        cout << "The conversion failed: " << to_parse << endl;
                    }else{
                        cout << "temp=" << temp << endl;
                        scale = temp;
                    }
                } else { // Uh-oh, there was no argument to the destination option.
                    cout << "--destination option requires one argument." << std::endl;
                    return 1;
                }
            }
        }
    }

    cout << "SelectionQuerySet::UNIFORM_COLUMN_PROBABILIY=" << SelectionQuerySet::UNIFORM_COLUMN_PROBABILIY <<endl;//just to ensure that it is inclduded
    cout << "Config::LOG_COST=" << Config::LOG_COST << " Elf pointer size= " <<sizeof(elf_pointer) << endl;

    cout << "scale="<<scale<<endl;
    //test_something(0.1);
    auto elf_dbms = new Elf_Dbms_Lvl();
    elf_dbms->get_TPC_H_lineitem(scale);

    //SelectionTests::check_mcsp_queries(scale, new Elf_Dbms_Lvl());

    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl_Cutoffs()};
    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl_Cutoffs(), new Elf_Dbms_Lvl_Cutoffs_External(), new Elf_Dbms_Lvl_Ranges(), new Elf_Dbms_Lvl_Ranges_External(), new MyHyper(), new MyMonetDB(), new MyMonetDB_Indexed(),new MyRowiseHyper()};
    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl(), new Elf_Dbms_Lvl_Cutoffs(), new Elf_Dbms_Lvl_Cutoffs_External(), new Elf_Dbms_Lvl_Ranges(), new Elf_Dbms_Lvl_Ranges_External()};
    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl(), new Elf_Dbms_Lvl_Cutoffs_External(), new Elf_Dbms_Lvl_Ranges_External(), new MyHyper()};
    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl_Cutoffs(), new MyRowiseHyper(), new MyMonetDB()};
    //vector<DatabaseSystem*> all_dbms = {new SortedProjectionDBMS(), new MyMonetDB_Indexed()};
    //vector<DatabaseSystem*> all_dbms = {new MyMonetDB_Indexed()};
    //vector<DatabaseSystem*> all_dbms = {new MyHyper()};
    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl_Ranges_External(),new MyMonetDB_Indexed()};
    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl(), new Elf_Dbms_Lvl_Ranges_External(), new MyMonetDB(), new MyHyper(), new MyMonetDB_Indexed(),new MyRowiseHyper() };
    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl(), new Elf_Dbms_Lvl_Ranges_External(),new MyMonetDB_Indexed()};
    vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl(),new MyMonetDB_Indexed()};
    //SelectionTests::run_mono_column_benchmark(all_dbms, scale , 100, true);
    //run_p_benchmark(scale, all_dbms,max_num_columns);
    //run_p_1_benchmark(all_dbms,max_num_columns);

    std::cout << "Bye, Bye!" << std::endl;
    return 0;
}

