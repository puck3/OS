#include <unistd.h>
#include <iostream>

using namespace std;

void throw_if(int err, string what) {
    if (err == -1) {
        throw runtime_error(what);
    }
}

int main() {
    freopen("in", "r", stdin);
    string file;
    cout << "Enter file name: ";
    cin >> file;
    cout << "Enter commands:" << endl;

    int pipe_fd[2];
    int err = pipe(pipe_fd);
    throw_if(err, "Pipe error");

    pid_t pid = fork();
    throw_if(pid, "Child proc error");

    if (pid == 0) {
        close(pipe_fd[1]);
        err = dup2(pipe_fd[0], 0);
        throw_if(err, "Redirection error");
        close(pipe_fd[0]);

        err = execl("lw1_child_proc", "lw1_child_proc", file.c_str(), NULL);
        throw_if(err, "Child file error");
    }

    if (pid > 0) {
        close(pipe_fd[0]);
        string s;
        while (getline(cin, s, '\n')) {
            if (s.size()) {
                s += '\n';
                write(pipe_fd[1], s.c_str(), s.size() * sizeof(char));
            }
        }
        close(pipe_fd[1]);
    }
}