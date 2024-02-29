#include <pthread.h>
#include <sys/inotify.h>

#include <chrono>
#include <vector>

#include "ClientSocket.hpp"

const size_t MAX_LEN_LOGIN = 11;
const size_t MAX_LEN_FILE_NAME = MAX_LEN_LOGIN + 5;
const size_t EVENT_SIZE = sizeof(struct inotify_event);
const size_t BUF_LEN = EVENT_SIZE + MAX_LEN_FILE_NAME;

void send_statistics(const ClientSocket& socket) {
  std::string path = "./data/" + socket.get_login();
  if (access(path.c_str(), F_OK) == 0) {
    FILE* file = fopen(path.c_str(), "rb");
    int win, lose;
    fread(&win, sizeof(win), 1, file);
    fread(&lose, sizeof(lose), 1, file);
    socket.send(std::to_string(win) + " " + std::to_string(lose));
    fclose(file);
  } else {
    FILE* file = fopen(path.c_str(), "wb");
    int zero{0};
    fwrite(&zero, sizeof(int), 1, file);
    fwrite(&zero, sizeof(int), 1, file);
    socket.send("0 0");
    fclose(file);
  }
}

void inc_win(const std::string& login) {
  std::string path = "./data/" + login;
  FILE* file = fopen(path.c_str(), "rb+");
  int win, lose;
  fread(&win, sizeof(win), 1, file);
  fread(&lose, sizeof(lose), 1, file);
  fseek(file, 0, 0);
  ++win;
  fwrite(&win, sizeof(int), 1, file);
  fwrite(&lose, sizeof(int), 1, file);
  fclose(file);
}

void inc_lose(const std::string& login) {
  std::string path = "./data/" + login;
  FILE* file = fopen(path.c_str(), "rb+");
  int win, lose;
  fread(&win, sizeof(win), 1, file);
  fread(&lose, sizeof(lose), 1, file);
  fseek(file, 0, 0);
  ++lose;
  fwrite(&win, sizeof(int), 1, file);
  fwrite(&lose, sizeof(int), 1, file);
  fclose(file);
}

void* game_thread(void* args) {
  auto players{static_cast<std::pair<std::string, std::string>*>(args)};
  ClientSocket first_socket{players->first}, second_socket{players->second};
  first_socket.send(second_socket.get_login());
  second_socket.send(first_socket.get_login());
  send_statistics(first_socket);
  send_statistics(second_socket);

  // game
  first_socket.receive(sizeof(char) * 2);
  second_socket.receive(sizeof(char) * 2);
  first_socket.send("1");
  second_socket.send("2");
  bool end = false;
  while (!end) {
    while (true) {
      std::string cell = first_socket.receive(sizeof(char) * 2);
      second_socket.send(cell);
      std::string res = second_socket.receive(sizeof(char));
      first_socket.send(res);
      if (res == "t") {
        std::string end_msg = first_socket.receive(sizeof(char));
        second_socket.send(end_msg);
        if (end_msg == "t") {
          second_socket.send(first_socket.get_login());
          end = true;
          inc_win(first_socket.get_login());
          inc_lose(second_socket.get_login());
          break;
        }
      } else {
        break;
      }
    }
    if (end) break;
    while (true) {
      std::string cell = second_socket.receive(sizeof(char) * 2);
      first_socket.send(cell);
      std::string res = first_socket.receive(sizeof(char));
      second_socket.send(res);
      if (res == "t") {
        std::string end_msg = second_socket.receive(sizeof(char));
        first_socket.send(end_msg);
        if (end_msg == "t") {
          first_socket.send(second_socket.get_login());
          inc_win(second_socket.get_login());
          inc_lose(first_socket.get_login());
          end = true;
          break;
        }
      } else {
        break;
      }
    }
  }

  send_statistics(first_socket);
  send_statistics(second_socket);
  return nullptr;
}

int main() {
  int fd, wd, length;
  char buffer[BUF_LEN];
  const std::string path = "./tmp";

  fd = inotify_init();
  if (fd < 0) {
    throw std::runtime_error("Couldn't initialize inotify");
  }

  wd = inotify_add_watch(fd, path.c_str(), IN_CREATE);
  if (wd == -1) {
    throw std::runtime_error("Couldn't add watch to " + path);
  }

  auto last_update = std::chrono::system_clock::now();

  std::string first_p{}, second_p{};

  while (std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now() - last_update) <
         std::chrono::minutes{15}) {
    length = read(fd, buffer, BUF_LEN);
    if (length < 0) {
      throw std::runtime_error("Read error");
    }
    struct inotify_event* event = (struct inotify_event*)buffer;
    if (event->len && event->mask & IN_CREATE) {
      std::string file_name{event->name};
      if (file_name.ends_with("_rep")) {
        last_update = std::chrono::system_clock::now();
        pthread_t thread;
        std::string login{file_name.substr(0, file_name.size() - 4)};
        if (first_p == "") {
          first_p = login;
        } else {
          second_p = login;
          std::pair<std::string, std::string> players{first_p, second_p};
          pthread_create(&thread, nullptr, game_thread, (void*)&players);
          pthread_detach(thread);
          first_p = second_p = "";
        }
      }
    }
  }

  inotify_rm_watch(fd, wd);
  close(fd);
  return 0;
}