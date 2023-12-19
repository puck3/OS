#include <zmq.hpp>
#include <iostream>

const int MAIN_PORT = 5050;

void send_message(zmq::socket_t& socket, const std::string& msg) {
    socket.send(zmq::buffer(msg), zmq::send_flags::none);
}

std::string receive_message(zmq::socket_t& socket) {
    zmq::message_t message;
    zmq::recv_result_t chars_read;
    chars_read = socket.recv(message, zmq::recv_flags::none);
    std::string received_msg(static_cast<char*>(message.data()), message.size());
    return received_msg;
}

void connect(zmq::socket_t& socket, int id) {
    std::string address = "tcp://localhost:" + std::to_string(MAIN_PORT + id);
    socket.connect(address);
}

void disconnect(zmq::socket_t& socket, int id) {
    std::string address = "tcp://localhost:" + std::to_string(MAIN_PORT + id);
    socket.disconnect(address);
}

void bind(zmq::socket_t& socket, int id) {
    std::string address = "tcp://*:" + std::to_string(MAIN_PORT + id);
    socket.bind(address);
}

void unbind(zmq::socket_t& socket, int id) {
    std::string address = "tcp://*:" + std::to_string(MAIN_PORT + id);
    socket.unbind(address);
}