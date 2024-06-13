#include "linear_solver.h"

//using Gauss solver
NTL::mat_GF2E solver(NTL::mat_GF2E &A, const NTL::vec_GF2E &b, NTL::vec_GF2E &x){
    NTL::mat_GF2E A_b;
    A_b.SetDims(A.NumRows(), A.NumCols()+1);
    for(int i=0; i<(int)A.NumRows(); i++){
        for(int j=0; j<(int)A.NumCols(); j++){
            A_b[i][j] = A[i][j];
        }
        A_b[i][A.NumCols()] = b[i];
    }
    // std::cout<<"Before gauss"<<std::endl;
    // std::cout<<A_b<<std::endl;
    
    // NTL::gauss(A_b, A_b.NumRows());
    NTL::gauss(A_b);

    // std::cout<<"After gauss"<<std::endl;
    // std::cout<<A_b<<std::endl;


    NTL::SetSeed(NTL::GetCurrentRandomStream());

    std::vector<bool> x_set_flags(x.length(), false);
    NTL::GF2X temp;
    for(int i=A.NumRows()-1; i>=0; i--){
        int first_non0_index = 0;
        NTL::GF2E tmp_sum;
        for(int j=A.NumCols()-1; j>=0; j--){
            if(A_b[i][j] == NTL::GF2E(1)){
                if(x_set_flags[j] == false){
                    //assign a random value
                    x[j] = NTL::random_GF2E();
                    x_set_flags[j] = true;
                }
                first_non0_index = j;
                tmp_sum += x[j];
            }
        }

        x[first_non0_index] = (tmp_sum + x[first_non0_index] + A_b[i][A.NumCols()]);
    }

    // std::cout<<"Solver finished"<<std::endl;

    return A_b;
}

int check_encoding(NTL::mat_GF2E &A, const NTL::vec_GF2E &b, const NTL::vec_GF2E &x){
    int unequal = 0;
    NTL::vec_GF2E b_prime;
    b_prime = A*x;
    for(int i=0; i<b.length(); i++){
        if(b[i] != b_prime[i]){
            unequal++;
        }
    }
    return unequal;
}
