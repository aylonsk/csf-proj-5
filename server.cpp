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
  Room* room;
  User* user;
};
struct worker_arg {
    Server* server;
    Server::client_data* cd;
};


////////////////////////////////////////////////////////////////////////
// Client thread functions
////////////////////////////////////////////////////////////////////////

void Server::chat_with_sender(client_data *cd){
  while (true){
    Message m;
    if (!cd->conn->receive(m)) {
        std::cerr << "Failed to read message\n";
        close(cd->sock);
        return;
    }
    ssize_t n = rio_readn(cd->sock, &m, sizeof(Message));
    if (n != sizeof(Message)) {
        cd->conn->send(Message(TAG_ERR, "Incomplete message"));
        return;
    }
    if (m.tag == TAG_JOIN){ 
      // register to room
      std::cout << "joined sender" << m.data << std::endl;;

      pthread_mutex_lock(&m_lock);
      cd->room = find_or_create_room(m.data);
      cd->room->add_member(cd->user);
      cd->conn->send(Message(TAG_OK, m.data));
      pthread_mutex_unlock(&m_lock);
      std::cout << "joined sender" << m.data << std::endl;;
    }
    if (m.tag == TAG_SENDALL){
      std::cout << "broadcasting" << std::endl;;
      if (!cd->room){
        cd->conn->send(Message(TAG_ERR, "Cannot send message while not in room"));
        return;
      }
      pthread_mutex_lock(&m_lock);
      cd->room->broadcast_message(cd->username, m.data);
      pthread_mutex_unlock(&m_lock);

      // broadcast
      cd->conn->send(Message(TAG_OK, m.data));
    }
    if (m.tag == TAG_LEAVE){
      // deregister room
      if (!cd->room){
        cd->conn->send(Message(TAG_ERR, "Cannot leave while not in room"));
        return;
      }
      cd->room->remove_member(cd->user);
      cd->room = nullptr;
      cd->conn->send(Message(TAG_OK, m.data));
    }
    if (m.tag == TAG_QUIT){
      cd->conn->send(Message(TAG_OK, "ok"));
      close(cd->sock);

      return;
    }
    if (m.tag == TAG_ERR){
      // destroy
      cd->conn->send(Message(TAG_ERR, m.data));
      return;
    }
    cd->conn->send(Message(TAG_ERR, "invalid tag"));
    return;

  }
}

void Server::chat_with_receiver(client_data *cd){
  Message m;
  bool t = false;
  if (!cd->conn->receive(m)) {
        std::cerr << "Failed to read message\n";
        close(cd->sock);
        return;
  }
  if (m.tag == TAG_JOIN){ 
      // register to room
      pthread_mutex_lock(&m_lock);
      cd->room = find_or_create_room(m.data);
      cd->room->add_member(cd->user);
      cd->conn->send(Message(TAG_OK, m.data));
      pthread_mutex_unlock(&m_lock);
      t = true;
      std::cout<< "joined receiver" << std::endl;
  } else if (m.tag == TAG_ERR){
      // destroy
      cd->conn->send(Message(TAG_ERR, m.data));
      return;
  } else {
      cd->conn->send(Message(TAG_ERR, "Invalid tag"));
      return;
  }
  while (t){
    Message* new_mess = cd->user->mqueue.dequeue();
    if (!new_mess) continue; 
    new_mess->tag = TAG_DELIVERY;
    std::cout<< new_mess->tag << std::endl;
    bool ok = cd->conn->send(*new_mess);
    std::cout << "[receiver] send returned=" << ok << std::endl;
    delete new_mess;
  }
}

namespace {

void *worker(void *arg) {
  std::cout << "hi ho" << std::endl;  
  pthread_detach(pthread_self());
  worker_arg* wa = static_cast<worker_arg*>(arg);
  Server* server = wa->server;
  Server::client_data* cd = wa->cd;
  delete wa;

  Message m;
    if (!cd->conn->receive(m)) {
        std::cerr << "Failed to read login message\n";
        close(cd->sock);
        delete cd->conn;
        delete cd;
        return nullptr;
    }

    if (m.tag == TAG_SLOGIN) {
        cd->type = 'S';
        cd->username = m.data;  
        cd->user = new User(m.data);
        Message reply(TAG_OK, "ok");
        cd->conn->send(reply);
        server->chat_with_sender(cd);
    }
    else if (m.tag == TAG_RLOGIN) {
        cd->type = 'R';
        cd->username = m.data;
        std::cout << "mdata:" << m.data;
        cd->user = new User(m.data);
        Message reply(TAG_OK, "ok");
        cd->conn->send(reply);
        server->chat_with_receiver(cd);
        // if (!cd->conn->receive(m)) {
        //   std::cerr << "Failed to read login message\n";
        //   close(cd->sock);
        //   delete cd->conn;
        //   delete cd;
        //   return nullptr;
        // }
        // if(m.tag == TAG_JOIN) {

        // }
      } else {
        Message reply(TAG_ERR, "Invalid login");
        cd->conn->send(reply);
        close(cd->sock);
        delete cd->conn;
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
  pthread_mutex_init(&m_lock, NULL);
}

Server::~Server() {
  pthread_mutex_destroy(&m_lock);

}

bool Server::listen() {
  // TODO: use open_listenfd to create the server socket, return true
  //       if successful, false if not
  std::ostringstream ss;
  ss << m_port;
  std::string port_str = ss.str();
  std::cout << "Calling open_listenfd(" << port_str << ")\n";

  m_ssock = open_listenfd(port_str.c_str());

    if (m_ssock < 0) {
        std::cerr << "open_listenfd FAILED\n";
        return false;
    }

    std::cout << "open_listenfd succeeded, socket=" << m_ssock << "\n";
    return true;
}

void Server::handle_client_requests() {
  while(true){
    std::cout << "About to Accept()\n" << std::flush;
    int clientfd = Accept(m_ssock, NULL, NULL);
    std::cout << "Accepted fd: " << clientfd << "\n" << std::flush;
    if (clientfd < 0) {
      std::cerr << "err: Failed to connect to server\n";
    }
    Server::client_data* cd = new Server::client_data;
    cd->sock = clientfd;          
    cd->conn = new Connection(clientfd);   
    pthread_t thr_id;
    worker_arg* arg = new worker_arg;
    arg->server = this;
    arg->cd = cd;
    if (pthread_create(&thr_id, NULL, worker, arg) != 0) {
      std::cerr << "err: Failed to connect to server\n";
    }
  }

  // TODO: infinite loop calling accept or Accept, starting a new
  //       pthread for each connected client
}

Room *Server::find_or_create_room(const std::string &room_name) {
  if (m_rooms.count(room_name)==0){
    Room* new_room = new Room(room_name);
    m_rooms[room_name] = new_room;
  }
  return m_rooms[room_name];

  // TODO: return a pointer to the unique Room object representing
  //       the named chat room, creating a new one if necessary
}


