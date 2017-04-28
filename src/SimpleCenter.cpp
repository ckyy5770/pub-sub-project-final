//
// Created by Chuilian Kong on 2/2/2017.
//
#include "../include/SimpleCenter.h"
#include "../include/helper.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

SimpleCenter::SimpleCenter(std::mutex *lock_msg_buf, std::string receiver_port, std::string sender_port, unsigned send_period,
                           unsigned resource_limit, std::string log_path)
        : _lock_msg_buf(lock_msg_buf),receiver_context(1), sender_context(1), _send_period(send_period), _resource_limit(resource_limit), _fout(log_path.c_str()),
          receiver_socket(receiver_context, ZMQ_SUB),
          sender_socket(sender_context, ZMQ_PUB) {
    // init receiver_socket
    std::string receiver_bind_str = "tcp://*:" + receiver_port;
    receiver_socket.bind(receiver_bind_str.c_str());
    receiver_socket.setsockopt(ZMQ_SUBSCRIBE, nullptr, 0);
    // init sender_socket
    std::string sender_bind_str = "tcp://*:" + sender_port;
    sender_socket.bind(sender_bind_str.c_str());
}

SimpleCenter::Message SimpleCenter::parseMessage(const char *data) {
    std::string in(data);
    std::vector<std::string> res = split(in, '\n');
    return Message(res[0].c_str(), res[1].c_str(), std::stoi(res[2]), std::stoi(res[3]), std::stoi(res[4]),
                   std::stoi(res[5]));
}

int SimpleCenter::sendOneMsg(Message msg) {
    zmq::message_t message(1000);
    snprintf((char *) message.data(), 1000, "%s\n%s\n%d\n%d", msg._topic.c_str(), msg._content.c_str(),
             msg._availableGroup,msg._isHistory);
    sender_socket.send(message);
    return 0;
}

int SimpleCenter::sendMsg(std::vector<Message> msgs) {
    for (int i = 0; i < msgs.size(); i++) {
        sendOneMsg(msgs[i]);
        sendMsgInfo(msgs[i]);
    }
    return 0;
}

int SimpleCenter::sendHistory() {
    for (auto item : _hisBuf) {
        auto cur_buf = *(item.second);
        for (auto entry : cur_buf) {
            sendOneMsg(entry);
            sendHisInfo(entry);
        }
    }
    return 0;
}

int SimpleCenter::oneMsgToHistory(Message msg) {
    // first change history tag to true
    msg._isHistory = 1;
    std::string topic = msg._topic;
    if (_hisBuf.find(topic) == _hisBuf.end()) {
        // topic not found, this is a new topic, initialize ring buffer
        _hisBuf[topic] = new boost::circular_buffer<Message>(_resource_limit);
    }
    _hisBuf[msg._topic]->push_back(msg);
    return 0;
}

int SimpleCenter::msgToHistory(std::vector<Message> msgs) {
    for (int i = 0; i < msgs.size(); i++) {
        oneMsgToHistory(msgs[i]);
    }
    return 0;
}

int SimpleCenter::receive() {
    // receive one message
    zmq::message_t update;
    receiver_socket.recv(&update);
    char *data = static_cast<char *>(update.data());
    Message msg = parseMessage(data);
    // put it into the message buffer
    // here comes critical section
    {
        // critical section
        // this guard will automatically release the mutex when out of scope
        std::lock_guard<std::mutex> guard(*_lock_msg_buf);
        _msgBuf.push_back(msg);
    }
    revMsgInfo(msg);
    return 0;
}

int SimpleCenter::send() {
    // a vector to store fetched message
    std::vector<Message> msgs;
    // retrieve all message from msg buffer
    {
        // critical section
        std::lock_guard<std::mutex> guard(*_lock_msg_buf);
        std::swap(msgs, _msgBuf);
    }
    // here we pre-process messages.
    // remove messages that life span passed
    msgs.erase( std::remove_if(msgs.begin(),msgs.end(),[](auto a){
        return a._lifeSpan == 0 || ((int)time(nullptr) - a._timeTag) > a._lifeSpan;
    }), msgs.end());
    // erase those who have the same topic and available group, only preserve the one with higher ownership strength (first appearance in this pre-ordered sequence)
    // first need to give it a order, topic(main), availableGroup(secondary), strength(third)
    std::sort(msgs.begin(),msgs.end(),[](auto a, auto b){
        if(a._topic < b._topic){
            return true;
        }else if(a._topic > b._topic){
            return false;
        }else{
            if(a._availableGroup < b._availableGroup){
                return true;
            }else if(a._availableGroup > b._availableGroup){
                return false;
            }else{
                return a._strength > b._strength;
            }
        }
    });
    msgs.erase( std::unique(msgs.begin(),msgs.end(),[](auto a, auto b){
        return a._topic == b._topic && a._availableGroup == b._availableGroup;
    }),msgs.end());
    // sort remaining messages according to time tag
    std::sort(msgs.begin(),msgs.end(),[](auto a, auto b){
        return a._timeTag < b._timeTag;
    });

    // first send a history message
    sendHistory();
    // then send all messages
    sendMsg(msgs);
    _fout.flush();
    // then put all sent messages into history buffer
    msgToHistory(msgs);
}

void SimpleCenter::sender(){
    std::cout << "Sender Thread Started.\n";
    while(true){
        // send function will be invoked every _send_period millisecond
        std::this_thread::sleep_for(std::chrono::milliseconds(_send_period));
        send();
    }
}

void SimpleCenter::receiver(){
    std::cout << "Receiver Thread Started.\n";
    while(true){
        receive();
    }
}

int SimpleCenter::sendMsgInfo(Message msg) {
    _fout << "*******Message Sent*******\n"
          << "topic: " << msg._topic.c_str() << "\n"
          << "content: " << msg._content.c_str() << "\n"
          << "availableGroup: " << msg._availableGroup << "\n"
          << "**************************\n";
    return 0;
}

int SimpleCenter::revMsgInfo(Message msg) {
    _fout << "*****Message Received*****\n"
          << "topic: " << msg._topic.c_str() << "\n"
          << "content: " << msg._content.c_str() << "\n"
          << "availableGroup: " << msg._availableGroup << "\n"
          << "ownershipStrength: " << msg._strength << "\n"
          << "lifeSpan: " << msg._lifeSpan << "\n"
          << "timeTag: " << msg._timeTag << "\n"
          << "**************************\n";
    return 0;
}

int SimpleCenter::sendHisInfo(Message msg) {
    _fout << "*******History Sent*******\n"
          << "topic: " << msg._topic.c_str()<< "\n"
          << "content: " << msg._content.c_str() << "\n"
          << "availableGroup: " << msg._availableGroup << "\n"
          << "isHistory: " << msg._isHistory << "\n"
          << "**************************\n";
    return 0;
}


