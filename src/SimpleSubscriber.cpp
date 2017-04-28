//
// Created by Chuilian Kong on 1/31/2017.
//

#include "../include/SimpleSubscriber.h"
#include "../include/helper.h"

SimpleSubscriber::SimpleSubscriber(std::mutex* lock_msg_buf, std::string ip, std::string port, int group, std::string log_path)
        :_lock_msg_buf(lock_msg_buf), context(1), _group(group), socket_to_center(context, ZMQ_SUB), _fout(log_path) {
    std::string connect_str = "tcp://" + ip + ":" + port;
    socket_to_center.connect(connect_str.c_str());
}

// a thread will keep checking history list, if a entry existed over 10 seconds, remove it.
int SimpleSubscriber::inspectHistoryList(){
    auto p = [](auto item) {
        std::chrono::duration<double> time_elapse = std::chrono::system_clock::now() - item.second;
        return time_elapse.count() > 10;
    };
    for(auto it = _history_list.begin(); it != _history_list.end();)
    {
        if(p(*it)) it = _history_list.erase(it);
        else ++it;
    }
    return 0;
}

int SimpleSubscriber::inspector(){
    std::cout << "Inspector Thread Started.\n";
    while(true){
        // inspectHistoryList function will be invoked every _send_period millisecond
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        inspectHistoryList();
    }
}

int SimpleSubscriber::subscribe(std::string name) {
    if(_subscription_list.find(name) == _subscription_list.end()){
        // no existed subscription, add it to list
        _subscription_list.insert(name);
        // create a new history entry with a time stamp
        _history_list[name] = std::chrono::system_clock::now();
        // add to filter
        socket_to_center.setsockopt(ZMQ_SUBSCRIBE, name.c_str(), name.size());
    }
    return 0;
}

int SimpleSubscriber::unsubscribe(std::string name) {
    _subscription_list.erase(name);
    socket_to_center.setsockopt(ZMQ_UNSUBSCRIBE, name.c_str(), name.size());
    return 0;
}

// a thread will run user interface
int SimpleSubscriber::interface(){
    std::cout << "Interface Thread Started.\n";
    std::string line;
    while(true){
        std::getline(std::cin, line);
        std::vector<std::string> res = split(line, ' ');
        if(res[0] == "sub"){
            std::cout<<"subscribe topic: "<< res[1]<<std::endl;
            subscribe(res[1]);
        }else if(res[0] == "unsub"){
            std::cout<<"unsubscribe topic: "<< res[1]<<std::endl;
            unsubscribe(res[1]);
        }else if(res[0] == "list"){
            std::cout<<"subscription list:"<<std::endl;
            printSubList();
        }else {
            std::cout<<"undefined command"<<std::endl;
        }
    }
    return 0;
}

int SimpleSubscriber::printSubList(){
    for(auto item : _subscription_list){
        std::cout<<item<<std::endl;
    }
    return 0;
}


int SimpleSubscriber::msgToBuf(SimpleCenter::Message msg){
    // here comes critical section
    {
        // critical section
        // this guard will automatically release the mutex when out of scope
        std::lock_guard<std::mutex> guard(*_lock_msg_buf);
        _msg_buf.push_back(msg);
    }
    return 0;
}

int SimpleSubscriber::msgOut(){
    // clear message buffer, print message to log file
    // a vector to store fetched message
    std::vector<SimpleCenter::Message> msgs;
    // retrieve all message from msg buffer
    {
        // critical section
        std::lock_guard<std::mutex> guard(*_lock_msg_buf);
        std::swap(msgs, _msg_buf);
    }
    // eliminate repeated messages
    std::sort(msgs.begin(),msgs.end(),[](auto a, auto b){
        if(a._topic < b._topic){
            return true;
        }else if(a._topic > b._topic){
            return false;
        }else {
            return a._content < b._content;
        }
    });
    msgs.erase(unique(msgs.begin(),msgs.end(),[](auto a, auto b){
        return a._topic == b._topic && a._content == b._content;
    }),msgs.end());
    // sort messages according to born time.
    std::sort(msgs.begin(),msgs.end(),[](auto a, auto b){
        return a._timeTag < b._timeTag;
    });
    // print messages to log file
    for(auto item : msgs){
        revMsgInfo(item);
    }
    // flush file io
    _fout.flush();
    return 0;

}

int SimpleSubscriber::logger(){
    std::cout << "Logger Thread Started.\n";
    // this thread will clear buffer and print message to log file every 30 seconds
    while(true){
        std::this_thread::sleep_for(std::chrono::milliseconds(30000));
        msgOut();
    }
}

int SimpleSubscriber::receive() {
    zmq::message_t update;
    socket_to_center.recv(&update);
    char *data = static_cast<char *>(update.data());
    SimpleCenter::Message msg = parseMessage(data);
    if (msg._availableGroup != _group) return 0;
    // from here the message is for this group
    if(!msg._isHistory){
        // this message is not a history, just accept it.
        msgToBuf(msg);
    }else{
        // this message is a history, see if on the list
        if(_history_list.find(msg._topic) != _history_list.end()){
            // on the list, accept
            msgToBuf(msg);
        }
    }
    return 0;
}

// a thread will run receiver
int SimpleSubscriber::receiver(){
    std::cout << "Receiver Thread Started.\n";
    int counter = 0;
    while(true){
        receive();
        counter++;
        if(counter == 30){
            // flush ofstream every 30 messages.
            _fout.flush();
            counter = 0;
        }
    }
}
SimpleCenter::Message SimpleSubscriber::parseMessage(const char *data) {
    std::string in(data);
    std::vector<std::string> res = split(in, '\n');
    return SimpleCenter::Message(res[0].c_str(), res[1].c_str(), std::stoi(res[2]), 0, 0, 0, std::stoi(res[3]));
}

int SimpleSubscriber::revMsgInfo(SimpleCenter::Message msg) {
    if (msg._isHistory) {
        _fout << "*****History Received*****\n";
    } else {
        _fout << "*****Message Received*****\n";
    }
    _fout << "topic: " << msg._topic.c_str() << "\n"
          << "content: " << msg._content.c_str() << "\n"
          << "**************************\n";
    return 0;
}