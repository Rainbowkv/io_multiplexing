# From raw_server to Reactor

select:
- A cross platform interface. 
- The FD_SETSIZE limits the upper bound of monitored events set's size, but typically equals to the maximum size of file descriptor table belong to one process, which you can use "ulimit -n" to check this value and modify it if needed.

poll: 
- It has no limit on the size of monitored events set.

epoll:
- Unlimited size of the set of events to be monitored.
- No need for repeated copying between kernel space and user space.
- Events are detected through callback functions, rather than through polling.
- The Red-Black tree is used as the underlying storage structure for adding, deleting and modifying events.

REUSEPORT: ???

 
