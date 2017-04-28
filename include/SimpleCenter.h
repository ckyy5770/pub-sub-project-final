//
// Created by Chuilian Kong on 2/2/2017.
//

#ifndef PUB_SUB_PROJECT_SIMPLECENTER_H
#define PUB_SUB_PROJECT_SIMPLECENTER_H

#include "zmq.hpp"
#include <cstring>
#include <iostream>
#include <mutex>
#include <vector>
#include <chrono>
#include <thread>
#include <boost/circular_buffer.hpp>
#include <unordered_map>
#include <fstream>
#include <string>


class SimpleCenter {
public:
    /**
     * @brief constructor
     * @details constructor will create two sockets.
     * one socket for receiving all messages from publishers.
     * one socket for sending messages to subscribers.
     * the receiver socket will bind to port 5555 by default.
     * the sender socket will bind to port 5556 by default.
     * the receiver socket will subscribe all messages from publisher, which means
     * it will set message filter to an empty option value.
     * send function will be invoked every _send_period millisecond
     * resource_limit will specify maximum length of history buffer
     * MUST provide a mutex to build the object
     */
    SimpleCenter(std::mutex* lock_msg_buf,
                 std::string receiver_port = "5555",
                 std::string sender_port = "5556",
                 unsigned send_period = 5000,
                 unsigned resource_limit = 20,
                 std::string log_path = "centerLog.log");

     // a thread will run receiver
     void receiver();


    // a thread will run sender
    void sender();

    // message structure for communicating
    struct Message {
        std::string _topic;
        std::string _content;
        int _availableGroup;
        int _strength;
        int _lifeSpan;
        int _timeTag;
        int _isHistory;

        Message(const char *topic = nullptr, const char *content = nullptr, int availableGroup = 0, int strength = 0,
                int lifeSpan = 0, int timeTag = 0, int isHistory = 0)
                : _topic(topic), _content(content), _availableGroup(availableGroup), _strength(strength),
                  _lifeSpan(lifeSpan), _timeTag(timeTag),_isHistory(isHistory) {
        };
    };

private:

    // useful functions, self-explanatory
    Message parseMessage(const char *data);

    int sendOneMsg(Message msg);

    int sendMsg(std::vector<Message> msgs);

    int sendHistory();

    int oneMsgToHistory(Message msg);

    int msgToHistory(std::vector<Message> msgs);

    int sendMsgInfo(Message msg);

    int revMsgInfo(Message msg);

    int sendHisInfo(Message msg);

    int receive();

    int send();

    // data members
    zmq::context_t receiver_context;
    // socket to publishers
    zmq::socket_t receiver_socket;

    zmq::context_t sender_context;
    // socket to subscribers
    zmq::socket_t sender_socket;

    // this para specified how long should it take between two send call. in milliseconds
    unsigned _send_period;

    // this para specified maximum length of history buffer
    unsigned _resource_limit;

    // message buffer for received message
    std::vector<Message> _msgBuf;

    // following message array is used to store the history information
    // this is a topic based history buffer
    // key is the topic, and value is the history buffer bind to the topic.
    // since only sending thread access history buffer, we don't need a mutex for accessing it
    std::unordered_map<std::string, boost::circular_buffer < Message>*> _hisBuf;

    // mutex to guard msg buffer
    std::mutex* _lock_msg_buf;

    // file io
    std::ofstream _fout;
};

#endif // PUB_SUB_PROJECT_SIMPLECENTER_H
