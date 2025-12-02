#include "guard.h"
#include "message.h"
#include "message_queue.h"
#include "user.h"
#include "room.h"

Room::Room(const std::string &room_name)
  : room_name(room_name) {
  pthread_mutex_init(&lock, NULL);
}

Room::~Room() {
  pthread_mutex_destroy(&lock);
}

void Room::add_member(User *user) {
  pthread_mutex_lock(&lock);
  members.insert(user);
  pthread_mutex_unlock(&lock);
}

void Room::remove_member(User *user) {
  pthread_mutex_lock(&lock);
  members.erase(user);
  pthread_mutex_unlock(&lock);
}

void Room::broadcast_message(const std::string &sender_username, const std::string &message_text) {
  for (User* user: members){
    std::string mdat = "delivery" + room_name + ":" + sender_username + ":" + message_text;
    
    user->mqueue.enqueue(new Message(TAG_SENDALL, mdat));
  }
}
