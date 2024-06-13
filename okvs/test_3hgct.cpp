
#include "okvs.h"
#include <chrono>

int main(int argc, char **argv){

    int m = 1<<atoi(argv[1]);
    int output_byte_length = (log2(m)+40+7)/8;
    AES aes(toBlock(234522));

    std::vector<oc::block> party_set(m);

    PRNG prng(sysRandomSeed()+toBlock(100));

    PRNG prng_common(toBlock(12345));
    prng_common.get(party_set.data(), 10);

    prng.get(party_set.data()+10, m-10);

    std::cout<<"Set gererated"<<std::endl;

    ThreeH_gct threeH_gct(party_set);
    threeH_gct.init();

    std::cout<<"The table size: "<<threeH_gct.getTableSize()<<std::endl;

    std::cout<<"Finish threeH_gct initiation"<<std::endl;
    
    std::vector<block> h1_values(m);
    aes.ecbEncBlocks(party_set, h1_values);

    threeH_gct.encodeOnlyUnpeeling(h1_values);

    std::cout<<"encodeOnlyUnpeeling"<<std::endl;

    int count = 0;
    // for(int i=0; i<threeH_gct.table.size(); i++){
    //     if(threeH_gct.table[i] == ZeroBlock){
    //         count++;
    //     }
    // }
    // std::cout<<"find zeroblocks in the OKVS table: "<<count<<std::endl;
    std::vector<oc::block> test_decoded_values;
    threeH_gct.decode(party_set, test_decoded_values);
    for(int i=0; i<m; i++){
        if(test_decoded_values[i] != h1_values[i]){
            count++;
            // std::cout<<"find unequal for decoding in h1: "<<i<<std::endl;
            // break;
        }
    }
    if(count > 0){
        std::cout<<"find unequal for decoding in h1: "<<std::endl;
    }else{
        std::cout<<"decoding h1 values succeed"<<std::endl;
    }
    std::cout<<"h1[0]: "<<h1_values[0]<<std::endl;

    std::cout<<"test first item decoding: "<<test_decoded_values[0]<<std::endl;

    //  std::vector<oc::block> test_decoded_values;
    std::cout<<"threeH_gct.table.size(): "<<threeH_gct.table.size()<<std::endl;
    std::cout<<"variables size: "<<threeH_gct.obdTable->variables.size()<<std::endl;

    count = 0;
    for(int i=0; i<threeH_gct.table.size(); i++){
        if(threeH_gct.table[i] == ZeroBlock){
            count++;
        }
    }
    std::cout<<"In threeH_gct table, zero count: "<<count<<std::endl;
    count = 0;
    for(int i=0; i<threeH_gct.table.size(); i++){
        if(threeH_gct.obdTable->variables[i] == 0){
            count++;
        }
    }
    //we still need to set the non-used bins in variables as non-zero to keep it oblivious
    std::cout<<"In variables, zero count: "<<count<<std::endl;

    test_decoded_values.clear();
    count = 0;
    threeH_gct.decodeByTable(threeH_gct.table, test_decoded_values);
    for(int i=0; i<m; i++){
        if(test_decoded_values[i] != h1_values[i]){
            count++;
            // std::cout<<"find unequal for decoding in h1 twice: "<<i<<std::endl;
            // break;
        }
    }
    std::cout<<"test first item decoding again: "<<test_decoded_values[0]<<std::endl;

    if(count > 0){
        std::cout<<"find unequal # for decoding in h1: "<<count<<std::endl;
    }else{
        std::cout<<"decoding h1 values succeed"<<std::endl;
    }
    // std::cout<<"threeH_gct.table: "<<std::endl;
    // for(int i=0; i<10; i++){
    //     std::cout<<threeH_gct.table[i]<<", ";
    // }
    // for(int i=0; i<3; i++){
    //     std::cout<<((threeH_gct.obdTable)->variables[i]);
    // }
    // std::cout<<std::endl;
    // std::cout<<"peeling count: "<<((threeH_gct.obdTable)->peelingCounter)<<std::endl;

    std::vector<oc::block> tmp = threeH_gct.table;

    std::vector<block> h2_values(m);
    prng.get(h2_values.data(), m);

    auto t0 = std::chrono::high_resolution_clock::now();
    for(int i=0; i<1; i++){
        threeH_gct.encodeOnlyUnpeeling(h2_values);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dur = t1 - t0;
    std::cout<<"encodeOnlyUnpeeling time (s): "<<dur.count()<<std::endl;

    threeH_gct.decode(party_set, test_decoded_values);
    count = 0;
    for(int i=0; i<m; i++){
        if(test_decoded_values[i] != h2_values[i]){
            count++;
            // break;
        }
    }
    std::cout<<"find unequal for decoding in h2: "<<count<<std::endl;

    t0 = std::chrono::high_resolution_clock::now();
    threeH_gct.encode(h2_values);
    t1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dur1 = t1 - t0;
    std::cout<<"encode time (s): "<<dur1.count()<<std::endl;

    t0 = std::chrono::high_resolution_clock::now();
    threeH_gct.decode(party_set, test_decoded_values);
    t1 = std::chrono::high_resolution_clock::now();
    dur1 = t1 - t0;
    std::cout<<"decode time (s): "<<dur1.count()<<std::endl;
    // std::cout<<"threeH_gct.table: "<<std::endl;
    // for(int i=0; i<10; i++){
    //     std::cout<<threeH_gct.table[i]<<", ";
    // }
    // for(int i=0; i<3; i++){
    //     std::cout<<((threeH_gct.obdTable)->variables[i]);
    // }
    // std::cout<<std::endl;
    // std::cout<<"peeling count: "<<((threeH_gct.obdTable)->peelingCounter)<<std::endl;
    int equal_num = 0;
    for(int i=0; i<threeH_gct.table.size(); i++){
        if(threeH_gct.table[i] == tmp[i]){
            equal_num++;
        }
    }
    std::cout<<"OKVS table size: "<<threeH_gct.table.size()<<std::endl;
    std::cout<<"re-encoding does not change "<<equal_num<<" bins"<<std::endl;

    return 0;
}