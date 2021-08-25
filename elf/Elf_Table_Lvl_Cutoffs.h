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
    {
        //Nothing to do
        cout << "values.size()=" << values.size() << "pointer.size()=" << pointer.size() << "mono_lists.size()=" << mono_lists.size() << endl;
        cout << "levels="<< Util::to_string(levels) << "levels_mono_lists=" << Util::to_string(levels_mono_lists) << endl;
        if(SAVE_MODE){
            //TODO
        }
    }

    void check_cutoffs(ColTable& table){
        cout << "check_cutoffs(ColTable&)";
        const int num_dim = table.num_dim;
        vector<int> point(num_dim);
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
    }
};


#endif //MY_MCSP_ELF_TABLE_LVL_CUTOFFS_H
