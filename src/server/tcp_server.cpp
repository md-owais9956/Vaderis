
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <csignal>
#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

volatile std::sig_atomic_t running = 1;

constexpr unsigned short PORT = 6379;
constexpr int BUFFER_SIZE = 4096;
constexpr long long MAX_EXPIRY_SECONDS = 315360000LL; // 10 years

using Clock = std::chrono::steady_clock;

// In-memory database: key -> value
std::unordered_map<std::string, std::string> database;

// Expiration time: key -> absolute monotonic deadline
std::unordered_map<std::string, Clock::time_point> expirationTimes;

void handleSignal(int) {
    running = 0;
}

std::string toUpper(std::string text) {
    std::transform(
        text.begin(), text.end(), text.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::toupper(c));
        }
    );
    return text;
}

std::string errorReply(const std::string& message) {
    return "-" + message + "\r\n";
}

std::string bulkReply(const std::string& value) {
    return "$" + std::to_string(value.size()) +
           "\r\n" + value + "\r\n";
}

// Returns false for missing or expired keys.
// Expired keys are removed lazily when accessed.
bool keyExists(const std::string& key) {
    auto it = database.find(key);

    if (it == database.end()) {
        expirationTimes.erase(key);
        return false;
    }

    auto expiry = expirationTimes.find(key);

    if (expiry != expirationTimes.end() &&
        Clock::now() >= expiry->second) {
        database.erase(it);
        expirationTimes.erase(expiry);
        return false;
    }

    return true;
}

// Parse a RESP array containing bulk strings.
bool parseCommand(
    const std::string& input,
    std::vector<std::string>& args,
    size_t& consumed,
    bool& invalid
) {
    consumed = 0;
    invalid = false;
    args.clear();

    if (input.empty()) {
        return false;
    }

    size_t pos = 0;

    if (input[pos++] != '*') {
        invalid = true;
        return false;
    }

    size_t lineEnd = input.find("\r\n", pos);

    if (lineEnd == std::string::npos) {
        return false;
    }

    int count = 0;

    try {
        size_t parsed = 0;
        count = std::stoi(
            input.substr(pos, lineEnd - pos), &parsed
        );

        if (parsed != lineEnd - pos ||
            count < 1 || count > 1024) {
            invalid = true;
            return false;
        }
    } catch (...) {
        invalid = true;
        return false;
    }

    pos = lineEnd + 2;

    for (int i = 0; i < count; ++i) {
        if (pos >= input.size()) {
            return false;
        }

        if (input[pos++] != '$') {
            invalid = true;
            return false;
        }

        lineEnd = input.find("\r\n", pos);

        if (lineEnd == std::string::npos) {
            return false;
        }

        int length = 0;

        try {
            size_t parsed = 0;
            length = std::stoi(
                input.substr(pos, lineEnd - pos), &parsed
            );

            if (parsed != lineEnd - pos ||
                length < 0 || length > 1024 * 1024) {
                invalid = true;
                return false;
            }
        } catch (...) {
            invalid = true;
            return false;
        }

        pos = lineEnd + 2;

        if (input.size() - pos <
            static_cast<size_t>(length) + 2) {
            return false;
        }

        args.push_back(input.substr(pos, length));
        pos += static_cast<size_t>(length);

        if (input.compare(pos, 2, "\r\n") != 0) {
            invalid = true;
            return false;
        }

        pos += 2;
    }

    consumed = pos;
    return true;
}

// Execute a parsed command.
std::string executeCommand(const std::vector<std::string>& args) {
    if (args.empty()) {
        return errorReply("empty command");
    }

    const std::string command = toUpper(args[0]);

    // PING [message]
    if (command == "PING") {
        if (args.size() == 1) {
            return "+PONG\r\n";
        }

        if (args.size() == 2) {
            return bulkReply(args[1]);
        }

        return errorReply("wrong number of arguments for PING");
    }

    // SET key value
    // Setting a value clears its previous expiration.
    if (command == "SET") {
        if (args.size() != 3) {
            return errorReply("wrong number of arguments for SET");
        }

        database[args[1]] = args[2];
        expirationTimes.erase(args[1]);

        return "+OK\r\n";
    }

    // GET key
    if (command == "GET") {
        if (args.size() != 2) {
            return errorReply("wrong number of arguments for GET");
        }

        if (!keyExists(args[1])) {
            return "$-1\r\n";
        }

        return bulkReply(database.at(args[1]));
    }

    // EXISTS key [key ...]
    if (command == "EXISTS") {
        if (args.size() < 2) {
            return errorReply("wrong number of arguments for EXISTS");
        }

        int count = 0;

        for (size_t i = 1; i < args.size(); ++i) {
            if (keyExists(args[i])) {
                ++count;
            }
        }

        return ":" + std::to_string(count) + "\r\n";
    }

    // KEYS * or KEYS exact_key
    if (command == "KEYS") {
        if (args.size() != 2) {
            return errorReply("wrong number of arguments for KEYS");
        }

        std::vector<std::string> keys;

        if (args[1] == "*") {
            // Snapshot keys first because keyExists may erase expired keys.
            std::vector<std::string> candidates;
            candidates.reserve(database.size());

            for (const auto& entry : database) {
                candidates.push_back(entry.first);
            }

            for (const auto& key : candidates) {
                if (keyExists(key)) {
                    keys.push_back(key);
                }
            }
        } else {
            if (keyExists(args[1])) {
                keys.push_back(args[1]);
            }
        }

        std::string response =
            "*" + std::to_string(keys.size()) + "\r\n";

        for (const auto& key : keys) {
            response += bulkReply(key);
        }

        return response;
    }

    // DEL key [key ...]
    if (command == "DEL") {
        if (args.size() < 2) {
            return errorReply("wrong number of arguments for DEL");
        }

        int deleted = 0;

        for (size_t i = 1; i < args.size(); ++i) {
            if (keyExists(args[i])) {
                database.erase(args[i]);
                expirationTimes.erase(args[i]);
                ++deleted;
            }
        }

        return ":" + std::to_string(deleted) + "\r\n";
    }

    // EXPIRE key seconds
    if (command == "EXPIRE") {
        if (args.size() != 3) {
            return errorReply("wrong number of arguments for EXPIRE");
        }

        if (!keyExists(args[1])) {
            return ":0\r\n";
        }

        long long seconds = 0;

        try {
            size_t parsed = 0;
            seconds = std::stoll(args[2], &parsed);

            if (parsed != args[2].size()) {
                return errorReply("invalid expire time");
            }
        } catch (...) {
            return errorReply("invalid expire time");
        }

        if (seconds > MAX_EXPIRY_SECONDS ||
            seconds < -MAX_EXPIRY_SECONDS) {
            return errorReply("invalid expire time");
        }

        // Non-positive expiration deletes the key immediately.
        if (seconds <= 0) {
            database.erase(args[1]);
            expirationTimes.erase(args[1]);
            return ":1\r\n";
        }

        expirationTimes[args[1]] =
            Clock::now() + std::chrono::seconds(seconds);

        return ":1\r\n";
    }

    // TTL key
    // -2 = missing/expired, -1 = no expiration, >= 0 = seconds remaining.
    if (command == "TTL") {
        if (args.size() != 2) {
            return errorReply("wrong number of arguments for TTL");
        }

        if (!keyExists(args[1])) {
            return ":-2\r\n";
        }

        auto expiry = expirationTimes.find(args[1]);

        if (expiry == expirationTimes.end()) {
            return ":-1\r\n";
        }

        auto now = Clock::now();

        if (now >= expiry->second) {
            database.erase(args[1]);
            expirationTimes.erase(expiry);
            return ":-2\r\n";
        }

        auto remaining =
            std::chrono::duration_cast<std::chrono::seconds>(
                expiry->second - now
            ).count();

        return ":" + std::to_string(remaining) + "\r\n";
    }

    return errorReply("unknown command '" + args[0] + "'");
}

bool sendAll(SOCKET client, const std::string& response) {
    size_t sent = 0;

    while (sent < response.size()) {
        int result = send(
            client,
            response.data() + sent,
            static_cast<int>(response.size() - sent),
            0
        );

        if (result == SOCKET_ERROR || result == 0) {
            return false;
        }

        sent += static_cast<size_t>(result);
    }

    return true;
}

void handleClient(SOCKET client) {
    char buffer[BUFFER_SIZE];
    std::string pending;

    std::cout << "Client connected.\n";

    while (running) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(client, &readSet);

        timeval timeout{};
        timeout.tv_sec = 1;

        int ready = select(0, &readSet, nullptr, nullptr, &timeout);

        if (ready == 0) {
            continue;
        }

        if (ready == SOCKET_ERROR) {
            break;
        }

        int received = recv(client, buffer, sizeof(buffer), 0);

        if (received <= 0) {
            break;
        }

        pending.append(buffer, static_cast<size_t>(received));

        // Process multiple commands received in one TCP read.
        while (!pending.empty()) {
            std::vector<std::string> args;
            size_t consumed = 0;
            bool invalid = false;

            bool complete = parseCommand(
                pending, args, consumed, invalid
            );

            if (invalid) {
                if (!sendAll(
                        client, errorReply("invalid RESP request"))) {
                    closesocket(client);
                    return;
                }

                pending.clear();
                break;
            }

            if (!complete) {
                break;
            }

            std::string response = executeCommand(args);

            if (!sendAll(client, response)) {
                closesocket(client);
                return;
            }

            pending.erase(0, consumed);
        }

        if (pending.size() > 2 * 1024 * 1024) {
            sendAll(client, errorReply("request too large"));
            break;
        }
    }

    closesocket(client);
    std::cout << "Client disconnected.\n";
}

} // namespace

int main() {
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    WSADATA wsaData{};

    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << '\n';
        return 1;
    }

    SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (server == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: "
                  << WSAGetLastError() << '\n';
        WSACleanup();
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(PORT);

    if (bind(
            server,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << '\n';
        closesocket(server);
        WSACleanup();
        return 1;
    }

    if (listen(server, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << '\n';
        closesocket(server);
        WSACleanup();
        return 1;
    }

    std::cout << "Vaderis server started on port "
              << PORT << '\n';

    while (running) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(server, &readSet);

        timeval timeout{};
        timeout.tv_sec = 1;

        int ready = select(0, &readSet, nullptr, nullptr, &timeout);

        if (ready == 0) {
            continue;
        }

        if (ready == SOCKET_ERROR) {
            break;
        }

        sockaddr_in clientAddress{};
        int clientLength = sizeof(clientAddress);

        SOCKET client = accept(
            server,
            reinterpret_cast<sockaddr*>(&clientAddress),
            &clientLength
        );

        if (client == INVALID_SOCKET) {
            if (running) {
                std::cerr << "Accept failed: "
                          << WSAGetLastError() << '\n';
            }
            continue;
        }

        // This server currently handles clients sequentially.
        handleClient(client);
    }

    closesocket(server);
    WSACleanup();

    std::cout << "Vaderis server shut down.\n";
    return 0;
}
