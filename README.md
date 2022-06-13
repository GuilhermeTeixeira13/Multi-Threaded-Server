# Part 1: Multi-threaded Server

The basic web server provided has a single thread of control. Single-threaded web servers suffer 
from a fundamental performance problem in that only a single HTTP request can be serviced at a 
time. Thus, every other client that is accessing this web server must wait until the current http request 
has finished; this is especially a problem if the current http request is a long-running CGI program 
(such as the proof of work program) or is resident only on disk (i.e., is not in memory). Thus, the 
most important extension that you will be adding is to make the basic web server multi-threaded. 

The simplest approach to building a multi-threaded server is to spawn a new thread for every new 
http request. The OS will then schedule these threads according to its own policy. The advantage of 
creating these threads is that now short requests will not need to wait for a long request to complete; 
further, when one thread is blocked (i.e., waiting for disk I/O to finish) the other threads can continue 
to handle other requests. However, the drawback of the one-thread-per-request approach is that the 
web server pays the overhead of creating a new thread on every request. 

Therefore, the generally preferred approach for a multi-threaded server is to create a fixed-size pool 
of worker threads when the web server is first started. With the pool-of-threads approach, each 
thread is blocked until there is an http request for it to handle. Therefore, if there are more worker 
threads than active requests, then some of the threads will be blocked, waiting for new http requests 
to arrive; if there are more requests than worker threads, then those requests will need to be buffered 
until there is a ready thread. 

In your implementation, you must have a master thread that begins by creating a pool of worker 
threads, the number of which is specified on the command line. Your master thread is then 
responsible for accepting new http connections over the network and placing the descriptor for this 
connection into a fixed-size buffer; in your basic implementation, the master thread should not read 
from this connection. The number of elements in the buffer is also specified on the command line. 
Note that the existing web server has a single thread that accepts a connection and then immediately 
handles the connection; in your web server, this thread should place the connection descriptor into a 
fixed-size buffer and return to accepting more connections. You should investigate how to create and 
manage posix threads with pthread_create and pthread_detach. 

Each worker thread is able to handle both static and dynamic requests. 

- A worker thread wakes when there is an http request in the queue; when there are multiple http 
requests available, which request is handled depends upon the scheduling policy, described 
below. 
- Once the worker thread wakes, it performs the read on the network descriptor, obtains the 
specified content (by either reading the static file or executing the CGI process), and then returns 
the content to the client by writing to the descriptor. 
- The worker thread then waits for another http request. 

Note that the master thread and the worker threads are in a producer-consumer relationship and 
require that their accesses to the shared buffer be synchronized. 

Specifically, 
- the master thread must block and wait if the buffer is full; 
- a worker thread must wait if the buffer is empty. 

In this project, you are advised to use condition variables.

#### Online Content that helped us
https://www.youtube.com/watch?v=Pg_4Jz8ZIH4&list=RDCMUCwd5VFu4KoJNjkWJZMFJGHQ&index=4
https://www.youtube.com/watch?v=FMNnusHqjpw&list=RDCMUCwd5VFu4KoJNjkWJZMFJGHQ&start_radio=1&t=5s
https://www.youtube.com/watch?v=P6Z5K8zmEmc&list=RDCMUCwd5VFu4KoJNjkWJZMFJGHQ&index=3
https://www.youtube.com/watch?v=0sVGnxg6Z3k
https://pages.cs.wisc.edu/~remzi/OSTEP/threads-cv.pdf
