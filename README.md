# CS 454 Distributed Systems Winter 2017 Assignment 03

## Assignment Goal
Implement a crude version of Remote Procedure Call. 

## Project Participants

- Ji Ho Park (20518866): jh49park@uwaterloo.ca
- Myungsun Jung (20511678): m22jung@uwaterloo.ca

## How to Compile

1. make to get librpc.a and binder executable in the linux.student.cs environment.
2. Prepare 'perfect' client and server source files.
3. `g++ -L. client.o -lrpc -o client` for compiling client.
4. `g++ -L. server_functions.o server_function_skels.o server.o -lrpc -o server` for compiling server.
5. `./binder`
6. Manually set the `BINDER_ADDRESS` and `BINDER_PORT` environment variables on the client and server machines.
7. `./server` and `./client` to run server(s) and client(s). Note that the binder, client, and server may be running on different machines.

## Dependencies

No other external libraries other than the default C++ library have been used in the code base.
