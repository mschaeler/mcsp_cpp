//
// Created by Martin on 23.03.2021.
//

#ifndef MY_MCSP_SYNOPSIS_H
#define MY_MCSP_SYNOPSIS_H

#include "vector"

class Synopsis {
    vector<int> array;
    int write_pointer = 0;
public:
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
};


#endif //MY_MCSP_SYNOPSIS_H
