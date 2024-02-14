#pragma once
#include <stdexcept>
#include <string>
#include <fcntl.h> 
#include <unistd.h> 
#include <sys/stat.h>


class ServerSocket {
private:
    std::string login;
    int fd_req, fd_rep;

public:
    ServerSocket(const std::string& _login) : login(_login) {
        std::string req_path = "./tmp/" + login + "_req", rep_path = "./tmp/" + login + "_rep";
        if (mkfifo(req_path.c_str(), O_RDWR) == -1) {
            throw std::runtime_error("Can't create FIFO");
        }
        if ((fd_req = open(req_path.c_str(), O_RDWR)) == -1) {
            throw std::runtime_error("Can't open FIFO");
        }
        if (mkfifo(rep_path.c_str(), O_RDWR) == -1) {
            throw std::runtime_error("Can't create FIFO");
        }
        if ((fd_rep = open(rep_path.c_str(), O_RDWR)) == -1) {
            throw std::runtime_error("Can't open FIFO");
        }
    }

    ~ServerSocket() {
        std::string req_path = "./tmp/" + login + "_req", rep_path = "./tmp/" + login + "_rep";
        close(fd_req);
        close(fd_rep);
        unlink(req_path.c_str());
        unlink(rep_path.c_str());
    }

    std::string receive(size_t size) {
        char tmp[++size];
        if (read(fd_rep, tmp, size) == -1) {
            throw std::runtime_error("Can't read from FIFO");
        }
        return std::string{tmp};
    }

    void send(const std::string& message) {
        if (write(fd_req, message.c_str(), message.size() + 1) == -1) {
            throw std::runtime_error("Can't write to FIFO");
        }
    }
};
