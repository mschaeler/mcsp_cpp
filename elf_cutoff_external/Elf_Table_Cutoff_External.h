//
// Created by Martin on 03.09.2021.
//

#ifndef MY_MCSP_ELF_TABLE_CUTOFF_EXTERNAL_H
#define MY_MCSP_ELF_TABLE_CUTOFF_EXTERNAL_H

#include "../elf/Elf_table_lvl_seperate.h"

class Elf_Table_Cutoff_External : public Elf_table_lvl_seperate{
public:
    /** This are all tids in elf order */
    const vector<int> tids_in_elf_order;
    /** Indicates the level where mono list containing the corresponding point starts*/
    const vector<int> tids_level;//TODO byte[]?
    const int first_level_with_monolists;
    const vector<int> cutoffs;

    static const bool USE_MEMCOPY = true;
    static const bool use_extended_cutoffs = false;

    Elf_Table_Cutoff_External(
            string& name
            , vector<string>& col_names
            , vector<int32_t>& _values
            , vector<elf_pointer>& _pointer
            , vector<int32_t>& _mono_lists
            , vector<elf_pointer>& _levels
            , vector<elf_pointer>& _levels_mono_lists
            , int32_t _num_dim
            //Elf_Table_Lvl_Cutoffs specific
            , vector<int32_t>& _tids_in_elf_order
            , vector<int32_t>& _tids_level
            , vector<int32_t>& _cutoffs
    ): Elf_table_lvl_seperate(name, col_names, _values, _pointer , _mono_lists, _levels, _levels_mono_lists, _num_dim)
            , tids_in_elf_order(_tids_in_elf_order)
            , tids_level(_tids_level)
            , cutoffs(_cutoffs)
            , first_level_with_monolists(get_first_level_with_monolists(_levels_mono_lists))
    {
        cout << " cutoffs.size()=" << cutoffs.size() << "first level with monolists= "<<first_level_with_monolists<< endl;
    }

    void check_cutoffs() {
        cout << "check_cutoffs() ";
        for(int e=0;e<level_stop(FIRST_DIM);e++) {
            elf_pointer elem_pointer = get_pointer(e);
            if(elem_pointer != EMPTY_ROOT_NODE) {
                if(points_to_monolist(elem_pointer)) {
                    int tid = get_tid_from_monolist(elem_pointer, 1);
                    int position = cutoff(e);
                    if(tids_in_elf_order.at(position)!=tid) {
                        cout << "Cutoff worng @"<<e<<" cutoff="<<position << endl;
                    }
                } else {
                    check_cutoffs(elem_pointer, 1);
                }
            }
        }
        cout << "[Done]" << endl;
    }

    void select_1_cutoff_first_level(const int lower, const int upper, Synopsis& tids) const {
        const int from = cutoff(lower);
        int to;
        if(upper==level_stop(FIRST_DIM)-1) {
            to = tids_in_elf_order.size();
        }else {
            to = cutoff(upper+1);//cutoff of the next element
        }
        copy(from, to, tids);
    }

    void select_1_cutoff(
            const elf_pointer start_level, const elf_pointer stop_level, const int level
            , Synopsis& tids, const int lower, const int upper
    ) const {
        int offset = start_level;
        if(level < first_level_with_monolists){
            //Special case: if there no mono lists until my level, we do not need to tkae of them.
            while (offset<stop_level) {//for each node in this level
                offset = select_1_node_no_monolists_above(offset, level, lower, upper, tids);
            }
        }else{
            while (offset<stop_level) {//for each node in this level
                offset = select_1_node(offset, level, lower, upper, tids);
            }
        }
    }

    void select_1_ranges(
            const elf_pointer start_level, const elf_pointer stop_level, const int level
            , Synopsis& tids, const int lower, const int upper
    ) const {
        if(SAVE_MODE){
            bool no_monolists_above = level<first_level_with_monolists;//TODO case distinction
            if(no_monolists_above){
                cout << "no_monolists_above should call call select_1_ranges_no_monolists_above" << endl;
            }
        }
        elf_pointer elem_offset = start_level;
        int from = NOT_FOUND;

        /*******************************
         * for each node in this level *
         *******************************/
        while (elem_offset<stop_level) {
            /***************************************************************
             * (1) Find the start of a result run 						   *
             ***************************************************************/
            elem_offset = find_start_of_run(elem_offset, stop_level, lower, upper);
            if(elem_offset != NOT_FOUND) {
                from = cutoff(elem_offset);
                elem_offset++;
            }else{
                return;//there is no (more) run
            }

            /***************************************************************
             * (2) Find the end of a result run and materialize			   *
             ***************************************************************/
            elem_offset = find_stop_of_run(elem_offset, stop_level, lower, upper);
            if(elem_offset == stop_level) {
               copy(from, tids_in_elf_order.size(), level, tids);
            }else{
                const int to = cutoff(elem_offset);
                copy(from, to, level, tids);
            }
        }
    }
    void select_1_ranges_no_monolists_above(
            const elf_pointer start_level, const elf_pointer stop_level, const int level
            , Synopsis& tids, const int lower, const int upper
    ) const {
        if(SAVE_MODE){
            bool no_monolists_above = level<first_level_with_monolists;//TODO case distinction
            if(!no_monolists_above){
                cout << "there may be monolists above must call select_1_ranges()" << endl;
            }
        }
        elf_pointer elem_offset = start_level;
        int from = NOT_FOUND;

        /*******************************
         * for each node in this level *
         *******************************/
        while (elem_offset<stop_level) {
            /***************************************************************
             * (1) Find the start of a result run 						   *
             ***************************************************************/
            elem_offset = find_start_of_run(elem_offset, stop_level, lower, upper);
            if(elem_offset != NOT_FOUND) {
                from = cutoff(elem_offset);
                elem_offset++;
            }else{
                return;//there is no (more) run
            }

            /***************************************************************
             * (2) Find the end of a result run and materialize			   *
             ***************************************************************/
            elem_offset = find_stop_of_run(elem_offset, stop_level, lower, upper);
            if(elem_offset == stop_level) {
                copy(from, tids_in_elf_order.size(), tids);
            }else{
                const int to = cutoff(elem_offset);
                copy(from, to, tids);
            }
        }
    }

    void select_mono_lists_until_first_predicate(const vector<int>& columns, const vector<vector<int>>& predicates, Synopsis& tids
    ) const {
        //check all before the first predicate
        for(int level=FIRST_DIM;level<=columns.at(0);level++){
            const elf_pointer start_level = mono_list_level_start(level);
            const elf_pointer stop_level  = mono_list_level_stop(level);
            const int list_length 	      = length_monolist(level);

            if(start_level!=stop_level){//there is at least one list
                for(elf_pointer list_offset = start_level;list_offset<stop_level;list_offset+=list_length){
                    select_monolist_add_tid(list_offset, level, columns, predicates, 0, tids);
                }
            }
        }
    }

    /**
     * Distributes to the right physical operator
     * @param start_range
     * @param stop_range
     * @param level
     * @param tids
     * @param columns
     * @param predicates
     * @param predicate_index
     */
    void select_mcsp_ranges(
            const elf_pointer start_range, const elf_pointer stop_range, const int level
            , Synopsis& tids, const vector<int>& columns, const vector<vector<int>>& predicates, const int predicate_index
        ) const {
        if(SAVE_MODE) {
            if(!is_node_length_offset(start_range)) {
                cout << "select_mcsp_ranges() !is_node_length_offset(start_range)" << endl;
            }
            if(stop_range!=pointer.size() && !is_node_length_offset(stop_range)) {
                cout << "select_mcsp_ranges() !is_node_length_offset(stop_range)"<< endl;
            }
            if(get_level(start_range)!=level) {
                cout << "select_mcsp_ranges() getLevel(start_range)!=level"<< endl;
            }
            if(get_level(stop_range-1)!=level) {
                cout << "select_mcsp_ranges() getLevel(stop_range)!=level"<< endl;
            }
            if(level>columns.at(predicate_index)) {
                cout << "select_mcsp_ranges() level!=columns[predicate_index]"<< endl;
            }
        }
        const bool has_selection 	   = columns.at(predicate_index)==level;
        const int last_selected_level  = columns.at(columns.size()-1);
        const bool has_final_selection = level == last_selected_level;

        if(has_selection) {
            if(has_final_selection) {
                select_mcsp_ranges_final_selection(start_range, stop_range, level, tids, columns, predicates, predicate_index);
            }else{
                select_mcsp_ranges_non_terminal_selection(start_range, stop_range, level, tids, columns, predicates, predicate_index);
            }
        }else{
            select_mcsp_ranges_descent(start_range, stop_range, level, tids, columns, predicates, predicate_index);
        }
    }

    void select_mcsp_ranges_first_level(const vector<int>& columns, const vector<vector<int>>& predicates, Synopsis& tids) const {
        if(SAVE_MODE){
            if(columns.at(0) != FIRST_DIM){
                cout << "select_c level != columns[check_predicate]" << endl;
            }
        }

        const int lower = predicates.at(FIRST_DIM).at(0);
        const int upper = predicates.at(FIRST_DIM).at(1);
        const int level = FIRST_DIM;

        elf_pointer elem_pointer = get_pointer(lower);
        if(elem_pointer == EMPTY_ROOT_NODE) {
            elem_pointer = get_pointer(lower+1);//TODO while
            if(SAVE_MODE) {
                if(elem_pointer == EMPTY_ROOT_NODE) {
                    cout << "select_mono_first_dim() from points to empty root node" << endl;
                }
            }
        }

        elf_pointer start_range = elem_pointer;
        elf_pointer stop_range;
        if(upper==level_stop(level)-1) {
            stop_range = level_stop(level+1);
        } else {
            elem_pointer = get_pointer(upper+1);
            if(SAVE_MODE) {
                if(elem_pointer == EMPTY_ROOT_NODE) {
                    cout << "select_mono_first_dim() to points to empty root node" << endl;
                }
            }
            stop_range = elem_pointer;
        }
        select_mcsp_ranges(start_range, stop_range, level+1,tids, columns, predicates, 1);
    }

private:
    /**
	 * In between there usually are levels without selections. We need to descent AND take care of all the monolists that start in this level.
	 *
	 * @param start_range - node head
	 * @param stop_range  - node head
	 * @param level
	 * @param tids
	 * @param columns
	 * @param predicates
	 * @param predicate_index
	 */
    void select_mcsp_ranges_descent(const elf_pointer start_range, const elf_pointer stop_range, const int level
                                    , Synopsis& tids, const vector<int>& columns, const vector<vector<int>>& predicates
                                    , const int predicate_index
    ) const {
        if(SAVE_MODE) {
            if(Util::isIn(level, columns)) {
                cout << "descent() trying to descent in level with selection" << endl;
            }
            if(!is_node_length_offset(start_range)){
                cout << "descent() start_range is no node head"<< endl;
            }
            if(stop_range!=pointer.size() &&!is_node_length_offset(stop_range)){
                cout << "descent() stop_range is no node head"<< endl;
            }
        }
        //These are the pointers we want to find below
        /** the first node in the next level in this range. It may not exist. */
        elf_pointer new_start_range = RECOVER_MASK;//MIN
        /** the first node in the next level not beeing in the range. It exists iff new_start_range exists*/
        elf_pointer new_stop_range  = LAST_ENTRY_MASK;//MAX
        /** the first node in the next level not beeing in the range*/
        elf_pointer first_mono_list = RECOVER_MASK;//MIN
        elf_pointer last_mono_list  = LAST_ENTRY_MASK;//MAX

        /*****************************************************************************************************
         * (1.1) find start of range in this dim. This is the first dim list pointer after
         * start range.
         * (1.2) find the mono lists that start in this range (i.e., level). We need to check all of them.
         *
         * We use a tiny optimization: the first dim elem after start range i either (1.1) or (1.2).
         * We then continue search for the one that is missing.
         *****************************************************************************************************/
        elf_pointer elem_offset = start_range+1;//first elem after node head
        elf_pointer elem_pointer = get_pointer(elem_offset);

        //the first dim element after start_range is either a normal dim elem or a mono list
        if(points_to_monolist(elem_pointer)) {
            first_mono_list = elem_pointer&RECOVER_MASK;
            new_start_range = get_new_start_range(elem_offset+1, stop_range);
        }else{
            new_start_range = elem_pointer;
            if(use_extended_cutoffs){
                first_mono_list = cutoff_monolist(start_range);
            }else{
                first_mono_list = get_first_mono_list(elem_offset+1, stop_range);
            }
        }

        /******************************************************************************************************
         * (2.1) and (2.2) - almost the same as above. But, we look for the range ends and iterate to the left.
         ******************************************************************************************************/
        elem_offset = stop_range-1;//first elem *before* node head
        elem_pointer = get_pointer(elem_offset);

        //TODO better abort flag?

        if(points_to_monolist(elem_pointer)) {
            last_mono_list = elem_pointer&RECOVER_MASK;
            if(stop_range==values.size()) {
                new_stop_range = stop_range;
            }else{
                elem_pointer = get_pointer(stop_range+1);
                if(!points_to_monolist(elem_pointer)) {
                    new_stop_range = elem_pointer;
                }else {
                    new_stop_range = get_new_stop_range(elem_offset-1, start_range);
                }
            }
        }else{
            new_stop_range = elem_pointer+get_node_size(elem_pointer)+NODE_HEAD_LENGTH;

            if(use_extended_cutoffs){
                last_mono_list = cutoff_monolist(stop_range);

                if(first_mono_list==last_mono_list) {//The next monolist is behind stop_range
                    first_mono_list = RECOVER_MASK;
                    last_mono_list  = LAST_ENTRY_MASK;
                }else{
                    //Now it gets a little bit complicated: Without using extended cutoff last_mono_list points to the last mono list to *include*
                    //By design, with extended cutoffs we point to the first to exclude, i.e., subtract one mono list length. That is length_monolist(level+1), as the mono lists start in the level below
                    last_mono_list -= length_monolist(level+1);
                }

                if(SAVE_MODE) {
                    elf_pointer first_mono_list_check = get_first_mono_list(start_range+1, stop_range);
                    elf_pointer last_mono_list_check  = get_last_mono_list(stop_range-1, start_range);

                    if(first_mono_list != first_mono_list_check) {
                        cout << "select_mcsp_ranges_descent() Next monolist: cutoff says=" << first_mono_list << " but is at "+first_mono_list_check << endl;
                    }
                    if(last_mono_list != last_mono_list_check) {
                        cout << "select_mcsp_ranges_descent() Last monolist: cutoff says=" << last_mono_list << " but is at "+last_mono_list_check << endl;
                    }
                }
            }else{
                last_mono_list = get_last_mono_list(elem_offset-1, start_range);
            }
        }
        /******************************************************************************************************
		 * (3.1) - We descend calling the function recursively.
		 * (3.2) - All MonoLists that start here are checked immediately, because they are not visible
		 * in the next level.
		 ******************************************************************************************************/
        scan_mono_lists_in_level(first_mono_list, last_mono_list, level+1, columns, predicates, predicate_index, tids);
        if(new_start_range!=LAST_ENTRY_MASK) {
            select_mcsp_ranges(new_start_range, new_stop_range, level+1, tids, columns, predicates, predicate_index);
        }
    }

    void scan_mono_lists_in_level(const elf_pointer first_mono_list, const elf_pointer last_mono_list, const int level
            , const vector<int>& columns, const vector<vector<int>>& predicates, const int predicate_index, Synopsis& tids
    ) const {
        const int list_length = length_monolist(level);
        for(elf_pointer list_offset = first_mono_list;list_offset<=last_mono_list;list_offset+=list_length){//note the <=
            select_monolist_add_tid(list_offset, level, columns, predicates, predicate_index, tids);
        }
    }

    elf_pointer get_first_mono_list(const elf_pointer start, const elf_pointer stop) const {
        elf_pointer elem_offset = start;
        while (elem_offset<stop) {
            if(!is_node_length_offset(elem_offset)) {//ignore node heads. Here we find the length of the node, with its MSB set.
                elf_pointer elem_pointer = get_pointer(elem_offset);
                if(points_to_monolist(elem_pointer)) {
                    return elem_pointer&RECOVER_MASK;
                }
            }
            elem_offset++;//next elem
        }
        return RECOVER_MASK;//MAX_INT->No dim element in this range
    }

    elf_pointer get_last_mono_list(const elf_pointer offset, const elf_pointer start_range) const {
        elf_pointer elem_offset = offset;
        while (elem_offset>=start_range) {
            if(!is_node_length_offset(elem_offset)) {//ignore node heads. Here we find the length of the node, with its MSB set.
                elf_pointer elem_pointer = get_pointer(elem_offset);
                if(points_to_monolist(elem_pointer)) {
                    return elem_pointer&RECOVER_MASK;
                }
            }
            elem_offset--;//next elem
        }
        return LAST_ENTRY_MASK;//INT_MIN -> No dim element in this range
    }

    /**
 * Non-final selection with ranges.
 * @param start_range
 * @param stop_range
 * @param level
 * @param tids
 * @param columns
 * @param predicates
 * @param predicate_index
 */
    void select_mcsp_ranges_non_terminal_selection(
            const elf_pointer start_range, const elf_pointer stop_range, const int level
            , Synopsis& tids, const vector<int>& columns, const vector<vector<int>>& predicates, const int predicate_index
    ) const {
        if(SAVE_MODE) {
            if(!is_node_length_offset(start_range)) {
                cout <<"select_mcsp_ranges_non_terminal_selection() !is_node_length_offset(start_range)" << endl;
            }
            if(stop_range!=pointer.size() && !is_node_length_offset(stop_range)) {
                cout <<"select_mcsp_ranges_non_terminal_selection() !is_node_length_offset(stop_range)"<< endl;
            }
            if(get_level(start_range)!=level) {
                cout <<"select_mcsp_ranges_non_terminal_selection() getLevel(start_range)!=level" << endl;
            }
            if(get_level(stop_range-1)!=level) {
                cout <<"select_mcsp_ranges_non_terminal_selection() getLevel(stop_range)!=level" << endl;
            }
            if(level!=columns[predicate_index]) {
                cout <<"select_mcsp_ranges_non_terminal_selection() level!=columns[predicate_index]" << endl;
            }
        }
        //the predicates as literals
        const int lower = predicates.at(predicate_index).at(0);
        const int upper = predicates.at(predicate_index).at(1);

        //stuff for the iteration
        elf_pointer elem_offset = start_range;
        elf_pointer elem_pointer_where_run_start=-1;
        /** Flag indicating that there is an un-materialized result run.*/
        bool materialized = false;

        /*******************************
         * for each node in this range *
         *******************************/
        while (elem_offset<stop_range) {
            /***************************************************************
             * (1) First inner while for finding the start of a result run *
             * The first while is equivalent to the MonoColumn Selection   *
             * Except that we also store the elem where run starts
             ***************************************************************/
            while (elem_offset<stop_range) {
                const int elem_val = get_value(elem_offset);
                if(!is_node_length(elem_val)) {//ignore node heads. Here we find the length of the node, with its MSB set.
                    if(Util::isIn(elem_val, lower, upper)) {
                        elf_pointer elem_pointer = get_pointer(elem_offset);
                        if(points_to_monolist(elem_pointer)) {
                            // We do not start result runs at MonoLists....
                            elem_pointer&=RECOVER_MASK;
                            select_monolist_add_tid(elem_pointer, level+1, columns, predicates, predicate_index+1, tids);
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
             * (2) Second inner while for finding the end of a result run  *
             * There is a similar while in  MonoColumn Selection. This one *
             * however is more complicated...
             ***************************************************************/
            while (elem_offset<stop_range) {
                const int elem_val = get_value(elem_offset);
                if(!is_node_length(elem_val)) {//ignore node heads. Here we find the length of the node, with its MSB set.
                    elf_pointer elem_pointer = get_pointer(elem_offset);
                    if(points_to_monolist(elem_pointer)) { // We treat monolists totally independent of the ranges
                        if(Util::isIn(elem_val, lower, upper)) {
                            elem_pointer&=RECOVER_MASK;
                            select_monolist_add_tid(elem_pointer, level+1, columns, predicates, predicate_index+1, tids);
                        }
                    }else{
                        //System.err.println("elem="+elem_offset+" val="+elem_val);
                        if(!Util::isIn(elem_val, lower, upper)) {
                            select_mcsp_ranges(elem_pointer_where_run_start, elem_pointer, level+1,tids, columns, predicates, predicate_index+1);
                            materialized = true;
                            elem_offset++;//next elem
                            break;//go to first while
                        }
                    }
                }
                elem_offset++;//next elem
            }
            //System.out.println("Level = "+level +" size="+tids.size()+" @elem="+(elem_offset));
        }

        //last run in this range. Not necessarily level end
        if(!materialized && elem_pointer_where_run_start != -1) {//the last run lasts until level end
            elf_pointer new_stop_range;
            if(elem_offset == level_stop(level)) {
                new_stop_range = level_stop(level+1);
            }else {
                elf_pointer elem_pointer = get_pointer(stop_range+1);//First *element* in the first node not part of the result
                if(!points_to_monolist(elem_pointer)) {
                    new_stop_range = elem_pointer;
                }else {
                    new_stop_range=get_new_stop_range(stop_range-1, start_range);//Init with last node still inside the result run
                }
            }
            select_mcsp_ranges(elem_pointer_where_run_start, new_stop_range, level+1,tids, columns, predicates, predicate_index+1);
        }
    }

    elf_pointer get_new_start_range(const elf_pointer start, const elf_pointer stop) const {
        elf_pointer elem_offset = start;
        while (elem_offset<stop) {
            if(!is_node_length_offset(elem_offset)) {//ignore node heads. Here we find the length of the node, with its MSB set.
                elf_pointer elem_pointer = get_pointer(elem_offset);
                if(!points_to_monolist(elem_pointer)) {
                    return elem_pointer;
                }
            }
            elem_offset++;//next elem
        }
        return LAST_ENTRY_MASK;//INT_MAX->No dim element in this range
    }

    elf_pointer get_new_stop_range(const elf_pointer offset, const elf_pointer start_range) const {//XXX finde ich so scheiße
        elf_pointer elem_offset = offset;
        while (elem_offset>=start_range) {
            if(!is_node_length_offset(elem_offset)) {//ignore node heads. Here we find the length of the node, with its MSB set.
                elf_pointer elem_pointer = get_pointer(elem_offset);
                if(!points_to_monolist(elem_pointer)) {
                    return elem_pointer+get_node_size(elem_pointer)+NODE_HEAD_LENGTH;
                }
            }
            elem_offset--;//next elem
        }
        return LAST_ENTRY_MASK;//INT_MAX->No dim element in this range
    }

    /**
 * Final selection level
 * @param start_range
 * @param stop_range
 * @param level
 * @param tids
 * @param columns
 * @param predicates
 * @param predicate_index
 */
    void select_mcsp_ranges_final_selection(
            const elf_pointer start_range, const elf_pointer stop_range, const int level
            , Synopsis& tids, const vector<int>& columns, const vector<vector<int>>& predicates, const int predicate_index
    ) const {
        if(SAVE_MODE) {
            if(!is_node_length_offset(start_range)) {
                cout << "scan_mcsp_level_preorder_cutoff_ranges() !is_node_length_offset(start_range)"<< endl;
            }
            if(stop_range!=pointer.size() && !is_node_length_offset(stop_range)) {
                cout << "scan_mcsp_level_preorder_cutoff_ranges() !is_node_length_offset(stop_range)"<< endl;
            }
            if(get_level(start_range)!=level) {
                cout << "scan_mcsp_level_preorder_cutoff_ranges() getLevel(start_range)!=level"<< endl;
            }
            if(get_level(stop_range-1)!=level) {
                cout << "scan_mcsp_level_preorder_cutoff_ranges() getLevel(stop_range)!=level"<< endl;
            }
            if(level!=columns[predicate_index]) {
                cout << "scan_mcsp_level_preorder_cutoff_ranges() level!=columns[predicate_index]"<< endl;
            }
            if(columns.at(columns.size()-1)!=level) {
                cout << "scan_mcsp_level_preorder_cutoff_ranges() should be last selection"<< endl;
            }
        }
        //the predicates as literals
        const int lower = predicates.at(predicate_index).at(0);
        const int upper = predicates.at(predicate_index).at(1);

        //stuff for the iteration
        elf_pointer elem_offset = start_range;
        elf_pointer elem_where_run_start = NOT_FOUND;
        /** Flag indicating that there is an un-materialized result run.*/
        bool materialized = false;

        /*******************************
         * for each node in this range *
         *******************************/
        while (elem_offset<stop_range) {
            /***************************************************************
             * (1) First inner while for finding the start of a result run *
             * The first while is equivalent to the MonoColumn Selection   *
             * Except that we also store the elem where run starts
             ***************************************************************/
            while (elem_offset<stop_range) {
                const int elem_val = get_value(elem_offset);
                if(!is_node_length(elem_val)) {//ignore node heads. Here we find the length of the node, with its MSB set.
                    if(Util::isIn(elem_val, lower, upper)) {
                        // We found a result run
                        elem_where_run_start = elem_offset;
                        materialized = false;
                        elem_offset++;//next elem
                        break;//Go to second inner while to find the end of the run. I know it is break...
                    }
                }
                elem_offset++;//next elem
            }
            /***************************************************************
             * (2) Second inner while for finding the end of a result run  *
             * There is a similar while in  MonoColumn Selection. This one *
             * however is more complicated...
             ***************************************************************/
            while (elem_offset<stop_range) {
                const int elem_val = get_value(elem_offset);
                if(!is_node_length(elem_val)) {//ignore node heads. Here we find the length of the node, with its MSB set.
                    if(!Util::isIn(elem_val, lower, upper)) {
                        const int from = cutoff(elem_where_run_start);
                        const int to = cutoff(elem_offset);
                        //Note, the case that we hit the level end may not occur here.
                        if(level < first_level_with_monolists){
                            copy(from, to,tids);//None safe version
                        }else{
                            copy(from, to, level, tids);//Safe version
                        }
                        materialized = true;
                        elem_offset++;//next elem
                        break;//go to first while
                    }
                }
                elem_offset++;//next elem
            }
            //System.out.println("Level = "+level +" size="+tids.size()+" @elem="+(elem_offset));
        }

        //last run in this range. Not necessarily level end
        if(!materialized && elem_where_run_start != NOT_FOUND) {//the last run lasts until level end
            const int from = cutoff(elem_where_run_start);
            int to;
            if(level_stop(level) == elem_offset) {
                to = tids_in_elf_order.size();
            }else{
                to = cutoff(elem_offset+1);
            }
            //Note, the case that we hit the level end may not occur here.
            if(level < first_level_with_monolists){
                copy(from, to,tids);//None safe version
            }else{
                copy(from, to, level, tids);//Safe version
            }
        }
    }

    int select_1_node(const elf_pointer node_offset, const int level, const int lower, const int upper, Synopsis& tids) const {
        const int length = get_node_size(node_offset);
        /**
         * For each elem in this node, Except the last One. There the cutoff is different.
         * Thus length-1
         */
        for(int elem=0;elem<length-1;elem++){
            const elf_pointer elem_offset = node_offset+NODE_HEAD_LENGTH+elem;//+NODE_HEAD_LENGTH for explicit length
            int elem_val = get_value(elem_offset);
            if(Util::isIn(elem_val, lower, upper)) {
                elf_pointer elem_pointer = get_pointer(elem_offset);
                if(points_to_monolist(elem_pointer)) {
                    elem_pointer&=RECOVER_MASK;
                    const int tid = get_tid_from_monolist(elem_pointer, level+1);
                    tids.add(tid);
                    if(LOG_COST) {write_cost++;}
                }else{
                    //Elf_Dbms_Lvl.get_tids(this, elem_pointer, tids, level+1);
                    collect_tids(elem_offset, elem_offset+1, tids);
                }
            }else if(elem_val > upper){
                return node_offset + NODE_HEAD_LENGTH+length;//point to next elem
            }
        }

        //Last elem separately, because the cuttoff is different. I cannot simply use the cuttoff of next level, due to non density issue
        const int elem_offset 	= node_offset+NODE_HEAD_LENGTH+length-1;//+NODE_HEAD_LENGTH for explicit length
        int elem_val 			= get_value(elem_offset);

        if(Util::isIn(elem_val, lower, upper)) {
            int elem_pointer = get_pointer(elem_offset);
            if(points_to_monolist(elem_pointer)) {
                elem_pointer&=RECOVER_MASK;
                const int tid = get_tid_from_monolist(elem_pointer, level+1);
                tids.add(tid);
                if(Config::LOG_COST) {write_cost++;}
            }else {
                collect_tids_last_node_element(elem_offset, level, tids);
            }
        }
        return node_offset + NODE_HEAD_LENGTH+length;//point to next node
    }

    /**
     * Special function which can be called if level < first_level_with_monolists
     * This simplifies the algorithm and reduces its cost in two ways.
     * (1) We do not need to check the pointer of an element whether it is start of mono lists halfing the costs for reading the level.
     * (2) At the last element, we can safely use the cutoff of the next element, as it is always defined and the non-density bug does not occur.
     *
     * @param node_offset
     * @param level
     * @param lower
     * @param upper
     * @param tids
     * @return
     */
    int select_1_node_no_monolists_above(const elf_pointer node_offset, const int level, const int lower, const int upper, Synopsis& tids) const {
        if(SAVE_MODE){
            if(level>=first_level_with_monolists){
                cout << "select_1_node_no_monolists_above() level>=first_level_with_monolists" << endl;
            }
        }
        const int length = get_node_size(node_offset);
        /**
         * For each elem in this node. s we know there are no MonoLists above it is save to take the cutoff of the next element.
         * Even if it belongs to the next node or next level.
         */
        for(int elem=0;elem<length;elem++){//Should work always, even if the next element is in the next level. There is at least one level below.
            const elf_pointer elem_offset = node_offset+NODE_HEAD_LENGTH+elem;//+NODE_HEAD_LENGTH for explicit length
            const int elem_val = get_value(elem_offset);
            if(Util::isIn(elem_val, lower, upper)) {
                elf_pointer from, to;
                from = cutoff(elem_offset);
                elf_pointer next_elem_offset = (elem == length-1) ? elem_offset+2 : elem_offset+1;//for the last node element a the next offset is a node head. Thus, we increment by 2.
                if(next_elem_offset>level_stop(level)){//next element is in the next elvel, which points to tid_offset 0, i.e., cannot use it.
                    to = tids_in_elf_order.size();
                }else{
                    to = cutoff(next_elem_offset);
                }
                copy(from,to,tids);
            }else if(elem_val > upper){
                return node_offset + NODE_HEAD_LENGTH+length;//point to next elem
            }
        }
        return node_offset + NODE_HEAD_LENGTH+length;//point to next node
    }

    void collect_tids(const elf_pointer elem_offset, const elf_pointer next_elem_offset, Synopsis& result_tids) const {
        if(SAVE_MODE) {
            if(is_node_length_offset(elem_offset)) {
                cout << "collect_tids(int,int,MyArrayLis) is_node_length_offset(elem_pointer)" << endl;
            }
            if(is_node_length_offset(next_elem_offset)) {
                cout << "collect_tids(int,int,MyArrayLis) is_node_length_offset(next_elem_pointer)" << endl;
            }
        }

        /** Index in this.tids_in_elf_order[] of the first tid in  this sub tree.*/
        const int from 	= cutoff(elem_offset);
        /** Index of the first tid in the next sub tree. So, we need to iterate until this one.*/
        const int to 	= cutoff(next_elem_offset);
        copy(from, to, result_tids);
    }

    void collect_tids_last_node_element(const elf_pointer elem_offset, const elf_pointer level, Synopsis& result_tids) const {
        if(SAVE_MODE) {
            if(!is_last_elem_in_level(elem_offset, level)) {
                int next_node_start = elem_offset + 1;
                if(!is_node_length_offset(next_node_start)) {
                    cout << "collect_tids_cuttoff_last_node_element() !is_node_length_offset(next_node_start)" << endl;
                }
            }
        }
        /** Index in this.tids_in_elf_order[] of the first tid in  this sub tree.*/
        const int from 	= cutoff(elem_offset);

        int to;
        if(is_last_elem_in_level(elem_offset, level)) {
            //System.err.println("collect_tids_cuttoff_last_node_element() last elem in level happens");
            to = tids_in_elf_order.size();
        } else {
            int next_elem_offset = elem_offset + 2;
            to 	= cutoff(next_elem_offset);
        }

        copy_abort(from, to, level, result_tids);
    }

    /**
     *
     * @param elem_offset
     * @param stop_level
     * @param lower
     * @param upper
     * @return elem_offset where run starts or NOT_FOUND
     */
    int find_start_of_run(elf_pointer elem_offset, const elf_pointer stop_level, const int lower, const int upper) const {
        while (elem_offset<stop_level) {
            const int elem_val = get_value(elem_offset);
            if(!is_node_length(elem_val)) {//ignore node heads. Here we find the length of the node, with its MSB set.
                if(Util::isIn(elem_val, lower, upper)) {
                    // We found a result run
                    return elem_offset;
                }
            }
            elem_offset++;//next elem
        }
        return NOT_FOUND;
    }

    /**
     *
     * @param elem_offset
     * @param stop_level
     * @param lower
     * @param upper
     * @return elem_offset where run stops or stop_level
     */
    int find_stop_of_run(elf_pointer elem_offset, const elf_pointer stop_level, const int lower, const int upper) const {
        while (elem_offset<stop_level) {
            const int elem_val = get_value(elem_offset);
            if(!is_node_length(elem_val)) {//ignore node heads. Here we find the length of the node, with its MSB set.
                if(!Util::isIn(elem_val, lower, upper)) {
                    return elem_offset;
                }
            }
            elem_offset++;//next elem
        }
        return stop_level;
    }

    void check_cutoffs(const elf_pointer node_offset, const int level) {
        if(!is_node_length_offset(node_offset)) {
            cout << "!is_node_length_offset(node_offset)" << endl;
        }
        const int length = get_node_size(node_offset);

        for(int elem=0;elem<length;elem++){
            const elf_pointer elem_offset = node_offset+NODE_HEAD_LENGTH+elem;//+1 for length
            elf_pointer elem_pointer = get_pointer(elem_offset);
            if(points_to_monolist(elem_pointer)) {
                elem_pointer &= RECOVER_MASK;
                int tid = get_tid_from_monolist(elem_pointer, level+1);
                int position = cutoff(elem_offset);
                if(tids_in_elf_order.at(position)!=tid) {
                    cout << "Cutoff wrong @"<<elem_offset<<" cutoff="<<position << endl;
                }
            } else {
                int tid = get_first_tid(elem_pointer, level+1);
                int position = cutoff(elem_offset);
                if(tids_in_elf_order.at(position)!=tid) {
                    cout << "Cutoff wrong sub tree @"<<elem_offset<<" cutoff="<<position << endl;
                }
                check_cutoffs(elem_pointer, level+1);
            }
        }
    }

    int get_first_tid(const elf_pointer node_start, const int level) {
        if(SAVE_MODE) {
            if(!is_node_length_offset(node_start)) {
                cout << "!is_node_length_offset(node_start)" << endl;
            }
        }
        elf_pointer first_elem_offset = node_start+1;
        elf_pointer elem_pointer = get_pointer(first_elem_offset);
        if(points_to_monolist(elem_pointer)) {
            elem_pointer &= RECOVER_MASK;
            return get_tid_from_monolist(elem_pointer, level+1);
        }
        return get_first_tid(elem_pointer, level+1);
    }

    /**
     * TODO redundant
* Returns the first level having mono list. The pointer to this mono list in the level above. The main reason for this function is to avoid the non-density bug.
* @return
 */
    static int get_first_level_with_monolists(const vector<elf_pointer>& mono_list_level) {
        for(int level=1;level<mono_list_level.size();level++) {
            if(mono_list_level.at(level)>0) {
                return level;
            }
        }
        cout << "get_first_level_with_monolists() - did not find first level with mono lists" << endl;
        return NOT_FOUND;
    }

    inline bool is_last_elem_in_level(const elf_pointer elem_offset, const int elem_level) const {
        return level_stop(elem_level)==elem_offset+1;
    }

    inline int cutoff(const elf_pointer elem_offset) const {
        if(SAVE_MODE) {
            if(is_node_length_offset(elem_offset) && elem_offset >= level_stop(1)) {
                cout << "cutoff(int) called for node, not for element" << endl;
            }
        }
        if(LOG_COST){read_cost++;}
        return cutoffs.at(elem_offset);
    }

    inline int cutoff_monolist(const elf_pointer elem_offset) const {
        if(SAVE_MODE) {
            if(!is_node_length_offset(elem_offset) && elem_offset >= level_stop(1)) {
                cout <<"cutoff_monolist(int) called for element, not for node" << endl;
            }
        }
        if(LOG_COST) {read_cost++;}
        return cutoffs.at(elem_offset);
    }

    inline bool is_node_length(const int value) const {
        return value<0;
    }

    /**
     * Fastest way to exploit cutoffs, but not safe concerning density bug.
     * @param from
     * @param to
     * @param result_tids
     */
    inline void copy(const int from, const int to, Synopsis& result_tids) const {
        /** Index of the first tid in the next sub tree. So, we need to iterate until this one.*/
        if(USE_MEMCOPY){
            result_tids.add(tids_in_elf_order.begin()+from,tids_in_elf_order.begin()+to);
            if(LOG_COST) {write_cost+=to-from;}
        }else{
            for (int i = from; i < to; i++) {
                result_tids.add(tids_in_elf_order.at(i));
                if(LOG_COST){write_cost++;}
            }
        }
    }

    /**
     * Safe towards non-density bug
     * @param from
     * @param to
     * @param level
     * @param MyArrayList
     */
    inline void copy(const int from, const int to, const int level, Synopsis& result_tids) const {
        /** Index of the first tid in the next sub tree. So, we need to iterate until this one.*/
        for (int i = from; i < to; i++) {
            if(tids_level.at(i)>level) {
                result_tids.add(tids_in_elf_order.at(i));//@me cannot use memcopy here....
                if(LOG_COST){write_cost++;}
            }
            if(LOG_COST){read_cost++;}
        }
    }

    /**
     * Density bug safe version. Slower than normal copy(int,int,MyArrayList)
     * @param from
     * @param to
     * @param level
     * @param result_tids
     */
    inline void copy_abort(const int from, const int to, const int level, Synopsis& result_tids) const {
        /** Index of the first tid in the next sub tree. So, we need to iterate until this one.*/
        //if(from== 22115 && to == 22135){
        //    //cout << from << endl;
        //}
        for (int i = from; i < to; i++) {
            if(LOG_COST){read_cost++;}
            if(tids_level.at(i)>level) {
                result_tids.add(tids_in_elf_order.at(i));
                if(LOG_COST){write_cost++;}
            }else{
                return;
            }
        }
    }
};


#endif //MY_MCSP_ELF_TABLE_CUTOFF_EXTERNAL_H
