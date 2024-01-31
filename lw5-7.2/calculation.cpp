#include <unistd.h>
#include <sstream>
#include <map>
#include "mq.hpp"


void create(std::istringstream& is, MQ& mq, int& child_id) {
    int new_child_id;
    is >> new_child_id;
    pid_t pid = fork();
    if (pid < 0) {
        std::string error_msg = "Error: Can't create new process";
        mq.send(error_msg);
        exit(-2);
    }
    if (pid == 0) {
        execl("./calculation", "./calculation", std::to_string(new_child_id).c_str(), std::to_string(child_id).c_str(), NULL);
        std::string error_msg = "Error: Can't execute new process";
        mq.send(error_msg);
        exit(-3);
    }
    child_id = new_child_id;
    mq.add_child(child_id);
    std::string ans = mq.receive(child_id);
    mq.send(ans);
}

void set(std::istringstream& is, MQ& mq, std::map<std::string, std::string>& dictionary, int id) {
    std::string key, value;
    is >> key >> value;
    dictionary[key] = value;
    std::string ans = "OK: " + std::to_string(id);
    mq.send(ans);
}

void get(std::istringstream& is, MQ& mq, std::map<std::string, std::string>& dictionary, int id) {
    std::string key, value, ans;
    is >> key;
    try {
        value = dictionary.at(key);
        ans = "OK:" + std::to_string(id) + ": " + value;
    }
    catch (...) {
        ans = "OK:" + std::to_string(id) + ": " + "'" + key + "' not found";
    }
    mq.send(ans);
}

void ping(MQ& mq) {
    std::string ans = "OK 1";
    mq.send(ans);
}

int main(int argc, char* argv[]) {
    if (argc != 2 && argc != 3) {
        exit(-1);
    }
    int cur_id = std::atoi(argv[1]);
    MQ mq{cur_id};

    int child_id;
    if (argc == 3) {
        child_id = std::atoi(argv[2]);
        mq.add_child(child_id);
    }

    std::string init_msg = "OK: " + std::to_string(getpid());
    mq.send(init_msg);

    std::map<std::string, std::string> dictionary;

    while (true) {
        std::string message = mq.receive();
        auto is = std::istringstream(message);

        int dest_id;
        is >> dest_id;

        std::string cmd;
        is >> cmd;

        if (cmd == "heartbeat") {
            int time;
            is >> time;
            mq.set_rcvtimeo(time * 4000);
        }

        if (dest_id == cur_id) {
            if (cmd == "create") {
                create(is, mq, child_id);
            } else if (cmd == "set") { // dest_id   set key value 
                set(is, mq, dictionary, cur_id);
            } else if (cmd == "get") { // dest_id get key
                get(is, mq, dictionary, cur_id);
            } else if (cmd == "heartbeat") { // dest_id heartbeat   time
                ping(mq);
            }
        } else {
            mq.send(child_id, message);
            std::string ans = mq.receive(child_id);
            mq.send(ans);
        }

        if (cmd == "heartbeat") {
            mq.set_rcvtimeo(3000);
        }
    }
}
