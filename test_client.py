
import socket
import time

HOST = "127.0.0.1"
PORT = 6379


def encode_command(*args):
    """Encode a command using the Redis RESP array format."""
    result = f"*{len(args)}\r\n"

    for arg in args:
        arg = str(arg)
        encoded = arg.encode("utf-8")
        result += f"${len(encoded)}\r\n{arg}\r\n"

    return result.encode("utf-8")


def read_line(sock):
    """Read bytes until CRLF."""
    data = b""

    while not data.endswith(b"\r\n"):
        chunk = sock.recv(1)

        if not chunk:
            raise ConnectionError("Incomplete response")

        data += chunk

    return data[:-2]


def read_exact(sock, length):
    """Read exactly the requested number of bytes."""
    data = bytearray()

    while len(data) < length:
        chunk = sock.recv(length - len(data))

        if not chunk:
            raise ConnectionError("Incomplete bulk response")

        data.extend(chunk)

    return bytes(data)


def read_response(sock):
    """Decode a RESP response."""
    prefix = read_exact(sock, 1)

    if prefix == b"+":
        return read_line(sock).decode("utf-8", errors="replace")

    if prefix == b"-":
        message = read_line(sock).decode(
            "utf-8", errors="replace"
        )
        return f"ERROR: {message}"

    if prefix == b":":
        return int(read_line(sock))

    if prefix == b"$":
        length = int(read_line(sock))

        if length == -1:
            return None

        data = read_exact(sock, length)

        if read_exact(sock, 2) != b"\r\n":
            raise ValueError("Invalid bulk string terminator")

        return data.decode("utf-8", errors="replace")

    if prefix == b"*":
        count = int(read_line(sock))

        if count == -1:
            return None

        return [read_response(sock) for _ in range(count)]

    raise ValueError(f"Unknown RESP prefix: {prefix!r}")


def command(sock, *args, expected=None, validator=None):
    """Send a command, print its result, and optionally verify it."""
    sock.sendall(encode_command(*args))
    response = read_response(sock)

    print(f"> {' '.join(map(str, args))}")
    print("(nil)" if response is None else response)

    if validator is not None:
        passed = validator(response)
    elif expected is not None:
        passed = response == expected
    else:
        passed = None

    if passed is True:
        print("[PASS]")
    elif passed is False:
        print("[FAIL]")

    print()

    if passed is False:
        raise AssertionError(
            f"Unexpected response for {' '.join(map(str, args))}: "
            f"{response!r}"
        )

    return response


def run_tests():
    print("=" * 50)
    print("             VADERIS SERVER TESTS")
    print("=" * 50)
    print()

    with socket.create_connection((HOST, PORT), timeout=10) as client:
        print(f"Connected to Vaderis at {HOST}:{PORT}\n")

        # -------------------------------
        # 1. Basic commands
        # -------------------------------
        print("=== BASIC COMMAND TESTS ===\n")

        command(client, "PING", expected="PONG")
        command(client, "PING", "hello", expected="hello")

        command(client, "SET", "name", "Owais", expected="OK")
        command(client, "GET", "name", expected="Owais")

        command(client, "DEL", "name", expected=1)
        command(client, "GET", "name", validator=lambda r: r is None)
        command(client, "GET", "missing", validator=lambda r: r is None)

        # -------------------------------
        # 2. Error handling
        # -------------------------------
        print("=== ERROR HANDLING TESTS ===\n")

        command(
            client, "HELLO",
            validator=lambda r: isinstance(r, str)
            and r.startswith("ERROR:")
        )

        command(
            client, "SET", "onlykey",
            validator=lambda r: isinstance(r, str)
            and "wrong number of arguments" in r
        )

        # -------------------------------
        # 3. EXISTS and KEYS
        # -------------------------------
        print("=== KEY MANAGEMENT TESTS ===\n")

        command(client, "SET", "language", "C++", expected="OK")
        command(client, "SET", "project", "Vaderis", expected="OK")

        command(client, "EXISTS", "name", expected=0)
        command(client, "EXISTS", "missing", expected=0)
        command(
            client, "EXISTS", "name", "language", "missing",
            expected=1
        )

        command(
            client, "KEYS", "*",
            validator=lambda r: isinstance(r, list)
            and "language" in r
            and "project" in r
        )

        command(client, "KEYS", "language", expected=["language"])
        command(client, "KEYS", "missing", expected=[])

        # -------------------------------
        # 4. Expiration and TTL
        # -------------------------------
        print("=== EXPIRATION TESTS ===\n")

        command(
            client, "SET", "temporary", "hello",
            expected="OK"
        )

        command(
            client, "EXPIRE", "temporary", "5",
            expected=1
        )

        command(
            client, "TTL", "temporary",
            validator=lambda r: isinstance(r, int)
            and 0 <= r <= 5
        )

        command(
            client, "EXISTS", "temporary",
            expected=1
        )

        print("Waiting 6 seconds for key expiration...\n")
        time.sleep(6)

        command(
            client, "GET", "temporary",
            validator=lambda r: r is None
        )

        command(client, "TTL", "temporary", expected=-2)
        command(client, "EXISTS", "temporary", expected=0)
        command(client, "DEL", "temporary", expected=0)

        # -------------------------------
        # 5. Additional expiration tests
        # -------------------------------
        print("=== EXPIRATION EDGE CASES ===\n")

        # Missing key
        command(client, "EXPIRE", "missing", "5", expected=0)
        command(client, "TTL", "missing", expected=-2)

        # Key without expiration
        command(client, "SET", "persistent", "data", expected="OK")
        command(client, "TTL", "persistent", expected=-1)

        # SET clears the previous expiration
        command(client, "EXPIRE", "persistent", "10", expected=1)
        command(client, "SET", "persistent", "updated", expected="OK")
        command(client, "TTL", "persistent", expected=-1)

        # Immediate expiration
        command(client, "SET", "instant", "value", expected="OK")
        command(client, "EXPIRE", "instant", "0", expected=1)
        command(
            client, "GET", "instant",
            validator=lambda r: r is None
        )

        # Invalid expiration value
        command(
            client, "EXPIRE", "persistent", "abc",
            validator=lambda r: isinstance(r, str)
            and r.startswith("ERROR:")
        )

        # Cleanup
        command(client, "DEL", "persistent", expected=1)
        command(client, "DEL", "language", "project", expected=2)

    print("=" * 50)
    print("          ALL VADERIS TESTS PASSED")
    print("=" * 50)


if __name__ == "__main__":
    try:
        run_tests()
    except (ConnectionError, OSError) as error:
        print(f"\n[CONNECTION ERROR] {error}")
        print("Make sure vaderis-server.exe is running.")
    except AssertionError as error:
        print(f"\n[TEST FAILED] {error}")
        raise
