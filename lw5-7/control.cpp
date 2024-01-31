#include <unistd.h>
#include <sstream>
#include <set>

#include "mq.hpp"
#include "topology.hpp"

class ControlNode {
private:
    topology children;
    std::vector<zmq::socket_t> child_sockets;
    std::vector<zmq::socket_t> child_ping_sockets;

    zmq::context_t context;

    // pthread_mutex_t mutex;

    int heartbeat_time;
    bool is_heartbeat;
    bool is_exit;

public:
    ControlNode() : heartbeat_time(1000), is_heartbeat(false), is_exit(false) {};

    ~ControlNode() {
        for (size_t i = 0; i < child_sockets.size(); ++i) {
            int first_node_id = children.get_first_id(i);
            unbind(child_sockets[i], first_node_id * 2);
            unbind(child_ping_sockets[i], first_node_id * 2 + 1);
        }
    }

    void m_lock() {
        // pthread_mutex_lock(&mutex);
    }

    void m_unlock() {
        // pthread_mutex_unlock(&mutex);
    }

    void create() {
        int node_id, parent_id;
        std::cin >> node_id >> parent_id;

        if (children.find(node_id) != -1 || node_id == -1) {
            std::cout << "Error: already exists" << std::endl;
        } else if (parent_id == -1) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("Error: Can't create new process");
                std::exit(-1);
            }
            if (pid == 0) {
                execl("./calculation", "./calculation", std::to_string(node_id).c_str(), NULL);
                perror("Error: Can't execute new process");
                std::exit(-2);
            }

            // pthread_mutex_lock(&mutex);
            child_sockets.emplace_back(context, ZMQ_REQ);
            child_sockets[child_sockets.size() - 1].set(zmq::sockopt::rcvtimeo, 3000);
            try {
                bind(child_sockets[child_sockets.size() - 1], node_id * 2);
            }
            catch (zmq::error_t& e) {
                // pthread_mutex_unlock(&mutex);
                std::cout << "Error: Address already in use" << std::endl;
                return;
            }

            child_ping_sockets.emplace_back(context, ZMQ_REQ);
            child_ping_sockets[child_ping_sockets.size() - 1].set(zmq::sockopt::rcvtimeo, heartbeat_time);
            try {
                bind(child_ping_sockets[child_ping_sockets.size() - 1], node_id * 2 + 1);
            }
            catch (zmq::error_t& e) {
                // pthread_mutex_unlock(&mutex);
                std::cout << "Error: Address already in use" << std::endl;
                return;
            }

            send_message(child_sockets[child_sockets.size() - 1], std::to_string(node_id) + "pid");
            // pthread_mutex_unlock(&mutex);

            children.insert(node_id, parent_id);
        }
    }

    void exec() {
        int dest_id, value;
        std::string key;
        std::cin >> dest_id >> key;
        if (ping(dest_id, 1000)) {
            // pthread_mutex_lock(&mutex);
            int list_num = children.find(dest_id);
            // pthread_mutex_unlock(&mutex);
            if (std::cin.peek() != '\n') {
                std::cin >> value;
                send_message(child_sockets[list_num], std::to_string(dest_id) + " set " + key + " " + std::to_string(value));
            } else {
                send_message(child_sockets[list_num], std::to_string(dest_id) + " get " + key);
            }
        } else {
            std::cout << "Error: node is unavailable" << std::endl;
        }
    }

    void print() {
        std::cout << -1 << std::endl << children;
    }

    bool check_heartbeat() const {
        return is_heartbeat;
    }

    void set_heartbeat(bool value) {
        is_heartbeat = value;
    }

    void set_heartbeat_time(int value) {
        heartbeat_time = value;
    }

    int get_heartbeat_time() const {
        return heartbeat_time;
    }

    bool ping(int id, int time) {
        int timeout = 4 * time;
        // pthread_mutex_lock(&mutex);
        int list_num = children.find(id);
        if (list_num == -1) {
            std::cout << "Error: incorrect node id" << std::endl;
            return false;
        }
        child_ping_sockets[list_num].set(zmq::sockopt::rcvtimeo, timeout);
        // pthread_mutex_unlock(&mutex);

        send_message(child_sockets[list_num], std::to_string(id) + " ping");
        std::string reply = receive_message(child_ping_sockets[list_num]);
        std::istringstream is(reply);
        std::string err;
        is >> err;
        if (err == "Error:") {
            return false;
        }
        return true;
    }

    void exit() {
        is_exit = true;
    }

    friend void* printFunction(void*);
    friend void* heartbeatFunction(void*);


};


void* heartbeatFunction(void* arg) {
    auto control_node = (ControlNode*) arg;
    while (control_node->check_heartbeat()) {
        // pthread_mutex_lock(&(control_node->mutex));
        std::list<int> tmp = control_node->children.get_nodes();
        control_node->m_unlock();
        bool answer = true;
        for (int& node : tmp) {
            if (!(control_node->ping(node, control_node->get_heartbeat_time()))) {
                answer = false;
                std::cout << "Heartbeat: node " << node << " is unavailable now" << std::endl;
            }
        }
        if (answer) {
            std::cout << "OK" << std::endl;
        }
    }
    return nullptr;
}

void heartbeat(ControlNode& node, pthread_t& heartbeatThread) {
    if (!node.check_heartbeat()) {
        int time;
        std::cin >> time;
        node.set_heartbeat_time(time);
        node.set_heartbeat(true);
        pthread_create(&heartbeatThread, nullptr, heartbeatFunction, static_cast<void*>(&node));

    } else {
        node.set_heartbeat(false);
        pthread_detach(heartbeatThread);

    }
}

void* printFunction(void* arg) {
    auto control_node = static_cast<ControlNode*>(arg);
    while (!control_node->is_exit) {
        for (size_t i = 0; i < control_node->child_sockets.size(); ++i) {
            zmq::pollitem_t items[] = {{control_node->child_sockets[i], 0, ZMQ_POLLIN, 0}};
            int rc = zmq_poll(items, 1, ZMQ_RCVTIMEO);
            if (rc > 0) {
                std::string reply = receive_message(control_node->child_sockets[i]);
                std::cout << reply << std::endl;
            }
        }
    }
    return nullptr;
}


int main() {
    ControlNode node;
    pthread_t printThread;
    pthread_t heartbeatThread;


    pthread_create(&printThread, nullptr, printFunction, static_cast<void*>(&node));

    std::string operation;
    while (std::cin >> operation) {
        if (operation == "create") {
            node.create();
        } else if (operation == "exec") {
            node.exec();
        } else if (operation == "print") {
            node.print();
        } else if (operation == "heartbeat") {
            heartbeat(node, heartbeatThread);
        } else if (operation == "exit" || operation == "q" || operation == "quit") {
            node.exit();
            break;
        } else {
            std::cout << "Incorrect operation" << std::endl;
        }
    }

    pthread_detach(printThread);
}