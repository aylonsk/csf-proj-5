
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
  Connection conn;


  std::string server_hostname;
  int server_port;
  std::string username;

  server_hostname = argv[1];
  server_port = std::stoi(argv[2]);
  username = argv[3];
  std::stringstream slog;
  slog << "slogin:"<< username;
  conn.connect(server_hostname, server_port);
  if (!conn.is_open()) {
    std::cerr << "Error: Failed to connect to server\n";
    return 1;
  }

  conn.send(Message(TAG_SLOGIN, username));
  Message response;
  if (!conn.receive(response)) {
    std::cerr << "Error: Failed to receive slogin response\n";
    conn.close();
    return 1;
  }

  if (response.tag == TAG_ERR) {
    std::cerr << response.data << std::endl;
    conn.close();
    return 1;
  }

  // TODO: send slogin message
  std::string line;
  for (std::string line; std::getline(std::cin, line);) {
    if(line.substr(0,6) == "/join "){
      std::stringstream join;
      join << "join:"<< line.substr(6);
      conn.send(Message(TAG_JOIN, line.substr(6)));
      Message response;
      if (!conn.receive(response)) {
        std::cerr << "ERROR: failed to receive response\n";
        conn.close();
        return 1;
      }
      if (response.tag == TAG_ERR) {
        std::cerr << response.data << std::endl;
      }

    } else if(line.substr(0,6) == "/leave"){
      std::stringstream leave;
      leave << "leave:";
      conn.send(Message(TAG_LEAVE, "leave"));
      Message response;
      if (!conn.receive(response)) {
        std::cerr << "ERROR: failed to receive response\n";
        conn.close();
        return 1;
      }
      if (response.tag == TAG_ERR) {
        std::cerr << response.data << std::endl;
      }

    } else if (line.substr(0,5) == "/quit"){
      std::stringstream quit;
      quit << "quit:";
      conn.send(Message(TAG_QUIT, "quit"));
      Message response;
      if (!conn.receive(response)) {
        std::cerr << "ERROR: failed to receive response\n";
        conn.close();
        return 1;
      }

      if (response.tag == TAG_ERR) {
        std::cerr << response.data << std::endl;
      }
      return 0;

    } else if(line.substr(0,1) == "/"){
      std::cerr << "ERRROR: improper command entered";
      conn.close();
      return 0;
    } else{
      std::stringstream message;
      message << "sendall:" << line;
      Message m = Message(TAG_SENDALL, line);
      conn.send(m);
      Message response;
      if (!conn.receive(response)) {
        std::cerr << "ERROR: failed to receive response\n";
        conn.close();
        return 1;
      }
      if (response.tag == TAG_ERR) {
        std::cerr << response.data << std::endl;
      }
    }
  }
      
  // TODO: loop reading commands from user, sending messages to
  //       server as appropriate
  conn.close();
  return 0;
}
