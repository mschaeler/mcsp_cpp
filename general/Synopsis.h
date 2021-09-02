//
// Created by Martin on 23.03.2021.
//

#ifndef MY_MCSP_SYNOPSIS_H
#define MY_MCSP_SYNOPSIS_H

#include "vector"
#include <algorithm> //copy(it,it,vec)

class Synopsis {
    int32_t write_pointer = 0;
public:
    vector<int> array;
    Synopsis(int size) : array(size){

    }
    Synopsis() : Synopsis(10000){

    }
    vector<int> getTrimmedArray(){//By value
        vector<int> newVec(array.begin(), array.begin() + write_pointer);
        return newVec;
    }
    void add(int value){
        //TODO add write cost here
        if(write_pointer>=array.capacity()){
            array.resize(2*array.capacity());
        }
        array[write_pointer++] = value;
    }
    int size(){return write_pointer;}

    int get(int index){
        return array[index];
    }

    void clear() {
        write_pointer = 0;
    }

    void ensure_capacity(const std::vector<int>::size_type size){
        if(array.capacity() < size){
            array.reserve(size);
        }
    }

    void copy(vector<int>::const_iterator begin, vector<int>::const_iterator end) {
        write_pointer = end-begin;
        ensure_capacity(write_pointer);
        std::copy(begin, end, array.begin());
    }
};


#endif //MY_MCSP_SYNOPSIS_H
