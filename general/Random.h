//
// Created by Martin on 22.07.2021.
//

#ifndef MY_MCSP_RANDOM_H
#define MY_MCSP_RANDOM_H


class Random{
    std::mt19937 mersenne_twister;
public:
    void seed(long seed){
        mersenne_twister.seed(seed);
    }

    int nextInt(int exclusive_max){
        std::uniform_int_distribution<int> distribution(0,exclusive_max-1);
        int rnd_number = distribution(mersenne_twister);
        return rnd_number;
    }

    double nextDouble(double exclusive_max){
        std::uniform_int_distribution<int> distribution(0,100000);
        int rnd_number = distribution(mersenne_twister);
        double temp = (double) rnd_number / (double) 100000;
        return temp*exclusive_max;
    }
};


#endif //MY_MCSP_RANDOM_H
