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

void run_p_1_benchmark(vector<DatabaseSystem*>& all_dbms, int num_columns){
    DatabaseSystem* dbms;
    Table* t;
    int num_queries_per_set = 100;
    int num_query_sets = 20;
    double p = 1.0;
    //vector<double> scales = {80,90,100};
    vector<double> scales = {1,5,10,15};
    //vector<double> scales = {60};
    for(double scale : scales) {
        cout << "***scale=" << scale << endl;
        SelectionTests experiment(scale, num_query_sets, num_queries_per_set, num_columns, p);
        experiment.p_benchmark(all_dbms, false);
    }
}

void run_p_1_p_benchmark(DatabaseSystem* dbms){
    int num_queries_per_set = 100;
    int num_query_sets = 20;
    double p = 1.0;
    vector<double> scales = {10,20,30,40,50};
    //vector<double> scales = {1,5,10,15,20};
    //vector<double> scales = {60};

    for(double scale : scales) {
        cout << endl << "**************** scale=" << scale << endl;
        vector<SelectionTests> experiments;
        for(int num_predicates=2; num_predicates < 5; num_predicates++){
            experiments.emplace_back(scale, num_query_sets, num_queries_per_set, num_predicates, p);
        }
        Table& t=*dbms->get_TPC_H_lineitem(scale);

        for(SelectionTests experiment : experiments){
            cout << "**** next #p=" << endl;
            experiment.p_benchmark(dbms, t, false);
        }
        t.~Table();
    }
}

void run_selectivity_experiments(DatabaseSystem* dbms){
    int num_queries_per_set = 100;
    int num_query_sets = 20;
    double p = 1.0;
    double scale = 50;
    int num_predicates = 3;

    vector<double> selectivities = {1,1.0/2.0, 1.0/4.0, 1.0/8.0, 1.0/16.0, 1.0/32.0, 1.0/64.0, 1.0/128.0, 1.0/256.0};
    cout << "run_selectivity_experiments() for sel=" << Util::to_string(selectivities) << " scale=" << scale << " #p=" << num_predicates << endl;

    vector<SelectionTests> experiments;
    for(auto selectivity : selectivities){
        experiments.emplace_back(scale, num_query_sets, num_queries_per_set, num_predicates, p, selectivity);
    }
    Table& t=*dbms->get_TPC_H_lineitem(scale);

    for(SelectionTests experiment : experiments){
        experiment.p_benchmark(dbms, t, false);
    }
    t.~Table();
}
void run_selectivity_experiments(vector<DatabaseSystem*>& all_dbms){
    for(auto dbms : all_dbms){
        run_selectivity_experiments(dbms);
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
    double scale =0.1;
    int num_columns = 3;

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
    cout << "Config::LOG_COST=" << Config::LOG_COST << " Elf pointer size= " <<sizeof(elf_pointer) << " #p=" << num_columns << endl;

    cout << "scale="<<scale<<endl;
    //test_something(0.1);
    //auto elf_dbms = new Elf_Dbms_Lvl();
    //elf_dbms->get_TPC_H_lineitem(scale);

    //SelectionTests::check_mcsp_queries(scale, new Elf_Dbms_Lvl_Ranges_External());

    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl_Cutoffs()};
    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl_Cutoffs(), new Elf_Dbms_Lvl_Cutoffs_External(), new Elf_Dbms_Lvl_Ranges(), new Elf_Dbms_Lvl_Ranges_External(), new MyHyper(), new MyMonetDB(), new MyMonetDB_Indexed(),new MyRowiseHyper()};
    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl(), new Elf_Dbms_Lvl_Cutoffs(), new Elf_Dbms_Lvl_Cutoffs_External(), new Elf_Dbms_Lvl_Ranges(), new Elf_Dbms_Lvl_Ranges_External()};
    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl(), new Elf_Dbms_Lvl_Cutoffs_External(), new Elf_Dbms_Lvl_Ranges_External(), new MyHyper()};
    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl_Cutoffs(), new MyRowiseHyper(), new MyMonetDB()};
    //vector<DatabaseSystem*> all_dbms = {new SortedProjectionDBMS(), new MyMonetDB_Indexed()};
    //vector<DatabaseSystem*> all_dbms = {new MyMonetDB_Indexed()};
    vector<DatabaseSystem*> all_dbms = {new MyMonetDB_Indexed(), new SortedProjectionDBMS()};
    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl_Ranges_External(),new MyMonetDB_Indexed()};
    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl(), new Elf_Dbms_Lvl_Ranges_External(), new MyMonetDB(), new MyHyper(), new MyMonetDB_Indexed(),new MyRowiseHyper(), new SortedProjectionDBMS()};
    //vector<DatabaseSystem*> all_dbms = {new Elf_Dbms_Lvl(), new Elf_Dbms_Lvl_Ranges_External(),new MyMonetDB_Indexed()};
    //vector<DatabaseSystem*> all_dbms = {new MyHyper()};
    //SelectionTests::run_mono_column_benchmark(all_dbms, scale , 100, true);
    //run_p_benchmark(scale, all_dbms,num_columns);
    //run_p_1_benchmark(all_dbms, num_columns);
    //run_p_1_p_benchmark(all_dbms.at(0));
    run_selectivity_experiments(all_dbms);

    std::cout << "Bye, Bye!" << std::endl;
    return 0;
}

