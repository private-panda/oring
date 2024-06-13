
#include "coproto/Socket/AsioSocket.h"

#include "libOTe/Vole/Silent/SilentVoleReceiver.h"
#include "libOTe/Vole/Silent/SilentVoleSender.h"

#include "okvs/okvs.h"

#include "util.h"

#include <fstream>

using coproto::AsioSocket;
using coproto::asioConnect;
using coproto::sync_wait;


void partyOringT1(int n, int t, int m, int party_idx, int topology, int net){
    std::cout<<"In party "<<party_idx<<std::endl;

    std::vector<oc::block> party_set(m);

    PRNG prng(sysRandomSeed()+toBlock(party_idx*100));
    PRNG prng_common(toBlock(12345));
    prng_common.get(party_set.data(), 10);
    prng.get(party_set.data()+10, m-10);

    std::string ip("127.0.0.1:");
    int port_base = 12121, port_step = 100;

    int output_byte_length = (log2(m)+40+7)/8;
    RandomOracle H(output_byte_length);
    AES aes(toBlock(234522));

    std::chrono::high_resolution_clock::time_point m_start, m_stop;

    m_start = std::chrono::high_resolution_clock::now();

    //get the channels
    std::vector<AsioSocket> chls(n);
    // std::cout<<"The network ports: ";
    for(int i=0; i<n; i++){
        int port = 0;
        if(i > party_idx){
            port = port_base+port_step*i+party_idx;
            chls[i] = cp::asioConnect(ip+to_string(port_base+port_step*i+party_idx), 1);
        }else if(i < party_idx){
            port = port_base+port_step*party_idx+i;
            chls[i] = cp::asioConnect(ip+to_string(port_base+port_step*party_idx+i), 0);
        }
        // std::cout<<port<<", ";
    }
    // std::cout<<std::endl;

    std::vector<std::thread> party_threads(n);

    //do the multi-sender oprf, every party is an OPRF receiver
    //to collect the oprf value sum
    // std::cout<<"Act as the receiver"<<std::endl;
    //act the receiver of the multi-sender oprf, use n threads
    std::vector<block> h1_values(m);
    aes.ecbEncBlocks(party_set, h1_values);
    // paxos.encode(h1_values);

    ThreeH_gct paxos(party_set);
    paxos.encode(h1_values);

    int okvs_table_size = paxos.getTableSize();
    int numOTs = okvs_table_size;


    std::vector<std::vector<oc::block>> receiver_oprf_values(n);

    auto receiver_oprf_worker = [&](int i){
        // std::cout<<"Party "<<party_idx<<" act as the receiver to "<<i<<std::endl;

        receiver_oprf_values[i].resize(m);

        RandomOracle H(output_byte_length);
        
        AlignedUnVector<oc::block> C(numOTs);
        AlignedUnVector<oc::block> A(numOTs);

        SilentVoleReceiver<oc::block, oc::block> receiver;
        receiver.mMultType = MultType::Tungsten;

        receiver.configure(numOTs);

        if(i > party_idx){
            cp::sync_wait(oc::sync(chls[i], Role::Sender));
        }else{
            cp::sync_wait(oc::sync(chls[i], Role::Receiver));
        }
        cp::sync_wait(receiver.silentReceive(A, C, prng, chls[i]));

        PRNG pr(sysRandomSeed());
        block omega = pr.get();
        cp::sync_wait(chls[i].send(omega));
        // cp::sync_wait(sock[0].send(omega));
        block omega_prime;
        cp::sync_wait(chls[i].recv(omega_prime));
        // cp::sync_wait(sock[0].recv(omega_prime));

        omega = omega^omega_prime;

        //send A'=A^P to the sender
        std::vector<block> A_prime(numOTs);// A_prime
        for(int j=0; j<numOTs; j++){
            A_prime[j] = A[j] ^ paxos.table[j];
            // choice_prime[j] = choice[j] + paxos.bins[j];
        }
        cp::sync_wait(chls[i].send(A_prime));

        std::vector<block> queried_result(m);
        // paxos.decodeByTable(msgs, party_set, queried_result);
        std::vector<oc::block> CC(C.begin(), C.end());

        paxos.decodeByTable(CC, queried_result);

        block tmp_result;
        for(int j=0; j<m; j++){
            tmp_result = queried_result[j] ^ omega;

            H.Reset();
            H.Update((u8*)&tmp_result, 16);
            H.Update((u8*)&party_set[j], 16);
            H.Final((u8 *)&receiver_oprf_values[i][j]);
        }
    };

    std::vector<std::vector<oc::block>> sender_oprf_values(n);

    auto sender_oprf_worker = [&](int i){
        // std::cout<<"Party "<<party_idx<<" act as the sender to party "<<i<<std::endl;

        sender_oprf_values[i].resize(m);

        // ThreeH_gct paxos(party_set);
        RandomOracle H(output_byte_length);

        //to act as a sender in the multi-sender oprf
        // std::cout<<"Act as the sender"<<std::endl;
        SilentVoleSender<oc::block, oc::block> sender;
        sender.mMultType = MultType::Tungsten;

        sender.configure(numOTs);

        AlignedUnVector<oc::block> B(numOTs);
        block delta = sysRandomSeed();
        // perform the OTs and write the random OTs to msgs.
        if(i > party_idx){
            cp::sync_wait(oc::sync(chls[i], Role::Sender));
        }else{
            cp::sync_wait(oc::sync(chls[i], Role::Receiver));
        }
        cp::sync_wait(sender.silentSend(delta, B, prng, chls[i]));
        // std::cout<<"B[0]: "<<B[0]<<std::endl;

        PRNG pr(sysRandomSeed());
        block omega_prime;
        cp::sync_wait(chls[i].recv(omega_prime));
        // cp::sync_wait(sock[1].recv(omega_prime));
        block omega = pr.get();
        cp::sync_wait(chls[i].send(omega));
        // cp::sync_wait(sock[1].send(omega));

        omega = omega^omega_prime;

        //to receive A_prime from the receiver
        std::vector<block> A_prime(numOTs);
        cp::sync_wait(chls[i].recv(A_prime));

        //to get K=B+A'*delta
        std::vector<oc::block> K(A_prime.size());
        for(int j=0; j<A_prime.size(); j++){
            K[j] = A_prime[j].gf128Mul(delta) ^ B[j]; 
        }

        std::vector<block> queried_result(m);
        paxos.decodeByTable(K, queried_result);

        block tmp_result;

        for(int j=0; j<m; j++){
            tmp_result = h1_values[j].gf128Mul(delta);
            tmp_result = queried_result[j] ^ tmp_result ^ omega;

            H.Reset();
            H.Update((u8*)&tmp_result, 16);
            H.Update((u8*)&party_set[j], 16);
            H.Final((u8*)&sender_oprf_values[i][j]);
        }
    };


    if(party_idx == 0){
        std::vector<oc::block> values(m);

        // std::cout<<"In party 0, masking"<<std::endl;
        std::vector<oc::block> mask_seeds(2);
        prng.SetSeed(oc::sysRandomSeed());
        prng.get(mask_seeds.data(), mask_seeds.size());
        // std::vector<std::vector<oc::block>> masks(2);
        auto mask_worker = [&](int i){
            // masks[i].resize(m);
            cp::sync_wait(chls[i].send(mask_seeds[i]));
            aes.setKey(mask_seeds[i]);
            aes.ecbEncBlocks(party_set, values);
            // std::cout<<std::endl<<"Party "<<party_idx<<" with "<<i<<" a mask: "<<masks[i][0]<<std::endl;
        };

        party_threads[1] = std::thread(mask_worker, 1);
        party_threads[n-1] = std::thread(receiver_oprf_worker, n-1);

        party_threads[1].join();
        party_threads[n-1].join();

        for(int j=0; j<m; j++){
            values[j] ^= receiver_oprf_values[n-1][j];
        }

        std::vector<u8> recv_buffer(okvs_table_size*output_byte_length);
        //receive an OKVS table from P_{n-1}
        cp::sync_wait(chls[n-1].recv(recv_buffer));
        u8 *recv_buffer_ptr = recv_buffer.data();
        std::vector<oc::block> result_okvs(okvs_table_size);
        for(int i=0; i<okvs_table_size; i++){
            std::memcpy((u8 *)&result_okvs[i], recv_buffer_ptr, output_byte_length);
            recv_buffer_ptr += output_byte_length;
        }
        
        std::vector<oc::block> query_results(m);
        paxos.decodeByTable(result_okvs, query_results);
        // paxos.decodeByTable(result_okvs, party_set, query_results);
        // std::cout<<"Party "<<party_idx<<" received a value "<<query_results[0]<<" from party n-1"<<std::endl;

        std::vector<oc::block> intersection;
        for(int i=0; i<m; i++){
            if(memcmp((u8*)&values[i], (u8 *)&query_results[i], output_byte_length)==0){
                intersection.emplace_back(party_set[i]);
            }
        }
        std::cout<<"The intersection size: "<<intersection.size()<<std::endl;
    }else if(party_idx < n-1){//for the parties that are OPRF receivers

        // std::cout<<"In partyyy "<<party_idx<<std::endl;
        std::vector<oc::block> values(m);
        if(party_idx == 1){
            oc::block mask_seed;
            // std::vector<oc::block> masks(m);
            cp::sync_wait(chls[0].recv(mask_seed));
            aes.setKey(mask_seed);
            aes.ecbEncBlocks(party_set, values);
        }

        //for oring
        if(party_idx != 1){
            std::vector<u8> recv_buffer(okvs_table_size*output_byte_length);
            u8 *recv_buffer_ptr = recv_buffer.data();
            cp::sync_wait(chls[(party_idx-1)].recv(recv_buffer));
            std::vector<oc::block> mask_okvs(okvs_table_size);
            for(int i=0; i<mask_okvs.size(); i++){
                std::memcpy((u8 *)&mask_okvs[i], recv_buffer_ptr, output_byte_length);
                recv_buffer_ptr += output_byte_length;
            }
            std::vector<oc::block> queried_result(m);
            paxos.decodeByTable(mask_okvs, queried_result);

            for(int j=0; j<m; j++){
                values[j] ^= queried_result[j];
            }
        }
        paxos.encodeOnlyUnpeeling(values);
        
        std::vector<u8> send_buffer(okvs_table_size*output_byte_length);
        u8 *send_buffer_ptr = send_buffer.data();
        for(int i=0; i<okvs_table_size; i++){
            std::memcpy(send_buffer_ptr, (u8 *)&paxos.table[i], output_byte_length);
            send_buffer_ptr += output_byte_length;
        }
        cp::sync_wait(chls[party_idx+1].send(send_buffer));
        // std::cout<<"Party "<<party_idx<<" sends an OKVS table to the next party"<<std::endl;

    }else{//for the last t parties
        party_threads[0] = std::thread(sender_oprf_worker, 0);
        party_threads[0].join();

        auto& values = sender_oprf_values[0];

        //for oring
        // std::cout<<"Party "<<party_idx<<" for O-Ring"<<std::endl;
        if(party_idx != 1){
            std::vector<u8> recv_buffer(okvs_table_size*output_byte_length);
            u8 *recv_buffer_ptr = recv_buffer.data();
            cp::sync_wait(chls[(party_idx-1)].recv(recv_buffer));
            std::vector<oc::block> okvs(okvs_table_size);
            for(int i=0; i<okvs.size(); i++){
                std::memcpy((u8 *)&okvs[i], recv_buffer_ptr, output_byte_length);
                recv_buffer_ptr += output_byte_length;
            }
            std::vector<oc::block> queried_result(m);
            paxos.decodeByTable(okvs, queried_result);

            for(int j=0; j<m; j++){
                values[j] ^= queried_result[j];
            }
        }
        paxos.encodeOnlyUnpeeling(values);
        
        std::vector<u8> send_buffer(okvs_table_size*output_byte_length);
        u8 *send_buffer_ptr = send_buffer.data();
        for(int i=0; i<okvs_table_size; i++){
            // share_sums[i] ^= (receiver_oprf_values[i] ^ sender_oprf_values[i]);
            // tmp_result = receiver_oprf_values[i] ^ sender_oprf_values[i];
            std::memcpy(send_buffer_ptr, (u8 *)&paxos.table[i], output_byte_length);
            send_buffer_ptr += output_byte_length;
        }
        
        cp::sync_wait(chls[0].send(send_buffer));

    }
    
    m_stop = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(m_stop-m_start) ;

    std::string performance_data;
    performance_data = to_string(dur.count())+"\n";
    // performance_data += to_string(offline_dur.count())+"\n";

    //for the communication cost
    int send_bytes = 0, recv_bytes = 0;
    for(int i=0; i<n; i++){
        if(i != party_idx){
            cp::sync_wait(chls[i].flush());
            // sync_wait(chls[i].flush());
            recv_bytes += chls[i].bytesReceived();
            send_bytes += chls[i].bytesSent();
            chls[i].close();
        }
    }

    // IoStream::lock;
    std::cout<<"Party idx: "<<party_idx<<std::endl;
    std::cout<<"The total time (ms): "<<dur.count()<<std::endl;
    std::cout<<"The communication cost (MB): "<<"send "<<(send_bytes*1.0/(1<<20))<<", recv "<<(recv_bytes*1.0/(1<<20))<<std::endl;
    std::cout<<"Total (MB): "<<((send_bytes+recv_bytes)*1.0/(1<<20))<<std::endl;
    // IoStream::unlock;


    std::string communication_data = std::to_string(party_idx)+": ";
    communication_data += to_string(send_bytes*1.0/(1<<20))+"\t";
    communication_data += to_string(recv_bytes*1.0/(1<<20))+"\t";
    communication_data += to_string((send_bytes+recv_bytes)*1.0/(1<<20))+"\n";

    performance_data += communication_data;
    if(party_idx == 0){
        performance_data += "\n";
    }

    // std::string file_name("./data/star_lan_");
    std::string file_name("./data/");
    if(topology == 0){
        if(net == 0){
            file_name += "ring_lan_";
        }else{
            file_name += "ring_wan_";
        }
        
    }else{
        if(net == 0){
            file_name += "star_lan_";
        }else{
            file_name += "star_wan_";
        }
    }

    file_name += to_string(n)+"_"; file_name += to_string(t)+"_"; file_name += to_string(m)+"_"; 
    file_name += "(t1).txt";
    // IoStream::lock;
    // std::string communication_file_name = file_name;
    // if(party_idx == 0 || party_idx == 1 || party_idx == n-1){
        // file_name += to_string(party_idx)+".txt";
        std::ofstream performance_file(file_name, std::ios::app);
        performance_file << party_idx <<std::endl;
        performance_file << performance_data;
        performance_file.close();
    // }
    // IoStream::unlock;
}

int main(int argc, char** argv){
    int n = atoi(argv[1]);
    int t = atoi(argv[2]);
    int m = 1 << atoi(argv[3]);
    int party_idx = atoi(argv[4]);
    int topology = atoi(argv[5]);
    int net = atoi(argv[6]);

    std::cout<<"n, t, m: "<<n<<", "<<t<<", "<<m<<std::endl;
    // testOkvsChain(okvs_type, n, m);
    if(topology == 0){
        std::cout<<"For O-Ring"<<std::endl;
    }else{
        std::cout<<"For K-Star"<<std::endl;
    }
    if(net == 0){
        std::cout<<"For LAN"<<std::endl;
    }else{
        std::cout<<"For WAN"<<std::endl;
    }

    //for PSI test
    partyOringT1(n, 1, m, party_idx, topology, net);
    
    return 0;

}
