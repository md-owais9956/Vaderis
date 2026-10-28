# Vaderis

**A Redis-inspired, in-memory key-value database built from scratch in C++ using Winsock.**

Vaderis is an educational systems-programming project exploring how an in-memory database works internally, from TCP networking and RESP-style communication to command execution, key management, and expiration.

> Vaderis is an independent educational project inspired by Redis. It is not affiliated with or endorsed by Redis.

## Features

### Implemented

- [x] TCP server using Winsock2
- [x] RESP-style command parsing and responses
- [x] `PING` — test server connectivity
- [x] `SET` — store a key-value pair
- [x] `GET` — retrieve a value
- [x] `DEL` — delete one or more keys
- [x] `EXISTS` — check whether keys exist
- [x] `KEYS` — retrieve keys using `*` or exact-key lookup
- [x] `EXPIRE` — assign an expiration time to a key
- [x] `TTL` — retrieve the remaining lifetime of a key
- [x] Lazy expiration of expired keys
- [x] `INCR` — increment an integer value
- [x] `DECR` — decrement an integer value
- [x] `MSET` — store multiple key-value pairs
- [x] `MGET` — retrieve multiple values
- [x] Python-based test client
- [x] Automated command and expiration tests

## Tech Stack

| Component | Technology |
|---|---|
| Language | C++17 |
| Networking | Winsock2 |
| Protocol | RESP-style protocol |
| In-memory storage | `std::unordered_map` |
| Test client | Python 3 |
| Development | VS Code, MinGW-w64, Git, GitHub |

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

```powershell
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

## Supported Commands

### Basic key-value operations

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

### Multiple key-value operations

`MSET` stores multiple key-value pairs in one command:

```text
MSET name Owais language C++ project Vaderis
OK
```

`MGET` retrieves multiple values in the same order as the requested keys:

```text
MGET name language project missing
["Owais", "C++", "Vaderis", (nil)]
```

Missing keys are represented by null bulk-string responses.

### Numeric operations

```text
SET counter 10
OK

INCR counter
11

DECR counter
10
```

`INCR` and `DECR` support integer values and return errors for invalid numeric values or overflow.

### Key expiration

```text
SET session active
OK

EXPIRE session 10
1

TTL session
10
```

`TTL` returns:

- A non-negative integer when the key has an expiration.
- `-1` when the key exists without an expiration.
- `-2` when the key is missing or expired.

### Lazy expiration

Vaderis uses lazy expiration. When a command checks an expired key, the server detects its expiration and removes it.

A background expiration worker has not yet been implemented.

## Architecture

```text
          Python Test Client
                  |
                  | TCP connection
                  | RESP-style commands
                  v
            Winsock Server
                  |
                  v
             RESP Parser
                  |
                  v
          Command Dispatcher
                  |
           +------+------+
           |             |
           v             v
       Key-Value     Expiration
         Store       Metadata
           |             |
           +------+------+
                  |
                  v
           RESP Response
```

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

## Current Limitations

- Data is stored in memory and is lost when the server stops.
- Client connections are handled sequentially.
- `KEYS` supports `*` and exact-key lookup, not general glob patterns.
- Expiration cleanup is lazy; there is no background expiration worker.
- The implementation supports its own command subset rather than the complete Redis protocol.
- Persistence and crash recovery are not implemented.

## Roadmap

| Feature | Status |
|---|---|
| TCP server and RESP-style parsing | Completed |
| Basic key-value commands | Completed |
| `EXISTS` and `KEYS` | Completed |
| `EXPIRE` and `TTL` | Completed |
| `INCR` and `DECR` | Completed |
| `MSET` and `MGET` | Completed |
| Python test client | Completed |
| `APPEND`, `STRLEN`, `GETSET` | Planned |
| Lists, sets, and hashes | Planned |
| Dedicated reusable TCP client | Planned |
| Data persistence | Planned |
| Concurrent client handling | Planned |
| Background expiration cleanup | Planned |
| Performance benchmarks | Planned |

## Testing

The Python client tests:

- Basic command behavior
- Missing keys and invalid commands
- Key existence and enumeration
- Expiration and TTL behavior
- Numeric operations and invalid integers
- Multi-key operations and missing values

Run the test suite using:

```powershell
py test_client.py
```

The test client reports individual results and displays a final success message when all assertions pass.

## Contributing

Vaderis is a learning project, and feedback, bug reports, and contributions are welcome.

1. Fork the repository.
2. Create a feature branch.
3. Implement and test your changes.
4. Submit a pull request describing the changes.

## License

Licensed under the MIT License. See [LICENSE](LICENSE) for details.

---

**Built to explore database internals, networking, and systems programming one feature at a time.**