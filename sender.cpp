#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"

int main(int argc, char **argv) {
  if (argc != 4) {
    std::cerr << "Usage: ./sender [server_address] [port] [username]\n";
    return 1;
  }

  std::string server_hostname;
  int server_port;
  std::string username;

  server_hostname = argv[1];
  server_port = std::stoi(argv[2]);
  username = argv[3];

  // TODO: connect to server

  // TODO: send slogin message
  std::string line;
  for (std::string line; std::getline(std::cin, line);) {
        std::cout << line << std::endl;
    }
    if(line.substr(0,5) == "/join "){

    } else if(line.substr(0,6) == "/leave "){

    } else if (line.substr(0,5) == "/quit "){

    } else if(line.substr(0) == "/"){
      // error
    } else{
      std::string message = line;
      conn.send(message)
    }
      
  // TODO: loop reading commands from user, sending messages to
  //       server as appropriate

  return 0;
}
