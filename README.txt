REPORT:

The main critical sections are: (1) MessageQueue
operations (enqueue/dequeue in message_queue.cpp:18-20, 44-48) protecting the
m_messages deque, (2) Server room map operations (server.cpp:58-62, 71-73, 115-119,
271-276) protecting the m_rooms map, and (3) Room member operations (room.cpp:19-21,
25-27, 32-41) protecting each Room's members set.

For MessageQueue, a mutex-semaphore pattern is used. pthread_mutex_t m_lock protects
the deque structure, while sem_t m_avail provides blocking behavior for receivers.
The semaphore maintains the available messages count and enables timeout-based
blocking from sem_timedwait(), implementing the standard producer-consumer pattern.

The Server class uses a single pthread_mutex_t m_lock to protect the m_rooms map,
ensuring atomic room lookup and creation. This prevents race conditions where
multiple threads simultaneously try to create the same room. Each Room instance
has its own pthread_mutex_t lock to protect its members set, allowing concurrent
operations on different rooms while preventing race conditions within a single room.

Deadlocks are prevented through consistent lock ordering. Server::m_lock is always
acquired before Room::lock (chat_with_sender and chat_with_receiver).
This ordering is maintained when Server::m_lock is held during broadcast_message()
or add_member() calls, which then acquire Room::lock. Since locks are always acquired
in the same order and protect different resources, no circular wait
dependencies can occur.

Race conditions are prevented because all shared data structure modifications are
atomic. The MessageQueue mutex ensures enqueue/dequeue operations don't interfere,
the semaphore coordinates producer-consumer synchronization, and all room map and
member set accesses are protected by their respective mutexes. The mutex is not
held during sem_timedwait() in dequeue(), preventing deadlock scenarios where
enqueue() might be waiting for the mutex.

