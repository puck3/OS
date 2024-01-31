#include <unistd.h>
#include <sstream>
#include <vector>
#include "mq.hpp"
#include "topology.hpp"

// 1 thread for all or different threads for different operations

// mutex for every child !!!
MQ mq{-1};

pthread_mutex_t control_node_mutex = PTHREAD_MUTEX_INITIALIZER;
std::vector<pthread_mutex_t> child_nodes_mutex;

Topology children;

bool heartbeat_status{false};
int heartbeat_time{0};
pthread_cond_t status_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t heartbeat_lock = PTHREAD_MUTEX_INITIALIZER;

void* create(void* args) {
    std::pair<int, int>* arg = static_cast<std::pair<int, int>*>(args);
    int node_id = arg->first, parent_id = arg->second;

    if (children.find_list(node_id) != -1 || node_id == -1) {
        std::cerr << "Error: already exists" << std::endl;
    }

    else if (parent_id == -1) {
        pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "Error: Can't create new process" << std::endl;
            return nullptr;
        }

        if (pid == 0) {
            execl("./calculation", "./calculation", std::to_string(node_id).c_str(), NULL);
            std::cerr << "Error: Can't execute new process" << std::endl;
            return nullptr;
        }
        pthread_mutex_t tmp;
        pthread_mutex_init(&tmp, NULL);
        child_nodes_mutex.emplace_back(tmp);
        pthread_mutex_lock(&child_nodes_mutex.back());

        mq.add_child(node_id);
        std::string msg = std::to_string(node_id) + " pid";
        mq.send(node_id, msg);
        std::string rep = mq.receive(node_id);
        std::cout << rep << std::endl;
        children.insert(node_id, parent_id);

        pthread_mutex_unlock(&child_nodes_mutex.back());

    }

    else {
        int list_id = children.find_list(parent_id);
        pthread_mutex_lock(&child_nodes_mutex[list_id]);

        std::string msg = std::to_string(parent_id) + " create" + std::to_string(node_id);
        int id = children.get_first_id(list_id);
        mq.send(id, msg);

        std::string rep = mq.receive(id);
        std::cout << rep << std::endl;
        children.insert(node_id, parent_id);

        pthread_mutex_unlock(&child_nodes_mutex[list_id]);
    }

    return nullptr;
}

typedef struct {
    int dest_id;
    std::string key, value;
} exec_args;

void* exec(void* args) {
    exec_args* arg = static_cast<exec_args*>(args);
    int dest_id = arg->dest_id;
    std::string key = arg->key, value = arg->value;
    int list_id = children.find_list(dest_id);
    int id = children.get_first_id(list_id);
    pthread_mutex_lock(&child_nodes_mutex[list_id]);
    std::string msg;
    if (value.size()) {
        msg = std::to_string(dest_id) + " set " + key + " " + value;
    } else {
        msg = std::to_string(dest_id) + " get " + key;
    }
    mq.send(id, msg);
    std::string rep = mq.receive(id);
    std::cout << rep << std::endl;

    pthread_mutex_unlock(&child_nodes_mutex[list_id]);
    return nullptr;
}


void set_heartbeat(int time = 0) {
    pthread_mutex_lock(&heartbeat_lock);
    heartbeat_time = time;
    heartbeat_status = time ? true : false;
    pthread_cond_signal(&status_cond);
    pthread_mutex_unlock(&heartbeat_lock);
}

void* Heartbeat(void* args) {
    while (true) {
        while (!heartbeat_status || !heartbeat_time) {
            pthread_mutex_lock(&heartbeat_lock);
            mq.set_rcvtimeo(3000);
            pthread_cond_wait(&status_cond, &heartbeat_lock);
            pthread_mutex_unlock(&heartbeat_lock);
        }

        std::list<int> tmp = children.get_nodes();
        mq.set_rcvtimeo(heartbeat_time * 4 * 1000);
        bool answer = true;
        for (int& node : tmp) {
            int list_id = children.find_list(node);
            int id = children.get_first_id(list_id);

            pthread_mutex_lock(&child_nodes_mutex[list_id]);
            std::string msg = std::to_string(node) + " heartbeat " + std::to_string(heartbeat_time);
            mq.send(id, msg);
            std::string ans = mq.receive(id);
            pthread_mutex_unlock(&child_nodes_mutex[list_id]);
            // if (!(control_node->ping(node, control_node->get_heartbeat_time()))) {
            //     answer = false;
            //     std::cout << "Heartbeat: node " << node << " is unavailable now" << std::endl;
            // }
            std::cout << "Heartbeat: " << ans << std::endl;
        }
        if (answer) {
            std::cout << "OK" << std::endl;
        }
        pthread_testcancel();
        sleep(heartbeat_time);
    }
    return nullptr;

}




int main() {
    pthread_t heartbeat_thread;
    pthread_create(&heartbeat_thread, nullptr, Heartbeat, nullptr);
    std::string operation;
    while (std::cin >> operation) {
        if (operation == "create") {
            int node_id, parent_id;
            std::pair<int, int> args;
            std::cin >> args.first >> args.second;

            pthread_t thread;
            pthread_create(&thread, nullptr, create, static_cast<void*>(&args));
            pthread_detach(thread);
        } else if (operation == "exec") {
            int dest_id;
            std::string key, value;
            std::cin >> dest_id >> key;
            if (std::cin.peek() != '\n') {
                std::cin >> value;
            }
            exec_args args;
            args.dest_id = dest_id;
            args.value = value;
            args.key = key;

            pthread_t thread;
            pthread_create(&thread, nullptr, exec, static_cast<void*>(&args));
            pthread_detach(thread);
        } else if (operation == "print") {
            std::cout << -1 << std::endl << children;
        } else if (operation == "heartbeat") {
            int time;
            if (std::cin.peek() != '\n') {
                std::cin >> time;
                set_heartbeat(time);
            } else {
                set_heartbeat();
            }
        } else if (operation == "exit" || operation == "q" || operation == "quit") {
            pthread_cancel(heartbeat_thread);
            pthread_join(heartbeat_thread, nullptr);
            break;
        } else {
            std::cerr << "Incorrect operation" << std::endl;
        }
    }
}
