## Introduction 
The source codes for "O-Ring and K-Star: Efficient Multi-party Private Set Intersection" in Usenix 24.

## Setup libraries
1. Install [NTL](https://libntl.org/doc/tour-unix.html).
2. Compile [xxHash](https://github.com/Cyan4973/xxHash).
2. Download [libOTe](https://github.com/osu-crypto/libOTe). `git clone https://github.com/osu-crypto/libOTe.git`

## How to compile the codes?
1. `cd libOTe`
2. `git clone `
3. put "xxHash" in "oring/okvs/"
3. In the CMakeLists.txt, add `add_subdirectory(oring)` after `add_subdirectory(frontend)`.
4. Compile libOTe as the "README.md" in libOTe

## How to run the codes?
1. put "run_protocols.sh" in "libOTe/out/build/linux/oring".
2. The parameters are set in "n t m topology net", where
   
    n: the number of parties
   
    t: the number of colluding parties
   
    m: the log_2 of the set size
   
    topology: 0 for O-Ring; others for K-Star
   
    net: 0 for LAN; others for WAN

    Since the topology is different from others for O-Ring when t=1, we seperate this case with others by having the command `oringt1' in "libOTe/out/build/linux/oring". For other cases, we have the command 'mpsi'. When n=3, K-Star gets the same topology as O-Ring. Then K-Star can use the data for O-Ring in these cases. 

***Running examples***.
   
    1). To run the case for for O-Ring when t=1. One can uncomment `oringt1` in "run_protocols.sh" and comment `mpsi`.  
       Then `./run_protocol.sh 10 1 20 0 0` for n=10, t=1, m=2**20.
   
    2). To run others. Uncomment `mpsi` in "run_protocols.sh" and comment `oringt1`.  Then  
        `./run_protocol.sh 10 4 20 0 0` for n=10, t=4, m=2**20 for O-Ring in LAN setting.
        `./run_protocol.sh 3 2 16 1 1` for n=3, t=2, m=2**16 for K-Star in WAN setting.
   
     It is noted that one can set the network by using command `tc`.

    



