#include <zmq.hpp>
#include <iostream>
#include <queue>
#include <vector>
#include <pthread.h>

const int MAIN_PORT = 4040;

void send_message(zmq::socket_t& socket, const std::string& msg) {
    zmq::message_t message(msg);
    socket.send(message, zmq::send_flags::none);
}

std::string receive_message(zmq::socket_t& socket) {
    zmq::message_t message;
    std::string received_msg;
    zmq::recv_result_t received;
    try {
        received = socket.recv(message, zmq::recv_flags::none);
        if (!received) {
            received_msg = "Error: node is unavailable";
        } else {
            received_msg = std::string(static_cast<char*>(message.data()), message.size());
        }
    }
    catch (zmq::error_t& e) {
        received_msg = "Error: node is unavailable";
    }

    return received_msg;
}

void connect(zmq::socket_t& socket, int id) {
    std::string address = "tcp://127.0.0.1:" + std::to_string(MAIN_PORT + id);
    socket.connect(address);
}

void disconnect(zmq::socket_t& socket, int id) {
    std::string address = "tcp://127.0.0.1:" + std::to_string(MAIN_PORT + id);
    socket.disconnect(address);
}

void bind(zmq::socket_t& socket, int id) {
    std::string address = "tcp://127.0.0.1:" + std::to_string(MAIN_PORT + id);
    socket.bind(address);
}

void unbind(zmq::socket_t& socket, int id) {
    std::string address = "tcp://127.0.0.1:" + std::to_string(MAIN_PORT + id);
    socket.unbind(address);
}
