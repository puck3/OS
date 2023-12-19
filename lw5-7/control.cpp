#include <unistd.h>
#include <sstream>
#include <set>

#include "mq.hpp"
#include "topology.hpp"

// broken
void create(Topology& node_lists, std::vector<zmq::socket_t>& children, zmq::context_t& context) {
    int node_id, parent_id;
    std::cin >> node_id >> parent_id;

    node_lists.throw_if_new_node_invalid(node_id, parent_id);

    std::string reply;
    if (parent_id == -1) {
        pid_t pid = fork();
        if (pid < 0) throw std::runtime_error("Can't create new process");

        if (pid == 0) {
            int err = execl("./calculation", "./calculation", std::to_string(node_id).c_str(), NULL);
            if (err == -1) throw std::runtime_error("Can't execute new process");
        }

        children.emplace_back(context, ZMQ_REQ);
        children[children.size() - 1].setsockopt(ZMQ_RCVTIMEO, 5000);
        bind(children[children.size() - 1], node_id);
        send_message(children[children.size() - 1], std::to_string(node_id) + " pid");

        reply = receive_message(children[children.size() - 1]);
    } else {
        int list_idx = node_lists.find(parent_id);
        send_message(children[list_idx], std::to_string(parent_id) + " create " + std::to_string(node_id));

        reply = receive_message(children[list_idx]);
    }

    node_lists.insert(node_id, parent_id);
    std::cout << reply << std::endl;
}

void exec(Topology& node_lists, std::vector<zmq::socket_t>& children) {
    int dest_id{}, value{};
    std::string key;
    std::cin >> dest_id >> key;
    if (std::cin.peek() != '\n') {
        std::cin >> value;
    }

    int list_idx = node_lists.find(dest_id);
    if (list_idx == -1) {
        throw std::runtime_error("Invalid node id");
    }

    if (value)
        send_message(children[list_idx], std::to_string(dest_id) + " set " + key + " " + std::to_string(value));
    else
        send_message(children[list_idx], std::to_string(dest_id) + " get " + key);

    std::string reply = receive_message(children[list_idx]);
    std::cout << reply << std::endl;
}

// broken
void kill(Topology& node_lists, std::vector<zmq::socket_t>& children) {
    int id;
    std::cin >> id;
    int list_idx = node_lists.find(id);
    if (list_idx == -1) {
        throw std::runtime_error("Invalid node id");
    }
    bool is_first = (node_lists.get_first_id(list_idx) == id);
    send_message(children[list_idx], std::to_string(id) + " kill");

    std::string reply = receive_message(children[list_idx]);
    std::cout << reply << std::endl;
    node_lists.erase(id);
    if (is_first) {
        unbind(children[list_idx], id);
        children.erase(children.begin() + list_idx);
    }
}

void print(Topology& node_lists) {
    std::cout << node_lists;
}

void ping(Topology& node_lists, std::vector<zmq::socket_t>& children) {
    int id;
    std::cin >> id;
    int list_idx = node_lists.find(id);
    if (list_idx == -1) {
        throw std::runtime_error("Not found");
    }
    send_message(children[list_idx], std::to_string(id) + " ping");
    std::string reply;
    try {
        reply = receive_message(children[list_idx]);
    }
    catch (...) {
        reply = "OK: 0";
    }
    std::cout << reply << std::endl;
}

void Exit(Topology& node_lists, std::vector<zmq::socket_t>& children) {
    for (size_t i = 0; i < children.size(); ++i) {
        int first_node_id = node_lists.get_first_id(i);
        send_message(children[i], std::to_string(first_node_id) + " kill");
        std::string reply = receive_message(children[i]);
        unbind(children[i], first_node_id);
    }
    exit(0);
}

int main() {
    Topology node_lists;
    std::vector<zmq::socket_t> children;
    zmq::context_t context;

    std::string action;
    std::cout << ">";
    while (std::cin >> action) {
        try {
            if (action == "create") {
                create(node_lists, children, context);
            } else if (action == "exec") {
                exec(node_lists, children);
            } else if (action == "kill") {
                kill(node_lists, children);
            } else if (action == "print") {
                print(node_lists);
            } else if (action == "ping") {
                ping(node_lists, children);
            } else if (action == "exit") {
                Exit(node_lists, children);
            } else {
                throw std::runtime_error("Incorrect action");
            }
        }
        catch (std::exception& e) {
            std::cerr << "ERROR: " << e.what() << std::endl;
        }
        std::cout << ">";
    }
}