//
// Created by Chuilian Kong on 4/25/2017.
//

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
#include "../include/SimpleSubscriber.h"
//#include "../include/helper.h"

struct Commands {
    std::string _ip;
    std::string _port;
    std::string _log_dir;
    int _is_first; // if this is the first publisher
    int _group;
    int _id;
    std::string _log_path;

    Commands() : _ip("localhost"), _port("5556"),
                 _log_dir("./"), _is_first(0), _group(0), _id(0), _log_path("") {};
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
        } else if ((pos = args[i].find("group=")) != std::string::npos) {
            cmd._group = std::stoi(args[i].substr(pos + 6));
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
              << "_group: " << cmd._group << std::endl
              << "_id: " << cmd._id << std::endl
              << "_log_path: " << cmd._log_path << std::endl;
}

void configID(Commands &cmd) {
    if (cmd._is_first) {
        std::ifstream in("curSubNum.log");
        // if this is the first publisher, just overwrite current curPubNum.log file with 1 and get id 0
        in.close();
        cmd._id = 0;
        std::ofstream out("curSubNum.log");
        out << 1;
        out.close();
    } else {
        // this is not the first publisher
        std::ifstream in("curSubNum.log");
        in >> cmd._id;
        in.close();
        std::ofstream out("curSubNum.log");
        out << cmd._id + 1;
        out.close();
    }
}

void configLogPath(Commands &cmd) {
    cmd._log_path = cmd._log_dir + "sub" + std::to_string(cmd._id) + ".log";
}

int main(int argc, char **argv) {
    std::vector<std::string> args(argv, argv + argc);
    std::mutex lock_msg_buf;
    // parse command line args
    Commands cmd = parseCommands(args);
    // get publisher ID, and config log path
    configID(cmd);
    configLogPath(cmd);
    ///// debug info
    print_command(cmd);
    // initialize subscriber
    SimpleSubscriber sub(&lock_msg_buf,cmd._ip, cmd._port, cmd._group, cmd._log_path);
    // three threads
    std::thread t_receiver(&SimpleSubscriber::receiver,std::ref(sub));
    std::thread t_inspector(&SimpleSubscriber::inspector,std::ref(sub));
    std::thread t_logger(&SimpleSubscriber::logger,std::ref(sub));
    std::thread t_interface(&SimpleSubscriber::interface,std::ref(sub));
    ///// debug info
    std::cout << "subscriber "<<std::to_string(cmd._id)<< " started." << std::endl;
    // wait threads finish and return
    t_receiver.join();
    t_inspector.join();
    t_logger.join();
    t_interface.join();
    return 0;
}
