#include <sys/inotify.h>
#include <map>
#include <pthread.h>
#include <chrono>
#include "ClientSocket.hpp"

const size_t MAX_LEN_LOGIN = 11;
const size_t MAX_LEN_FILE_NAME = MAX_LEN_LOGIN + 5;
const size_t EVENT_SIZE = sizeof(struct inotify_event);
const size_t BUF_LEN = EVENT_SIZE + MAX_LEN_FILE_NAME;

std::map<std::string, ClientSocket> sockets;
pthread_mutex_t sockets_lock = PTHREAD_MUTEX_INITIALIZER;

void* requests_thread(void* args) {
    auto socket{static_cast<ClientSocket*>(args)};
    ClientSocket* other_socket = nullptr;
    // finding opponent
    while (true) {
        // "Do you want to send the invite to another player? [Y/n/q]";
        std::string ans = socket->receive(sizeof(char));
        if (ans == "n") {
            socket->set_search_status(true);
            return nullptr;
        } else if (ans == "q") {
            pthread_mutex_lock(&sockets_lock);
            sockets.erase(socket->get_login());
            pthread_mutex_unlock(&sockets_lock);
            return nullptr;
        } else {
            try {
                std::string login = socket->receive(sizeof(char) * MAX_LEN_LOGIN);
                pthread_mutex_lock(&sockets_lock);
                other_socket = &sockets.at(login);
                pthread_mutex_unlock(&sockets_lock);
                if (other_socket->is_searching()) {
                    other_socket->send(socket->get_login());
                    // "Accept invitation? [Y/n/q]";
                    ans = other_socket->receive(sizeof(char));
                    if (ans == "y") {
                        socket->send("y");
                        break;
                    } else if (ans == "q") {
                        pthread_mutex_lock(&sockets_lock);
                        sockets.erase(other_socket->get_login());
                        pthread_mutex_unlock(&sockets_lock);
                    }
                }
                socket->send("n");
            }
            catch (...) {
                socket->send("n");
                pthread_mutex_unlock(&sockets_lock);
            }
        }
    }

    // game
    socket->set_search_status(false);
    other_socket->set_search_status(false);
    socket->receive(sizeof(char) * 2);
    other_socket->receive(sizeof(char) * 2);
    socket->send("1");
    other_socket->send("2");
    bool end = false;
    while (!end) {
        while (true) {
            std::string cell = socket->receive(sizeof(char) * 2);
            other_socket->send(cell);
            std::string res = other_socket->receive(sizeof(char));
            socket->send(res);
            if (res == "t") {
                std::string end_msg = socket->receive(sizeof(char));
                other_socket->send(end_msg);
                if (end_msg == "t") {
                    other_socket->send(socket->get_login());
                    end = true;
                    break;
                }
            } else {
                break;
            }
        }

        if (end) break;

        while (true) {
            std::string cell = other_socket->receive(sizeof(char) * 2);
            socket->send(cell);
            std::string res = socket->receive(sizeof(char));
            other_socket->send(res);
            if (res == "t") {
                std::string end_msg = other_socket->receive(sizeof(char));
                socket->send(end_msg);
                if (end_msg == "t") {
                    socket->send(other_socket->get_login());
                    end = true;
                    break;
                }
            } else {
                break;
            }
        }
    }

    // clean
    pthread_mutex_lock(&sockets_lock);
    sockets.erase(socket->get_login());
    sockets.erase(other_socket->get_login());
    pthread_mutex_unlock(&sockets_lock);

    return nullptr;
}


int main() {
    int fd, wd, length;
    char buffer[BUF_LEN];
    const std::string path = "./tmp";

    fd = inotify_init();
    if (fd < 0) {
        throw std::runtime_error("Couldn't initialize inotify");
    }

    wd = inotify_add_watch(fd, path.c_str(), IN_CREATE);
    if (wd == -1) {
        throw std::runtime_error("Couldn't add watch to " + path);
    }

    auto last_update = std::chrono::system_clock::now();

    while (!sockets.empty() || std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - last_update) < std::chrono::minutes{3}) {
        length = read(fd, buffer, BUF_LEN);
        if (length < 0) {
            throw std::runtime_error("Read error");
        }
        struct inotify_event* event = (struct inotify_event*) buffer;
        if (event->len) {
            if (event->mask & IN_CREATE) {
                std::string file_name{event->name};
                if (file_name.ends_with("_rep")) {
                    last_update = std::chrono::system_clock::now();
                    pthread_t thread;
                    std::string login{file_name.substr(0, file_name.size() - 4)};
                    pthread_mutex_lock(&sockets_lock);
                    sockets.try_emplace(login, login);
                    pthread_create(&thread, nullptr, requests_thread, (void*) &sockets.at(login));
                    pthread_mutex_unlock(&sockets_lock);
                    pthread_detach(thread);
                }
            }
        }
    }


    inotify_rm_watch(fd, wd);
    close(fd);
    return 0;
}