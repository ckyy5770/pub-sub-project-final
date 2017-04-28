//
// Created by Chuilian Kong on 1/31/2017.
//

#ifndef PUB_SUB_PROJECT_SIMPLEPUBLISHER_H
#define PUB_SUB_PROJECT_SIMPLEPUBLISHER_H

#include "../include/SimpleCenter.h"
#include "zmq.hpp"
#include <fstream>

#if defined(WIN32)

#include <iostream>
#include <zhelpers.hpp>

#endif // WIN32

class SimplePublisher {
public:
    /**
     * @brief Default constructor
     * @details default constructor will initialize context and connect socket to
     * central service. Default port for central service (receive message) is
     * :5555. Default ip address of central service is localhost.
     * strength is a publisher specific notion,it specifies how powerful a
     * publisher is.
     * strength info will be attached to every piece of message the publisher send
     * to central service.
     * if central service see the same topic published by different publishers at
     * the nearly same time, central service will only send the one will highest
     * strength to subscribers.
     */
    SimplePublisher(std::string ip = "localhost", std::string port = "5555",
                    int strength = 1, std::string log_path = "pubLog.log");

    /**
     * @brief send message to central service
     * @details users can use send method to send message to publisher so that it
     * send them to central service. Users should specify the topic and content it
     * send.
     */
    void send(const char *topic = nullptr, const char *content = nullptr, int availableGroup = 0, int lifeSpan = 0,
              int timeTag = 0);
    int msgInfo(SimpleCenter::Message msg);
private:
    zmq::context_t context;
    // socket to central service
    zmq::socket_t socket_to_center;
    // publisher ownershipStrength
    int _strength;
    // file io
    std::ofstream _fout;
};

#endif // PUB_SUB_PROJECT_SIMPLEPUBLISHER_H
