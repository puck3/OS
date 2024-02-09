#include "board.hpp"

int main() {

    if (!freopen("in.txt", "r", stdin)) {
        throw std::runtime_error("File error");
    }

    Board p1, p2;
    std::cout << "Set p1 board" << std::endl;
    p1.set_ships(std::cout, std::cin);

    std::cout << "Set p2 board" << std::endl;
    p2.set_ships(std::cout, std::cin);

    while (true) {
        p1.attack(std::cout, std::cin, p2);
        if (p1.get_points() == 20) {
            std::cout << "Winner: p1" << std::endl;
            break;
        }
        std::cout << p1;
        p2.attack(std::cout, std::cin, p1);
        if (p2.get_points() == 20) {
            std::cout << "Winner: p2" << std::endl;
            break;
        }
        std::cout << p2;
    }
}