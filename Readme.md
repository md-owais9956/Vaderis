# Vaderis

**A Redis-inspired, in-memory key-value database built from scratch in C++ using Winsock.**

Vaderis is a learning-oriented project focused on understanding how an in-memory database works internally—from TCP networking and the RESP protocol to command execution, key management, and expiration.

> Vaderis is an independent educational project inspired by Redis. It is not affiliated with or endorsed by Redis.

---

## Features

### Implemented

- [x] TCP server using Winsock
- [x] RESP array and bulk-string command parsing
- [x] `PING` — test server connectivity
- [x] `SET` — store a key-value pair
- [x] `GET` — retrieve a value
- [x] `DEL` — delete one or more keys
- [x] `EXISTS` — check whether keys exist
- [x] `KEYS *` — list existing keys
- [x] `EXPIRE` — assign a time-to-live to a key
- [x] `TTL` — retrieve the remaining lifetime of a key
- [x] `INCR` — increment an integer value
- [x] `DECR` — decrement an integer value
- [x] Lazy expiration of expired keys
- [x] Python-based RESP test client
- [x] Automated command and expiration tests

*Confirm that the latest `INCR`, `DECR`, `EXPIRE`, and `TTL` source code is committed to this branch before treating all items above as complete.*

### Planned

- [ ] Dedicated reusable TCP client
- [ ] `MSET` and `MGET` — multiple key-value operations
- [ ] Lists and sets
- [ ] Hashes
- [ ] Data persistence
- [ ] Background expiration cleanup
- [ ] Concurrent client handling
- [ ] Improved error handling and protocol coverage
- [ ] Performance benchmarks

---

## Tech Stack

| Component | Technology |
|---|---|
| Language | C++17 |
| Networking | Winsock2 |
| Protocol | RESP-inspired command protocol |
| Storage | `std::unordered_map` |
| Testing | Python 3 and sockets |
| Development | VS Code, Git, GitHub |

---

## Getting Started

### Prerequisites

- Windows
- MinGW-w64 with `g++`
- Python 3

### 1. Clone the repository

```bash
git clone https://github.com/md-owais9956/Vaderis.git
cd Vaderis
```

### 2. Compile the server

```bash
g++ -std=c++17 -Wall -Wextra src/server/tcp_server.cpp -o vaderis-server.exe -lws2_32
```

### 3. Start the server

```powershell
.\vaderis-server.exe
```

The server listens on port `6379`.

### 4. Run the test client

Open another terminal in the project directory:

```powershell
py test_client.py
```

Keep the server running while the tests execute.

---

## Command Examples

The examples below illustrate the intended command behavior.

### Basic operations

```text
PING
PONG

SET language C++
OK

GET language
C++

EXISTS language
1

DEL language
1
```

### Numeric operations

```text
SET visits 10
OK

INCR visits
11

DECR visits
10
```

`INCR` and `DECR` operate on integer values. Invalid integer values should return an error without changing the stored value.

### Key expiration

```text
SET session active
OK

EXPIRE session 10
1

TTL session
10
```

`EXPIRE` assigns a lifetime in seconds. `TTL` returns the remaining lifetime, `-1` when a key has no expiration, and `-2` when a key is missing or expired.

### Lazy expiration

Vaderis uses lazy expiration: when an expired key is accessed by a command that checks its existence, the server detects the expiration and removes the key.

Background expiration cleanup is a planned improvement.

---

## Architecture

```text
Python Test Client
        |
        | TCP / RESP-style commands
        v
   Winsock TCP Server
        |
        v
    RESP Parser
        |
        v
  Command Dispatcher
        |
        +---- Key-Value Store
        |
        +---- Expiration Metadata
        |
        v
   RESP Response
```

### Current limitations

- Data is stored in memory and is lost when the server stops.
- Client connections are handled sequentially.
- `KEYS` currently supports `*` and exact-key lookup, not general glob patterns.
- Expired keys are cleaned lazily rather than by a background worker.
- The protocol implementation covers the commands implemented by Vaderis, not the entire Redis protocol.

---

## Project Structure

```text
Vaderis/
├── src/
│   └── server/
│       └── tcp_server.cpp
├── test_client.py
├── README.md
├── LICENSE
└── .gitignore
```

---

## Development Roadmap

| Feature | Status |
|---|---|
| TCP server and RESP parsing | Completed |
| Basic key-value commands | Completed |
| `EXISTS` and `KEYS` | Completed |
| `EXPIRE` and `TTL` | Implemented; verify current branch |
| `INCR` and `DECR` | Implemented; verify current branch |
| Python test client | Completed |
| `MSET` and `MGET` | Planned |
| Lists, sets, and hashes | Planned |
| Persistence | Planned |
| Concurrent clients | Planned |
| Background expiration cleanup | Planned |

---

## Contributing

Vaderis is a learning project, and contributions, feedback, bug reports, and suggestions are welcome.

1. Fork the repository.
2. Create a feature branch.
3. Make and test your changes.
4. Submit a pull request describing the implementation.

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

---

**Built as a journey into database internals, networking, and systems programming.**