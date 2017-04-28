//
// Created by Chuilian Kong on 1/31/2017.
//

#ifndef PUB_SUB_PROJECT_SIMPLESUBSCRIBER_H
#define PUB_SUB_PROJECT_SIMPLESUBSCRIBER_H

#include "zmq.hpp"
#include <iostream>
#include <string>
#include <mutex>
#include <unordered_set>
#include <unordered_map>
#include "SimpleCenter.h"
#include <chrono>

class SimpleSubscriber {
public:
    /**
     * @brief constructor
     * @details constructor will initialize context and connect socket to central
     * service. Default port for central service (send message) is :5556. Default
     * ip address of central service is localhost.
     * MUST provide a mutex to build the object
     */
    SimpleSubscriber(std::mutex* lock_msg_buf, std::string ip = "localhost", std::string port = "5556",int group = 0, std::string log_path = "subLog.log");



    /**
     * @brief receive one message from central service, decode them into topic and
     * content and storing them into corresponding input pointers.
     */


    int receiver();

    int inspector();

    int interface();

    int logger();

private:
    //  member functions
    /**
    * @brief add the subscription to the list specified by name, and add
    * corresponding filter.
    *
    */
    int subscribe(std::string name);
    /**
    * @brief remove the subscription from the list specified by name, and delete
    * corresponding filter.
    *
    */
    int unsubscribe(std::string name);

    int printSubList();

    SimpleCenter::Message parseMessage(const char *data);

    int revMsgInfo(SimpleCenter::Message msg);

    int receive();

    int inspectHistoryList();

    int msgToBuf(SimpleCenter::Message msg);

    int msgOut();



    //member vars
    zmq::context_t context;
    // socket to central service
    zmq::socket_t socket_to_center;
    // group category of this subscriber
    int _group;
    // a string list storing all subscriptions the subscriber currently has.
    std::unordered_set<std::string> _subscription_list;
    // a topic list that should receive history
    typedef std::chrono::time_point<std::chrono::system_clock> Time;
    std::unordered_map<std::string,Time> _history_list;
    // file io
    std::ofstream _fout;
    // message buffer
    std::vector<SimpleCenter::Message> _msg_buf;
    // mutex to access msg buffer
    std::mutex* _lock_msg_buf;
};

#endif // PUB_SUB_PROJECT_SIMPLESUBSCRIBER_H
