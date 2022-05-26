# TP-SO
Trabalho Prático da Unidade Curricular de SO


## You will add a new cgi-bin function.  ✔️✔️

You can usa as a model the existing adder function.
This new function will be called proofofwork and accepts a string and difficulty number (n)
and attempts to find a nonce (an integer) such that
o HASH( string || atoi(nonce) ) = 20 bytes where the first n bytes are zeros
o It then returns the nonce and hash
o Thus its simple (and rapid) for the client to check that the server did this work.


## You will make the web server multi-threaded, with the appropriate synchronization. 

####  Spawn a new thread for every new http request. ✔️
#### Create a fixed-size pool of worker threads when the web server is first started.
####  You must have a master thread that begins by creating a pool of worker threads, the number of which is specified on the command line.
Accepting new http connections over the network.<br>
Placing the descriptor for this connection into a fixed-size buffer.<br>
Number of elements in the buffer is also specified on the command line.<br>
this thread should place the connection descriptor into a fixed-size buffer and return to accepting more connections.<br>
You should investigate how to create and manage posix threads with pthread_create and pthread_detach.<br>
#### Worker thread - Static and Dynamic requests. 
A worker thread wakes when there is an http request in the queue; when there are multiple http
requests available, which request is handled depends upon the scheduling policy, described
below.<br>
Once the worker thread wakes, it performs the read on the network descriptor, obtains the
specified content (by either reading the static file or executing the CGI process), and then returns
the content to the client by writing to the descriptor.<br>
The worker thread then waits for another http request.<br>
#### Master thread and the worker threads --> Producer-Consumer Relationship
The master thread must block and wait if the buffer is full.<br>
A worker thread must wait if the buffer is empty.<br>

##### Side Notes - Conditional Variables
To declare such a condition variable, one simply writes something
like this: pthread cond t c;, which declares c as a condition variable
(note: proper initialization is also required). A condition variable has two
operations associated with it: wait() and signal(). The wait() call
is executed when a thread wishes to put itself to sleep; the signal() call.

https://pages.cs.wisc.edu/~remzi/OSTEP/threads-cv.pdf
