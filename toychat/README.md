# [How to detect when the client closes the connection?](http://stefan.buettcher.org/cs/conn_closed.html)

The solution to the problem is called recv, and it does a wonderful job. The trick is that recv supports two very nice flags: MSG_PEEK tells recv to not remove anything from the stream, while MSG_DONTWAIT makes the function return immediately if there is no data available to be read. Below, you find a code snippet that accepts a new client connection and spawns a new process that responds to queries received over that connection. The parent process waits until the socket has been closed. This can either be the case because the child process has finished execution (and closed the socket) or because the client has closed the socket before the child process could finish its task. 


```cpp
else {
	// use the poll system call to be notified about socket status changes
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN | POLLHUP | POLLRDNORM;
	pfd.revents = 0;
	while (pfd.revents == 0) {
		// call poll with a timeout of 100 ms
		if (poll(&pfd, 1, 100) > 0) {
			// if result > 0, this means that there is either data available on the
			// socket, or the socket has been closed
			char buffer[32];
			if (recv(fd, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT) == 0) {
				// if recv returns zero, that means the connection has been closed:
				// kill the child process
				kill(childProcess, SIGKILL);
				waitpid(childProcess, &status, WNOHANG);
				close(fd);
				// do something else, e.g. go on vacation
			}
		}
	}
}
```
