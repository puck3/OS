#include <unistd.h>
#include <fcntl.h>
#include <iostream>
using namespace std;

int main(int argc, char* argv[]) {

    int fd = open(argv[1], O_CREAT | O_WRONLY, S_IRWXU);
    if (fd == -1) {
        cerr << "File error" << '\n';
        return 1;
    }

    int err = ftruncate(fd, 0);
    if (err == -1) {
        cerr << "File cleaning error" << '\n';
        return 1;
    }

    int x, res = 0;
    while (cin >> x) {
        res += x;
    }
    dprintf(fd, "%d\n", res);
    return 0;
}