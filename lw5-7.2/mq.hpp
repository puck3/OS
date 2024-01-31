#include <zmq.hpp>
#include <vector>
#include <map>
#include <iostream>

class MQ {
private:
    const int MAIN_PORT = 4040;

    zmq::context_t _context;
    int _id;
    zmq::socket_t _parent;
    std::map<int, zmq::socket_t> _children;

public:
    MQ(int id) : _id(id) {
        _parent = zmq::socket_t{_context, ZMQ_REQ};
        if (id != -1) {
            std::string address = "tcp://127.0.0.1:" + std::to_string(MAIN_PORT + id);
            _parent.bind(address);
        }
    }

    virtual ~MQ() {
        std::string address = "tcp://127.0.0.1:" + std::to_string(MAIN_PORT + _id);
        _parent.unbind(address);
        for (auto& item : _children) {
            std::string address = "tcp://127.0.0.1:" + std::to_string(MAIN_PORT + item.first);
            item.second.disconnect(address);
        }
    }

    void add_child(int id) {
        if (_id == -1 || _children.empty()) {
            _children[id] = zmq::socket_t{_context, ZMQ_REP};
            std::string address = "tcp://127.0.0.1:" + std::to_string(MAIN_PORT + id);
            _children[id].connect(address);
            _children[id].set(zmq::sockopt::rcvtimeo, 3000);

        } else {
            for (auto& item : _children) {
                _children[id] = std::move(item.second);
                _children.erase(item.first);
            }
            std::string address = "tcp://127.0.0.1:" + std::to_string(MAIN_PORT + id);
            _children[id].connect(address);
        }
    }

    void send(int id, std::string& message) {
        _children[id].send(zmq::message_t(message), zmq::send_flags::none);
    }

    void send(std::string& message) {
        _parent.send(zmq::message_t(message), zmq::send_flags::none);
    }

    std::string receive(int id) {
        zmq::message_t message;
        std::string received_msg;
        zmq::recv_result_t result = _children[id].recv(message, zmq::recv_flags::none);
        if (result.has_value()) {
            received_msg = std::string(static_cast<char*>(message.data()), message.size());
        } else {
            received_msg = "Error: node is unavailable";
        }
        return received_msg;
    }

    std::string receive() {
        if (_id == -1) return "";
        zmq::message_t message;
        zmq::recv_result_t result;
        std::string received_msg;
        while (!result.has_value()) {
            result = _parent.recv(message, zmq::recv_flags::none);
        }
        received_msg = std::string(static_cast<char*>(message.data()), message.size());
        return received_msg;
    }

    void set_rcvtimeo(int timeout) {
        for (auto& items : _children) {
            items.second.set(zmq::sockopt::rcvtimeo, timeout);
        }
    }
};

