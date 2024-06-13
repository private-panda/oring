#pragma once

#include "cryptoTools/Crypto/PRNG.h"
// #include "Common/Defines.h"
// #include "Common/Log.h"
// #include "Common/Log1.h"
#include <set>
#include <NTL/ZZ_p.h>
#include <NTL/vec_ZZ_p.h>
#include <NTL/ZZ_pX.h>
#include <NTL/ZZ.h>
// #include <ObliviousDictionary.h>
#include "paxos.h"



#include "xxHash/xxhash.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace osuCrypto;

class Paxos{
public:

    // std::vector<Hasher128> hashers;
    std::vector<oc::block> table;

    std::vector<oc::block>& keys;
    // ObliviousDictionary* dic;

    OBDTables *obdTable;

    
public:
    Paxos(std::vector<oc::block>& keys):keys(keys){
        int hashSize=keys.size(), gamma = 60, v=20;
        double c1 = 2.4;
        int fieldSize = 128;

        // dic = new OBD3Tables(hashSize, c1, fieldSize, gamma, v);
        // dic->init();

        obdTable = new OBD2Tables(hashSize, c1, fieldSize, gamma, v);
        obdTable->init();


    }

    ~Paxos(){
        delete obdTable;
    }

    //do the first half of encoding, for concurrency
    void init(){
        // std::vector<u8> tmpValues(values.size()*sizeof(block));
        // std::memcpy(tmpValues.data(), (u8*)values.data(), values.size()*sizeof(block));
        // obdTable->setVals(values);
        // obdTable->setKeysAndVals(keys, tmpValues);
        obdTable->keys = keys;
        obdTable->fillTables();
        obdTable->peeling();
    }
    //do the second half of encoding, for concurrency
    bool encodeOnlyUnpeeling(std::vector<oc::block>& values){
    // bool encodeOnlyUnpeeling(){

        // obdTable->values.resize(values.size()*sizeof(block));
        // std::memcpy((obdTable->values).data(), (u8*)values.data(), values.size()*sizeof(block));
        // std::vector<u8> tmpValues(values.size()*sizeof(block));
        // std::memcpy(tmpValues.data(), (u8*)values.data(), values.size()*sizeof(block));
        obdTable->setVals(values);
        // obdTable->setKeysAndVals(keys, tmpValues);

 
        // obdTable->generateExternalToolValues();
        // obdTable->unpeeling();

        // vector<uint8_t> x = obdTable->getVariables();
        // std::cout << "after getting the okvs table" << std::endl;
        // std::cout<<x.size()<<std::endl;

        // table.resize(x.size()/16);
        // std::cout<<table.size()<<std::endl;
        // memcpy((uint8_t *)table.data(), x.data(), x.size());
        // std::cout << "before check the output" << std::endl;

        // obdTable->checkOutput();
        // std::cout << "here" << endl;


        // obdTable->fillTables();
        // auto res = obdTable->peeling();
        obdTable->generateExternalToolValues();
        obdTable->unpeeling();

        vector<uint8_t> x = obdTable->getVariables();
        // std::cout << "after getting the okvs table" << std::endl;
        // std::cout<<x.size()<<std::endl;

        table.resize(x.size()/16);
        // std::cout<<table.size()<<std::endl;
        memcpy((uint8_t *)table.data(), x.data(), x.size());
        // std::cout << "before check the output" << std::endl;

        // obdTable->checkOutput();
        // std::cout << "here" << endl;

        return true;
    }

    //the complete encoding process, inclusing peeling and unpeeling
    bool encode(std::vector<oc::block>& values){
        assert(keys.size() == values.size());

        auto t0 = std::chrono::high_resolution_clock::now();
        std::vector<u8> tmpValues(values.size()*sizeof(block));
        std::memcpy(tmpValues.data(), (u8*)values.data(), values.size()*sizeof(block));
        obdTable->setKeysAndVals(keys, tmpValues);

        auto t1 = std::chrono::high_resolution_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0);
        std::cout<<"set keys and values takes (ms): "<<dur.count()<<std::endl;

        obdTable->encode();

        // std::cout<<"finish encoding"<<std::endl;
        auto t2 = std::chrono::high_resolution_clock::now();
        vector<uint8_t> x = obdTable->getVariables();
        // std::cout << "after getting the okvs table" << std::endl;
        // std::cout<<x.size()<<std::endl;

        table.resize(x.size()/16);
        // std::cout<<table.size()<<std::endl;
        memcpy((uint8_t *)table.data(), x.data(), x.size());
        // std::cout << "before check the output" << std::endl;
        auto t3 = std::chrono::high_resolution_clock::now();
        dur = std::chrono::duration_cast<std::chrono::milliseconds>(t3-t2);
        std::cout<<"copy table takes (ms): "<<dur.count()<<std::endl;

        // obdTable->checkOutput();
        // std::cout << "here" << endl;

        return true;
    }

    //decode the set of keys by using the party's OKVS table
    void decode(std::vector<block>& keys, std::vector<block>& queryValues){
        queryValues.resize(keys.size());
        for (int i = 0; i < keys.size(); i++) {
            // keys[i] = XXH64(&setKeys[i], 64, 0);
            auto value = obdTable->decode(keys[i]);
            memcpy(&queryValues[i], value.data(), sizeof(block));
        }
        return;
    }

    // NTL::GF2X irredPoly = NTL::BuildSparseIrred_GF2X(128);
    //decode a party's key set by a new OKVS table
    void decodeByTable(std::vector<block>& okvsTable, std::vector<block>& queryValues){
        queryValues.resize(keys.size());
        // NTL::GF2E::init(irredPoly); //it is necessary to init the gf2e, otherwise cannot convert the gf2x to gf2e
        
        auto okvs_ptr = (u8 *)okvsTable.data();
        for(int i=0; i<okvsTable.size(); i++){
            GF2X temp;
            // // GF2XFromBytes(temp, ((u8 *)okvsTable.data()) + i*16 ,16);
            GF2XFromBytes(temp, okvs_ptr ,16);
            // // (obdTable->variables)[i] = to_GF2E(temp);
            // // to_GF2E(temp);
            conv((obdTable->variables)[i], temp);
            okvs_ptr += 16;
        }
        // std::cout<<"In decode by table, has got the okvs table"<<std::endl;
        for (int i = 0; i < keys.size(); i++) {
            // keys[i] = XXH64(&setKeys[i], 64, 0);
            auto value = obdTable->decode(keys[i]);
            memcpy(&queryValues[i], value.data(), sizeof(block));
        }
        return;
    }


};

class ThreeH_gct{
public:

    // std::vector<Hasher128> hashers;
    std::vector<oc::block> table;

    std::vector<oc::block>& keys;
    // ObliviousDictionary* dic;

    OBDTables *obdTable;
    
public:
    ThreeH_gct(std::vector<oc::block>& keys):keys(keys){
        int hashSize=keys.size(), gamma = 52, v=20;
        double c1 = 1.3;
        int fieldSize = 128;

        // dic = new OBD3Tables(hashSize, c1, fieldSize, gamma, v);
        // dic->init();

        obdTable = new OBD3Tables(hashSize, c1, fieldSize, gamma, v);
        obdTable->init();

        table.resize((obdTable->variables).size());
    }

    ~ThreeH_gct(){
        delete obdTable;
    }

    //do the first half of encoding, for concurrency
    void init(){
        // std::vector<u8> tmpValues(values.size()*sizeof(block));
        // std::memcpy(tmpValues.data(), (u8*)values.data(), values.size()*sizeof(block));
        // obdTable->setVals(values);
        // obdTable->setKeysAndVals(keys, tmpValues);
        obdTable->keys = keys;
        obdTable->fillTables();
        obdTable->peeling();
    }
    //do the second half of encoding, for concurrency
    bool encodeOnlyUnpeeling(std::vector<oc::block>& values){
    // bool encodeOnlyUnpeeling(){

        // obdTable->values.resize(values.size()*sizeof(block));
        // std::memcpy((obdTable->values).data(), (u8*)values.data(), values.size()*sizeof(block));
        // std::vector<u8> tmpValues(values.size()*sizeof(block));
        // std::memcpy(tmpValues.data(), (u8*)values.data(), values.size()*sizeof(block));
        // std::cout<<"In encodeOnlyUnpeeling"<<std::endl;
        obdTable->setVals(values);
        // std::cout<<"finish setVals()"<<std::endl;

        // obdTable->setKeysAndVals(keys, tmpValues);

        // obdTable->generateExternalToolValues();
        // obdTable->unpeeling();

        // vector<uint8_t> x = obdTable->getVariables();
        // std::cout << "after getting the okvs table" << std::endl;
        // std::cout<<x.size()<<std::endl;

        // table.resize(x.size()/16);
        // std::cout<<table.size()<<std::endl;
        // memcpy((uint8_t *)table.data(), x.data(), x.size());
        // std::cout << "before check the output" << std::endl;

        // obdTable->checkOutput();
        // std::cout << "here" << endl;


        // obdTable->fillTables();
        // auto res = obdTable->peeling();
        obdTable->generateExternalToolValues();

        // std::cout<<"finish generateExternalToolValues()"<<std::endl;

        obdTable->unpeeling();

        vector<uint8_t> x = obdTable->getVariables();
        // std::cout << "after getting the okvs table" << std::endl;
        // std::cout<<x.size()<<std::endl;
        // std::cout<<"in only unpeeling, obdTable variables 0: \n"<<obdTable->variables[0]<<std::endl;
        // // std::cout<<"x: "<<x[0]<<x[1]<<x[2]<<x[3]<<x[4]<<x[5]<<std::endl;
        // std::cout<<"[";
        // for(int i=0; i<10; i++){
        //     uint8_t tmp = x[i];
        //     for(int j=0; j<8; j++){
        //         if(tmp&1){
        //             std::cout<<"1 ";
        //         }else{
        //             std::cout<<"0 ";
        //         }
        //         tmp >>= 1;
        //     }
        // }
        // std::cout<<std::endl;

        // table.resize(x.size()/16);
        // std::cout<<table.size()<<std::endl;
        memcpy((uint8_t *)table.data(), x.data(), x.size());
        // std::cout << "before check the output" << std::endl;

        //to make it oblivious
        // for(int i=0; i<table.size(); i++){
        //     if(table[i] == ZeroBlock){
        //         table[i] = sysRandomSeed();
        //     }
        // }

        // obdTable->checkOutput();
        // std::cout << "here" << endl;

        return true;
    }

    //the complete encoding process, inclusing peeling and unpeeling
    bool encode(std::vector<oc::block>& values){
        assert(keys.size() == values.size());

        auto t0 = std::chrono::high_resolution_clock::now();
        std::vector<u8> tmpValues(values.size()*sizeof(block));
        std::memcpy(tmpValues.data(), (u8*)values.data(), values.size()*sizeof(block));
        obdTable->setKeysAndVals(keys, tmpValues);
        auto t1 = std::chrono::high_resolution_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0);
        // std::cout<<"set keys and values takes (ms): "<<dur.count()<<std::endl;

        obdTable->encode();

        // std::cout<<"finish encoding"<<std::endl;
        auto t2 = std::chrono::high_resolution_clock::now();

        vector<uint8_t> x = obdTable->getVariables();
        // std::cout << "after getting the okvs table" << std::endl;
        // std::cout<<x.size()<<std::endl;

        // table.resize(x.size()/16);
        // std::cout<<table.size()<<std::endl;
        memcpy((uint8_t *)table.data(), x.data(), x.size());
        // std::cout << "before check the output" << std::endl;
        auto t3 = std::chrono::high_resolution_clock::now();
        dur = std::chrono::duration_cast<std::chrono::milliseconds>(t3-t2);
        // std::cout<<"copy table takes (ms): "<<dur.count()<<std::endl;
        // obdTable->checkOutput();
        // std::cout << "here" << endl;

        return true;
    }

    //decode the set of keys by using the party's OKVS table
    void decode(std::vector<block>& keys, std::vector<block>& queryValues){
        queryValues.resize(keys.size());
        for (int i = 0; i < keys.size(); i++) {
            // keys[i] = XXH64(&setKeys[i], 64, 0);
            auto value = obdTable->decode(keys[i]);
            memcpy(&queryValues[i], value.data(), sizeof(block));
        }
        return;
    }

    void decodeByTable(std::vector<block>& okvsTable, std::vector<block>& queryValues){
        assert(okvsTable.size() == table.size());
        queryValues.resize(keys.size());

        // std::cout<<"In decode by table, has got the okvs table"<<std::endl;
            // auto indices = obdTable->dec(keys[0]);
            // for(int j=0; j<indices.size(); j++){
            //     std::cout<<indices[j]<<", ";
            // }
            // std::cout<<std::endl;
        for (int i = 0; i < keys.size(); i++) {
            auto indices = obdTable->dec(keys[i]);
            // keys[i] = XXH64(&setKeys[i], 64, 0);
            for(int j=0; j<indices.size(); j++){
                // if(i == 0){
                //     std::cout<<okvsTable[indices[j]]<<", "<<obdTable->variables[indices[j]]<<";  ";
                // }
                queryValues[i] ^= okvsTable[indices[j]];
            }
        }
        return;
    }

    int getTableSize(){
        return table.size();
    }

};
