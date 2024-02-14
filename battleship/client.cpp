#include <iostream>
#include "ServerSocket.hpp"
#include "Board.hpp"

const size_t MAX_LEN_LOGIN = 11;

std::string set_login() {
    std::string login, req_path, rep_path;
    while (true) {
        std::cout << "Enter your login (max length " << MAX_LEN_LOGIN << "): ";
        std::cin >> login;
        req_path = "./tmp/" + login + "_req";
        rep_path = "./tmp/" + login + "_rep";
        if (login.size() > MAX_LEN_LOGIN || access(req_path.c_str(), F_OK) == 0 || access(rep_path.c_str(), F_OK) == 0) {
            std::cout << "Login is unavailable! Try again." << std::endl;
        } else {
            break;
        }
    }
    return login;
}

bool attack(Board& board, ServerSocket& socket) {
    while (true) {
        std::cout << "Enter the coordinates of cell to attack (format: A0):" << std::endl;
        std::string pos;
        std::cin >> pos;
        int y = static_cast<int>(toupper(pos[0]) - 'A'), x = static_cast<int>(pos[1] - '0');
        try {
            board.throw_if_invalid_cell(x, y);
            socket.send(pos);

            std::string res = socket.receive(sizeof(char));
            if (res == "t") {
                board.set_hit(x, y, hit);
                std::cout << static_cast<char>(y + 'A') << x << ": hit!" << std::endl;
                std::cout << board;
                return true;
            } else {
                board.set_hit(x, y, miss);
                std::cout << static_cast<char>(y + 'A') << x << ": miss" << std::endl;
                std::cout << board;
                return false;
            }

        }
        catch (std::exception& e) {
            std::cout << e.what() << " Try again." << std::endl;
        }
    }
}

bool defense(Board& board, ServerSocket& socket) {
    std::cout << "The enemy is attacking..." << std::endl;
    std::string pos = socket.receive(sizeof(char) * 2);
    int y = static_cast<int>(toupper(pos[0]) - 'A'), x = static_cast<int>(pos[1] - '0');
    if (board.success(x, y)) {
        board.set_main(x, y, hit);
        std::cout << static_cast<char>(y + 'A') << x << ": hit!" << std::endl;
        socket.send("t");
        std::cout << board;
        return true;
    } else {
        board.set_main(x, y, miss);
        std::cout << static_cast<char>(y + 'A') << x << ": miss" << std::endl;
        socket.send("f");
        std::cout << board;
        return false;
    }
}


void play(ServerSocket& socket) {
    freopen("./in.txt", "r", stdin);
    std::cout << "Start..." << std::endl;
    Board board;
    std::cout << "Set your ships:" << std::endl;
    board.set_ships();
    socket.send("OK");
    std::string turn = socket.receive(sizeof(char));
    bool end = false;
    while (!end) {
        if (turn == "1") {
            while (attack(board, socket)) {
                if (board.check_win()) {
                    socket.send("t");
                    std::cout << "You won!" << std::endl;
                    end = true;
                    break;
                } else {
                    socket.send("f");
                }
            }

            if (end) break;

            while (defense(board, socket)) {
                if (socket.receive(sizeof(char)) == "t") {
                    std::cout << socket.receive(MAX_LEN_LOGIN) << " won" << std::endl;
                    end = true;
                    break;
                }
            }
        }

        else {
            while (defense(board, socket)) {
                if (socket.receive(sizeof(char)) == "t") {
                    std::cout << socket.receive(MAX_LEN_LOGIN) << " won" << std::endl;
                    end = true;
                    break;
                }
            }

            if (end) break;

            while (attack(board, socket)) {
                if (board.check_win()) {
                    socket.send("t");
                    std::cout << "You won!" << std::endl;
                    end = true;
                    break;
                } else {
                    socket.send("f");
                }
            }
        }
    }

}

int main() {

    ServerSocket socket{set_login()};

    size_t len;
    std::string opponent;
    bool accept_invite = false;
    while (!accept_invite) {
        std::cout << "Do you want to send the invite to another player? [Y/n/q]" << std::endl;
        char ans;
        while (true) {
            std::cin >> ans;
            ans = tolower(ans);
            if (ans == 'y' || ans == 'n' || ans == 'q') {
                break;
            }
            std::cout << "Wrong answer. Try again." << std::endl;
        }
        socket.send(std::string{ans});
        if (ans == 'q') {
            return 0;
        } else if (ans == 'y') {
            std::cout << "Enter player's login: ";
            std::cin >> opponent;
            socket.send(opponent);
            std::string rep = socket.receive(sizeof(char));
            if (rep == "y") {
                std::cout << "The player " << opponent << " accepted your invite." << std::endl;
                accept_invite = true;
            } else {
                std::cout << "The player " << opponent << " is not searching for a game or have declined your invite." << std::endl;
            }
        } else {

            while (true) {
                opponent = socket.receive(MAX_LEN_LOGIN * sizeof(char));
                std::cout << "The player " << opponent << " invited you to the game. Accept invitation? [Y/n/q]" << std::endl;
                while (true) {
                    std::cin >> ans;
                    ans = tolower(ans);
                    if (ans == 'y' || ans == 'n' || ans == 'q') {
                        break;
                    }
                    std::cout << "Wrong answer. Try again." << std::endl;
                }
                socket.send(std::string{ans});
                if (ans == 'q') {
                    return 0;
                } else if (ans == 'y') {
                    accept_invite = true;
                    break;
                }
            }
        }
    }

    play(socket);

}