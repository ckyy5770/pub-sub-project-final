//
// Created by Chuilian Kong on 4/25/2017.
//

#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include "../include/SimpleCenter.h"

struct Commands {
    std::string _receiver_port;
    std::string _sender_port;
    unsigned _send_period; // this para specified how long should it take between two send call. in milliseconds
    unsigned _resource_limit;
    std::string _log_path;

    Commands() : _receiver_port("5555"), _sender_port("5556"), _send_period(5000), _resource_limit(10),
                 _log_path("centerLog.log") {};
};

Commands parseCommands(std::vector<std::string> args) {
    Commands cmd;
    for (uint32_t i = 1; i < args.size(); i++) {
        size_t pos;
        if ((pos = args[i].find("rec=")) != std::string::npos) {
            cmd._receiver_port = args[i].substr(pos + 4);
        } else if ((pos = args[i].find("send=")) != std::string::npos) {
            cmd._sender_port = args[i].substr(pos + 5);
        } else if ((pos = args[i].find("path=")) != std::string::npos) {
            cmd._log_path = args[i].substr(pos + 5);
        } else if ((pos = args[i].find("period=")) != std::string::npos) {
            cmd._send_period = static_cast<unsigned>(std::stoi(args[i].substr(pos + 7)));
        } else if ((pos = args[i].find("reslimit=")) != std::string::npos) {
            cmd._resource_limit = static_cast<unsigned>(std::stoi(args[i].substr(pos + 9)));
        } else {
            std::cout << "invalid command line argument: " << args[i] << std::endl;
        }
    }
    return cmd;
}

void print_command(Commands cmd) {
    std::cout << "system config:\n"
              << "_receiver_port: " << cmd._receiver_port << std::endl
              << "_sender_port: " << cmd._sender_port << std::endl
              << "_send_period: " << cmd._send_period << std::endl
              << "_resource_limit: " << cmd._resource_limit << std::endl
              << "_log_path: " << cmd._log_path << std::endl;
}


int main(int argc, char **argv) {
    std::vector<std::string> args(argv, argv + argc);
    std::mutex lock_msg_buf;
    // parse command line args
    Commands cmd = parseCommands(args);
    ///// debug info
    print_command(cmd);
    // initialize center
    SimpleCenter center( &lock_msg_buf, cmd._receiver_port, cmd._sender_port, cmd._send_period, cmd._resource_limit,
                        cmd._log_path);
    // one thread run receiver, one thread run sender
    std::thread t_receiver(&SimpleCenter::receiver, std::ref(center));
    std::thread t_sender(&SimpleCenter::sender,std::ref(center));
    ///// debug info
    std::cout << "system started." << std::endl;
    // wait threads finish and return
    t_receiver.join();
    t_sender.join();
    return 0;
}

