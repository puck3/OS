#pragma once
#include <stdexcept>
#include <string>
#include <fcntl.h> 
#include <unistd.h> 

class ClientSocket {
private:
    std::string login;
    int fd_req, fd_rep;
    bool searching_game;

public:
    ClientSocket(const std::string& _login) : login(_login), searching_game(false) {
        std::string req_path = "./tmp/" + login + "_req", rep_path = "./tmp/" + login + "_rep";
        if ((fd_req = open(req_path.c_str(), O_RDWR)) == -1) {
            throw std::runtime_error("Can't open FIFO");
        }
        if ((fd_rep = open(rep_path.c_str(), O_RDWR)) == -1) {
            throw std::runtime_error("Can't open FIFO");
        }
    }

    ~ClientSocket() {
        close(fd_req);
        close(fd_rep);
    }

    std::string receive(size_t size) const {
        char tmp[++size];
        if (read(fd_req, tmp, size) == -1) {
            throw std::runtime_error("Can't read from FIFO");
        }
        return std::string{tmp};
    }

    void send(const std::string& message) const {
        if (write(fd_rep, message.c_str(), message.size() + 1) == -1) {
            throw std::runtime_error("Can't write to FIFO");
        }
    }

    std::string get_login() const noexcept {
        return login;
    }

    bool is_searching() {
        return searching_game;
    }

    void set_search_status(bool val) {
        searching_game = val;
    }
};
