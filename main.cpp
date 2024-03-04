#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

#include <defer.h>
#include <socklib.h>

const char* const HOST = "127.0.0.1";
const int PORT = 20000;

class SortServer {
 private:
  Socket* socket;

 public:
  SortServer(const char* host, int port) {
    socket = new Socket(Socket::Family::INET, Socket::Type::STREAM);
    socket->Bind(Address(host, port));
    socket->Listen();
  }
  static bool sort_ascending(float a, float b) {
    return a < b;
  }
  static bool sort_descending(float a, float b) {
    return a > b;
  }
  static bool sort_alphabetically(float a, float b) {
    return std::to_string(a) < std::to_string(b);
  }
  void run_server() {
    Socket client = socket->Accept();

    while (true) {
      // Allocate memory
      char buffer[1024] = {0};
      int bytes = client.Recv(buffer, 1023);
      if (bytes <= 0) {
        break;
      }

      // Create string + debug print
      std::string buf_str(buffer);
      std::cout << buf_str << std::endl;

      bool error = false;

      auto sorter = sort_ascending;
      char mode = buf_str[buf_str.size() - 1];
      char sep = buf_str[buf_str.size() - 2];
      std::cout << "\tMODE: " << mode << "\tSEP: " << sep << std::endl;
      if (sep == '|') {
        if (mode == 'a') {
            // Pass
        } else if (mode == 'd') {
            sorter = sort_descending;
        } else if (mode == 's') {
            sorter = sort_alphabetically;
        } else {
            error = true;
        }
      }

      // Split into "words"
      size_t pos = 0;
      std::string token;
      std::vector<float> data;
      bool found_list = false;
      while (pos != std::string::npos) {
        pos = buf_str.find(" ");
        token = buf_str.substr(0, pos);
        if (token.compare("LIST") != 0) {
          try {
            float number = std::stof(token);
            data.push_back(number);
            std::cout << number << std::endl;
          } catch (std::invalid_argument) {
            std::cout << "Inv arg " << token << std::endl;
            error = true;
            break;
          }
        } else {
          found_list = true;
        }
        buf_str.erase(0, pos + 1);
      };

      if (error || !found_list || data.size() == 0) {
        client.Send("ERROR", 5);
      } else {
        // Sort numbers
        std::sort(data.begin(), data.end(), sorter);

        std::stringstream output_ss;
        output_ss << "SORTED";
        for (float number : data)
          output_ss << " " << number;

        std::string output_str = output_ss.str();
        client.Send(output_str.c_str(), output_str.size());
      }
    }
  }
};

int main(int argc, char* argv[]) {
  SockLibInit();

  SortServer* server = new SortServer(HOST, PORT);
  server->run_server();

  SockLibShutdown();

  return 0;
}
