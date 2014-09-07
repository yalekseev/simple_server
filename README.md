simple_server
=============

A multithreaded server (with per-thread accept) that takes absolute file path
as a request and returns file contents as a reply.

How to build
============
```bash
$ git clone https://github.com/yalekseev/simple_server.git
$ mkdir build
$ cd build
$ cmake ../simple_server
$ make
```

How to use
==========

```bash
$ ./simple_server
$ echo -n "/etc/passwd" | nc -w2 localhost 50000
```
