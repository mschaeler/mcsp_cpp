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

    static const bool USE_MEMCOPY = false;

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
        while (offset<stop_level) {//for each node in this level
            offset = select_1_node(offset, level, lower, upper, tids);
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

private:
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
    static int get_first_level_with_monolists(const vector<int>& mono_list_level) {
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

    inline int cutoff(const int elem_offset) const {
        if(SAVE_MODE) {
            if(is_node_length_offset(elem_offset) && elem_offset >= level_stop(1)) {
                cout << "cutoff(int) called for node, not for element" << endl;
            }
        }
        if(LOG_COST){read_cost++;}
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
