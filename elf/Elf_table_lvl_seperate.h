//
// Created by Martin on 20.07.2021.
//

#ifndef MY_MCSP_ELF_TABLE_LVL_SEPERATE_H
#define MY_MCSP_ELF_TABLE_LVL_SEPERATE_H

class Elf_table_lvl_seperate : public Elf {
    const vector<elf_pointer> levels;
    const vector<elf_pointer> levels_mono_lists;

    const vector<int32_t> values;
    const vector<elf_pointer> pointer;
    const vector<int32_t> mono_lists;
    const int32_t num_dim;

    int exists_mono(const elf_pointer start_list, const vector<int>& query, const int start_dim) const {
        for(int dim=start_dim;dim<num_dim;dim++){
            int val = get_attribute_monolist(start_list,start_dim,dim);// mono_lists[start_list+dim-start_dim];
            if(query.at(dim)!=val){
                return NOT_FOUND;
            }
        }
        int tid = get_tid_from_monolist(start_list, start_dim);//this->mono_lists[start_list+num_dim-start_dim];
        if(Elf::SAVE_MODE) {
            vector<int> tid_problems{
                    2004, 351817
            };
            if(Util::isIn(tid, tid_problems)) {
                cout << "exists() tid=" << tid << "start@=" << start_list << "level=" << start_dim << endl;
                //System.out.println("exists() tid=\t"+tid+"\tstart_list=\t"+start_list+"\tstart_level=\t"+getMonoListLevel(start_list)+"\t q=\t"+Util.outTSV(query));
            }
        }
        return tid;
    }

    int exists(const elf_pointer START_LIST, const vector<int>& query, const int dimension) const {
        int toCompare;
        int length = get_node_length(START_LIST);
        int q_val = query.at(dimension);

        for(int elem=0;elem<length;elem++){
            elf_pointer elem_offset = START_LIST+1+elem;//+1 for length
            toCompare = get_value(elem_offset);
            if(toCompare==q_val){
                elf_pointer my_pointer = get_pointer(elem_offset);
                if(my_pointer>NOT_FOUND){//normal dim list
                    return exists(my_pointer,query,dimension+1);
                }else{//mono list
                    my_pointer&=RECOVER_MASK;
                    return exists_mono(my_pointer,query,dimension+1);
                }
            }else if(toCompare>query.at(dimension)){
                return NOT_FOUND;
            }
        }
        return NOT_FOUND;
    }

    inline int get_node_length(const int node_offset) const {
        if(LOG_COST){read_cost++;}
        int length = values.at(node_offset);
        if(SAVE_MODE) {
            if(length>-1) {
                cout <<"intGetNodeLength() no length at this offset"<<endl;
            }
        }
        return length & RECOVER_MASK;//un mask
    }

    inline bool is_node_length_offset(const elf_pointer offset) const {
        return values.at(offset)<0;
    }

protected:
    /**
     * Returns the dim_to_return'th attribute values of the tuples. In contrast to the standard Elf the mono lists have an extra array and are not part of the ELF array.
     *
     * [SAVE_MODE] - checks sanity of dimensionality
     * [SAVE_MODE] - checks whether mono list contains the attribute (can be in upper parts of the tree)
     * @param start_list - start offset of list, i.e., list starts at ELF[start_list]
     * @param start_level_list - start level of the mono list, i.e., getMonoListLevel(start_list) = start_dim_list should hold
     * @param attribute_to_return - tuple[dim_to_return] in  row store would give the desired values
     * @return
     */
    inline int get_attribute_monolist(const elf_pointer start_list, const int start_level_list, const int attribute_to_return) const{
        if(SAVE_MODE){
            if(attribute_to_return<FIRST_DIM || attribute_to_return >= num_dim){
                cout << "Error: get_attribute_monolist(int,int,int) wrong dim_to_return " << attribute_to_return << endl;
                return NOT_FOUND;
            }
            if(attribute_to_return<start_level_list){
                cout << "Error: get_attribute_monolist(int,int,int) mono list does not contain the attribute" << endl;
                return NOT_FOUND;
            }
        }
        if(LOG_COST){read_cost++;}

        return mono_lists.at(start_list+attribute_to_return-start_level_list);
    }


    inline elf_pointer mono_list_level_start(const int level) const {
        if(level==FIRST_DIM){
            return 0;
        }
        return levels_mono_lists.at(level-1);
    }

    inline elf_pointer mono_list_level_stop(const int level) const {
        return levels_mono_lists.at(level);
    }

    inline  int length_monolist(const int level) const {
        return num_dim-level+1;
    }

    inline int get_node_size(elf_pointer start_node) const {
        if(Elf::SAVE_MODE) {
            if(!is_node_length_offset(start_node)){
                cout << "Error: intGetNodeLength() no length at this offset" << endl;
            }
        }
        if(LOG_COST){read_cost++;}

        const int length = values.at(start_node);
        return length & RECOVER_MASK;//un mask
    }

    inline int get_value(elf_pointer elem_offset) const {
        if(LOG_COST) {read_cost++;}
        return values.at(elem_offset);
    }
    inline int get_pointer(elf_pointer elem_offset) const {
        if(LOG_COST) {read_cost++;}
        return pointer.at(elem_offset);
    }

    /**
    * This is the method collecting al the tuple ids in the subtree. In this implementation it simply traverses the entire
    * tree in pre-order fashion.
    * Overwrite for more advanced algorithms.
    *
    * @param elf
    * @param start_node
    * @param tids
    * @param level
    */
    void get_tids(const elf_pointer start_node, Synopsis& tids, const int level) const {
        const int length = get_node_size(start_node);
        const elf_pointer offset_first_elem = start_node+NODE_HEAD_LENGTH;

        for(int elem=0;elem<length;elem++){
            const elf_pointer elem_offset = offset_first_elem+elem;
            elf_pointer elem_pointer = get_pointer(elem_offset);
            if(elem_pointer>NOT_FOUND){//normal dim list
                get_tids(elem_pointer,tids,level+1);
            }else{//mono list
                elem_pointer&=RECOVER_MASK;
                int tid = get_tid_from_monolist(elem_pointer, level+1);
                if(LOG_COST){write_cost++;}
                tids.add(tid);
            }
        }
    }

    inline int get_tid_from_monolist(const elf_pointer start_list, const int start_level) const {
        if(LOG_COST){read_cost++;}
        return mono_lists.at(start_list+num_dim-start_level);
    }

    inline bool points_to_monolist(const elf_pointer pointer) const {
        return (pointer<0) ? true : false;//msb is set, i.e., pointer is negative
    }
    /**
     * Scans the entire rest of the predicate starting at columns[rest_to_check]. Also adds the tid iff the predicate is satisfied.
     * @param elf - the table
     * @param start_list
     * @param start_level
     * @param columns
     * @param predicates
     * @param rest_to_check
     * @return
     */
    inline void select_monolist_add_tid(const elf_pointer start_list, const int start_level
            , const vector<int>& columns, const vector<vector<int>>& predicates, const int rest_to_check
            , Synopsis& result_tids
    ) const {
        if(select_monolist(start_list, start_level, columns, predicates, rest_to_check)) {
            int tid = get_tid_from_monolist(start_list, start_level);
            result_tids.add(tid);
            if(LOG_COST){write_cost++;}
        }
    }
    /**
     * Scans the entire rest of the predicate starting at columns[rest_to_check]
     * @param elf - the table
     * @param start_list
     * @param start_level
     * @param columns
     * @param predicates
     * @param rest_to_check
     * @return
     */
    inline bool select_monolist(const int start_list, const int start_level
            , const vector<int>& columns, const vector<vector<int>>& predicates, const int rest_to_check
    ) const {
        if(SAVE_MODE) {
            if(get_monolist_level(start_list)!=start_level) {
                cout << "Error: select_monolist() Pointer does not fit designated level: level=" << start_level << " pointer->" << get_monolist_level(start_list) << endl;
            }
        }
        bool survives = true;
        for(int p=rest_to_check;p<columns.size();p++) {
            const int value = get_attribute_monolist(start_list, start_level, columns.at(p));
            survives &= Util::isIn(value, predicates.at(p).at(0), predicates.at(p).at(1));
            //if(LOG_COST){read_cost+=num_dim-start_level;}//XXX we only count the number of attributes accessed. this is done in get_attribute_monolist()
        }
        return survives;
    }

    inline int get_monolist_level(const elf_pointer start_list) const {
        for(int dim=0;dim<num_dim;dim++){
            if(start_list<levels_mono_lists.at(dim)){//this the difference to getPointerLevel(int)
                return dim;
            }
        }
        if(SAVE_MODE) {cout << "Error: getPointerLevel(): no valid pointer " << start_list << endl;}
        return NOT_FOUND;//no valid pointer
    }

    /**
	 * Determines the levels of the dimension list starting at *start_list*.
	 * [SAVE_MODE] If it cannot be a dimension list, NOT_FOUND is returned.
	 * @param start_list
	 * @return
	 */
   inline int get_level(const elf_pointer start_list) const {
        for(int dim=0;dim<num_dim;dim++){
            if(start_list<levels.at(dim)){
                return dim;
            }
        }
        if(SAVE_MODE) {
            cout << "Error: get_level(): no valid pointer " << start_list << endl;
        }
        return NOT_FOUND;//no valid pointer
    }

    void scan_next_level_preorder(const elf_pointer node_offset, const int level
            , const vector<int>& columns, const vector<vector<int>>& predicates, const int current_predicate
            , Synopsis& result_tids
    ) const {
        if(SAVE_MODE) {
            if(get_level(node_offset)!=level) {
                cout << "Error: scan_next_level_preorder() level(node_offset)!=level" << endl;
            }
        }
        /********************************************************************************
         *  There two major cases here: (1) we hit a level with a selection or 			*
         *  (2) we simply need to descend, because there is no selection in this level. *
         *  The latter one is the else path of the most outer if.						*
         ********************************************************************************/
        int next_level_for_select = columns.at(current_predicate);
        if(next_level_for_select == level) {//There is a selection in this level
            const int size  = get_node_size(node_offset);
            const int lower = predicates.at(current_predicate).at(0);
            const int upper = predicates.at(current_predicate).at(1);

            /***********************************************************
             * In case there is a selection on this level: There are again two cases:
             * (1.1) It is the final selection. Everything that survives is part of the result.
             * (1.2) There is at least one selection on  a deeper level remaining, i.e., we need to recursively call this function.
             **********************************************************/
            const int last_selected_level = columns.at(columns.size()-1);
            const bool is_final_selection = next_level_for_select == last_selected_level;
            if(is_final_selection) {
                /***********************************************************
                 * Case (1.1) - This is the final selection: check the predicate and collect tids
                 **********************************************************/
                for(int i=0;i<size;i++) {
                    const elf_pointer elem_offset = node_offset+NODE_HEAD_LENGTH+i;
                    const int elem_value  = get_value(elem_offset);

                    if(Util::isIn(elem_value, lower, upper)){//Check predicate
                        elf_pointer elem_pointer = get_pointer(elem_offset);
                        if(points_to_monolist(elem_pointer)) {
                            elem_pointer &= RECOVER_MASK;
                            select_monolist_add_tid(elem_pointer, level+1, columns, predicates, current_predicate+1, result_tids);//scan entire rest of
                        }else{
                            get_tids(elem_pointer, result_tids, level+1);
                        }
                    }
                }
            }else{
                /***********************************************************
                 * Case (1.2) this is not the final selection: check predicate and recursively call this function.
                 ***********************************************************/
                for(int i=0;i<size;i++) {
                    const elf_pointer elem_offset = node_offset+NODE_HEAD_LENGTH+i;
                    const int elem_value  = get_value(elem_offset);
                    if(Util::isIn(elem_value, lower, upper)){//Check predicate
                        elf_pointer elem_pointer = get_pointer(elem_offset);
                        if(points_to_monolist(elem_pointer)) {
                            elem_pointer &= RECOVER_MASK;
                            select_monolist_add_tid(elem_pointer, level+1, columns, predicates, current_predicate+1, result_tids);//scan entire rest of
                        }else{
                            scan_next_level_preorder(elem_pointer, level+1, columns, predicates, current_predicate+1, result_tids);//Recursive call
                        }
                    }
                }
            }
            /***********************************************************
             *  There two major cases here: (1) we hit a level with a selection or
             *  (2) we simply need to descend, because there is no selection in this level.
             *  The latter one is the else path below.
             **********************************************************/
        }else{//There is no selection, simply descent
            int size = get_node_size(node_offset);
            for(int i=0;i<size;i++) {
                const elf_pointer elem_offset = node_offset+NODE_HEAD_LENGTH+i;
                elf_pointer elem_pointer = get_pointer(elem_offset);
                if(points_to_monolist(elem_pointer)) {
                    elem_pointer &= RECOVER_MASK;
                    select_monolist_add_tid(elem_pointer, level+1, columns, predicates, current_predicate, result_tids);//scan entire rest of
                }else{
                    scan_next_level_preorder(elem_pointer, level+1, columns, predicates, current_predicate, result_tids);
                }
            }
        }
    }

    /**
     * Called upon Preorder traversal
     *
     * Scans one Dim List (node) whether its elements satisfy the predicate. For all elements that do satisfy the predicate the function...
     * (1) case: MonoList - adds the tid to result_tids in case it satisfies also all remaining predicates
     * (2) case: normal Dim List: it calls scan_level to scan the other predicates as well
     *
     * @param start_list
     * @param level
     * @param result_tids
     * @param columns
     * @param predicates
     * @param predicate_index
     * @return
     */
    elf_pointer scan_node_mcsp_preorder(
            const elf_pointer start_list, const int level
            , Synopsis& result_tids
            , const vector<int>& columns, const vector<vector<int>>& predicates, const int predicate_index
    ) const {
        if(SAVE_MODE) {
            if(get_level(start_list)!=level) {
                cout << "Error: Elf.SAVE_MODE - scan_node_mcsp() level(start_list) != level" << endl;
            }
            if(!(get_value(start_list)<0)) {// I masked the lengths for safety
                cout << "Error: Elf.SAVE_MODE - scan_node_mcsp() not this.values[start_list] < 0 " << get_value(start_list) << endl;
            }
        }
        const int length = get_node_size(start_list);
        const int lower  = predicates.at(predicate_index).at(DatabaseSystem::LOWER);
        const int upper  = predicates.at(predicate_index).at(DatabaseSystem::UPPER);

        for(int elem=0;elem<length;elem++){
            const int elem_offset = start_list+NODE_HEAD_LENGTH+elem;//+1 for length
            const int my_val      = get_value(elem_offset);

            if(Util::isIn(my_val, lower, upper)) {
                elf_pointer my_pointer = get_pointer(elem_offset);

                if(points_to_monolist(my_pointer)){
                    my_pointer&=RECOVER_MASK;
                    //we checked the predicate in this level, check the rest as well. That is why we need +1 for level and predicate_index
                    select_monolist_add_tid(my_pointer, level+1, columns, predicates, predicate_index+1, result_tids);
                }else{//normal dim list
                    scan_next_level_preorder(my_pointer, level+1, columns, predicates, predicate_index+1, result_tids);//Recursive call
                }
            }
        }
        return start_list+NODE_HEAD_LENGTH+length;//s.t. it points to the next node
    }

public:
    //Specific infos for this Elf
    static const int NODE_HEAD_LENGTH = 1;

    Elf_table_lvl_seperate(
            string& name
            , vector<string>& col_names
            , vector<int32_t>& _values
            , vector<elf_pointer>& _pointer
            , vector<int32_t>& _mono_lists
            , vector<int32_t>& _levels
            , vector<int32_t>& _levels_mono_lists
            , int32_t _num_dim
            ) :
            Elf(name, col_names)
            , values(_values)           // call copy constructor
            , pointer(_pointer)         // call copy constructor
            , mono_lists(_mono_lists)   // call copy constructor
            , levels(_levels)           // call copy constructor
            , levels_mono_lists(_levels_mono_lists) // call copy constructor
            , num_dim(_num_dim)
    {
        //Nothing to do
        cout << "values.size()=" << values.size() << "pointer.size()=" << pointer.size() << "mono_lists.size()=" << mono_lists.size() << endl;
        cout << "levels="<< Util::to_string(levels) << "levels_mono_lists=" << Util::to_string(levels_mono_lists) << endl;
    }

    int exists(vector<int>& query) const {
        int val = query.at(FIRST_DIM);
        elf_pointer pointer = get_pointer(val);
        if(pointer==EMPTY_ROOT_NODE){
            return NOT_FOUND;
        }else if(pointer>NOT_FOUND){//normal dim list
            return exists(pointer,query,FIRST_DIM+1);
        }else{//mono list
            pointer&=RECOVER_MASK;
            return exists_mono(pointer,query,FIRST_DIM+1);
        }
    }
    int size(){
        return 0;//XXX
    }

    inline elf_pointer level_start(const int level) const {
        return (level==FIRST_DIM) ? 0 : levels.at(level-1);
    }

    inline elf_pointer level_stop(const int level) const {
        return levels.at(level);
    }

    /*********************************** Begin mono column selection stuff *****************************************/


    /**
     * Jumps to the first elem in the desired range (of the first level). The desired element equals [lower,...]
     * For each elem in [lower,upper] collects all tuple ids in the corresponding sub tree by traversing the tree in pre-order-like fashion.
     *
     * @param lower - [lower,upper]
     * @param upper - [lower,upper]
     * @param tids - the result buffer
     * @return - a result buffer containing all point ids satisfying the int predicate.
     */
    void select_1_first_dim(const int lower, const int upper, Synopsis& tids) const {
        for(int i=lower;i<=upper;i++) {
            const elf_pointer elem_pointer = get_pointer(i);
            if(elem_pointer != EMPTY_ROOT_NODE) {
                get_tids(elem_pointer, tids, FIRST_DIM+1);
            }
        }
    }

    /**
     * For mono column selection predicates.
     * @param level
     * @param lower
     * @param upper
     * @param tids
     * @param dim_to_return
     */
    void scan_mono_list_level(const int level, const int lower, const int upper, Synopsis& tids, const int dim_to_return) const {
        const elf_pointer start_level = mono_list_level_start(level);
        const elf_pointer stop_level  = mono_list_level_stop(level);

        const int list_length = length_monolist(level);

        for(elf_pointer list_offset = start_level;list_offset<stop_level;list_offset+=list_length){
            const int val = get_attribute_monolist(list_offset, level, dim_to_return);
            if(Util::isIn(val, lower, upper)){
                int tid = get_tid_from_monolist(list_offset, level);
                if(LOG_COST){write_cost++;}
                tids.add(tid);
            }
        }
    }

    /**
     * Scans an entire node - element by element - determines corresponding tids immediately via pre-order traversal of Elf
     * iff predicate is satisfied.
     * @param elf
     * @param start_node
     * @param tids
     * @param lower
     * @param upper
     * @param level
     * @return
     */
    elf_pointer scan_1_node(const elf_pointer start_node, Synopsis& tids, const int lower, const int upper, const int level) const {
        const int length = get_node_size(start_node);
        const int offset_first_elem = start_node+NODE_HEAD_LENGTH;

        for(int elem=0;elem<length;elem++){
            const int elem_offset = offset_first_elem+elem;
            const int elem_val    = get_value(elem_offset);
            if(Util::isIn(elem_val, lower, upper)) {
                elf_pointer elem_pointer = get_pointer(elem_offset);
                if(elem_pointer>NOT_FOUND){//normal dim list // TODO exchenge with functions s.t. that I have only a single point of failure
                    get_tids(elem_pointer, tids, level+1);
                }else{//mono list
                    elem_pointer&=RECOVER_MASK;
                    int tid = get_tid_from_monolist(elem_pointer, level+1);
                    tids.add(tid);
                    if(LOG_COST){write_cost++;}

                }
            }
        }
        return start_node+NODE_HEAD_LENGTH+length;//s.t. it points to the next node
    }

    /*********************************** Begin multi column selection stuff *****************************************/

    void select_first_dim_preorder(const vector<int>& columns, const vector<vector<int>>& predicates, Synopsis& result_tids) const {
        if(SAVE_MODE){
            if(columns.at(0) != FIRST_DIM){
                cout << "Error: select_c level != columns[check_predicate]" << endl;
            }
        }

        const int lower = predicates.at(FIRST_DIM).at(DatabaseSystem::LOWER);
        const int upper = predicates.at(FIRST_DIM).at(DatabaseSystem::UPPER);

        for(elf_pointer offset=lower;offset<=upper;offset++){
            elf_pointer pointerNextDim = get_pointer(offset);

            if(pointerNextDim!=EMPTY_ROOT_NODE){//non-dense data in first dimension
                if(points_to_monolist(pointerNextDim)){
                    //check entire rest of predicate
                    pointerNextDim&=RECOVER_MASK;
                    select_monolist_add_tid(pointerNextDim, FIRST_DIM+1, columns, predicates, FIRST_DIM+1,result_tids);
                }else{
                    scan_next_level_preorder(pointerNextDim, FIRST_DIM+1
                            , columns, predicates, 1
                            , result_tids
                    );
                }
            }
        }
    }

    /**
     * For MCSPs
     * @param columns
     * @param predicates
     * @param result_tids
     */
    void select_mono_lists_until_first_predicate(const vector<int>& columns, const vector<vector<int>>& predicates, Synopsis& result_tids) const {
        //check all before the first predicate
        for(int level=FIRST_DIM;level<=columns.at(0);level++){
            const elf_pointer start_level 	= mono_list_level_start(level);
            const elf_pointer stop_level 	= mono_list_level_stop(level);
            const int list_length 	        = length_monolist(level);

            if(start_level!=stop_level){//there is at least one list
                for(int list_offset = start_level;list_offset<stop_level;list_offset+=list_length){
                    if(SAVE_MODE){
                        if(list_offset == 9001456 || list_offset == 9001470){
                            cout << "select_mono_lists_until_first_predicate() list_offset=" << list_offset<< endl;
                        }
                    }
                    select_monolist_add_tid(list_offset, level, columns, predicates, 0, result_tids);
                }
            }
        }
    }

    void select_mcsp_level_preorder(const vector<int>& columns, const vector<vector<int>>& predicates, Synopsis& result_tids) {
        const int level = columns.at(0);

        const int stop_level = level_stop(level);
        int next_list_start  = level_start(level);

        //scan entire level, node by node.
        while (next_list_start<stop_level) {
            next_list_start = scan_node_mcsp_preorder(
                    next_list_start, level
                    , result_tids
                    , columns, predicates, FIRST_DIM
            );
        }
    }

    static Elf_table_lvl_seperate* from_file(double scale){
        if(exists(scale)) {
            string file_name = "..//data/" + to_string(scale) + "_elf_separate";
            cout << "from_file("+to_string(scale)+") reading from " << file_name <<  endl;
            vector<int> _levels;
            vector<int> _levels_mono_lists;
            {
                vector<int> meta_info = Util::read_file(file_name + ".meta.elf");
                int num_dim_read = meta_info.at(0);
                if(num_dim_read!=Util::NUM_DIM_TPCH){
                    cout << "Error elf.from_file() num_dim_read!=Util::NUM_DIM_TPCH" << endl;
                }
                //TODO copy level info [2,num_dim+2)
                for(int i=2;i<2+num_dim_read;i++){
                    _levels.push_back(meta_info.at(i));
                }
                //TODO copy level mono list level info
                for(int i=2+num_dim_read;i<2+num_dim_read+num_dim_read;i++){
                    _levels_mono_lists.push_back(meta_info.at(i));
                }
            }
            vector<int> _values      = Util::read_file(file_name + ".values.elf");
            vector<int> _pointer     = Util::read_file(file_name + ".pointer.elf");
            vector<int> _mono_lists  = Util::read_file(file_name + ".mono_lists.elf");

            string name = "lineitem";
            vector<string> col_names = Util::get_tpch_linetime_column_names();

           /* cout <<"name="<< name << endl;
            cout <<"col_names="<< col_names.size() << endl;
            cout <<"_values="<< _values.size() << endl;
            cout <<"_pointer="<< _pointer.size() << endl;
            cout <<"_mono_lists="<< _mono_lists.size() << endl;
            cout <<"_levels="<< _levels.size() << endl;
            cout <<"_levels_mono_lists="<< _levels_mono_lists.size() << endl;
*/
            Elf_table_lvl_seperate* elf =  new Elf_table_lvl_seperate(
                    name
                    , col_names
                    , _values
                    , _pointer
                    , _mono_lists
                    , _levels
                    , _levels_mono_lists
                    , Util::NUM_DIM_TPCH
                    );
            return elf;
        }else{
            cout << "Error called from_file(s="<< to_string(scale) << "), but file does not exist" << endl;
            return nullptr;
        }
    }

    static bool exists(double scale) {
        return false;
        string file = "..//data/"+to_string(scale)+"_elf_separate"+".meta.elf";
        ifstream f(file.c_str());
        return f.good();
    }

    static bool materialize(Elf_table_lvl_seperate& elf, double scale){
        if(!exists(scale)){
            //create folder
            cout << "materialize(db,double): Materializing s=" << scale << endl;
            to_file(elf, scale);
            return true;
        }
        return false;
    }

    static bool to_file(Elf_table_lvl_seperate& elf, double scale){
        string file_name = "..//data/"+to_string(scale)+"_elf_separate";
        {//write Meta File
            vector<int> buffer;
            //NUM_DIM
            buffer.push_back(elf.num_dim);
            //REAL_SIZE_FIRST_DIM
            buffer.push_back(0);//legacy info
            //levels
            for(int i : elf.levels){
                buffer.push_back(i);
            }
            //levels
            for(int i : elf.levels_mono_lists){
                buffer.push_back(i);
            }
            Util::write_file(file_name+".meta.elf", buffer);
        }
        vector<int> temp;

        temp = elf.values;
        Util::write_file(file_name+".values.elf",temp);

        temp = elf.pointer;
        Util::write_file(file_name+".pointer.elf",temp);

        temp = elf.mono_lists;
        Util::write_file(file_name+".mono_lists.elf",temp);

        /*
        Util::write_file(file_name,elf.values);
        Util::write_file(file_name,elf.values);*/
        return true;
    }
};



#endif //MY_MCSP_ELF_TABLE_LVL_SEPERATE_H
