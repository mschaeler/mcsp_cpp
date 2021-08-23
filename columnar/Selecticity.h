//
// Created by Martin on 30.07.2021.
//

#include <vector>

#ifndef MY_MCSP_SELECTICITY_H
#define MY_MCSP_SELECTICITY_H

#endif //MY_MCSP_SELECTICITY_H

/**
 * For sorting column seelction by selectivity using the ones returning the least amount of tuples first.
 */
class Selecticity{
public:
    int column_indexes;
    int lower;
    int upper;
    double selectivity;
    Selecticity(int _column_indexes, int _lower, int _upper, double _selectivity)
            : column_indexes(_column_indexes)
            , lower(_lower)
            , upper(_upper)
            , selectivity(_selectivity)
    {

    }

    void out_instance(){
        std::cout << "column_indexes=" << column_indexes << " selectivity=" << selectivity;
    }

    bool operator < (const Selecticity& str) const
    {
        return (selectivity < str.selectivity);
    }

    static void out(std::vector<Selecticity>& vector){
        for(auto s : vector){
            s.out_instance();
            std::cout << ", ";
        }
        std::cout << std::endl;
    }
};