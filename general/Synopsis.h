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
    Synopsis(int size) : array{}{
        array.reserve(size);
    }
    Synopsis() : Synopsis(10000){

    }
    vector<int> getTrimmedArray(){//By value
        return array;
    }
    void add(int value){
        if(write_pointer>=array.capacity()){
            array.reserve(2*array.capacity());
        }
        array.push_back(value);
    }
    int size(){return array.size();}

    int get(int index){
        return array[index];
    }

    void clear() {
        array.clear();
    }

    void ensure_capacity(const std::vector<int>::size_type size){
        if(array.capacity() < size){
            array.reserve(size);
        }
    }

    void copy(vector<int>::const_iterator begin, vector<int>::const_iterator end) {
        ensure_capacity(std::distance(begin, end));
        array.clear();
        array.insert(array.begin(), begin, end);
    }

    void move(vector<int> other) {
        array = std::move(other);
    }

    void copy_unsafe(vector<int>::const_iterator begin, vector<int>::const_iterator end) {
        array.clear();
        array.insert(array.begin(), begin, end);
    }
    void add(vector<int>::const_iterator begin, vector<int>::const_iterator end) {
        std::copy(begin, end, std::back_inserter(array));
    }
};


#endif //MY_MCSP_SYNOPSIS_H
