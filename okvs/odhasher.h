#pragma once

#include "xxHash/xxhash.h"
// #include "../../../libOTe/cryptoTools/cryptoTools/Common/block.h"
// #include "xxHash/xxh_x86dispatch.h"

#include "cryptoTools/Common/block.h"
// using namespace osuCrypto;

class Hasher32{
    private:

    unsigned long long seed;

public:

    Hasher32() {
        seed = 0;
    }

    Hasher32(unsigned long long seed) : seed(seed){    }

    Hasher32(const Hasher32 & hash){
        seed = hash.seed;
    }

    size_t operator() (uint64_t const key) const
    {
        unsigned long const hash = XXH32(&key, 4, seed);
        return hash;
    }
};

class Hasher64{//can be faster than Hasher32
    private:

    unsigned long long seed;

public:

    Hasher64() {
        seed = 0;
    }

    Hasher64(unsigned long long seed) : seed(seed){    }

    Hasher64(const Hasher64 & hash){
        seed = hash.seed;
    }

    size_t operator() (uint64_t const key) const
    {
        unsigned long const hash = XXH3_64bits_withSeed(&key, 8, seed);
        // unsigned long const hash = XXH64(&key, 8, seed);//slower than above XXH3_64bits_withSeed

        return hash;
    }
};

class Hasher128{
    private:

    unsigned long long seed;

public:

    Hasher128() {
        seed = 0;
    }

    Hasher128(unsigned long long seed) : seed(seed){    }

    Hasher128(const Hasher128 & hash){
        seed = hash.seed;
    }

    size_t operator() (osuCrypto::block const key) const
    {
        XXH128_hash_t const hash = XXH3_128bits_withSeed((unsigned char *)&key, 16, seed);
        return hash.high64;
    }
};

