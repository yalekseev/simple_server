simple_server
=============

A multithreaded server that accepts absolute file path as a request and returns file contents as a reply.

Usage
=====

```bash
$ ./simple_server
$ echo -n "/etc/passwd" | nc -w2 localhost 50000
```
