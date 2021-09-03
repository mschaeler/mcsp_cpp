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
            , first_level_with_monolists(get_first_level_with_monolists(_mono_lists))
    {
        cout << "values.size()=" << values.size() << "pointer.size()=" << pointer.size() << "mono_lists.size()=" << mono_lists.size() << endl;
        cout << "levels="<< Util::to_string(levels) << "levels_mono_lists=" << Util::to_string(levels_mono_lists) << endl;
        cout << "tids_in_elf_order.size()=" << tids_in_elf_order.size() << " tids_level.size()=" << tids_level.size() << " cutoffs.size()=" << cutoffs.size() << endl;
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

    const inline int cutoff(const int elem_offset) const {
        if(SAVE_MODE) {
            if(is_node_length_offset(elem_offset) && elem_offset >= level_stop(1)) {
                cout << "cutoff(int) called for node, not for element" << endl;
            }
        }
        if(LOG_COST){read_cost++;}
        return cutoffs.at(elem_offset);
    }

    /**
     * Fastest way to exploit cutoffs, but not safe concerning density bug.
     * @param from
     * @param to
     * @param result_tids
     */
    inline void copy(const int from, const int to, Synopsis& result_tids) const {
        /** Index of the first tid in the next sub tree. So, we need to iterate until this one.*/
        for (int i = from; i < to; i++) {
            result_tids.add(tids_in_elf_order.at(i));
            if(LOG_COST){write_cost++;}
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
