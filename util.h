#pragma once
// This file and the associated implementation has been placed in the public domain, waiving all copyright. No restrictions are placed on its use.


#include <cryptoTools/Common/CLP.h>
#include <cryptoTools/Common/Log.h>
#include <cryptoTools/Network/Channel.h>
#include "libOTe/config.h"
#include <functional>
#include <string>
#include "libOTe/Tools/Coproto.h"

namespace osuCrypto
{
    enum class Role
    {
        Sender,
        Receiver
    };

#ifdef ENABLE_BOOST
    void senderGetLatency(Channel& chl);

    void recverGetLatency(Channel& chl);
    void getLatency(CLP& cmd);
    void sync(Channel& chl, Role role);
#endif

    task<> sync(Socket& chl, Role role);



    // using ProtocolFunc = std::function<void(Role, int, int, std::string, std::string, CLP&)>;

    // inline bool runIf(ProtocolFunc protocol, CLP & cmd, std::vector<std::string> tag,
    //                   std::vector<std::string> tag2 = std::vector<std::string>())
    // {
    //     auto n = cmd.isSet("nn")
    //         ? (1 << cmd.get<int>("nn"))
    //         : cmd.getOr("n", 0);

    //     auto t = cmd.getOr("t", 1);
    //     auto ip = cmd.getOr<std::string>("ip", "localhost:1212");

    //     // std::cout<<"tags: "<<tag.back()<<", "<<tag2.back()<<std::endl;

    //     if (!cmd.isSet(tag))
    //         return false;

    //     if (!tag2.empty() && !cmd.isSet(tag2))
    //         return false;

    //     if (cmd.hasValue("r"))
    //     {
    //         auto role = cmd.get<int>("r") ? Role::Sender : Role::Receiver;
    //         protocol(role, n, t, ip, tag.back(), cmd);
    //     }
    //     else
    //     {
    //         auto thrd = std::thread([&] {
    //             try { protocol(Role::Sender, n, t, ip, tag.back(), cmd); }
    //             catch (std::exception& e)
    //             {
    //                 lout << e.what() << std::endl;
    //             }
    //             });

    //         try { protocol(Role::Receiver, n, t, ip, tag.back(), cmd); }
    //         catch (std::exception& e)
    //         {
    //             lout << e.what() << std::endl;
    //         }
    //         thrd.join();
    //     }

    //     return true;
    // }



}
