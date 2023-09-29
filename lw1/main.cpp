#include <sys/wait.h>
#include <iostream>
using namespace std;

int main() {
    string file;
    cout << "Enter file name: ";
    cin >> file;

    int pipe_fd[2];
    int err = pipe(pipe_fd);
    if (err == -1) {
        cerr << "Pipe error" << '\n';
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        cerr << "Child proc error" << '\n';
        return 1;
    }

    if (pid == 0) {
        close(pipe_fd[1]);
        err = dup2(pipe_fd[0], 0);
        if (err == -1) {
            cerr << "Redirection error" << '\n';
            return 1;
        }

        err = execl("child_proc", "child_proc", file.c_str());
        if (err == -1) {
            cerr << "Child file error" << '\n';
            return 1;
        }
    }

    if (pid > 0) {
        close(pipe_fd[0]);

        int x;
        while (cin >> x) {
            dprintf(pipe_fd[1], "%d ", x);
        }

        close(pipe_fd[1]);
    }
}