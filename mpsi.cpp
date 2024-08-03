
#include "coproto/Socket/AsioSocket.h"


#include "libOTe/Vole/Silent/SilentVoleReceiver.h"
#include "libOTe/Vole/Silent/SilentVoleSender.h"

#include "okvs/okvs.h"

#include "util.h"

#include <fstream>

using coproto::AsioSocket;
using coproto::asioConnect;
using coproto::sync_wait;

void party(int n, int t, int m, int party_idx, int topology, int net){
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

    std::chrono::high_resolution_clock::time_point m_start, m_stop, m_online_start;
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
        block omega_prime;
        cp::sync_wait(chls[i].recv(omega_prime));

        omega = omega^omega_prime;

        //send A'=A^P to the sender
        std::vector<block> A_prime(numOTs);// A_prime
        for(int j=0; j<numOTs; j++){
            A_prime[j] = A[j] ^ paxos.table[j];
        }
        cp::sync_wait(chls[i].send(A_prime));

        std::vector<block> queried_result(m);
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

    //for the t parties P1...Pt, they need to both act as a receiver and a sender for another party at the same time by using the same channel.
    //Therefore, when Pi interact with Pj (i<j), it can be an OPRF sender first and an OPRF later by using the same channel. It is noted that cannot
    //act as the sender and receiver both by using two threads because they use the same channel; otherwise there will be channle issue.
    auto sender_receiver_oprf_worker = [&](int i){
        // std::cout<<"Party "<<party_idx<<" act as the sender to party "<<i<<std::endl;
        //be an OPRF sender first
        sender_oprf_worker(i);
        //be an OPRF receiver later by using the same channle
        receiver_oprf_worker(i);

        // std::cout<<std::endl<<"In double invokation, Party "<<party_idx<<" with "<<i<<", sender_oprf_values[i][0]: "<<sender_oprf_values[i][0]<<std::endl;
        // std::cout<<std::endl<<"In double invokation, Party "<<party_idx<<" with "<<i<<", receiver_oprf_values[i][0]: "<<receiver_oprf_values[i][0]<<std::endl;
    
    };
    auto receiver_sender_oprf_worker = [&](int i){
        // std::cout<<"Party "<<party_idx<<" act as the sender to party "<<i<<std::endl;
        //To first as an OPRF receiver
        receiver_oprf_worker(i);
        //be an OPRF sender later by using the same channle
        sender_oprf_worker(i);

        // std::cout<<std::endl<<"In double invokation, Party "<<party_idx<<" with "<<i<<", receiver_oprf_values[i][0]: "<<receiver_oprf_values[i][0]<<std::endl;
        // std::cout<<std::endl<<"In double invokation, Party "<<party_idx<<" with "<<i<<", sender_oprf_values[i][0]: "<<sender_oprf_values[i][0]<<std::endl;
    };

    if(party_idx == 0){
        std::vector<oc::block> values(m);
        if(t != n-1){
            // std::cout<<"In party 0, masking"<<std::endl;
            std::vector<oc::block> mask_seeds(n-t);
            prng.SetSeed(oc::sysRandomSeed());
            prng.get(mask_seeds.data(), mask_seeds.size());
            std::vector<std::vector<oc::block>> masks(n-t);
            auto mask_worker = [&](int i){
                masks[i].resize(m);
                AES aes(mask_seeds[i]);
                cp::sync_wait(chls[i].send(mask_seeds[i]));
                aes.ecbEncBlocks(party_set, masks[i]);
                // std::cout<<std::endl<<"Party "<<party_idx<<" with "<<i<<" a mask: "<<masks[i][0]<<std::endl;
            };

            for(int i=1; i < n-t; i++){
                party_threads[i] = std::thread(mask_worker, i);
            }
            for(int i=1; i < n-t; i++){
                party_threads[i].join();
            }

            values = masks[1];
            for(int i=2; i<n-t; i++){
                for(int j=0; j<m; j++){
                    values[j] ^= masks[i][j];
                }
            }
        }
        for(int i=n-t; i<n; i++){
            // std::cout<<"Party 0 as a receiver with "<<i<<std::endl;
            party_threads[i] = std::thread(receiver_oprf_worker, i);
        }
        for(int i=n-t; i<n; i++){
            party_threads[i].join();
        }
        for(int i=n-t; i<n; i++){
            for(int j=0; j<m; j++){
                values[j] ^= receiver_oprf_values[i][j];
            }
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

        std::vector<oc::block> intersection;
        for(int i=0; i<m; i++){
            if(memcmp((u8*)&values[i], (u8 *)&query_results[i], output_byte_length)==0){
                intersection.emplace_back(party_set[i]);
            }
        }
        std::cout<<"The intersection size: "<<intersection.size()<<std::endl;
    }else if(party_idx < n-t){//for the parties that are OPRF receivers

        // std::cout<<"In party "<<party_idx<<std::endl;
        std::vector<oc::block> values(m);
        if(t != n-1){
            oc::block mask_seed;
            cp::sync_wait(chls[0].recv(mask_seed));
            aes.setKey(mask_seed);
            aes.ecbEncBlocks(party_set, values);

        }

        if(t != 1){
            for(int i=n-t; i<n; i++){
                party_threads[i] = std::thread(receiver_oprf_worker, i);
            }
            for(int i=n-t; i<n; i++){
                party_threads[i].join();
            }

            for(int i=n-t; i<n; i++){
                for(int j=0; j<m; j++){
                    values[j] ^= receiver_oprf_values[i][j];
                }
            }
        }

        if(topology == 0){
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

        }else{
            //for k-star
            // std::cout<<"party "<<party_idx<<" sends the mask"<<" to party 1"<<std::endl;
            paxos.encodeOnlyUnpeeling(values);
            
            std::vector<u8> send_buffer(okvs_table_size*output_byte_length);
            u8 *send_buffer_ptr = send_buffer.data();
            for(int i=0; i<okvs_table_size; i++){
                std::memcpy(send_buffer_ptr, (u8 *)&paxos.table[i], output_byte_length);
                send_buffer_ptr += output_byte_length;
            }

            cp::sync_wait(chls[n-1].send(send_buffer));
        }
    }else{//for the last t parties
        //to respond to party 0...n-t-1 as OPRF senders
        party_threads[0] = std::thread(sender_oprf_worker, 0);

        if(t != 1){
            for(int i=1; i<n-t; i++){
                party_threads[i] = std::thread(sender_oprf_worker, i);
            }  
            if(party_idx == n-t){
                for(int i=n-t+1; i<n; i++){
                    party_threads[i] = std::thread(sender_receiver_oprf_worker, i);
                }
            }else{
                for(int i = n-t; i<party_idx; i++){
                    party_threads[i] = std::thread(receiver_sender_oprf_worker, i);
                }
                for(int i=party_idx+1; i<n; i++){
                    party_threads[i] = std::thread(sender_receiver_oprf_worker, i);
                }
            }
            for(int i=0; i<n; i++){
                if( i != party_idx){
                    party_threads[i].join();
                }
            }            
        }else{
            party_threads[0].join();
        }


        auto& values = sender_oprf_values[0];
        if(t != 1){
            for(int i=1; i<n-t; i++){
                for(int j=0; j<m; j++){
                    values[j] ^= sender_oprf_values[i][j];
                    // values[j] ^= receiver_oprf_values[i][j];
                }
            }
            for(int i=n-t; i<party_idx; i++){
                for(int j=0; j<m; j++){
                    values[j] ^= sender_oprf_values[i][j];
                    values[j] ^= receiver_oprf_values[i][j];
                }
            }        
            for(int i=party_idx+1; i<n; i++){
                for(int j=0; j<m; j++){
                    values[j] ^= sender_oprf_values[i][j];
                    values[j] ^= receiver_oprf_values[i][j];
                }
            }
        }

        if(topology == 0){
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
                std::memcpy(send_buffer_ptr, (u8 *)&paxos.table[i], output_byte_length);
                send_buffer_ptr += output_byte_length;
            }
            
            if(party_idx == n-1){
                cp::sync_wait(chls[0].send(send_buffer));
            }else{
                cp::sync_wait(chls[party_idx+1].send(send_buffer));
            }

        }else{
            //for k-star
            // std::cout<<"Party "<<party_idx<<" for K-Star"<<std::endl;

            if(party_idx == n-1){
                std::vector<std::vector<oc::block>> queried_result(n);

                auto okvs_receiver_worker = [&](int i){
                    queried_result[i].resize(m);
                    std::vector<u8> recv_buffer(okvs_table_size*output_byte_length);
                    u8 *recv_buffer_ptr = recv_buffer.data();
                    cp::sync_wait(chls[i].recv(recv_buffer));

                    std::vector<oc::block> okvs(okvs_table_size);
                    for(int j=0; j<okvs.size(); j++){
                        std::memcpy((u8 *)&okvs[j], recv_buffer_ptr, output_byte_length);
                        recv_buffer_ptr += output_byte_length;
                    }

                    paxos.decodeByTable(okvs, queried_result[i]);

                };
                for(int i=1; i<n-1; i++){
                    party_threads[i] = std::thread(okvs_receiver_worker, i);
                    party_threads[i].join();
                }
                for(int i=1; i<n-1; i++){
                    for(int j=0; j<m; j++){
                        values[j] ^= queried_result[i][j];
                    }
                }
            }
            paxos.encodeOnlyUnpeeling(values);
            
            std::vector<u8> send_buffer(okvs_table_size*output_byte_length);
            u8 *send_buffer_ptr = send_buffer.data();
            for(int i=0; i<okvs_table_size; i++){
                std::memcpy(send_buffer_ptr, (u8 *)&paxos.table[i], output_byte_length);
                send_buffer_ptr += output_byte_length;
            }

            if(party_idx != n-1){
                cp::sync_wait(chls[n-1].send(send_buffer));
            }else{
                cp::sync_wait(chls[0].send(send_buffer));
            }
        }

    }
    
    m_stop = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(m_stop-m_start) ;

    std::string performance_data;
    performance_data = to_string(dur.count())+"\n";

    //for the communication cost
    int send_bytes = 0, recv_bytes = 0;
    for(int i=0; i<n; i++){
        if(i != party_idx){
            cp::sync_wait(chls[i].flush());
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
    // std::string file_name("./data/star_wan_party_"); file_name += to_string(okvs_type)+"_"+to_string(n)+"_"; file_name += to_string(t)+"_"; file_name += to_string(m)+"_"; 
    file_name += ".txt";
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
    //when n==3, K-Star gets the same topology as O-Ring
    // if(n == 3){
    //     party(n, t, m, party_idx, 0, net);
    // }else{
    //     party(n, t, m, party_idx, topology, net);
    // }
    party(n, t, m, party_idx, topology, net);


    return 0;

}
