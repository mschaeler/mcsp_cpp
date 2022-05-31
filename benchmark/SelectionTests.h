//
// Created by Martin on 22.07.2021.
//

#ifndef MY_MCSP_SELECTIONTESTS_H
#define MY_MCSP_SELECTIONTESTS_H

#include "vector"
#include "../general/DatabaseSystem.h"
#include "SelectionQuerySet.h"
#include "../columnar/MyHyper.h"
#include <algorithm>    // std::sort std::min
#include <memory>
#include <new>

class Error{
public:
    int tid;
    vector<int> point;
    string label;

    Error(int _tid, vector<int> copy_me, string _label) : tid(_tid), point(copy_me), label(_label)
    {

    }

    bool operator < (const Error& arg0) const {
        for (int c = 0; c < point.size(); c++) {
            if (point.at(c) < arg0.point.at(c)) {
                return true;
            } else if (point.at(c) > arg0.point.at(c)) {
                return false;
            }//else take next column
        }
        return false;
    }

    string toString() {
        string s = "tid=";
        s.append(to_string(tid));
        s.append("\t");
        s.append(Util::to_string(point));
        s.append(" label= ");
        s.append(label);
        return s;
    }
};

class ErrorHandler{
public:
    vector<Error> errors;

    void push (int tid, vector<int> point, string label){
        Error e(tid, point, label);
        errors.push_back(e);
    }

    /*void push (int tid, int[] point){
        this.errors.add(this.new Error(tid, point, ""));
    }*/

    void out_in_elf_order(){
        out_in_elf_order(10);
    }

    void out_in_elf_order(int num){
        sort(errors.begin(), errors.end());//using bool operator < (const Error& arg0)
        for(int i=0;i<min(num,(int)errors.size());i++) {
            cout << errors.at(i).toString() << endl;
        }
    }
};

extern int64_t read_cost; // declaration (not definition)
extern int64_t write_cost; // declaration (not definition)

class SelectionTests {
    Random rand;
    /**** Begin Cost Logging stuff ***********/
    static const bool LOG_COST = Config::LOG_COST;

    static  void reset_cost(){
        write_cost=0;
        read_cost=0;
    }

    void p_benchmark(DatabaseSystem* dbms, Table& t, double scale, int num_query_sets, int num_queries, bool repeat) {
        //TPC_H.out_operator = false;
        //TPC_H.out_cost = false;

        double start, stop;
        do {
            cout << dbms->name() << endl;
            for(int i=0;i<all_queries.size();i++) {
                cout << "Starting benchmark for p=" << to_string(p_values[i]) << "\t";
                uint64_t check_sum = 0;
                if(LOG_COST){reset_cost();}
                for(SelectionQuerySet& set : all_queries.at(i)) {
                    //cout << "loop" << endl;
                    auto start = chrono::system_clock::now();
                    for(auto& query : set.myQueries) {
                        //cout << "inner loop" << endl;
                        //System.out.println(query);
                        auto& columns = query.getColumns();
                        auto& predicates = query.getPredicate();
                        auto& selectivities = query.getSelectivities();
                        auto& synopsis = dbms->select(t, columns, predicates,selectivities);
                        check_sum += synopsis.size();
                    }
                    auto stop = chrono::system_clock::now();
                    cout << chrono::duration_cast<chrono::milliseconds>(stop-start).count() << "\t" << flush;
                }
                cout  << "check sum=\t" << check_sum;
                if(LOG_COST){
                    long num_q = num_query_sets*num_queries;
                    long avg_read_cost  = read_cost / num_q;
                    long avg_write_cost = write_cost/ num_q;
                    cout << "\tavg read and write \t" << avg_read_cost << "\t" << avg_write_cost;
                }
                cout << endl;
            }
        }while(repeat);
    }

    void p_benchmark(DatabaseSystem* dbms, double scale, int num_query_sets, int num_queries, bool repeat) {
        Table& t=*dbms->get_TPC_H_lineitem(scale);
        p_benchmark(dbms, t, scale, num_query_sets, num_queries, repeat);
        t.~Table();
    }

    void compare(Synopsis& correct_result, Synopsis& result, Table& t) {
        cout << "compare(Synopsis, Synopsis)";
        ColTable& data = dynamic_cast<ColTable &>(t);
        const int size = correct_result.size();
        if(size!=result.size()) {
            cout << "compare(MyArrayList,MyArrayList) - result sizes wrong: " << size << " (correct) vs. " << result.size() << endl;
        }

        vector<int> tid_array_correct = correct_result.getTrimmedArray();
        sort (tid_array_correct.begin(), tid_array_correct.end());

        vector<int> tid_array_to_verify = result.getTrimmedArray();
        sort (tid_array_to_verify.begin(), tid_array_to_verify.end());

        int pntr_corect = 0;
        int pntr_verify = 0;

        vector<int> point_bufffer(data.num_dim,-1);
        int error = 0;

        //SelectionTests st =  new SelectionTests();
        ErrorHandler e;
        while(pntr_corect < size) {//until explicit break...
            int tid_correct 	= tid_array_correct.at(pntr_corect);
            int tid_to_verify 	= tid_array_to_verify.at(pntr_verify);

            if(tid_correct == tid_to_verify) {
                pntr_corect++;
                pntr_verify++;
            }else if(tid_correct > tid_to_verify) {
                data.copyPoint(tid_to_verify, point_bufffer);

                if(pntr_verify>1 && tid_array_to_verify.at(pntr_verify) == tid_array_to_verify.at(pntr_verify-1)) {
                    if(error<100){
                        cout << (error++) << " Wrong duplicate result tid=" << tid_to_verify << "\t" << Util::to_string(point_bufffer) << endl;
                    }
                    e.push(tid_to_verify, point_bufffer,"duplicate");
                }else {
                    if(error<100){
                        cout << (error++) << " Wrong extra result tid=" << tid_to_verify << "\t" << Util::to_string(point_bufffer) << endl;
                    }
                    e.push(tid_to_verify, point_bufffer,"extra");
                }
                pntr_verify++;
            }else {
                data.copyPoint(tid_correct, point_bufffer);
                if(error<100){
                    cout << (error++) << " Missing result tid=" << tid_correct << "\t" << Util::to_string(point_bufffer) << endl;
                }
                pntr_corect++;
                e.push(tid_correct, point_bufffer,"extra");
            }

            //avoid out of bounds exception
            if(pntr_verify>=tid_array_to_verify.size()) {
                pntr_verify = tid_array_to_verify.size()-1;
            }
        }

        if(pntr_verify < tid_array_to_verify.size()-1) {//wrong extra results
            for(int i=pntr_verify;i<tid_array_to_verify.size();i++) {
                if(error<100){
                    cout << (error++) << " Wrong extra result tid=" << tid_array_to_verify.at(i) << "\t" << Util::to_string(point_bufffer) << endl;
                }
                e.push(tid_array_to_verify[i], point_bufffer,"extra am ende");
            }
        }
        if(e.errors.size()>0) {
            e.out_in_elf_order();
        }

        /*
        int max_tid = correct_result.get(size-1);
        boolean[] temp = new boolean[max_tid+1];
        for(int i=0;i<size;i++) {
            temp[correct_result.get(i)]=true;
        }
        for(int i=0;i<result.size();i++) {
            int tid = result.get(i);
            if(!temp[tid]) {
                System.err.println("tid no reulst "+tid);
            }
        }*/


        /*
        int error = 0;
        for(int r=0;r<size;r++) {
            int tid = correct_result.get(r);
            if(!result.isIn(tid)) {
                System.err.println(error+" Tid not contained "+tid);
                error ++;
            }
        }*/
        cout << "[Done]" << endl;
    }

public:
    //vector<double> p_values = {1.0,1.1,1.2,1.3,1.4,1.5,1.6,1.7,1.8,1.9,2.0};
    vector<double> p_values = {0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0,1.1,1.2,1.3,1.4,1.5,1.6,1.7,1.8,1.9,2.0};
    //vector<double> p_values = {2.0,1.9,1.8,1.7,1.6,1.4,1.3,1.2,1.1,1.0,0.9,0.8,0.7,0.6,0.4,0.3,0.2,0.1};
    vector<double> default_selectivities = {1.0/8};
    /*vector<double> default_selectivities = {
            1,1.0/2.0
            , 1.0/4.0
            , 1.0/8.0
            , 1.0/16.0
            , 1.0/32.0
            , 1.0/64.0
            , 1.0/128.0
            , 1.0/256.0
    };*/
    vector<vector<SelectionQuerySet>> all_queries;// (p_values.size(), vector<SelectionQuerySet*>(num_query_sets));
    double scale;
    int num_query_sets;
    int num_queries;

    /**
     * Mono column constructor
     * @param _scale
     * @param _num_query_sets
     * @param _num_queries
     * @param _max_column
     */
    SelectionTests(double _scale, vector<double> selectivities, int _num_queries, int max_column)
    : scale(_scale)
    , num_query_sets(selectivities.size())
    , num_queries(_num_queries)
    , all_queries(0)
    {
        rand.seed(123);
        for(int column=0; column<max_column;column++) {
            vector<SelectionQuerySet> queries;
            for(int s=0; s<selectivities.size();s++) {
                SelectionQuerySet set(column, selectivities.at(s), scale, num_queries, rand);
                queries.push_back(set);
            }
            all_queries.push_back(queries);
        }
        cout << "Created" << all_queries.size() << " p values" << endl;
        for(vector<SelectionQuerySet> set : all_queries) {
            cout << "Printing statistics for " << set.size() << " query sets" << endl;
            SelectionQuerySet::statisticsQuerySet(set);
        }
    }


    /**
     * Common MCSP constructor
     */
    SelectionTests(double _scale, int _num_query_sets, int _num_queries, int max_num_columns)
    : scale(_scale)
    , num_query_sets(_num_query_sets)
    , num_queries(_num_queries)
    //, all_queries(p_values.size(), vector<SelectionQuerySet*>(num_query_sets))
    {
        rand.seed(123);
        for(int i=0;i<p_values.size();i++) {
            double p = p_values[i];
            vector<SelectionQuerySet> current;// = all_queries.at(i);
            //SelectionQuerySet.PROB_INCREASE_PER_COLUMN = p;
            cout << "Creating queries for p="+ to_string(p) << " max_num_columns=" << max_num_columns << endl;
            for(int set=0;set<num_query_sets;set++) {
                SelectionQuerySet temp (default_selectivities, p, scale, num_queries, max_num_columns, rand);
                current.push_back(temp);
            }
            all_queries.push_back(current);
        }
        cout << "Created" << all_queries.size() << " p values" << endl;
        for(vector<SelectionQuerySet> set : all_queries) {
            cout << "Printing statistics for " << set.size() << " query sets" << endl;
            SelectionQuerySet::statisticsQuerySet(set);
        }
    }

    SelectionTests(double _scale, int _num_query_sets, int _num_queries, int max_num_columns, double p)
            : scale(_scale)
            , num_query_sets(_num_query_sets)
            , num_queries(_num_queries)
    //, all_queries(p_values.size(), vector<SelectionQuerySet*>(num_query_sets))
    {
        rand.seed(123);

        vector<SelectionQuerySet> current;// = all_queries.at(i);
        //SelectionQuerySet.PROB_INCREASE_PER_COLUMN = p;
        cout << "Creating queries for p="+ to_string(p) << " max_num_columns=" << max_num_columns << endl;
        for(int set=0;set<num_query_sets;set++) {
            SelectionQuerySet temp (default_selectivities, p, scale, num_queries, max_num_columns, rand);
            current.push_back(temp);
        }
        all_queries.push_back(current);

        cout << "Created" << all_queries.size() << " p values" << endl;
        for(vector<SelectionQuerySet> set : all_queries) {
            cout << "Printing statistics for " << set.size() << " query sets" << endl;
            SelectionQuerySet::statisticsQuerySet(set);
        }
    }

    SelectionTests(double _scale, int _num_query_sets, int _num_queries, int max_num_columns, double p, double selectivity)
            : scale(_scale)
            , num_query_sets(_num_query_sets)
            , num_queries(_num_queries)
    //, all_queries(p_values.size(), vector<SelectionQuerySet*>(num_query_sets))
    {
        rand.seed(123);

        vector<SelectionQuerySet> current;// = all_queries.at(i);
        //SelectionQuerySet.PROB_INCREASE_PER_COLUMN = p;
        cout << "Creating queries for p="+ to_string(p) << " max_num_columns=" << max_num_columns << endl;
        vector<double> selectivities = {selectivity};

        for(int set=0;set<num_query_sets;set++) {
            SelectionQuerySet temp (selectivities, p, scale, num_queries, max_num_columns, rand);
            current.push_back(temp);
        }
        all_queries.push_back(current);

        cout << "Created" << all_queries.size() << " p values" << endl;
        for(vector<SelectionQuerySet> set : all_queries) {
            cout << "Printing statistics for " << set.size() << " query sets" << endl;
            SelectionQuerySet::statisticsQuerySet(set);
        }
    }

    static void run_mono_column_benchmark(vector<std::unique_ptr<DatabaseSystem>> &all_dbms, const double scale, int num_queries, bool repeat){
        vector<double> default_selectivities = {//XXX
                1,1.0/2.0
                , 1.0/4.0
                , 1.0/8.0
                , 1.0/16.0
                , 1.0/32.0
                , 1.0/64.0
                , 1.0/128.0
                , 1.0/256.0
        };
        cout << "run_mono_column_benchmark(DatabaseSystem[], scale="<<scale<<",sel="<<Util::to_string(default_selectivities)<<") num_queries="<<num_queries << endl;
        int max_column = 10;//generate queries for all columns
        SelectionTests tester(scale, default_selectivities, num_queries, max_column);

        do {
            for(auto & dbs : all_dbms) {
                cout << "\n"<<dbs->name()<<"\n";
                auto t = dbs->get_TPC_H_lineitem(scale);

                string header = "\t";

                for(double s : default_selectivities) {
                    header+=to_string(s)+"\t";
                }
                if(LOG_COST) {
                    header+="\t";
                    for(double s : default_selectivities) {
                        header+="read_"+to_string(s)+"\t";
                    }
                    header+="\t";
                    for(double s : default_selectivities) {
                        header+="write_"+to_string(s)+"\t";
                    }
                    header+="check_sum";
                }
                cout << header << endl;

                for(int column=0; column<max_column;column++) {//XXX < max_column
                    //only use if DISPLAY_COST == true
                    vector<uint64_t> read_cost_vec(default_selectivities.size());
                    vector<uint64_t> write_cost_vec(default_selectivities.size());

                    cout << column << "\t";
                    uint64_t check_sum = 0;
                    for(int s=0; s<default_selectivities.size();s++) {
                        SelectionQuerySet& set = tester.all_queries.at(column).at(s);
                        if(LOG_COST) {
                            reset_cost();
                        }
                        auto start = chrono::system_clock::now();
                        for(auto& query : set.myQueries) {
                            //cout << "inner loop" << endl;
                            //System.out.println(query);
                            auto& columns = query.getColumns();
                            auto& predicates = query.getPredicate();
                            auto& selectivities = query.getSelectivities();
                            auto& synopsis = dbs->select(*t, columns, predicates,selectivities);
                            check_sum += synopsis.size();
                            //cout << "check_sum=" << check_sum << endl;
                        }
                        auto stop = chrono::system_clock::now();
                        cout << chrono::duration_cast<chrono::milliseconds>(stop-start).count() << "\t" << flush;
                        if(LOG_COST) {
                            read_cost_vec.at(s)  += read_cost;
                            write_cost_vec.at(s) += write_cost;
                        }
                    }
                    if(LOG_COST) {
                        cout << "\t";
                        for(int s=0; s<default_selectivities.size();s++) {
                            cout << to_string(read_cost_vec.at(s)/num_queries)<<"\t";
                        }
                        cout << "\t";
                        for(int s=0; s<default_selectivities.size();s++) {
                            cout << to_string(write_cost_vec.at(s)/num_queries)<<"\t";
                        }
                    }
                    cout << check_sum << endl;
                }

                dbs->clear();//the snyopsis must go away
            }
        }while(repeat);
    }

    void p_benchmark(DatabaseSystem* dbms, Table& t, bool repeat) {
        do {
            p_benchmark(dbms, t, scale, num_query_sets, num_queries, false);
        }while(repeat);
    }

    void p_benchmark(vector<DatabaseSystem*>& all_dbms, bool repeat) {
        do {
            for(DatabaseSystem* dbms : all_dbms) {
                p_benchmark(dbms, scale, num_query_sets, num_queries, false);
            }
        }while(repeat);
    }

    void dummy_out(){
        cout << "SelectionTests dummy out" << endl;
    }

    static void check_mcsp_queries(double scale, DatabaseSystem* to_test){
        int num_query_sets = 10;
        int num_queries = 10;
        SelectionTests tester(scale, num_query_sets, num_queries,3);
        tester.check_mcsp(to_test);
    }

    void check_mcsp(DatabaseSystem* to_test){
        DatabaseSystem* base_line = new MyHyper();

        Table& t=*to_test->get_TPC_H_lineitem(scale);
        Table& t_base_line=*base_line->get_TPC_H_lineitem(scale);
        //ColTable& table = dynamic_cast<ColTable &>(t_base_line);

        //TPC_H.out_operator = false;
        //TPC_H.out_cost = false;

        double start, stop;
        bool use_benchmark = true;

        cout << to_test->name() << endl;
        if(use_benchmark) {
            for (int i = 0; i < p_values.size(); i++) {
                cout << "Starting benchmark for p=" << to_string(p_values[i]) << "\t";

                for (SelectionQuerySet &set : all_queries.at(i)) {
                    //cout << "loop" << endl;
                    auto start = chrono::system_clock::now();
                    for (auto &query : set.myQueries) {
                        //cout << "inner loop" << endl;
                        //System.out.println(query);
                        auto &columns = query.getColumns();
                        auto &predicates = query.getPredicate();
                        cout <<"columns="<< Util::to_string(columns) << " predicates=" << Util::to_string(predicates) << endl;
                        auto &selectivities = query.getSelectivities();
                        Synopsis &synopsis = to_test->select(t, columns, predicates, selectivities);
                        Synopsis &synopsis_base_line = base_line->select(t_base_line, columns, predicates,
                                                                         selectivities);
                        compare(synopsis_base_line, synopsis, t_base_line);
                    }
                    auto stop = chrono::system_clock::now();
                    cout << chrono::duration_cast<chrono::milliseconds>(stop - start).count() << "\t" << endl;
                }
            }
        } else {
            vector<int> columns = {11, 12, 14};
            vector<vector<int>> predicates = {{8442, 8500} , {8213, 8838} , {86466, 386466}};
            vector<double> selectivities = {1, 1, 1};
            cout << Util::to_string(columns) << " " << Util::to_string(predicates) << endl;
            Synopsis &synopsis = to_test->select(t, columns, predicates, selectivities);
            Synopsis &synopsis_base_line = base_line->select(t_base_line, columns, predicates,
                                                             selectivities);
            compare(synopsis_base_line, synopsis, t_base_line);
        }

        t.~Table();
        t_base_line.~Table();
    }

};


#endif //MY_MCSP_SELECTIONTESTS_H

