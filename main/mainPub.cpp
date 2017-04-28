//
// Created by Chuilian Kong on 4/25/2017.
//
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <string>
#include <mutex>
#include "../include/SimplePublisher.h"
#include "../include/SimpleCenter.h"

struct Commands {
    std::string _ip;
    std::string _port;
    std::string _log_dir;
    int _is_first; // if this is the first publisher
    int _strength;
    int _id;
    std::string _log_path;

    Commands() : _ip("localhost"), _port("5555"),
                 _log_dir("./"), _is_first(0), _strength(1), _id(0), _log_path("") {};
};

Commands parseCommands(std::vector<std::string> args) {
    Commands cmd;
    for (uint32_t i = 1; i < args.size(); i++) {
        size_t pos;
        if ((pos = args[i].find("ip=")) != std::string::npos) {
            cmd._ip = args[i].substr(pos + 3);
        } else if ((pos = args[i].find("port=")) != std::string::npos) {
            cmd._port = args[i].substr(pos + 5);
        } else if ((pos = args[i].find("dir=")) != std::string::npos) {
            cmd._log_dir = args[i].substr(pos + 4);
        } else if ((pos = args[i].find("first=")) != std::string::npos) {
            cmd._is_first = std::stoi(args[i].substr(pos + 6));
        } else if ((pos = args[i].find("strength=")) != std::string::npos) {
            cmd._strength = std::stoi(args[i].substr(pos + 9));
        } else {
            std::cout << "invalid command line argument: " << args[i] << std::endl;
        }
    }
    return cmd;
}

void print_command(Commands cmd) {
    std::cout << "system config:\n"
              << "_ip: " << cmd._ip << std::endl
              << "_port: " << cmd._port << std::endl
              << "_log_dir: " << cmd._log_dir << std::endl
              << "_is_first: " << cmd._is_first << std::endl
              << "_strength: " << cmd._strength << std::endl
              << "_id: " << cmd._id << std::endl
              << "_log_path: " << cmd._log_path << std::endl;
}

void configID(Commands &cmd) {
    if (cmd._is_first) {
        std::ifstream in("curPubNum.log");
        // if this is the first publisher, just overwrite current curPubNum.log file with 1 and get id 0
        in.close();
        cmd._id = 0;
        std::ofstream out("curPubNum.log");
        out << 1;
        out.close();
    } else {
        // this is not the first publisher
        std::ifstream in("curPubNum.log");
        in >> cmd._id;
        in.close();
        std::ofstream out("curPubNum.log");
        out << cmd._id + 1;
        out.close();
    }
}

void configLogPath(Commands &cmd) {
    cmd._log_path = cmd._log_dir + "pub" + std::to_string(cmd._id) + ".log";
}

SimpleCenter::Message makeMsg(Commands &cmd,int cur_msg_num){
    std::string topic = "topic" + std::to_string(std::rand()%5);
    int availableGroup = rand()%2;
    int strength = cmd._strength;
    int lifeSpan = std::rand()%2 * 10;
    int timeTag = (int)time(nullptr);
    std::string content = "msg from pubID: " + std::to_string(cmd._id)
                          + " msgID:" + std::to_string(cur_msg_num) + " "
                          + "availableGroup: " + std::to_string(availableGroup) + " "
                          + "strength: " + std::to_string(strength) + " "
                          + "lifeSpan: " + std::to_string(lifeSpan) + " "
                          + "timeTag: " + std::to_string(timeTag) + " ";
    SimpleCenter::Message msg(topic.c_str(),content.c_str(),availableGroup,strength,lifeSpan,timeTag);
    return msg;
}

void publisher(Commands &cmd){
    // initialize publisher
    SimplePublisher pub(cmd._ip, cmd._port, cmd._strength, cmd._log_path);
    int cur_num = 0;
    while(true){
        SimpleCenter::Message msg = makeMsg(cmd,cur_num);
        pub.send(msg._topic.c_str(),msg._content.c_str(),msg._availableGroup,msg._lifeSpan,msg._timeTag);
        pub.msgInfo(msg);
        cur_num++;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

int main(int argc, char **argv) {
    std::vector<std::string> args(argv, argv + argc);
    // parse command line args
    Commands cmd = parseCommands(args);
    // get publisher ID, and config log path
    configID(cmd);
    configLogPath(cmd);
    ///// debug info
    print_command(cmd);
    // one thread run publisher
    std::thread t_pub(publisher,std::ref(cmd));
    ///// debug info
    std::cout << "publisher "<<std::to_string(cmd._id)<< " started." << std::endl;
    // wait threads finish and return
    t_pub.join();
    return 0;
}
