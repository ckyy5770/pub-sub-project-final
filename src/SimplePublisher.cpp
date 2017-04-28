//
// Created by Chuilian Kong on 1/31/2017.
//

#include "../include/SimplePublisher.h"

#include <iostream>

SimplePublisher::SimplePublisher(std::string ip, std::string port,
                                 int strength, std::string log_path)
        : context(1), _strength(strength), socket_to_center(context, ZMQ_PUB),_fout(log_path) {
    std::string connect_str = "tcp://" + ip + ":" + port;
    socket_to_center.connect(connect_str.c_str());
}

void SimplePublisher::send(const char *topic, const char *content, int availableGroup, int lifeSpan, int timeTag) {
    zmq::message_t message(1000);
    snprintf((char *) message.data(), 1000, "%s\n%s\n%d\n%d\n%d\n%d", topic, content, availableGroup, _strength,
             lifeSpan, timeTag);
    socket_to_center.send(message);
}

int SimplePublisher::msgInfo(SimpleCenter::Message msg) {
    _fout << "*****Message Sent*****\n"
          << "topic: " << msg._topic.c_str() << "\n"
          << "content: " << msg._content.c_str() << "\n"
          << "availableGroup: " << msg._availableGroup << "\n"
          << "ownershipStrength: " << msg._strength << "\n"
          << "lifeSpan: " << msg._lifeSpan << "\n"
          << "timeTag: " << msg._timeTag << "\n"
          << "**************************\n";
    return 0;
}