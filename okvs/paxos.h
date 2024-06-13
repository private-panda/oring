//
// Created by moriya on 7/17/19, from https://github.com/asu-crypto/mPSI/blob/paxos/libPaXoS/ObliviousDictionary.h
//
#pragma once

// #ifndef BENNYPROJECT_OBLIVIOUSDICTIONARY_H
// #define BENNYPROJECT_OBLIVIOUSDICTIONARY_H

#include <unordered_set>
#include <unordered_map>
//#include <libscapi/include/primitives/Prg.hpp>
//#include <libscapi/include/comm/MPCCommunication.hpp>
#include "odhasher.h"
#include <NTL/mat_GF2E.h>
#include <NTL/GF2E.h>
#include <NTL/GF2X.h>
#include <NTL/GF2XFactoring.h>
// #include <span>

// #include "gf2e_mat_solve.h"
#include "linear_solver.h"

// #include "cryptoTools/Crypto/PRNG.h"
// #include "/home/ubuntu/mytest/libOTe/cryptoTools/cryptoTools/Crypto/PRNG.h"
// #include "cryptoTools/Common/Defines.h"


#include <chrono>
#include <queue>
#include <fstream>
typedef unsigned char byte;

using namespace std::chrono;

using namespace std;
using namespace NTL;

using namespace osuCrypto;


typedef std::vector< NTL::GF2E > GF2EVector;
typedef std::vector< GF2EVector > GF2EMatrix;

// using Hasher = Hasher64;
using Hasher = Hasher128;


class ObliviousDictionary {
// protected:
public:

    int hashSize;
    int fieldSize, fieldSizeBytes;
    int gamma, v;

//    PrgFromOpenSSLAES prg;
    // vector<uint64_t> keys;
    vector<osuCrypto::block> keys;

    vector<uint8_t> values;

    // unordered_map<uint64_t, GF2E> vals;
    unordered_map<osuCrypto::block, GF2E> vals; //the map from the key to the value

//    vector<vector<uint64_t>> indices;

    int reportStatistics=0;
    ofstream statisticsFile;

    GF2EVector variables;
    vector<uint8_t> sigma;


public:

    ObliviousDictionary(int hashSize, int fieldSize, int gamma, int v);

     ~ObliviousDictionary(){
         if (reportStatistics == 1) {

             statisticsFile.close();
         }
    }


    // virtual void setKeysAndVals(vector<uint64_t>& keys, vector<uint8_t>& values){
    virtual void setKeysAndVals(vector<osuCrypto::block>& keys, vector<uint8_t>& values){


         this->keys = keys;
         this->values = values;

         vals.clear();
         vals.reserve(hashSize);
         GF2X temp;
         for (int i=0; i < hashSize; i++){

//        for (int j=0; j<fieldSizeBytes; j++){
//            cout<<"val in bytes = "<<(int)(values[i*fieldSizeBytes + j]) << " ";
//        }
//        cout<<endl;

             GF2XFromBytes(temp, values.data() + i*fieldSizeBytes ,fieldSizeBytes);
             vals.insert({keys[i], to_GF2E(temp)});
//        auto tempval = to_GF2E(temp);
//        cout<<"val in GF2E = "<<tempval<<endl;

//        vector<byte> tempvec(fieldSizeBytes);
//        BytesFromGF2X(tempvec.data(), rep(tempval), fieldSizeBytes);
//        for (int j=0; j<fieldSizeBytes; j++){
//            cout<<"returned val in bytes = "<<(int)*(tempvec.data() + j)<< " ";
//        }
//        cout<<endl;
         }
//        for (int i=0; i<hashSize; i++){
//            cout << "key = " << keys[i] << " val = ";
//
//            for (int j=0; j<fieldSizeBytes; j++){
//                cout<<(int)(values[i*fieldSizeBytes + j])<<" ";
//            }
//            cout<<endl;
//        }
     }

    
        // virtual void setKeysAndVals(vector<uint64_t>& keys, vector<uint8_t>& values){
    virtual void setVals(vector<osuCrypto::block>& values){
        this->values.resize(values.size()*sizeof(osuCrypto::block));
        std::memcpy((this->values).data(), (uint8_t*)values.data(), values.size()*sizeof(osuCrypto::block));
        //  this->values = values;

         vals.clear();
         vals.reserve(hashSize);
        //  std::cout<<"hashSize: "<<hashSize<<std::endl;
        //  std::cout<<"fieldSizeBytes: "<<fieldSizeBytes<<std::endl;
        //  std::cout<<"this->values: "<<this->values.size()<<std::endl;
        //  std::cout<<"keys size: "<<this->keys.size()<<std::endl;


        // NTL::GF2X irredPoly = NTL::BuildSparseIrred_GF2X(128);
        // GF2E::init(irredPoly);


         GF2X temp;
         for (int i=0; i < hashSize; i++){

//        for (int j=0; j<fieldSizeBytes; j++){
//            cout<<"val in bytes = "<<(int)(values[i*fieldSizeBytes + j]) << " ";
//        }
//        cout<<endl;
            // std::cout<<temp<<std::endl;
             GF2XFromBytes(temp, (this->values).data() + i*fieldSizeBytes ,fieldSizeBytes);
            // GF2X temp = GF2XFromBytes((this->values).data() + i*fieldSizeBytes ,fieldSizeBytes);
            // std::cout<<temp<<std::endl;
            // std::cout<<to_GF2E(temp)<<std::endl;
             vals.insert({keys[i], to_GF2E(temp)});
//        auto tempval = to_GF2E(temp);
//        cout<<"val in GF2E = "<<tempval<<endl;
        // if(i < 10){
        //     std::cout<<temp<<std::endl;
        // }

//        vector<byte> tempvec(fieldSizeBytes);
//        BytesFromGF2X(tempvec.data(), rep(tempval), fieldSizeBytes);
//        for (int j=0; j<fieldSizeBytes; j++){
//            cout<<"returned val in bytes = "<<(int)*(tempvec.data() + j)<< " ";
//        }
//        cout<<endl;
         }
//        for (int i=0; i<hashSize; i++){
//            cout << "key = " << keys[i] << " val = ";
//
//            for (int j=0; j<fieldSizeBytes; j++){
//                cout<<(int)(values[i*fieldSizeBytes + j])<<" ";
//            }
//            cout<<endl;
//        }
     }

    virtual void init() = 0;

    virtual vector<uint64_t> dec(osuCrypto::block key) = 0;
    virtual vector<uint64_t> decOptimized(osuCrypto::block key) = 0;

    virtual vector<uint8_t> decode(osuCrypto::block key) = 0;

    virtual bool encode() = 0;

    // //do encoding works without unpeeling 
    // virtual bool encodeWithoutUnpeeling() = 0;
    // virtual bool encodeWithUnpeeling() = 0;

    void generateRandomEncoding() {
        // cout<<"variables.size() = "<<variables.size()<<endl;
        // cout<<"fieldSizeBytes = "<<fieldSizeBytes<<endl;

        sigma.resize(variables.size()*fieldSizeBytes);
//        sigma.insert(0);
//        prg.getPRGBytes(sigma, 0, sigma.size());
        int zeroBits = 8 - fieldSize % 8;
        for (int i=0; i<variables.size(); i++){
//        cout << "key = " << keys[i] << " val = ";
//        for (int j=0; j<fieldSizeBytes; j++){
//            cout<<(int)(vals[i*fieldSizeBytes + j])<<" " << std::bitset<8>(vals[i*fieldSizeBytes + j]) << " ";
//        }
//        cout<<endl;

            sigma[(i+1)*fieldSizeBytes-1] = sigma[(i+1)*fieldSizeBytes-1]  >> zeroBits;
//
//        cout << "key = " << keys[i] << " val = ";
//        for (int j=0; j<fieldSizeBytes; j++){
//            cout<<(int)(vals[i*fieldSizeBytes + j])<<" " << std::bitset<8>(vals[i*fieldSizeBytes + j]) << " ";
//        }
//        cout<<endl;
        }
        GF2X temp;
//        vector<byte> temp1(fieldSizeBytes);
        for (int i=0; i < variables.size(); i++){

            GF2XFromBytes(temp, sigma.data() + i*fieldSizeBytes ,fieldSizeBytes);
            variables[i] = to_GF2E(temp);

//            BytesFromGF2X(temp1.data(), rep(variables[i]), fieldSizeBytes);
//
//            for (int j=0; j<fieldSizeBytes; j++){
//                if (temp1[j] != sigma[i*fieldSizeBytes + j])
//                    cout<<"error!! sigma[i*fieldSizeBytes + j] = "<<(int)sigma[i*fieldSizeBytes + j]<<" temp1[j] = "<<(int)temp1[j]<<endl;
//                //            for (int j=0; j<fieldSizeBytes; j++){
//                //                cout<<(int)*(sigma.data() + i*fieldSizeBytes + j)<< " ";
//                //            }
//                //            cout<<endl;
//            }
//        auto tempval = to_GF2E(temp);
//        cout<<"val in GF2E = "<<tempval<<endl;

//        vector<byte> tempvec(fieldSizeBytes);
//        BytesFromGF2X(tempvec.data(), rep(tempval), fieldSizeBytes);
//        for (int j=0; j<fieldSizeBytes; j++){
//            cout<<"returned val in bytes = "<<(int)*(tempvec.data() + j)<< " ";
//        }
//        cout<<endl;
        }

    }

    void setReportStatstics(int flag){
        reportStatistics = flag;
        if (reportStatistics == 1) {

            cout<<"statistics file created"<<endl;
            statisticsFile.open("statistics.csv");
            statisticsFile << "-------------Statistics-----------.\n";
        }};

    virtual vector<uint8_t> getVariables() {

        // if (sigma.size() == 0) { //If the variables do not randomly chosen
        if(sigma.size() == 0){
            sigma.resize(variables.size() * fieldSizeBytes);
        }
            for (int i = 0; i < variables.size(); i++) {
                //            cout<<"variables["<<i<<"] = "<<variables[i]<<endl;
                BytesFromGF2X(sigma.data() + i * fieldSizeBytes, rep(variables[i]), fieldSizeBytes);
                //            for (int j=0; j<fieldSizeBytes; j++){
                //                cout<<(int)*(sigma.data() + i*fieldSizeBytes + j)<< " ";
                //            }
                //            cout<<endl;
            }
//        } else {
//            cout<<"variables.size() = "<<variables.size()<<endl;
//            vector< byte> temp(variables.size() * fieldSizeBytes);
//            for (int i = 0; i < variables.size(); i++) {
//                            cout<<"variables["<<i<<"] = "<<variables[i]<<endl;
//                BytesFromGF2X(temp.data() + i * fieldSizeBytes, rep(variables[i]), fieldSizeBytes);
//                            for (int j=0; j<fieldSizeBytes; j++){
//                                cout<<(int)*(temp.data() + i*fieldSizeBytes + j)<< " ";
//                            }
//                            cout<<endl;
//            }
//            bool error = false;
//            for (int i=0; i<sigma.size(); i++){
//                if (sigma[i] != temp[i]){
//                    error = true;
////                    cout<<"sigma[i] = "<<(int)sigma[i]<<" temp[i] = "<<(int)temp[i]<<endl;
//                }
//            }
//            if (error)
//                cout<<"values have been changed!"<<endl;
        // }

        return sigma;
    }

    virtual bool checkOutput() = 0;

    int getHashSize(){return hashSize;}
    virtual int getTableSize() = 0;
    int getGamma() {return gamma; }

};

class OBDTables : public ObliviousDictionary{
// protected:
public:

    int tableRealSize;

    uint64_t dhSeed;
    // osuCrypto::block dhSeed;

    double c1;

    vector<osuCrypto::block> peelingVector;
    int peelingCounter;

    Hasher DH;

    vector<uint8_t> sign;

    uint64_t getDHBits(osuCrypto::block key);

public:

    OBDTables(int hashSize, double c1, int fieldSize, int gamma, int v) : ObliviousDictionary(hashSize, fieldSize, gamma, v), c1(c1){
        //the value is fixed for tests reasons
        dhSeed = 5;
        DH = Hasher(dhSeed);

//        prg = PrgFromOpenSSLAES(hashSize*fieldSizeBytes*4);
//        auto key = prg.generateKey(128);
//        prg.setKey(key);
    }

    void init() override;

    virtual void createSets() = 0;

    bool encode() override;

    //     //do encoding works without unpeeling 
    // bool encodeWithoutUnpeeling() override;
    // bool encodeWithUnpeeling() override;

    virtual void fillTables() = 0;

    virtual int peeling() = 0;

    virtual void generateExternalToolValues() = 0;

    virtual void unpeeling() = 0;

    virtual bool hasLoop() = 0;

};

class OBD2Tables : public OBDTables{

private:
    uint64_t firstSeed, secondSeed;
    unordered_set<osuCrypto::block, Hasher> first;
    unordered_set<osuCrypto::block, Hasher> second;

public:
    OBD2Tables(int hashSize, double c1, int fieldSize, int gamma, int v);

    void createSets() override;

    void init() override;

    vector<uint64_t> dec(osuCrypto::block key) override;
    vector<uint64_t> decOptimized(osuCrypto::block key) override;

    vector<uint8_t> decode(osuCrypto::block key) override;

    void fillTables() override;

    int peeling() override;

    void generateExternalToolValues() override;

    void unpeeling() override;

    bool checkOutput() override;

    bool hasLoop() override;

    int getTableSize() override {return 2*tableRealSize + gamma;}
};

class OBD3Tables : public OBDTables {
private:
    uint64_t firstSeed, secondSeed, thirdSeed;
    unordered_set<osuCrypto::block, Hasher> first;
    unordered_set<osuCrypto::block, Hasher> second;
    unordered_set<osuCrypto::block, Hasher> third;

    std::vector<uint8_t> nonOccupiedBins;
    void handleQueue(queue<int> &queueMain, unordered_set<osuCrypto::block, Hasher> &main,
                     queue<int> &queueOther1, unordered_set<osuCrypto::block, Hasher> &other1,
                     queue<int> &queueOther2,unordered_set<osuCrypto::block, Hasher> &other2);

public:

    OBD3Tables(int hashSize, double c1, int fieldSize, int gamma, int v);

    void createSets() override;

    void init() override;

    vector<uint64_t> dec(osuCrypto::block key) override;

    vector<uint64_t> decOptimized(osuCrypto::block key) override;

    vector<uint8_t> decode(osuCrypto::block key) override;

    void fillTables() override;

    int peeling() override;

    void generateExternalToolValues() override;

    void unpeeling() override;

    bool checkOutput() override;

    bool hasLoop() override;

    int getTableSize() override {return 3*tableRealSize + gamma;}


};

class StarDictionary : public ObliviousDictionary {
private:
    vector<OBD3Tables*> bins;
    vector<vector<osuCrypto::block>> keysForBins;
    vector<vector<uint8_t>> valsForBins;

    int q;

    Hasher hashForBins;
    int numItemsForBin;
    int center;
    int numThreads = 1;

    void peelMultipleBinsThread(int start, int end, vector<int> &failureIndices, int threadId);

    void unpeelMultipleBinsThread(int start, int end, int failureIndex);

    void setNewValsThread(int start, int end, int failureIndex);

public:

    StarDictionary(int numItems, double c1, double c2, int q, int fieldSize, int gamma, int v, int numThreads = 1);

    void setKeysAndVals(vector<osuCrypto::block>& keys, vector<uint8_t>& values) override;

    void init() override;

    vector<uint64_t> dec(osuCrypto::block key) override;

    vector<uint64_t> decOptimized(osuCrypto::block key) override;

    vector<uint8_t> decode(osuCrypto::block key);

    bool encode() override;

    //     //do encoding works without unpeeling 
    // bool encodeWithoutUnpeeling() override;
    // bool encodeWithUnpeeling() override;
    

    bool checkOutput() override;

    int getTableSize() override {
        return (q+1)*(bins[0]->getTableSize());
    }

    vector<uint8_t> getVariables() override;

    bool checkOutput(osuCrypto::block key, int valIndex);
};




// #endif //BENNYPROJECT_OBLIVIOUSDICTIONARY_H
