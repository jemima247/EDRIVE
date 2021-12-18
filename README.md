# EDRIVE

# WHAT DOES EDRIVE DO?
Edrive is the CSC213 version of Google drive built with the following goals ib:
1. To use Networks and Distributed Systems: to establish a client to server network sysytem
2. To use Files and File Systems: so that clients can send, receive and update files across the network
3. To use Parallelism with Threads: to achieve parrallel programming so that the server is always listening for connections and so is the clients

# COMMANDS
Some basic commands for the clients are
```
send (used to send a file to the server)
receive (used to request a file from the server)
quit (used when client wants to disconnect from server)
```

# HOW TO RUN EDRIVE?
1. START UP the server by running the following command:
```
./server
```
2. START UP THE CLIENTS BY:
```
./client {name of client} {name of host} {server port number}
```
3. To send a file to the server:
```
send
You'll be prompted to enter file name: Enter the file name
```
4. To receive a file from the server
```
receive
You'll be prompted for a file name
```
5. If you edit a file the file will autometically be updated in the server as long as the client is still connected
6. To disconnect client from server
```
quit
```

# LINK TO PROJECT PRESENTATION:
https://grinco-my.sharepoint.com/:p:/g/personal/alabijem_grinnell_edu/EX47CKZajTRCnrUNZ4YZVt4BWm75bRqaG2befrjWS0bdmQ?e=eed0L6

