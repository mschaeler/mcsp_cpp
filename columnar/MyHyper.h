//
// Created by Martin on 30.07.2021.
//

#ifndef MY_MCSP_MYHYPER_H
#define MY_MCSP_MYHYPER_H

#include "../general/DatabaseSystem.h"


class MyHyper : public DatabaseSystem{
    Synopsis intermediate_result;

    Synopsis& select(ColTable& t, const vector<int>& column_indexes, const vector<vector<int>>& predicates){
        const int numPredicates = column_indexes.size();
        intermediate_result.clear();//Here we clear hte buffer s.t. we do not care anywhere later
        const int size = t.size();

        if(numPredicates==1) {
            if(!Config::COUNT_ONLY) {//default case
                select_1(t, column_indexes.at(0), predicates.at(0).at(0), predicates.at(0).at(1));
            }else{
                //select_1_count(t, columns[0], predicates[0][0], predicates[0][1]);
            }
        }else if(numPredicates == 2){
            const vector<int>& col_1 = t.columns.at(column_indexes.at(0));
            const vector<int>& col_2 = t.columns.at(column_indexes.at(1));
            const int l_1 = predicates.at(0).at(0);
            const int l_2 = predicates.at(1).at(0);
            const int u_1 = predicates.at(0).at(1);
            const int u_2 = predicates.at(1).at(1);

            if(!Config::COUNT_ONLY) {
                select_2(col_1, l_1, u_1
                        , col_2, l_2, u_2
                        , size
                );
            }else{
                select_2_count(col_1, l_1, u_1
                        , col_2, l_2, u_2
                        , size
                );
            }
        }else if(numPredicates == 3) {
            const vector<int> &col_1 = t.columns.at(column_indexes[0]);
            const vector<int> &col_2 = t.columns.at(column_indexes[1]);
            const vector<int> &col_3 = t.columns.at(column_indexes[2]);
            const int l_1 = predicates[0][0];
            const int l_2 = predicates[1][0];
            const int l_3 = predicates[2][0];
            const int u_1 = predicates[0][1];
            const int u_2 = predicates[1][1];
            const int u_3 = predicates[2][1];

            if (!Config::COUNT_ONLY) {//default case
                select_3(col_1, l_1, u_1, col_2, l_2, u_2, col_3, l_3, u_3, size
                );
            } else {
                select_3_count(col_1, l_1, u_1, col_2, l_2, u_2, col_3, l_3, u_3, size
                );
            }
        }else if(numPredicates == 4){
            const vector<int>& col_1 = t.columns.at(column_indexes[0]);
            const vector<int>& col_2 = t.columns.at(column_indexes[1]);
            const vector<int>& col_3 = t.columns.at(column_indexes[2]);
            const vector<int>& col_4 = t.columns.at(column_indexes[3]);

            const int l_1 = predicates[0][0];
            const int l_2 = predicates[1][0];
            const int l_3 = predicates[2][0];
            const int l_4 = predicates[3][0];

            const int u_1 = predicates[0][1];
            const int u_2 = predicates[1][1];
            const int u_3 = predicates[2][1];
            const int u_4 = predicates[3][1];

            if(!Config::COUNT_ONLY) {//default case
                select_4(col_1, l_1, u_1
                        , col_2, l_2, u_2
                        , col_3, l_3, u_3
                        , col_4, l_4, u_4
                        , size
                );
            }else{
                select_4_count(col_1, l_1, u_1
                        , col_2, l_2, u_2
                        , col_3, l_3, u_3
                        , col_4, l_4, u_4
                        , size
                );
            }


        }else{
           cout << "MyHyper.select(): Not implemented " << numPredicates << endl;
        }
        if(LOG_COST){
            if(Config::COUNT_ONLY){
                    write_cost+=1;
            }else{
                write_cost+=intermediate_result.size();
            }
        }
        return intermediate_result;
    }

    void select_1(ColTable& t, const int col_index, const int lower, const int upper){
        const vector<int>& column_values = t.columns.at(col_index);
        const int size=t.size();
        for(int tid=0;tid<size;tid++) {
            if (Util::isIn(column_values.at(tid), lower, upper)) {
                intermediate_result.add(tid);
            }
        }
        if(LOG_COST){read_cost+=size*1;}
    }
    void select_1_count(ColTable& t, const int col_index, const int lower, const int upper){
        int counter = 0;
        const vector<int>& column_values = t.columns.at(col_index);
        const int size=t.size();
        for(int tid=0;tid<size;tid++) {
            if (Util::isIn(column_values.at(tid), lower, upper)) {
                counter++;//This is the difference only increment the counter
            }
        }
        intermediate_result.add(counter);//the result is a single value
        if(LOG_COST){read_cost+=size*1;}
    }

    void select_2(const vector<int>& col_1, const int l_1, const int u_1
    , const vector<int>& col_2, const int l_2, const int u_2
    , const int SIZE)
    {
        for(int tid=0;tid<SIZE;tid++){
            if(Util::isIn(col_1.at(tid),l_1,u_1)
               && Util::isIn(col_2.at(tid),l_2,u_2)
            ){
                intermediate_result.add(tid);
            }
        }
        if(LOG_COST){read_cost+=SIZE*2;}
    }

    void select_2_count(const vector<int>& col_1, const int l_1, const int u_1
            , const vector<int>& col_2, const int l_2, const int u_2
            , const int SIZE)
    {
        int counter = 0;
        for(int tid=0;tid<SIZE;tid++){
            if(Util::isIn(col_1.at(tid),l_1,u_1)
               && Util::isIn(col_2.at(tid),l_2,u_2)
            ){
                counter++;
            }
        }
        intermediate_result.add(counter);
        if(LOG_COST){read_cost+=SIZE*2;}
    }

    void select_3(const vector<int>& col_1, const int l_1, const int u_1
            , const vector<int>& col_2, const int l_2, const int u_2
            , const vector<int>& col_3, const int l_3, const int u_3
            , const int SIZE)
    {
        for(int tid=0;tid<SIZE;tid++){
            if(Util::isIn(col_1.at(tid),l_1,u_1)
               && Util::isIn(col_2.at(tid),l_2,u_2)
               && Util::isIn(col_3.at(tid),l_3,u_3)
                    ){
                intermediate_result.add(tid);
            }
        }
        if(LOG_COST){read_cost+=SIZE*3;}
    }

    void select_3_count(const vector<int>& col_1, const int l_1, const int u_1
            , const vector<int>& col_2, const int l_2, const int u_2
            , const vector<int>& col_3, const int l_3, const int u_3
            , const int SIZE)
    {
        int counter = 0;
        for(int tid=0;tid<SIZE;tid++){
            if(Util::isIn(col_1.at(tid),l_1,u_1)
               && Util::isIn(col_2.at(tid),l_2,u_2)
               && Util::isIn(col_3.at(tid),l_3,u_3)
                    ){
                counter++;
            }
        }
        intermediate_result.add(counter);
        if(LOG_COST){read_cost+=SIZE*3;}
    }

    void select_4(const vector<int>& col_1, const int l_1, const int u_1
            , const vector<int>& col_2, const int l_2, const int u_2
            , const vector<int>& col_3, const int l_3, const int u_3
            , const vector<int>& col_4, const int l_4, const int u_4
            , const int SIZE)
    {
        for(int tid=0;tid<SIZE;tid++){
            if(Util::isIn(col_1.at(tid),l_1,u_1)
               && Util::isIn(col_2.at(tid),l_2,u_2)
               && Util::isIn(col_3.at(tid),l_3,u_3)
               && Util::isIn(col_4.at(tid),l_4,u_4)
                    ){
                intermediate_result.add(tid);
            }
        }
        if(LOG_COST){read_cost+=SIZE*4;}
    }

    void select_4_count(const vector<int>& col_1, const int l_1, const int u_1
            , const vector<int>& col_2, const int l_2, const int u_2
            , const vector<int>& col_3, const int l_3, const int u_3
            , const vector<int>& col_4, const int l_4, const int u_4
            , const int SIZE)
    {
        int counter = 0;
        for(int tid=0;tid<SIZE;tid++){
            if(Util::isIn(col_1.at(tid),l_1,u_1)
               && Util::isIn(col_2.at(tid),l_2,u_2)
               && Util::isIn(col_3.at(tid),l_3,u_3)
               && Util::isIn(col_4.at(tid),l_4,u_4)
            ){
                counter++;
            }
        }
        intermediate_result.add(counter);
        if(LOG_COST){read_cost+=SIZE*4;}
    }

public:

    MyHyper() : intermediate_result(){

    }

    Synopsis& select(Table& t, vector<int>& column_indexes, vector<vector<int>>& predicates, vector<double>& selectivities){
        ColTable& table = dynamic_cast<ColTable &>(t);
        return select(table, column_indexes, predicates);
    }

    std::unique_ptr<Table> get_TPC_H_lineitem(double scale){

        auto table = std::make_unique<ColTable>(scale);
        //std::cout << table->out() << std::endl;

       /* if(Config::MATERIALIZE_DATA){
            ColTable::materialize(*table, scale);
        }*/
        return table;
    }

    string name(){
        return "MyHyper";
    }

    void clear() override
    {
        intermediate_result.clear();
    }
};


#endif //MY_MCSP_MYHYPER_H
