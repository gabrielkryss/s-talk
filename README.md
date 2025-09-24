
# 🧵 S-tALK UDP Chat Messenger

A lightweight, terminal-based peer-to-peer chat application built in C++ using
WinSock and multithreading. Goal of this project was to learn and implement the
producer consumer thread model.

## 📦 Features

- Asynchronous message sending and receiving using `std::jthread`
- Thread-safe send/receive queues with `std::mutex` and `std::condition_variable`
- Simple command-line interface with ANSI escape codes for clean input
- Graceful shutdown via the `!` command
- Cross-platform Makefile (Windows/Linux)
- Uses WinSock2 for networking on Windows

## 🛠️ Requirements

- C++23-compatible compiler (e.g., `clang++` or `g++`)
- Windows OS (WinSock2)
- GNU Make or compatible `make`
- Terminal that supports ANSI escape codes

## 🧰 Build Instructions

```bash
make
```

This compiles main.cpp into main.exe using strict warning flags and links
against ws2_32 on Windows.

To remove the executable:

```bash
make clean
```

## 🚀 How to Run

You need two instances of the program (same machine or across a LAN).

```bash
main.exe <localhostIP> <yourPort> <peerPort>
```

### Example (same machine)

Open two terminals:
Terminal 1

```bash
main.exe 127.0.0.1 5000 5001
```

Terminal 2

```bash
main.exe 127.0.0.1 5001 5000
```

Type messages and press Enter to send. Enter ! and press Enter to exit gracefully.

## 🧯 Graceful Exit

Typing `!` will:

- Signal all threads to stop
- Unblock any waiting threads
- Close sockets
- Cleanup WinSock
- Exit the program

## 📎 Notes

- Only works on Windows due to WinSock dependency.
- UDP is connectionless; delivery is not guaranteed.
- Adjust IPs and ports to test across different machines.
