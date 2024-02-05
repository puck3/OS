#include <zmq.hpp>
#include <vector>
#include <map>
#include <iostream>
#include <chrono>

class MQ {
private:
    const int MAIN_PORT = 4040;

    zmq::context_t _context;
    int _id;
    zmq::socket_t _parent;
    std::map<int, zmq::socket_t> _children;

public:
    MQ(int id) : _id(id) {
        _parent = zmq::socket_t{_context, ZMQ_REP};
        if (id != -1) {
            std::string address = "tcp://127.0.0.1:" + std::to_string(MAIN_PORT + id);
            try {
                _parent.bind(address);
            }
            catch (...) {
                exit(-1);
            }
        }
    }

    virtual ~MQ() {
        if (_id != -1) {
            std::string address = "tcp://127.0.0.1:" + std::to_string(MAIN_PORT + _id);
            _parent.unbind(address);
        }
        for (auto& item : _children) {
            std::string address = "tcp://127.0.0.1:" + std::to_string(MAIN_PORT + item.first);
            item.second.disconnect(address);
        }
        zmq_ctx_destroy(&_context);
    }

    void add_child(int id) {

        if (_id != -1 && !_children.empty()) {
            _children.begin()->second.close();
            _children.clear();
        }
        _children[id] = zmq::socket_t{_context, ZMQ_REQ};
        std::string address = "tcp://127.0.0.1:" + std::to_string(MAIN_PORT + id);
        _children[id].connect(address);
        _children[id].set(zmq::sockopt::rcvtimeo, 3000);
    }

    void send(int id, std::string& message) {
        std::vector<zmq::pollitem_t> items = {{_children[id], 0, ZMQ_POLLOUT, 0}};
        int rc = zmq::poll(items, std::chrono::seconds{3});
        if (rc != -1 && items[0].revents & ZMQ_POLLOUT) {
            _children[id].send(zmq::message_t(message), zmq::send_flags::none);
        }
    }

    void send(std::string& message) {
        std::vector<zmq::pollitem_t> items = {{_parent, 0, ZMQ_POLLOUT, 0}};
        int rc = zmq::poll(items, std::chrono::seconds{3});
        if (rc != -1 && items[0].revents & ZMQ_POLLOUT) {
            _parent.send(zmq::message_t(message), zmq::send_flags::none);
        }
    }

    std::string receive(int id) {
        std::vector<zmq::pollitem_t> items = {{_children[id], 0, ZMQ_POLLIN, 0}};
        std::string received_msg;
        zmq::message_t message;

        int rc = zmq::poll(items, std::chrono::milliseconds{_children[id].get(zmq::sockopt::rcvtimeo)});
        if (rc != -1 && items[0].revents & ZMQ_POLLIN) {
            zmq::recv_result_t result = _children[id].recv(message, zmq::recv_flags::none);
            if (result.has_value()) {
                received_msg = std::string(static_cast<char*>(message.data()), message.size());
            } else {
                received_msg = "Error: node is unavailable";
            }
        } else {
            received_msg = "Error: node is unavailable";
        }
        return received_msg;
    }

    std::string receive() {
        if (_id == -1) return "";

        std::vector<zmq::pollitem_t> items = {{_parent, 0, ZMQ_POLLIN, 0}};

        zmq::message_t message;
        std::string received_msg;
        while (true) {
            int rc = zmq::poll(items, std::chrono::seconds{300});
            if (rc != -1 && items[0].revents & ZMQ_POLLIN) {
                zmq::recv_result_t result = _parent.recv(message, zmq::recv_flags::none);
                if (result.has_value()) {
                    received_msg = std::string(static_cast<char*>(message.data()), message.size());
                    return received_msg;
                }
            }
        }
    }

    void set_rcvtimeo(int timeout) {
        for (auto& items : _children) {
            items.second.set(zmq::sockopt::rcvtimeo, timeout);
        }
    }
};

