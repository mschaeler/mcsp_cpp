//
// Created by Martin on 06.04.2021.
//

#ifndef MY_MCSP_ELF_BUILDER_SEPARATE_H
#define MY_MCSP_ELF_BUILDER_SEPARATE_H

#include "../columnar/ColTable.h"
#include "Elf.h"
#include "Elf_table_lvl_seperate.h"

class BecomesMonoList{
public:
    BecomesMonoList(int _tid, int _start_dim, int _elf_sub_tree) :
        tid(_tid), start_dim(_start_dim), elf_sub_tree(_elf_sub_tree)
    {}

    int start_dim;
    int elf_sub_tree;
    int tid;
};

class SubTree{
public:
    SubTree(int _start_pointer, vector<int32_t>& _contained_points) :
            start_pointer(_start_pointer), contained_points(_contained_points)
    {}

    /** This pointer has be set upon writing the next level*/
    int start_pointer;
    vector<int32_t> contained_points;
};

class Elf_builder_separate {
    ColTable& table;
    int num_dim;
    int32_t write_pointer = 0;

    vector<int32_t> values;
    vector<int32_t> pointer;
    vector<int32_t> mono_list_array;

    vector<int32_t> levels;
    vector<int32_t> levels_mono_lists;
    vector<int32_t> max_values;

    vector<BecomesMonoList> mono_lists;

    void get_max_column_value(vector<int32_t>& vector) {
        for(int col=0;col<num_dim;col++){
            vector[col] = max(this->table.columns[col]);
        }
    }

    static int32_t max(const vector<int>& column){
        int32_t max = INT_MIN;
        for(int tid : column){
            if(tid>max){
                max = tid;
            }
        }
        return max;
    }

    void test_exact(ColTable &table, Elf_table_lvl_seperate* elf) {
        cout << "test_exact()" << endl;
        int size = table.size();
        int num_dim = table.columns.size();
        vector<int> point(num_dim);
        for(int tid=0;tid<size;tid++){
            //copy point into vector
            for(int c=0;c<num_dim;c++){
                point.at(c)=table.columns.at(c).at(tid);
            }
            int found_tid = elf->exists(point);
            if(found_tid != tid){
                cout << "test_exact() tid=" << tid << ", but got " << found_tid << endl;
            }
        }
        cout << "test_exact() [DONE]"<< endl;
    }

    Elf_table_lvl_seperate* build(){
        cout << "Elf_builder_separate.build() " << table.size() << endl;
        auto begin = chrono::system_clock::now();

        /*****************************************
		 * (1) first and second level are special
		 ****************************************/
        vector<vector<int32_t>> root_level = buildFirstLevel();
        vector<SubTree> level = buildSecondLevel(root_level);//TODO - by value

        /*****************************************
		 * (2) linearization of all remaining levels works the same way
		 ****************************************/
        int dim = 2;
        while(!level.empty()){
            level = buildLevel(level, dim);
            dim++;
        }

        /*****************************************
		 * (3) We reached the level where all paths
		 * became unique. Let't write all the Mono Lists
		 ****************************************/
        dim--;//the level itself shall be included
        for(;dim<num_dim;dim++){
            //set the begin pointer of all subsequent levels (including me)
            //to the same value to indicate that they do not exist.
            this->levels[dim] = write_pointer;
        }

        //usually the first mono lists is not in the first level. Find out where it starts
        int current_level = this->mono_lists.at(0).start_dim;
        //For all levels above, there is no Mono List -> set is begin pointer to 0
        for(int l=0;l<current_level;l++){
            this->levels_mono_lists[l] = 0;
        }
        //sanity check - all points arrived at a mono list?
        if(this->mono_lists.size()!=table.size()){
            cout << "size does not match " << mono_lists.size() << endl;
        }else {
            cout << "Start writing "<<mono_lists.size()<<" mono lists" << endl;
        }
        /************************************
		 * (3.1) Really write the Mono Lists - Note they are still in an intermediate format
		 ************************************/
        for(BecomesMonoList mono : this->mono_lists){
            if(mono.start_dim!=current_level){
                if(mono.start_dim<current_level){
                    cout << "Boah, they should be sorted"<<endl;
                }else{
                    this->levels_mono_lists[current_level] = mono_list_array.size();
                    current_level = mono.start_dim;//may be multiple levels deeper
                }
            }
            writeMonoList(mono);
        }
        cout << "Done writing mono lists" << endl;
        /************************************
		 * (4) Copy all create data items, i.e. levels and MonoList into trimmed arrays for the actual Elf
		 ************************************/
        values.resize(write_pointer);
        pointer.resize(write_pointer);
        //mono_list_array.resize(mono_list_array.size());

        for(;current_level<num_dim;current_level++){
            this->levels_mono_lists[current_level] = mono_list_array.size();
        }

        cout << "Almost done level statistics levels=" << Util::to_string(levels) << " mono list levels=" << Util::to_string(levels_mono_lists) <<  endl;

        cout << levels[0] << endl;
        //cout << Util::to_string(levels_mono_lists) << endl;
        auto end = chrono::system_clock::now();

        cout << "Elf_separate.build() " <<table.size()<< " in "<<chrono::duration_cast<chrono::milliseconds>(end - begin).count()<<"  ms" << endl;
        Elf_table_lvl_seperate* to_return = new Elf_table_lvl_seperate(
                this->table.my_name
                , this->table.column_names
                , this->values
                , this->pointer
                , this->mono_list_array
                , this->levels
                , this->levels_mono_lists
                , this->num_dim
                );
        if(Elf::SAVE_MODE){
            test_exact(this->table,to_return);
        }
        return to_return;
    }

    void writeMonoList(BecomesMonoList& mono) {
        //write missing pointer to mono list
        int write_to_location = mono_list_array.size() | Elf::LAST_ENTRY_MASK;
        pointer[mono.elf_sub_tree] =  write_to_location;

        //write mono list
        for(int dim=mono.start_dim;dim<num_dim;dim++){
            int val = this->table.columns.at(dim).at(mono.tid);
            mono_list_array.push_back(val);
        }
        //store TID at the end of the MonoList
        mono_list_array.push_back(mono.tid);

        /*if(Elf::SAVE_MODE){
            int size = mono_list_array.size();
            cout << "write_to_location="<< (write_to_location&Elf::RECOVER_MASK) << " size=" << size << endl;
        }*/
    }

    /**
	 * For building the first level in Elf. Recap it contains each existing value once.
	 * Thus we distribute the points based on their value in the first column (i.e., level).
	 *
	 * @param column - the first column of the table
	 * @param level - level[value] allows to access all points with that value
	 */
    vector<vector<int32_t>> buildFirstLevel(){
        const vector<int>& column = this->table.columns[0];//First column
        int size = column.size();
        vector<vector<int32_t>> level(this->max_values[0]+1);//one entry for each value including the largest one (i.e., +1)

        /**
         * (1) distribute all the points according to their value in first column
         */
        for(int tid=0;tid<size;tid++){					    // for each point in table
            int value = column[tid];			            // value of point with TID=tid
            vector<int32_t>& my_sub_tree = level.at(value);	// you need to go here
            if(tid>size){
                cout << "tid to large: " << tid << " of " << size << endl;
            }
            my_sub_tree.push_back(tid);						// I am in here
            //cout << my_sub_tree.size() << "vs." << level.at(value).size() << endl;
        }
        return level;//TODO - by value
    }

    vector<SubTree> buildSecondLevel(vector<vector<int32_t>>& last_level) {
        int my_dim = 0 + 1;
        const vector<int>& raw_column = table.columns.at(my_dim);
        size_t dim_max = this->max_values[my_dim] + 1;//including largest value
        vector<SubTree> level;
        write_pointer = this->max_values[0] + 1;    // Assume that we really need all the values
        levels[0] = write_pointer;            // This is the position where the first node in the 2nd level starts, i.e., we store the start offset of the 2nd level

        // For each node in this level
        for (int32_t node = 0; node < last_level.size(); node++) {
            if (!last_level[node].empty()) {
                /*******************************************
                 * (1) handle root of sub tree in last level
                 *******************************************/
                values[node] = node;
                pointer[node] = write_pointer;//XXX - this is the problem, we can set the pointers one level later only, i.e., these are pointers in the root level.

                /*******************************************
                 * (2.1) Now create the new dim list (i.e., node) in this level
                 *******************************************/
                // store length of dim list
                int32_t length = 0;//the point is we do not know the real length yet, but only an upper bound
                int32_t length_pointer = write_pointer;
                write_pointer++;

                /*******************************************
                 * (2.2) Write all unique values into the node
                 * Recap. the corresponding pointers are written
                 * one level deeper, because we do not know them yet.
                 *******************************************/
                //Assign each point to its value in this dim. Sort the values.
                vector<vector<int32_t>> my_dim_elements = distribute(raw_column, last_level[node], dim_max, my_dim);
                // my_dim_elements[value] -> gives all tids having this value in this level
                for (int i = 0; i < my_dim_elements.size(); i++) {//so many may exist, there may be empty elements, we do not materialize
                    vector<int32_t> &contained_points = my_dim_elements[i];
                    if (!contained_points.empty()) {
                        int32_t val = i;//XXX trick
                        values[write_pointer] = val;
                        SubTree sub(write_pointer, contained_points);

                        if (contained_points.size() > 1) {
                            level.push_back(sub);
                        } else {// sub tree is a mono list containing only one tid.
                            int tid = contained_points.at(0);
                            makeMonoList(tid, my_dim + 1, write_pointer);
                        }
                        length++;
                        write_pointer++;
                    }//else ignore this one
                }
                //now we know the real length
                values[length_pointer] = length | Elf::LAST_ENTRY_MASK;
                last_level[node].clear();
            } else {
                values[node] = Elf::EMPTY_ROOT_NODE;
                pointer[node] = Elf::EMPTY_ROOT_NODE;
            }
        }
        cout << "dim=" << my_dim << " non unique sub trees " << level.size() << endl;
        return level;
    }

    vector<SubTree> buildLevel(const vector<SubTree>& last_level, int my_dim) {
        vector<int> column = table.columns[my_dim];
        int dim_max = this->max_values[my_dim] + 1;//including largest value
        vector<SubTree> level;
        this->levels[my_dim-1] = write_pointer;//The one before ends here

        //for each node s (sub tree) in this level
        for(auto me : last_level){
            //Now we know the start position. Tell this to the root node of this sub tree
            pointer[me.start_pointer] = write_pointer;
            vector<vector<int>> my_dim_elements = distribute(column, me.contained_points, dim_max, my_dim);

            // store length of dim list
            int length = 0;//the point is we do not know the real length yet, but only an upper bound
            int length_pointer = write_pointer;
            write_pointer++;

            for(int i=0;i<my_dim_elements.size();i++){//so many sub trees may exist, there may be null elements, we do not materialize
                vector<int>& contained_points = my_dim_elements[i];
                if(!contained_points.empty()){
                    int val = i;//XXX trick
                    values[write_pointer] = val;
                    SubTree sub(write_pointer, contained_points);

                    if(contained_points.size()>1){
                        level.push_back(sub);
                    }else{// sub tree is a mono list containing only one tid.
                        int tid = contained_points.at(0);
                        makeMonoList(tid,my_dim+1,write_pointer);
                    }
                    length++;
                    write_pointer ++;
                }//else ignore this one
            }
            //now we the real length
            values[length_pointer] = length | Elf::LAST_ENTRY_MASK;
        }
        cout << "dim=" << my_dim << " non unique sub trees " << level.size() << endl;
        return level;
    }

    static vector<vector<int>> distribute(const vector<int>& column, vector<int>& node_tids, int dim_max, int my_dim) {
        vector<vector<int>> sub_tree_level(dim_max);//Hier optimieren?
        /*for(int i=0;i<dim_max;i++){
            vector<int> temp;
            sub_tree_level.push_back(temp);
        }*/
        int size = node_tids.size();
        for(int i=0;i<size;i++){
            int tid = node_tids.at(i);
            if(tid>column.size()){
                cout << "level="<< my_dim <<" tid to large: " << tid << " of " << column.size() << endl;
            }
            int v = column.at(tid);
            vector<int>& my_sub_tree = sub_tree_level.at(v);
            my_sub_tree.push_back(tid);// I am in here
        }

        for(auto& my_sub_tree : sub_tree_level){
            //cout << "(v=" <<i<<", s="<<my_sub_tree.size()<<") ";
            for(auto v : my_sub_tree){
                if(v>column.size()){
                    cout << "check level="<< my_dim <<" tid to large: " << v << " of " << column.size() << endl;
                }
            }
        }

        return sub_tree_level;//TODO - by value
    }


    void makeMonoList(int tid, int start_dim, int elf_sub_tree) {
        BecomesMonoList temp(tid, start_dim, elf_sub_tree);
        mono_lists.push_back(temp);
    }

public:



    explicit Elf_builder_separate(ColTable& t)
        : table(t)
        , num_dim(t.columns.size())
        , max_values(t.columns.size())
        , mono_lists()
        , levels(t.columns.size())
        , levels_mono_lists(t.columns.size())
        , values(t.size()*num_dim)
        , pointer(t.size()*num_dim)
        //, mono_list_array(t.size()*num_dim)
            , mono_list_array()
    {


        /*size_t size = t.size()*num_dim;
        levels = *new vector<int32_t>(num_dim);
        levels_mono_lists = *new vector<int32_t>(num_dim);

        values = *new vector<int32_t>(size);
        pointer= *new vector<int32_t>(size);
        mono_list_array= *new vector<int32_t>(size);*/

        get_max_column_value(max_values);
    }
    static Elf_table_lvl_seperate* build(ColTable& table){
        Elf_builder_separate separate (table);
        cout << "build()" << endl;
        return separate.build();
    }
};


#endif //MY_MCSP_ELF_BUILDER_SEPARATE_H
