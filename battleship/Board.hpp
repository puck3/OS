#include <vector>
#include <iostream>
#include <unistd.h> 


const int SIZE = 10;

typedef enum {
    empty = 0,
    ship,
    hit,
    miss,
} cell_t;

class Board {
private:
    std::vector<std::vector<cell_t>> main_board;
    std::vector<std::vector<cell_t>> hit_board;
    int points;

public:
    void throw_if_invalid_cell(int x, int y) const {
        if (x < 0 || x >= SIZE || y < 0 || y >= SIZE) {
            throw std::runtime_error("Cell must be on the board!");
        }
    }

    void throw_if_invalid_ship(int length, int x1, int y1, int x2, int y2) const {
        if (x1 < 0 || x2 >= SIZE || y1 < 0 || y2 >= SIZE) {
            throw std::runtime_error("Ships must be on the board!");
        }
        if (!((x2 - x1 == length - 1 && y1 == y2) || (y2 - y1 == length - 1 && x1 == x2))) {
            throw std::runtime_error("Wrong length or orientation!");
        }
        int left = x1 > 0 ? x1 - 1 : x1,
            right = x2 < SIZE - 1 ? x2 + 1 : x2,
            bottom = y1 > 0 ? y1 - 1 : y1,
            top = y2 < SIZE - 1 ? y2 + 1 : y2;
        for (int i{bottom}; i <= top; ++i) {
            for (int j{left}; j <= right; ++j) {
                if (main_board[i][j] != empty) {
                    throw std::runtime_error("The ships are too close to each other!");
                }
            }
        }
    }

    void add_ship(int length) noexcept {
        int x1, x2, y1, y2;
        while (true) {
            std::cout << "Enter the coordinates of the " << length << "-decker ship ";
            if (length == 1) {
                std::cout << "(format: A0): " << std::endl;
                std::string pos;
                std::cin >> pos;
                y1 = y2 = static_cast<int>(toupper(pos[0]) - 'A');
                x1 = x2 = static_cast<int>(pos[1] - '0');
            } else {
                std::cout << "(format: A0 A0): " << std::endl;
                std::string pos1, pos2;
                std::cin >> pos1 >> pos2;
                y1 = static_cast<int>(toupper(pos1[0]) - 'A'); x1 = static_cast<int>(pos1[1] - '0');
                y2 = static_cast<int>(toupper(pos2[0]) - 'A'); x2 = static_cast<int>(pos2[1] - '0');
                if (x1 > x2) std::swap(x1, x2);
                if (y1 > y2) std::swap(y1, y2);
            }

            try {
                throw_if_invalid_ship(length, x1, y1, x2, y2);
            }
            catch (std::exception& e) {
                std::cout << e.what() << " Try again." << std::endl;
                continue;
            }

            for (int i{y1}; i <= y2; ++i) {
                for (int j{x1}; j <= x2; ++j) {
                    main_board[i][j] = ship;
                }
            }
            break;
        }
    }

    bool success(int x, int y) const noexcept {
        return (main_board[y][x] == ship);
    }

    void set_hit(int x, int y, cell_t cell) noexcept {
        if (hit_board[y][x] != hit && hit_board[y][x] != miss) {
            hit_board[y][x] = cell;
            if (cell == hit) {
                ++points;
            }
        }
    }

    void set_main(int x, int y, cell_t cell) noexcept {
        if (main_board[y][x] != hit && main_board[y][x] != miss) {
            main_board[y][x] = cell;
        }
    }

    Board() :
        main_board(std::vector<std::vector<cell_t>>(SIZE, std::vector<cell_t>(SIZE, empty))),
        hit_board(std::vector<std::vector<cell_t>>(SIZE, std::vector<cell_t>(SIZE, empty))),
        points(0) {}

    virtual ~Board() = default;

    bool check_win() const noexcept {
        return points == 20;
    }

    void set_ships() noexcept {
        print(std::cout);
        for (int i{0}; i < 4; ++i) {
            for (int j{0}; j <= i; ++j) {
                add_ship(4 - i);
                print(std::cout);
            }
        }
    }

    void clear() {
        points = 0;
        for (int i{0}; i < SIZE; ++i) {
            for (int j{0}; j < SIZE; ++j) {
                main_board[i][j] = empty;
                hit_board[i][j] = empty;
            }
        }
    }

    void print(std::ostream& os) const noexcept {
        os << "  ";
        for (int i{0}; i < SIZE; ++i) {
            os << i << " ";
        }

        os << "\t";

        os << "  ";
        for (int i{0}; i < SIZE; ++i) {
            os << i << " ";
        }

        os << std::endl;
        for (int i{0}; i < SIZE; ++i) {
            os << static_cast<char>('A' + i) << " ";
            for (int j{0}; j < SIZE; ++j) {
                switch (main_board[i][j]) {
                    case ship:
                        os << "# ";
                        break;
                    case hit:
                        os << "x ";
                    break;case miss:
                        os << "* ";
                        break;
                    default:
                        os << ". ";
                        break;
                }
            }

            os << "\t";

            os << static_cast<char>('A' + i) << " ";
            for (int j{0}; j < SIZE; ++j) {
                switch (hit_board[i][j]) {
                    case ship:
                        os << "# ";
                        break;
                    case hit:
                        os << "x ";
                    break;case miss:
                        os << "* ";
                        break;
                    default:
                        os << ". ";
                        break;
                }
            }

            os << std::endl;
        }
    }
};

std::ostream& operator<<(std::ostream& os, Board& board) {
    board.print(os);
    return os;
}

