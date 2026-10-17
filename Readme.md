# Vaderis

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)

**Vaderis** is a Redis-inspired, in-memory key-value database built from scratch in C++ to explore database internals, TCP networking, protocol parsing, and data structures.

> Vaderis is an independent educational project inspired by Redis. It is not affiliated with or endorsed by Redis.

**Status:** Work in Progress

## Overview

Vaderis aims to implement a lightweight, Redis-inspired database from scratch. The project explores how an in-memory database works internally, including TCP networking, the Redis Serialization Protocol (RESP), command processing, data storage, and persistence.

## Goals

- Understand TCP networking and client-server communication.
- Implement RESP protocol parsing and responses.
- Build a command execution engine.
- Implement an in-memory key-value store.
- Explore data structures and persistence mechanisms.
- Learn how databases handle concurrent clients and performance.

## Features and Roadmap

### Core

- [x] TCP server
- [x] Client connection handling
- [x] Command parser
- [x] Command execution engine
- [x] In-memory key-value store
- [ ] Dedicated TCP client

### Supported Commands

- [x] `PING`
- [x] `SET`
- [x] `GET`
- [x] `DEL`
- [x] `EXISTS`
- [x] `KEYS`
- [ ] `EXPIRE`
- [ ] `TTL`

### Data Structures

- [ ] Lists
- [ ] Sets
- [ ] Hashes
- [ ] Sorted sets

### Persistence

- [ ] Snapshot-based persistence
- [ ] Append-only file (AOF)

### Advanced Features

- [ ] Multiple-client concurrency
- [ ] Pub/Sub
- [ ] Transactions

### Performance

- [ ] Benchmarking
- [ ] Performance optimization

## Architecture

Vaderis follows a client-server architecture:

1. A client connects to the server over TCP.
2. The server receives commands encoded using RESP.
3. The command parser extracts the command and its arguments.
4. The command execution engine processes the request.
5. The in-memory database stores or retrieves the requested data.
6. The server sends the response back to the client.

## Example

Example commands supported by Vaderis:

```text
PING
+PONG

SET name Owais
+OK

GET name
$5
Owais

EXISTS name
:1

DEL name
:1
```

These examples illustrate the expected results. Exact response formatting may vary depending on the client.

## Project Structure

```text
Vaderis/
├── src/
│   └── server/
│       └── tcp_server.cpp
├── test_client.py
├── vaderis-server.exe
├── README.md
└── LICENSE
```

The executable name shown above is the intended name. Update your build configuration to generate `vaderis-server.exe` if your current executable is still named `redis-server.exe`.

## Development Roadmap

1. Build the TCP server.
2. Implement RESP command parsing.
3. Add basic Redis-inspired commands.
4. Expand the supported data structures.
5. Implement key expiration and TTL.
6. Add persistence.
7. Support multiple concurrent clients.
8. Benchmark and optimize performance.

## Learning Objectives

Vaderis is a hands-on project for learning:

- Computer networks and TCP sockets
- Client-server architecture
- Protocol parsing and serialization
- C++ programming and memory management
- Hash tables and data structures
- Database internals
- Persistence and concurrency

## Why Vaderis?

Building a Redis-inspired database from scratch is an opportunity to explore the systems engineering behind fast in-memory data stores. Rather than simply using an existing database, Vaderis focuses on implementing its core concepts and understanding how the pieces work together.

## Current Progress

| Feature | Status |
|---|---|
| Project setup | Completed |
| TCP server | Completed |
| Client connection handling | Completed |
| RESP command parsing | Completed |
| Command execution | Completed |
| In-memory key-value store | Completed |
| `PING`, `SET`, `GET`, `DEL` | Completed |
| `EXISTS`, `KEYS` | Completed |
| Dedicated TCP client | Planned |
| Key expiration and TTL | Planned |
| Lists, sets, and hashes | Planned |
| Persistence | Planned |
| Concurrent clients | Planned |
| Pub/Sub | Planned |
| Benchmarking | Planned |

## Disclaimer

Vaderis is an educational project inspired by Redis. It is independently developed and is not intended to be a production-ready replacement for Redis. It does not currently implement all Redis commands, features, or guarantees.

## License

This project is licensed under the [MIT License](LICENSE).

## Future Ideas

Some features I may explore after the core implementation:

- Event-driven architecture
- Non-blocking I/O
- Memory-efficient data structures
- Replication
- Sharding
- Leader-follower architecture
- Distributed caching
- Cluster communication
- Fault tolerance
- Monitoring and metrics
