#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "client_util.h"
#include "connection.h"

int main(int argc, char **argv) {
  if (argc != 5) {
    std::cerr << "Usage: ./receiver [server_address] [port] [username] [room]\n";
    return 1;
  }

  std::string server_hostname = argv[1];
  int server_port = std::stoi(argv[2]);
  std::string username = argv[3];
  std::string room_name = argv[4];

  Connection conn;

  // connect to server
  conn.connect(server_hostname, server_port);
  if (!conn.is_open()) {
    std::cerr << "Error: Failed to connect to server\n";
    return 1;
  }

  // rlogin message
  Message rlogin_msg(TAG_RLOGIN, username);
  if (!conn.send(rlogin_msg)) {
    std::cerr << "Error: Failed to send rlogin message\n";
    return 1;
  }

  // receive response to rlogin
  Message response;
  if (!conn.receive(response)) {
    std::cerr << "Error: Failed to receive rlogin response\n";
    return 1;
  }

  if (response.tag == TAG_ERR) {
    std::cerr << response.data << std::endl;
    return 1;
  }

  // join message
  Message join_msg(TAG_JOIN, room_name);
  if (!conn.send(join_msg)) {
    std::cerr << "Error: Failed to send join message\n";
    return 1;
  }

  // receive response to join
  if (!conn.receive(response)) {
    std::cerr << "Error: Failed to receive join response\n";
    return 1;
  }

  if (response.tag == TAG_ERR) {
    std::cerr << response.data << std::endl;
    return 1;
  }

  // main loop to receive messages
  while (true) {
    Message msg;
    if (!conn.receive(msg)) {
      break;
    }

    if (msg.tag == TAG_DELIVERY) {
      // parse delivery message
      std::string payload = msg.data;
      size_t first_colon = payload.find(':');
      size_t second_colon = payload.find(':', first_colon + 1);
      
      if (first_colon != std::string::npos && second_colon != std::string::npos) {
        std::string sender = payload.substr(first_colon + 1, second_colon - first_colon - 1);
        std::string message_text = payload.substr(second_colon + 1);
        std::cout << sender << ": " << message_text << std::endl;
      }
    }
  }

  return 0;
}