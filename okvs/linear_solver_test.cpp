// #pragma once

#include <vector>
#include <NTL/GF2E.h>
#include <NTL/mat_GF2E.h>
#include <NTL/GF2XFactoring.h>

#include <NTL/ZZ.h>

#include <chrono>

#include "linear_solver.h"


int main(int argc, char **argv){
    NTL::GF2X irredPoly = NTL::BuildSparseIrred_GF2X(128);
    NTL::GF2E::init(irredPoly);

    int core_size = 50;
    int set_size = 1<<atoi(argv[1]);
    int table_size = 1.3*set_size;
    int gamma = 60;

    NTL::mat_GF2E mat;
    NTL::vec_GF2E b;
    NTL::vec_GF2E x;

    int numRows = core_size;
    int numCols = table_size+gamma;
    mat.SetDims(numRows, numCols);
    b.SetLength(core_size);
    x.SetLength(numCols);

    std::vector<NTL::ZZ> my_set(core_size);
    for(int i=0; i<core_size; i++){
        NTL::RandomBits(my_set[i], 128);
        b[i] = NTL::random_GF2E();
    }

    std::vector<int> indices(3);
    NTL::ZZ prf_indices;
    for(int i=0; i<core_size; i++){
        NTL::SetSeed(my_set[i]);
        indices[0] = NTL::RandomBnd(1<<30) % table_size;
        indices[1] = NTL::RandomBnd(1<<30) % table_size;
        indices[2] = NTL::RandomBnd(1<<30) % table_size;
        prf_indices = NTL::RandomBnd(NTL::to_ZZ(1)<<gamma);
        // std::cout<<"prf_indices for "<<i<<": "<<prf_indices<<std::endl;

        mat[i][indices[0]] = NTL::to_GF2E(1);
        mat[i][indices[1]] = NTL::to_GF2E(1);
        mat[i][indices[2]] = NTL::to_GF2E(1);

        for(int j=numCols-gamma; j<numCols; j++){
            if((prf_indices & NTL::to_ZZ(1)) == NTL::to_ZZ(1)){
                mat[i][j] = NTL::to_GF2E(1);
            }
            prf_indices >>= 1;
        }
    }
    // std::cout<<"Before solver: "<<std::endl;
    // std::cout<<"Mat: "<<mat<<std::endl;
    // std::cout<<"b: "<<b<<std::endl;
    auto m_start = std::chrono::high_resolution_clock::now();

    auto  row_echelon_mat = solver(mat, b, x);

    auto  m_stop = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(m_stop-m_start) ;
    std::cout<<"Solver takes (ms): "<<dur.count()<<std::endl;

    // std::cout<<"Checking one equation: "<<std::endl;
    // NTL::GF2E tmp = NTL::to_GF2E(0);
    // for(int i=0; i<numCols; i++){
    //     if(row_echelon_mat[numRows-1][i] == NTL::to_GF2E(1)){
    //         tmp += x[i];
    //         // std::cout<<i<<": "<<tmp<<std::endl;
    //     }
    // }
    // std::cout<<std::endl;


    // std::cout<<"After solver: x "<<x<<std::endl;
    // std::cout<<"After solver: x "<<std::endl;
    // for(int i=0; i<x.length(); i++){
    //     std::cout<<i<<": "<<x[i]<<std::endl;
    // }

    int num_unequal = check_encoding(mat, b, x);
    if(num_unequal == 0){
        std::cout<<"Encoding succeeds!"<<std::endl;
    }else{
        std::cout<<"Encoding fails!, Find # of unequals: "<<num_unequal<<std::endl;
    }

    return 0;
}
