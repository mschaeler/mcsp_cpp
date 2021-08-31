//
// Created by Martin on 24.08.2021.
//

#ifndef MY_MCSP_ELF_TABLE_LVL_CUTOFFS_H
#define MY_MCSP_ELF_TABLE_LVL_CUTOFFS_H

class Elf_Table_Lvl_Cutoffs : public Elf_table_lvl_seperate {

private:
    /** This are all tids in elf order */
    const vector<int> tids_in_elf_order;
    /** Indicates the level where mono list containing the corresponding point starts*/
    const vector<int> tids_level;//TODO byte[]?
    const int first_level_with_monolists;

public:
/*    Elf_Table_Lvl_Cutoffs(Elf_table_lvl_seperate& elf)
    : Elf_table_lvl_seperate(elf.my_name, elf.column_names, elf.values, elf.pointer , elf.mono_lists, elf.levels, elf.levels_mono_lists, elf.num_dim)
    {
        //Nothing to do
        cout << "values.size()=" << values.size() << "pointer.size()=" << pointer.size() << "mono_lists.size()=" << mono_lists.size() << endl;
        cout << "levels="<< Util::to_string(levels) << "levels_mono_lists=" << Util::to_string(levels_mono_lists) << endl;
    }
*/
    Elf_Table_Lvl_Cutoffs(
            string& name
            , vector<string>& col_names
            , vector<int32_t>& _values
            , vector<elf_pointer>& _pointer
            , vector<int32_t>& _mono_lists
            , vector<int32_t>& _levels
            , vector<int32_t>& _levels_mono_lists
            , int32_t _num_dim
            //Elf_Table_Lvl_Cutoffs specific
            , vector<int32_t>& _tids_in_elf_order
            , vector<int32_t>& _tids_level
    ): Elf_table_lvl_seperate(name, col_names, _values, _pointer , _mono_lists, _levels, _levels_mono_lists, _num_dim)
    , tids_in_elf_order(_tids_in_elf_order)
    , tids_level(_tids_level)
    , first_level_with_monolists(get_first_level_with_monolists(_mono_lists))
    {
        //Nothing to do
        cout << "values.size()=" << values.size() << "pointer.size()=" << pointer.size() << "mono_lists.size()=" << mono_lists.size() << endl;
        cout << "levels="<< Util::to_string(levels) << "levels_mono_lists=" << Util::to_string(levels_mono_lists) << endl;
        cout << "tids_in_elf_order.size()=" << tids_in_elf_order.size() << " tids_level.size()=" << tids_level.size() << endl;
    }

    void check_cutoffs(ColTable& table){
        cout << "check_cutoffs(ColTable&) checking tid order";
        const int num_dim = table.num_dim;
        vector<int> point(num_dim);
        auto sum_of_elems = std::accumulate(tids_in_elf_order.begin(), tids_in_elf_order.end(), 0);
        cout << " check_sum=" << sum_of_elems;
        sum_of_elems = std::accumulate(tids_level.begin(), tids_level.end(), 0);
        cout << " check_sum=" << sum_of_elems;
        for(int i=1;i<tids_in_elf_order.size();i++) {
            int my_tid = tids_in_elf_order.at(i);
            int s_tid = tids_in_elf_order.at(i - 1);

            for (int col = 0; col < num_dim; col++) {
                int my_val = table.columns.at(col).at(my_tid);
                int s_val = table.columns.at(col).at(s_tid);
                if (my_val > s_val) {
                    break;//all correct
                }

                if (my_val < s_val) {
                    table.copyPoint(s_tid, point);
                    string s1 = Util::to_string(point);

                    table.copyPoint(my_tid, point);
                    string s2 = Util::to_string(point);
                    cout << "order does not match at i=" << i << " " << s1 << " > " << s2 << endl;
                    break;
                }
            }
        }
        cout << " [Done]" << endl;
        cout << " checking cutoffs" << endl;
        for(elf_pointer elem_offset = 0;elem_offset<level_stop(0);elem_offset++){
            elf_pointer elem_pointer = get_pointer(elem_offset);
            if(points_to_monolist((elem_pointer))){
                cout << "Monolist in first level @" << elem_offset << endl;
            }else{
                check_cutoff_node(elem_pointer, 1);
            }
        }
    }

    /**
     * Jumps to the first elem in the desired range (of the first level). The desired element equals [lower,...]
     * For each elem in [lower,upper] collects all tuple ids in the corresponding sub tree by traversing the tree in pre-order-like fashion.
     *
     * @param lower - [lower,upper]
     * @param upper - [lower,upper]
     * @param tids - the result buffer
     * @return - a result buffer containing all point ids satisfying the int predicate.
     */
    void select_1_first_level(const int lower, const int upper, Synopsis& tids) const {
        elf_pointer elem_pointer = get_pointer(lower);
        if(elem_pointer == EMPTY_ROOT_NODE) {
            elem_pointer = get_pointer(lower+1);//TODO while
            if(SAVE_MODE) {
                if(elem_pointer == EMPTY_ROOT_NODE) {
                    cout << "select_mono_first_dim() from points to empty root node" << endl;
                }
            }
        }

        elf_pointer from = cutoff_start(elem_pointer);
        elf_pointer to;
        if(upper==level_stop(FIRST_DIM)-1) {
            to = tids_in_elf_order.size();
        } else {
            elem_pointer = get_pointer(upper+1);
            if(SAVE_MODE) {
                if(elem_pointer == EMPTY_ROOT_NODE) {
                    cout << "select_mono_first_dim() to points to empty root node" << endl;
                }
            }
            to = cutoff_start(elem_pointer);
        }

        for(int i=from;i<to;i++) {
            tids.add(tids_in_elf_order.at(i));
            if(LOG_COST) {write_cost++;}
        }
    }

    /********************************* Start mono columns selection Elf with Cutoffs **********************************/


    /**
     *
     * @param start_subtree_root - start of a dimension list.
     * @return
     */
    elf_pointer cutoff_start(const elf_pointer start_subtree_root) const {
        if (SAVE_MODE) {
            if (!is_node_length_offset(start_subtree_root)) {
                cout
                        << "cutoff_start(int) called for normal dim element. This function expects a dim list. For dim elements use cutoff_elem(int)."
                        << endl;
            }
        }
        const elf_pointer cutoff = pointer.at(start_subtree_root);
        if (LOG_COST) { read_cost++; }
        return cutoff;
    }

    /**
     * Scans the entire level and determines corresponding tids immediately via Cutoffs but no ranges.
     *
     * @param start_list
     * @param level_synopsis
     * @param tids
     * @param lower
     * @param upper
     * @param dimension
     * @return
     */
    void select_1(
            const int start_level
            , const int stop_level
            , const int level
            , Synopsis& tids
            , const int lower
            , const int upper
    ) const {
        elf_pointer offset = start_level;
        while (offset<stop_level) {//for each node in this level
            offset = select_1_node(offset, level, lower, upper, tids);
        }
    }
private:

    elf_pointer select_1_node(const elf_pointer node_offset, const int level, const int lower, const int upper, Synopsis& tids) const {//inlined node processing. I need to this, because I need access to the next node sometimes...
        const elf_pointer length = get_node_size(node_offset);
        /**
         * For each elem in this node, Except the last One. There the cutoff is different.
         * Thus length-1
         */
        for(int elem=0;elem<length-1;elem++){
            const elf_pointer elem_offset = node_offset+NODE_HEAD_LENGTH+elem;//+NODE_HEAD_LENGTH for explicit length
            int elem_val = get_value(elem_offset);
            if(Util::isIn(elem_val, lower, upper)) {
                collect_tids_cuttoff(elem_offset, elem_offset+1, level, tids);
            }
        }

        //Last elem separately, because the cuttoff is different. I cannot simply use the cuttoff of next level, due to non density issue
        const elf_pointer elem_offset 	= node_offset+NODE_HEAD_LENGTH+length-1;//+NODE_HEAD_LENGTH for explicit length
        int elem_val = get_value(elem_offset);

        if(Util::isIn(elem_val, lower, upper)) {
            int elem_pointer = get_pointer(elem_offset);
            if(points_to_monolist(elem_pointer)) {
                elem_pointer&=RECOVER_MASK;
                int tid = get_tid_from_monolist(elem_pointer, level+1);
                tids.add(tid);
                if(LOG_COST) {write_cost++;}
            }else {
                collect_tids_cuttoff_last_node_element(elem_offset, elem_pointer, level, tids);
            }
        }
        return node_offset + NODE_HEAD_LENGTH+length;//point to next elem
    }

    /**
     * Collects all tids referring to this subtree using cutoffs. It must not be called for the last element in a node.
     * Otherwise, we may see the non-density bug.
     *
     * @param elem_offset
     * @param next_elem_offset
     * @param elem_level
     * @param tids
     */
    void collect_tids_cuttoff(const int elem_offset,  const int next_elem_offset, const int elem_level, Synopsis& tids) const {
        if(SAVE_MODE) {
            int temp_value = values.at(next_elem_offset);
            if(temp_value<0) {//we hit the next node
                cout << "collect_tids_cuttoff() next elem appears not refer to an element, but a node head" << endl;
            }
        }
        /**
         * We are in a node and want to collect all tids in the sub tree below elem_offset (starting at get_pointer(elem_offset)).
         * That is all tids until the next tid run of next_elem_offset starts.
         *
         * We have two major cases then:
         * (1) get_pointer(elem_offset) is real tree (i.e., no mono list) then there are two sub-cases
         * (1.1) next_elem_offset also refers to real sub tree: get its cutoff and collect until it (exclusively) without checking the tids, we only need the offsets.
         * (1.2) next_elem_offset also refers to monolist: get its tid and collect everything until we see this tid (exclusively)
         * (2) elem_offset refers to a mono list, simply add the corresponding tid. XXX does this ever happen
         */
        int elem_pointer = get_pointer(elem_offset);
        if(elem_pointer>NOT_FOUND){//normal dim list
            elf_pointer next_pointer = get_pointer(next_elem_offset);
            if(points_to_monolist(next_pointer)) {
                get_tids_elem_next_elem_is_monolist(elem_pointer, next_pointer, elem_level, tids);
            } else {
                get_tids_elem(elem_pointer, next_pointer, tids);
            }
        }else{//mono list
            elem_pointer&=RECOVER_MASK;
            int tid = get_tid_from_monolist(elem_pointer, elem_level+1);
            tids.add(tid);
            if(LOG_COST) {write_cost++;}
        }
    }

    void get_tids_elem(const elf_pointer elem_pointer, const elf_pointer next_elem_pointer, Synopsis& result_tids) const {
        if(SAVE_MODE) {
            if(points_to_monolist(elem_pointer)) {
                cout << "get_tids_elem(int,int,MyArrayLis) pointsToMonoList(elem_pointer)" << endl;
            }
            if(points_to_monolist(next_elem_pointer)) {
                cout << "get_tids_elem(int,int,MyArrayLis) pointsToMonoList(next_elem_pointer)" << endl;
            }
        }

        /** Index in this.tids_in_elf_order[] of the first tid in  this sub tree.*/
        const elf_pointer from 	= cutoff_start(elem_pointer);
        /** Index of the first tid in the next sub tree. So, we need to iterate until this one.*/
        const elf_pointer to 	= cutoff_start(next_elem_pointer);

        for (elf_pointer i = from; i < to; i++) {
            result_tids.add(tids_in_elf_order.at(i));
            if(LOG_COST){write_cost++; read_cost++;}//XXX here the read cost may be a problem, but i think it is fair this way. similar as for indexed MonetDB
        }
    }

    void get_tids_elem_next_elem_is_monolist(
            const elf_pointer elem_pointer
            , elf_pointer next_elem_pointer
            , const int elem_level
            , Synopsis& result_tids
    ) const {
        if(SAVE_MODE) {
            if(points_to_monolist(elem_pointer)) {
                cout << "get_tids_elem(int,int,MyArrayLis) pointsToMonoList(elem_pointer)" << endl;
            }
            if(!points_to_monolist(next_elem_pointer)) {
                cout << "get_tids_elem(int,int,MyArrayLis) *not* pointsToMonoList(next_elem_pointer)" << endl;
            }
        }

        /** Index in this.tids_in_elf_order[] of the first tid in  this sub tree.*/
        const elf_pointer from = cutoff_start(elem_pointer);

        next_elem_pointer &= RECOVER_MASK;
        const elf_pointer first_non_result_tid = get_tid_from_monolist(next_elem_pointer, elem_level+1);
        elf_pointer i = from;
        int tid;
        //collect all tids until we hit the first tid of next subtree
        while((tid = tids_in_elf_order.at(i))!=first_non_result_tid) {
            result_tids.add(tid);
            if(LOG_COST){write_cost++; read_cost++;}//XXX here the read cost may be a problem, but i think it is fair this way. similar as for indexed MonetDB
            i++;
        }
    }

    void collect_tids_cuttoff_last_node_element(const elf_pointer elem_offset, const elf_pointer node_offset, const int level, Synopsis& result_tids) const {
        if(SAVE_MODE) {
            int temp_value = values.at(node_offset);
            if(temp_value>-1) {//we hit the next node
                cout << "collect_tids_cuttoff_last_node_element() no node head passed" << endl;
            }

            if(!is_last_elem_in_level(elem_offset, level)) {
                int next_node_start = elem_offset +1;
                if(!is_node_length_offset(next_node_start)) {
                    cout << "collect_tids_cuttoff_last_node_element() !is_node_length_offset(next_node_start)" << endl;
                }
            }
        }

        if(is_last_elem_in_level(elem_offset, level)) {
            //System.err.println("collect_tids_cuttoff_last_node_element() last elem in level happens");
            get_tids_until_level_end(node_offset, result_tids, level);
        } else {
            get_tids_elem_abort(node_offset, elem_offset+1, result_tids, level);
        }

        /*int last_elem_in_next_level=node_offset+get_node_size(node_offset);//jump to last element in root of subtree
        int last_tid_in_subtree = get_last_tid_in_subtree(last_elem_in_next_level, level+1);
        this.myCutoffs.get_tids_elem_with_known_last_tid_in_subtree(node_offset, last_tid_in_subtree, result_tids);
        */
    }

    void get_tids_until_level_end(const elf_pointer node_offset, Synopsis& result_tids, const int level) const {
        /** Index in this.tids_in_elf_order[] of the first tid in  this sub tree.*/
        const int from 	= cutoff_start(node_offset);
        const int to 	= tids_in_elf_order.size();
        if(level<first_level_with_monolists){
            for (int i = from; i < to; i++) {
                result_tids.add(tids_in_elf_order.at(i));
                if(LOG_COST){read_cost+=1;}//XXX +1 we read tids_level  array and write the tid (so we do not really look at the tid)
            }
        }else{
            for (int i = from; i < to; i++) {
                if(tids_level.at(i)>level) {//we look for all tids deeper than this level
                    result_tids.add(tids_in_elf_order.at(i));
                    if(LOG_COST){write_cost++;}
                }
                if(LOG_COST){read_cost+=1;}//XXX +1 we read tids_level  array and write the tid (so we do not really look at the tid)
            }
        }
    }

    /**
     * Used only for collect_tids_cuttoff_last_node_element()
     * Aborts scanning if either to index is reached, or we see the first tid from a to high level.
     */
    void get_tids_elem_abort(const elf_pointer elem_pointer, const elf_pointer next_elem_pointer, Synopsis& result_tids, const int level) const {
        if(SAVE_MODE) {
            if(points_to_monolist(elem_pointer)) {
                cout << "get_tids_elem(int,int,MyArrayLis) pointsToMonoList(elem_pointer)" << endl;
            }
            if(points_to_monolist(next_elem_pointer)) {
                cout << "get_tids_elem(int,int,MyArrayLis) pointsToMonoList(next_elem_pointer)" << endl;
            }
            //if(getLevel(elem_pointer)!=level) {
            //	System.err.println("get_tids_elem() getLevel(elem_pointer="+elem_pointer+")="+getLevel(elem_pointer)+" but should be "+level);
            //}
            //if(getLevel(next_elem_pointer)!=level) {
            //	System.err.println("get_tids_elem() getLevel(next_elem_pointer="+next_elem_pointer+")="+getLevel(next_elem_pointer)+" but should be "+level);
            //}
        }

        /** Index in this.tids_in_elf_order[] of the first tid in  this sub tree.*/
        const elf_pointer from 	= cutoff_start(elem_pointer);
        /** Index of the first tid in the next sub tree. So, we need to iterate until this one.*/
        const elf_pointer to 	= cutoff_start(next_elem_pointer);

        for (int i = from; i < to; i++) {
            if(tids_level.at(i)>level) {
                result_tids.add(tids_in_elf_order.at(i));
                if(LOG_COST){write_cost++;}
            }else {
                return;
            }
            if(LOG_COST){read_cost+=2;}//XXX +2 we read the tid array and the level array
        }
    }

    /********************************* Start mono columns selection Elf with Cutoffs AND Ranges **********************************/
public:
    /**
     * Scans the entire level and determines corresponding tids immediately via Cutoffs and ranges.
     * What is special about this function is that the non-density bug cannot occur as there are no mono lists that we do not see until this level.
     *
     * @param start_list
     * @param level_synopsis
     * @param tids
     * @param lower
     * @param upper
     * @param dimension
     * @return
     */
    void scan_mono_column_cutoff_ranges(
            const elf_pointer start_level, const elf_pointer stop_level, const int level
            , Synopsis& tids, const int lower, const int upper
    ) const {
        if(SAVE_MODE) {
            if(!(get_first_level_with_monolists()>level)) {
                cout << "scan_mono_level_preorder_cutoff_ranges_no_grnaularity() !get_first_level_with_monolists()>level" << endl;
            }
        }

        elf_pointer elem_offset = start_level;
        elf_pointer elem_pointer_where_run_start=-1;
        /** Flag indicating that there is an un-materialized result run.*/
        bool materialized = false;

        /*******************************
         * for each node in this level *
         *******************************/
        while (elem_offset<stop_level) {
            /***************************************************************
             * (1) First inner while for finding the start of a result run *
             ***************************************************************/
            while (elem_offset<stop_level) {
                const int elem_val = get_value(elem_offset);
                if(!is_node_length(elem_val)) {//ignore node heads. Here we find the length of the node, with its MSB set.
                    if(Util::isIn(elem_val, lower, upper)) {
                        // We found a result run
                        elf_pointer elem_pointer = get_pointer(elem_offset);
                        if(points_to_monolist(elem_pointer)) {
                            // We do not start result runs at MonoLists, since they don't have cutoffs in my implementation.
                            elem_pointer&=RECOVER_MASK;
                            int tid = get_tid_from_monolist(elem_pointer, level+1);
                            tids.add(tid);
                            if(LOG_COST) {write_cost++;}
                        }else{
                            elem_pointer_where_run_start = elem_pointer;
                            materialized = false;
                            elem_offset++;//next elem
                            break;//Go to second inner while to find the end of the run. I know it is break...
                        }
                    }
                }
                elem_offset++;//next elem
            }
            /***************************************************************
             * (2) Second inner while for finding the end of a result run *
             ***************************************************************/
            while (elem_offset<stop_level) {
                const int elem_val = get_value(elem_offset);
                if(!is_node_length(elem_val)) {//ignore node heads. Here we find the length of the node, with its MSB set.
                    if(!Util::isIn(elem_val, lower, upper)) {
                        materialize_run(elem_pointer_where_run_start, elem_offset, level, tids);
                        materialized = true;
                        elem_offset++;//next elem
                        break;//go to first while
                    }
                }
                elem_offset++;//next elem
            }
        }
        if(!materialized && elem_pointer_where_run_start != -1) {//the last run lasts until level end
            get_tids_until_level_end(elem_pointer_where_run_start, tids, level);
        }
    }
private:
    /**************************************************************************************************************************
     * Ok, we have a result run starting @elem_pointer_where_run_start. The first element not in the run is elem_offset.      *
     * Let's materialize it. elem_offset may refer to a normal element or a mono list. So, we have two cases:                 *
     * (1) elem_offset is MonoList start: determine its tid and then copy every tid from cutoff(elem_pointer_where_run_start) *
     * until hitting that tid (exclusively)                                                                                   *
     * (2) elem_offset is normal node: default case, use both cutoffs and copy the tids between.							  *
     **************************************************************************************************************************/
    void materialize_run(const elf_pointer elem_pointer_where_run_start, const elf_pointer elem_offset, const int level, Synopsis& tids) const {
        int first_tid_not_in_run;
        elf_pointer elem_pointer = get_pointer(elem_offset);
        if(points_to_monolist(elem_pointer)) {
            elem_pointer&=RECOVER_MASK;
            first_tid_not_in_run = get_tid_from_monolist(elem_pointer, level+1);
            get_tids_elem_with_known_first_tid_not_in_result(elem_pointer_where_run_start, first_tid_not_in_run, tids, level);
        }else{
            if(level < first_level_with_monolists){
                get_tids_elem(elem_pointer_where_run_start, elem_pointer, tids);
            }else{
                get_tids_elem(elem_pointer_where_run_start, elem_pointer, tids, level);
            }
        }
    }

    void get_tids_elem_with_known_first_tid_not_in_result(const elf_pointer elem_pointer, const elf_pointer first_tid_not_in_result, Synopsis& result_tids, const int level) const {
        if(SAVE_MODE) {
            if(points_to_monolist(elem_pointer)) {
                cout << "get_tids_elem(int,int,MyArrayLis) pointsToMonoList(elem_pointer)" << endl;
            }
        }
        /** Index in this.tids_in_elf_order[] of the first tid in  this sub tree.*/
        const elf_pointer from 	= cutoff_start(elem_pointer);
        elf_pointer i = from;
        int tid;

        while((tid = tids_in_elf_order.at(i)) !=first_tid_not_in_result) {
            if(tids_level.at(i)>level) {
                result_tids.add(tid);
                if(LOG_COST){write_cost++;}
            }
            if(LOG_COST){read_cost+=2;}//XXX +2 we read the tid array and the level array
            i++;
        };
    }

    /*
     * In contrast to its sibling method get_tids_elem(elf_pointer,elf_pointer,Synopsis&), i.e., without int level, we also check whether
     * tids_level.at(i)>level. This way we avoid the non-density bug.
     */
    void get_tids_elem(const elf_pointer elem_pointer, const elf_pointer next_elem_pointer, Synopsis& result_tids, const int level) const {
        if(SAVE_MODE) {
            if(points_to_monolist(elem_pointer)) {
                cout << "get_tids_elem(int,int,MyArrayLis) pointsToMonoList(elem_pointer)" << endl;
            }
            if(points_to_monolist(next_elem_pointer)) {
                cout << "get_tids_elem(int,int,MyArrayLis) pointsToMonoList(next_elem_pointer)" << endl;
            }
        }

        /** Index in this.tids_in_elf_order[] of the first tid in  this sub tree.*/
        const elf_pointer from 	= cutoff_start(elem_pointer);
        /** Index of the first tid in the next sub tree. So, we need to iterate until this one.*/
        const elf_pointer to 	= cutoff_start(next_elem_pointer);

        for (int i = from; i < to; i++) {
            if(tids_level.at(i)>level) {
                result_tids.add(tids_in_elf_order.at(i));
                if(LOG_COST){write_cost++;}
            }
            if(LOG_COST){read_cost+=2;}//XXX +2 we read the tid array and the level array
        }
    }
    /********************************* Start misc functions **********************************/

    /**
     * Returns the first level having mono list. The pointer to this mono list in the level above. The main reason for this function is to avoid the non-density bug.
     * @return
     */
    int get_first_level_with_monolists() const {
        for(int level=1;level<num_dim;level++) {
            if(mono_list_level_stop(level)>0) {
                return level;
            }
        }
        cout << "get_first_level_with_monolists() - did not find first elvel with mono lists" << endl;
        return NOT_FOUND;
    }

    /**
     * For Elf::SAVE_MODE
     * @param elem_offset
     * @param elem_level
     * @return
     */
    inline bool is_last_elem_in_node(const elf_pointer elem_offset, const int elem_level) const {
        if(is_last_elem_in_level(elem_offset, elem_level)) {
            return true;
        }
        return (values.at(elem_offset+1)<0);//length of next node has MSB set
    }

    inline bool is_last_elem_in_level(const elf_pointer elem_offset, const int elem_level) const {
        return level_stop(elem_level)==elem_offset+1;
    }

    static inline bool is_node_length(const int value){
        return value<0;
    }

    void check_cutoff_node(const elf_pointer node_offset, const int my_level){
        if(!is_node_length_offset(node_offset)){
            cout << "Error get_first_tid_in_subtre() called for node_start not element@" << node_offset << endl;
        }
        const int node_length = get_node_length(node_offset);
        elf_pointer cutoff = cutoff_start(node_offset);
        int tid_by_cutoff = tids_in_elf_order.at(cutoff);
        int tid_by_traversal = get_first_tid_in_subtree(node_offset, my_level);
        if(tid_by_cutoff!=tid_by_traversal){
            cout << "Error tid_by_cutoff!=tid_by_traversal @" << node_offset << " in level " << my_level << endl;
        }
        if(SAVE_MODE){
            if(tid_by_cutoff==tid_by_traversal){
                cout << "tid_by_cutoff=tid_by_traversal @" << node_offset << " in level " << my_level << endl;
            }
        }

        for(int elem=0;elem<node_length;elem++){
            elf_pointer elem_offset = node_offset+1+elem;//+1 for length
            elf_pointer elem_pointer = get_pointer(elem_offset);//sub tree root node
            if(!points_to_monolist(elem_pointer)){
                check_cutoff_node(elem_pointer,my_level+1);
            }
        }
    }

    int get_first_tid_in_subtree(const elf_pointer node_offset, const int my_level){
        if(!is_node_length_offset(node_offset)){
            cout << "Error get_first_tid_in_subtre() called for normal element@" << node_offset << endl;
        }
        elf_pointer elem_pointer = get_pointer(node_offset+1);//first element in this node
        if(points_to_monolist(elem_pointer)){
            elem_pointer &= RECOVER_MASK;
            int tid = get_tid_from_monolist(elem_pointer, my_level+1);
            return tid;
        }else{
            return get_first_tid_in_subtree(elem_pointer, my_level+1);
        }
    }

    /**
    * Returns the first level having mono list. The pointer to this mono list in the level above. The main reason for this function is to avoid the non-density bug.
    * @return
     */
    static int get_first_level_with_monolists(const vector<int>& mono_list_level) {
        for(int level=1;level<mono_list_level.size();level++) {
            if(mono_list_level.at(level)>0) {
                return level;
            }
        }
        cout << "get_first_level_with_monolists() - did not find first level with mono lists" << endl;
        return NOT_FOUND;
    }
};


#endif //MY_MCSP_ELF_TABLE_LVL_CUTOFFS_H
