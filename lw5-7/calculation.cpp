#include <unistd.h>
#include <sstream>
#include <map>
#include "mq.hpp"

class CalculationNode {
private:
    int cur_id;
    int child_id;
    zmq::context_t context;
    zmq::socket_t parent_socket, parent_ping_socket, child_socket, child_ping_socket;
    std::map<std::string, int> dictionary;
public:
    CalculationNode(int _cur_id) {
        cur_id = _cur_id;
        child_id = -1;
        zmq::socket_t parent_socket{context, ZMQ_REP};
        connect(parent_socket, cur_id * 2);

        zmq::socket_t parent_ping_socket{context, ZMQ_REP};
        connect(parent_ping_socket, cur_id * 2 + 1);

        zmq::socket_t child_socket(context, ZMQ_REQ);
        child_socket.set(zmq::sockopt::rcvtimeo, 2000);

        zmq::socket_t child_ping_socket(context, ZMQ_REQ);
        child_ping_socket.set(zmq::sockopt::rcvtimeo, 2000);
    }

    ~CalculationNode() {
        disconnect(parent_socket, cur_id * 2);
        disconnect(parent_ping_socket, cur_id * 2 + 1);

        if (child_id != -1) {
            unbind(child_socket, child_id * 2);
            unbind(child_ping_socket, child_id * 2 + 1);
        }
    }

    void set_child_socket(int child_id) {
        this->child_id = child_id;
        bind(child_socket, child_id * 2);
        bind(child_ping_socket, child_id * 2 + 1);
    }

    void pid() {
        send_message(parent_socket, "OK: " + std::to_string(getpid()));
    }

    void create(std::istringstream& is) {
        int new_child_id;
        is >> new_child_id;

        if (child_id != -1) {
            unbind(child_socket, child_id * 2);
            unbind(child_ping_socket, child_id * 2 + 1);
        }
        try {
            bind(child_socket, new_child_id * 2);
            bind(child_ping_socket, new_child_id * 2 + 1);
        }
        catch (zmq::error_t& e) {
            send_message(parent_socket, "Error: Address already in use");
            exit(-1);
        }
        pid_t pid = fork();
        if (pid < 0) {
            send_message(parent_socket, "Error: Can't create new process");
            exit(-2);
        }
        if (pid == 0) {
            execl("./calculation", "./calculation", std::to_string(new_child_id).c_str(), std::to_string(child_id).c_str(), NULL);
            send_message(parent_socket, "Error: Can't execute new process");
            exit(-3);
        }
        send_message(child_socket, std::to_string(new_child_id) + "pid");
        child_id = new_child_id;
        send_message(parent_socket, receive_message(child_socket));
    }

    void set(std::istringstream& is) {
        std::string key;
        int value;
        is >> key >> value;
        dictionary[key] = value;
        send_message(parent_socket, "OK:" + std::to_string(cur_id));
    }

    void get(std::istringstream& is) {
        std::string key, res;
        int value;
        is >> key;
        try {
            value = dictionary.at(key);
            res = std::to_string(value);
        }
        catch (...) {
            res = "'" + key + "' not found";
        }
        send_message(parent_socket, "OK:" + std::to_string(cur_id) + ": " + res);
    }

    void ping() {
        send_message(parent_ping_socket, "OK 1");
    }

    std::string receive() {
        return receive_message(parent_socket);
    }

    void send(std::string message, std::string cmd) {
        if (cmd != "ping") {
            if (child_id != -1) {
                send_message(child_socket, message);
                send_message(parent_socket, receive_message(child_socket));
            } else {
                send_message(parent_socket, "Error: node is unavailable");
            }
        } else {
            if (child_id != -1) {
                send_message(child_socket, message);
                send_message(parent_ping_socket, receive_message(child_socket));
            } else {
                send_message(parent_ping_socket, "Error: node is unavailable");
            }
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2 && argc != 3) {
        throw std::runtime_error("Wrong args for counting node");
    }
    int cur_id = std::atoi(argv[1]);

    CalculationNode node(cur_id);

    if (argc == 3) {
        int child_id = std::atoi(argv[2]);
        node.set_child_socket(child_id);
    }

    while (true) {
        std::string message = node.receive();
        auto is = std::istringstream(message);
        int dest_id;
        is >> dest_id;

        std::string cmd;
        is >> cmd;

        if (dest_id == cur_id) {
            if (cmd == "pid") {
                node.pid();
            } else if (cmd == "create") {
                node.create(is);
            } else if (cmd == "set") {
                node.set(is);
            } else if (cmd == "get") {
                node.get(is);
            } else if (cmd == "ping") {
                node.ping();
            }
        } else {
            node.send(message, cmd);
        }
    }
}
