#include <unistd.h>
#include <sstream>
#include <map>

#include "mq.hpp"

void pid(zmq::socket_t& parent_socket) {
    send_message(parent_socket, "OK: " + std::to_string(getpid()));
}

// broken
void create(int& child_id, zmq::socket_t& parent_socket, zmq::socket_t& child_socket, std::istringstream& request) {
    int new_child_id;
    request >> new_child_id;

    if (child_id != -1) {
        unbind(child_socket, child_id);
    }
    bind(child_socket, new_child_id);
    pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("Can't create new process");
    }
    if (pid == 0) {
        int err = execl("./calculation", "./calculation", std::to_string(new_child_id).c_str(), std::to_string(child_id).c_str(), NULL);
        if (err == -1) throw std::runtime_error("Can't execute new process");
    }
    send_message(child_socket, std::to_string(new_child_id) + "pid");
    child_id = new_child_id;
    send_message(parent_socket, receive_message(child_socket));
}

void set(int cur_id, zmq::socket_t& parent_socket, std::map<std::string, int>& dictionary, std::istringstream& request) {
    std::string key;
    int value;
    request >> key >> value;
    dictionary[key] = value;
    send_message(parent_socket, "OK:" + std::to_string(cur_id));
}

void get(int cur_id, zmq::socket_t& parent_socket, std::map<std::string, int>& dictionary, std::istringstream& request) {
    std::string key, res;
    int value;
    request >> key;
    try {
        value = dictionary.at(key);
        res = std::to_string(value);
    }
    catch (...) {
        res = "'" + key + "' not found";
    }
    send_message(parent_socket, "OK:" + std::to_string(cur_id) + ": " + res);
}

// broken
void kill(int& cur_id, int& child_id, zmq::socket_t& parent_socket, zmq::socket_t& child_socket) {
    if (child_id != -1) {
        send_message(child_socket, std::to_string(child_id) + " kill");
        std::string msg;
        try {
            msg = receive_message(child_socket);
        }
        catch (...) {
            msg = "kill failed " + std::to_string(child_id);
            send_message(parent_socket, msg);
        }
        unbind(child_socket, child_id);
    } else {
        send_message(parent_socket, "OK");
    }
    disconnect(parent_socket, cur_id);
}


int main(int argc, char* argv[]) {
    if (argc != 2 && argc != 3) {
        throw std::runtime_error("Wrong args for calculating node");
    }
    int cur_id = std::atoi(argv[1]);

    int child_id = -1;
    if (argc == 3) {
        child_id = std::atoi(argv[2]);
    }

    zmq::context_t context;
    zmq::socket_t parent_socket(context, ZMQ_REP);

    connect(parent_socket, cur_id);

    zmq::socket_t child_socket(context, ZMQ_REQ);
    child_socket.setsockopt(ZMQ_RCVTIMEO, 5000);

    if (child_id != -1) {
        bind(child_socket, child_id);
    }

    std::map<std::string, int> dictionary;

    std::string message;
    try {
        while (true) {
            message = receive_message(parent_socket);
            std::istringstream request(message);
            int dest_id;
            request >> dest_id;

            std::string cmd;
            request >> cmd;

            if (dest_id == cur_id) {
                if (cmd == "pid") {
                    pid(parent_socket);
                }

                else if (cmd == "create") {
                    create(child_id, parent_socket, child_socket, request);
                }

                else if (cmd == "set") {
                    set(cur_id, parent_socket, dictionary, request);
                }

                else if (cmd == "get") {
                    get(cur_id, parent_socket, dictionary, request);
                }

                else if (cmd == "ping") {
                    send_message(parent_socket, "OK: 1");
                }

                else if (cmd == "kill") {
                    kill(cur_id, child_id, parent_socket, child_socket);
                }
            } else if (child_id != -1) {
                send_message(child_socket, message);
                send_message(parent_socket, receive_message(child_socket));
                if (child_id == dest_id && cmd == "kill") {
                    child_id = -1;
                }
            } else {
                throw std::runtime_error("Node is unavailable");
            }
        }
    }
    catch (std::exception& e) {
        send_message(parent_socket, e.what());
        if (child_id != -1) {
            send_message(child_socket, std::to_string(child_id) + " kill");
            std::string msg;
            receive_message(child_socket);
            unbind(child_socket, child_id);
        }
        disconnect(parent_socket, cur_id);
    }
}
