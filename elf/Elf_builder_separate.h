//
// Created by Martin on 06.04.2021.
//

#ifndef MY_MCSP_ELF_BUILDER_SEPARATE_H
#define MY_MCSP_ELF_BUILDER_SEPARATE_H

#include "../columnar/ColTable.h"
#include "Elf.h"
#include "Elf_table_lvl_seperate.h"
#include "Elf_Table_Lvl_Cutoffs.h"
#include "../elf_cutoff_external/Elf_Table_Cutoff_External.h"

class BecomesMonoList{
public:
    BecomesMonoList(int _tid, int _start_dim, elf_pointer _elf_sub_tree) :
        tid(_tid), start_dim(_start_dim), elf_sub_tree(_elf_sub_tree)
    {
        if(Elf::SAVE_MODE){
            if(elf_sub_tree<0) cout << "BecomesMonoList() elf_sub_tree<0 for tid=" << tid << " start_dim=" << _start_dim << " pointer=" << _elf_sub_tree << endl;
        }
    }

    int start_dim;
    elf_pointer elf_sub_tree;
    int tid;
};

class SubTree{
public:
    SubTree(elf_pointer _start_pointer, vector<int32_t>& _contained_points) :
            start_pointer(_start_pointer), contained_points(_contained_points)
    {}

    /** This pointer has be set upon writing the next level*/
    elf_pointer start_pointer;
    vector<int32_t> contained_points;
};

class Elf_builder_separate {
    ColTable& table;
    int num_dim;
    /** for writing the cutoffs**/
    int32_t current_tid_offset = 0;
    vector<int32_t> tids_in_elf_order;
    vector<int32_t> tids_level;
    /** for writing external cutoffs**/
    vector<int32_t> cutoffs;

    elf_pointer write_pointer = 0;

    vector<int32_t> values;
    vector<elf_pointer> pointer;
    vector<int32_t> mono_list_array;

    vector<elf_pointer> levels;
    vector<elf_pointer> levels_mono_lists;
    vector<int32_t> max_values;

    vector<BecomesMonoList> mono_lists;

    void get_max_column_value(vector<int32_t>& vector) {
        for(int col=0;col<num_dim;col++){
            vector.at(col) = max(this->table.columns.at(col));
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

    void build(){
        cout << "Elf_builder_separate.build() " << table.size() << " ";
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
        cout << endl;

        /*****************************************
		 * (3) We reached the level where all paths
		 * became unique. Let't write all the Mono Lists
		 ****************************************/
        dim--;//the level itself shall be included
        for(;dim<num_dim;dim++){
            //set the begin pointer of all subsequent levels (including me)
            //to the same value to indicate that they do not exist.
            this->levels.at(dim) = write_pointer;
        }

        //usually the first mono lists is not in the first level. Find out where it starts
        int current_level = this->mono_lists.at(0).start_dim;
        //For all levels above, there is no Mono List -> set is begin pointer to 0
        for(int l=0;l<current_level;l++){
            this->levels_mono_lists.at(l) = 0;
        }
        //sanity check - all points arrived at a mono list?
        if(this->mono_lists.size()!=table.size()){
            cout << "size does not match " << mono_lists.size() << endl;
        }else {
            cout << "Start writing "<<mono_lists.size()<<" mono lists ";
        }
        /************************************
		 * (3.1) Really write the Mono Lists - Note they are still in an intermediate format
		 ************************************/
        for(BecomesMonoList& mono : this->mono_lists){
            if(mono.start_dim!=current_level){
                if(mono.start_dim<current_level){
                    cout << "Boah, they should be sorted"<<endl;
                }else{
                    elf_pointer temp = mono_list_array.size();
                    cout << temp;
                    this->levels_mono_lists.at(current_level) = temp;
                    current_level = mono.start_dim;//may be multiple levels deeper
                }
            }
            writeMonoList(mono);
        }
        cout << "[DONE]" << endl;
        //cout << "pointer @56287=" << pointer.at(56287) << endl;
        /************************************
		 * (4) Copy all create data items, i.e. levels and MonoList into trimmed arrays for the actual Elf
		 ************************************/
        values.resize(write_pointer);
        pointer.resize(write_pointer);
        //mono_list_array.resize(mono_list_array.size());

        for(;current_level<num_dim;current_level++){
            this->levels_mono_lists.at(current_level) = mono_list_array.size();
        }

        cout << "Almost done level statistics levels=" << Util::to_string(levels) << " mono list levels=" << Util::to_string(levels_mono_lists) <<  endl;

        //cout << levels.at(0) << endl;
        //cout << Util::to_string(levels_mono_lists) << endl;
        auto end = chrono::system_clock::now();
        //cout << "pointer @56287=" << pointer.at(56287) << endl;
        cout << "Elf_separate.build() " <<table.size()<< " in "<<chrono::duration_cast<chrono::milliseconds>(end - begin).count()<<"  ms" << endl;
    }
    Elf_table_lvl_seperate* create_elf_instance(){
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

    inline bool points_to_monolist(elf_pointer pointer) const {
        return (pointer<0) ? true : false;//msb is set, i.e., pointer is negative
    }

    int get_node_size(elf_pointer start_node) {
        int length = values.at(start_node);
        return length & Elf::RECOVER_NODE_LENGTH_MASK;//un mask
    }

    int get_tid_from_monolist(elf_pointer start_list, int start_level) {
        return mono_list_array.at(start_list+table.num_dim-start_level);
    }

    //find tids in preorder traversal
    void determine_and_write_external_cutoffs() {
        const elf_pointer start = 0;//by definition
        const elf_pointer stop = levels.at(Elf::FIRST_DIM);
        current_tid_offset = 0;

        for(elf_pointer elem=start;elem<stop;elem++) {//for each entry in the root
            write_cuttoff_external(elem);
            elf_pointer elem_pointer = pointer.at(elem);

            if(elem_pointer!=Elf::EMPTY_ROOT_NODE){//non-dense data in first dimension
                if(!points_to_monolist(elem_pointer)) {
                    determine_and_write_external_cutoffs(elem_pointer, Elf::FIRST_DIM+1);
                }
            }
        }
    }

    void write_cuttoff_external(elf_pointer elem_offset) {
        if(Elf::SAVE_MODE) {
            int temp_cuttoff = cutoffs.at(elem_offset);
            if(temp_cuttoff!=Elf::NOT_FOUND && temp_cuttoff!=current_tid_offset) {//may be loaded cutt ofss from file
                cout << "write_cuttoff(" << elem_offset << ","+current_tid_offset << ") write to non empty position and existing cutoff incorrect" << endl;
            }
        }
        cutoffs.at(elem_offset) = current_tid_offset;
    }

    /**
     * Processes one node and recursively propagates to next level in pre order traversal.
     * We use pre-order traversal, since we then find tids according to Elf sort order.
     *
     * @param start_list
     * @return - the total number of tids, we have already seen.
     */
    void determine_and_write_external_cutoffs(const elf_pointer start_list, const int level){
        const int length = get_node_size(start_list);

        /*if(Elf::SAVE_MODE) {
            if(level!=get_level(start_list)) {
                System.err.println("determine_and_write_cutoffs(int,int) level!=getLevel()");
            }
        }*/

        for(int elem=0;elem<length;elem++) {
            const elf_pointer elem_offset = start_list + Elf_table_lvl_seperate::NODE_HEAD_LENGTH + elem;//+1 for length
            write_cuttoff_external(elem_offset);
            elf_pointer start_list_next_dim = pointer.at(elem_offset);
            if (points_to_monolist(start_list_next_dim)) {
                //no cutoff to write
                start_list_next_dim &= Elf::RECOVER_MASK;
                int tid = get_tid_from_monolist(start_list_next_dim, level + 1);
                tids_in_elf_order.at(current_tid_offset) = tid;
                tids_level.at(current_tid_offset) = level + 1;//mono list starts in the next level
                current_tid_offset++;
            } else {
                determine_and_write_external_cutoffs(start_list_next_dim, level + 1);
            }
        }
    }

            //find tids in preorder traversal
    void determine_and_write_cutoffs() {
        cout << "determine_and_write_cutoffs()" << endl;
        elf_pointer start = 0;//by definition
        elf_pointer stop  = levels.at(0);

        for(elf_pointer e=start;e<stop;e++) {//for each entry in the root
            elf_pointer start_list_next_dim = pointer.at(e);

            if(start_list_next_dim!=Elf::EMPTY_ROOT_NODE){//non-dense data in first dimension
                if(!points_to_monolist(start_list_next_dim)) {
                    write_cuttoff(start_list_next_dim, current_tid_offset);
                    determine_and_write_cutoffs(start_list_next_dim, 1);
                } else {
                    cout << "determine_and_write_cutoffs() - monolist in first dim @" << e << endl;
                }
            }
        }
    }

    void write_cuttoff(const elf_pointer start_list, const int current_tid_offset) {
        if(Elf::SAVE_MODE) {
            elf_pointer temp_cuttoff = pointer.at(start_list);
            if(temp_cuttoff!=0 && temp_cuttoff!=current_tid_offset) {//may be loaded cutt ofss from file
                cout << "write_cuttoff(" << start_list << "," << current_tid_offset << ") write to non empty position and existing cutoff incorrect" << endl;
            }
        }
        pointer.at(start_list) = current_tid_offset;
    }

    /**
     * Processes one node and recursively propagates to next level in pre order traversal.
     * We use pre-order traversal, since we then find tids according to Elf sort order.
     *
     * @param start_list
     * @return - the total number of tids, we have already seen.
     */
    void determine_and_write_cutoffs(const elf_pointer start_list, const int level){
        const int length = get_node_size(start_list);

        /*if(Elf::SAVE_MODE) {
            if(level!=get_level(start_list)) {
                cout << "determine_and_write_cutoffs(int,int) level!=getLevel()" << endl;
            }
        }*/

        for(elf_pointer elem=0;elem<length;elem++){
            const elf_pointer elem_offset = start_list+Elf_Table_Lvl_Cutoffs::NODE_HEAD_LENGTH+elem;//+1 for length
            elf_pointer start_list_next_dim = pointer.at(elem_offset);
            if(points_to_monolist(start_list_next_dim)) {
                //no cutoff to write
                start_list_next_dim &= Elf::RECOVER_MASK;
                const int tid = get_tid_from_monolist(start_list_next_dim, level+1);
                tids_in_elf_order.at(current_tid_offset)= tid;
                if(tids_in_elf_order.at(current_tid_offset)!=tid){
                    cout << tid << endl;
                }
                tids_level.at(current_tid_offset)       = level+1;//mono list starts in the next level
                current_tid_offset++;
            } else {
                write_cuttoff(start_list_next_dim, current_tid_offset);
                determine_and_write_cutoffs(start_list_next_dim, level+1);
            }
        }
    }

    Elf_Table_Lvl_Cutoffs* create_elf_instance_with_cuttoffs(){
        tids_in_elf_order.resize(table.size(), -1);
        tids_level.resize(table.size(),-1);
        determine_and_write_cutoffs();

        Elf_Table_Lvl_Cutoffs* to_return = new Elf_Table_Lvl_Cutoffs(
                this->table.my_name
                , this->table.column_names
                , this->values
                , this->pointer
                , this->mono_list_array
                , this->levels
                , this->levels_mono_lists
                , this->num_dim
                , this->tids_in_elf_order
                , this->tids_level
        );
        if(Elf::SAVE_MODE){
            test_exact(this->table,to_return);
            to_return->check_cutoffs(this->table);
        }
        return to_return;
    }

    Elf_Table_Cutoff_External* create_elf_instance_with_external_cuttoffs(){
        tids_in_elf_order.resize(table.size(), -1);
        tids_level.resize(table.size(),-1);
        cutoffs.resize(pointer.size(),-1);//this vector simply has the size of the pointer array as there is a cutoff for every pointer
        determine_and_write_external_cutoffs();

        Elf_Table_Cutoff_External* to_return = new Elf_Table_Cutoff_External(
                this->table.my_name
                , this->table.column_names
                , this->values
                , this->pointer
                , this->mono_list_array
                , this->levels
                , this->levels_mono_lists
                , this->num_dim
                , this->tids_in_elf_order
                , this->tids_level
                , this->cutoffs
        );
        if(Elf::SAVE_MODE){
            test_exact(this->table,to_return);
            to_return->check_cutoffs();
        }
        return to_return;
    }

    void writeMonoList(BecomesMonoList& mono) {
        //write missing pointer to mono list
        /*if(mono_list_array.size() == 243628){
            cout << "writeMonoList()"<<endl;
        }*/
        elf_pointer write_to_location = mono_list_array.size() | Elf::LAST_ENTRY_MASK;
        pointer.at(mono.elf_sub_tree) =  write_to_location;

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
        const vector<int>& column = this->table.columns.at(0);//First column
        int size = column.size();
        vector<vector<int32_t>> level(this->max_values.at(0)+1);//one entry for each value including the largest one (i.e., +1)

        /**
         * (1) distribute all the points according to their value in first column
         */
        for(int tid=0;tid<size;tid++){					    // for each point in table
            int value = column.at(tid);			            // value of point with TID=tid
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
        size_t dim_max = this->max_values.at(my_dim) + 1;//including largest value
        vector<SubTree> level;
        write_pointer = this->max_values.at(0) + 1;    // Assume that we really need all the values
        levels.at(0) = write_pointer;            // This is the position where the first node in the 2nd level starts, i.e., we store the start offset of the 2nd level

        // For each node in this level
        for (int32_t node = 0; node < last_level.size(); node++) {
            if (!last_level.at(node).empty()) {
                /*******************************************
                 * (1) handle root of sub tree in last level
                 *******************************************/
                values.at(node) = node;
                pointer.at(node) = write_pointer;//XXX - this is the problem, we can set the pointers one level later only, i.e., these are pointers in the root level.

                /*******************************************
                 * (2.1) Now create the new dim list (i.e., node) in this level
                 *******************************************/
                // store length of dim list
                int32_t length = 0;//the point is we do not know the real length yet, but only an upper bound
                elf_pointer length_pointer = write_pointer;
                write_pointer++;

                /*******************************************
                 * (2.2) Write all unique values into the node
                 * Recap. the corresponding pointers are written
                 * one level deeper, because we do not know them yet.
                 *******************************************/
                //Assign each point to its value in this dim. Sort the values.
                vector<vector<int32_t>> my_dim_elements = distribute(raw_column, last_level.at(node), dim_max, my_dim);
                // my_dim_elements[value] -> gives all tids having this value in this level
                for (int i = 0; i < my_dim_elements.size(); i++) {//so many may exist, there may be empty elements, we do not materialize
                    vector<int32_t> &contained_points = my_dim_elements.at(i);
                    if (!contained_points.empty()) {
                        int32_t val = i;//XXX trick
                        values.at(write_pointer) = val;
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
                values.at(length_pointer) = length | Elf::NODE_LENGTH_MASK;
                last_level.at(node).clear();
            } else {
                values.at(node) = Elf::EMPTY_ROOT_NODE;
                pointer.at(node) = Elf::EMPTY_ROOT_NODE;
            }
        }
        cout << "l=" << my_dim << " non unique=" << level.size() << ", "; cout.flush();
        return level;
    }

    vector<SubTree> buildLevel(const vector<SubTree>& last_level, int my_dim) {
        vector<int> column = table.columns.at(my_dim);
        int dim_max = this->max_values.at(my_dim) + 1;//including largest value
        vector<SubTree> level;
        //cout << "Next level starts@" << write_pointer << endl;
        this->levels.at(my_dim-1) = write_pointer;//The one before ends here

        //for each node s (sub tree) in this level
        for(auto me : last_level){
            //Now we know the start position. Tell this to the root node of this sub tree
            pointer.at(me.start_pointer) = write_pointer;
            vector<vector<int>> my_dim_elements = distribute(column, me.contained_points, dim_max, my_dim);

            // store length of dim list
            int length = 0;//the point is we do not know the real length yet, but only an upper bound
            int length_pointer = write_pointer;
            write_pointer++;

            for(int i=0;i<my_dim_elements.size();i++){//so many sub trees may exist, there may be null elements, we do not materialize
                vector<int>& contained_points = my_dim_elements.at(i);
                if(!contained_points.empty()){
                    int val = i;//XXX trick
                    values.at(write_pointer) = val;
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
            values.at(length_pointer) = length | Elf::NODE_LENGTH_MASK;
        }
        cout << "l=" << my_dim << " non unique=" << level.size() << ", "; cout.flush();
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
        , tids_in_elf_order(0)
        , tids_level(0)
        , cutoffs(0)
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
        cout << "before build()" << endl;
        Elf_builder_separate separate (table);
        cout << "build()" << endl;
        separate.build();
        return separate.create_elf_instance();
    }
    static Elf_Table_Lvl_Cutoffs* build_with_cuttoffs(ColTable& table){
        Elf_builder_separate separate (table);
        cout << "build()" << endl;
        separate.build();
        Elf_Table_Lvl_Cutoffs* elf = separate.create_elf_instance_with_cuttoffs();
        return elf;
    }
    static Elf_Table_Cutoff_External* build_with_external_cuttoffs(ColTable& table){
        Elf_builder_separate separate (table);
        cout << "build()" << endl;
        separate.build();
        Elf_Table_Cutoff_External* elf = separate.create_elf_instance_with_external_cuttoffs();
        return elf;
    }
};


#endif //MY_MCSP_ELF_BUILDER_SEPARATE_H
