#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
using namespace std;

int main(int argc, char* argv[]) {
    if (!freopen(argv[1], "w", stdout)) {
        throw runtime_error("File error");
    }
    string s;
    while (getline(cin, s, '\n')) {
        if (s.size()) {
            s += " ";
            int res{0}, pos{0};
            for (int i{0}; i < s.size(); ++i) {
                if (isspace(s[i]) && (i - pos)) {
                    res += stoi(s.substr(pos, i - pos));
                    pos = i + 1;
                }
            }
            cout << res << endl;
        }
    }
    fclose(stdout);
}
