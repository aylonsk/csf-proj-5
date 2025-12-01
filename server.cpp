#include <pthread.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <set>
#include <vector>
#include <cctype>
#include <cassert>
#include "message.h"
#include "connection.h"
#include "user.h"
#include "room.h"
#include "guard.h"
#include "server.h"

////////////////////////////////////////////////////////////////////////
// Server implementation data types
////////////////////////////////////////////////////////////////////////

// TODO: add any additional data types that might be helpful
//       for implementing the Server member functions
struct client_data{
  int sock; 
  char type;
  std::string username;
  Connection* conn;
  pthread_t thread;

};

////////////////////////////////////////////////////////////////////////
// Client thread functions
////////////////////////////////////////////////////////////////////////

namespace {

void *worker(void *arg) {
  pthread_detach(pthread_self());
  client_data* cd = static_cast<client_data*>(arg);
  Message m;
    if (!cd->conn->receive(m)) {
        std::cerr << "Failed to read login message\n";
        delete cd;
        return nullptr;
    }

    if (m.tag == TAG_SLOGIN) {
        cd->type = 'S';
        cd->username = m.data;  
        Message reply(TAG_OK, "Welcome sender");
        cd->conn->send(reply);
    }
    else if (m.tag == TAG_RLOGIN) {
        cd->type = 'R';
        cd->username = m.data;
        Message reply(TAG_OK, "Welcome receiver");
        cd->conn->send(reply);
    }
    else {
        Message reply(TAG_ERR, "Invalid login");
        cd->conn->send(reply);
        delete cd;
        return nullptr;
    }
    return nullptr;

  // TODO: use a static cast to convert arg from a void* to
  //       whatever pointer type describes the object(s) needed
  //       to communicate with a client (sender or receiver)

  // TODO: read login message (should be tagged either with
  //       TAG_SLOGIN or TAG_RLOGIN), send response

  // TODO: depending on whether the client logged in as a sender or
  //       receiver, communicate with the client (implementing
  //       separate helper functions for each of these possibilities
  //       is a good idea)

  return nullptr;
}

}

////////////////////////////////////////////////////////////////////////
// Server member function implementation
////////////////////////////////////////////////////////////////////////

Server::Server(int port)
  : m_port(port)
  , m_ssock(-1) {
  pthread_mutex_t lock;
  pthread_mutex_init(&lock, NULL);
  m_lock = lock;
}

Server::~Server() {
  pthread_mutex_destroy(&m_lock);

}

bool Server::listen() {
  // TODO: use open_listenfd to create the server socket, return true
  //       if successful, false if not
  m_ssock = open_listenfd((char*) m_port);
  if (m_ssock<0){
    return false;
  }
  return true;
}

void Server::handle_client_requests() {
  while(true){
    int clientfd = Accept(m_ssock, NULL, NULL);
    if (clientfd < 0) {
      std::cerr << "err: Failed to connect to server\n";
    }
    client_data *cd = new client_data;
    cd->sock = clientfd;               // from accept()
    cd->conn = new Connection(clientfd);    // if your Connection works like this
    pthread_t thr_id;
    if (pthread_create(&thr_id, NULL, worker, cd) != 0) {
      std::cerr << "err: Failed to connect to server\n";
    }
  }

  // TODO: infinite loop calling accept or Accept, starting a new
  //       pthread for each connected client
}

Room *Server::find_or_create_room(const std::string &room_name) {
  if (m_rooms.count(room_name)==0){
    Room new_room = Room(room_name);
    m_rooms[room_name] = &new_room;
  }
  return m_rooms[room_name];

  // TODO: return a pointer to the unique Room object representing
  //       the named chat room, creating a new one if necessary
}
