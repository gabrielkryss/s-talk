#include "Tracy.hpp"
#include "TracyLock.hpp"

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib") // Link with WinSock library

std::atomic<bool> running{true};
TracyLockable(std ::mutex, sendMutex);
TracyLockable(std ::mutex, receiveMutex);
std::condition_variable_any sendCV, receiveCV;
std::queue<std::string> sendQueue, receiveQueue;

void inputThread(const std::string &userIp, unsigned short userPort) {
  tracy::SetThreadName("Input Thread");
  ZoneScopedN("Input Loop");

  std::string message;
  while (running) {
    ZoneScopedN("getline");
    std::getline(std::cin, message);
    if (message == "!") {
      running = false;
      sendCV.notify_all();    // Notify send thread for shutdown
      receiveCV.notify_all(); // Notify display thread for shutdown
      break;
    }
    {
      ZoneScopedN("enqueue send");
      std::lock_guard<LockableBase(std::mutex)> lock(sendMutex);
      LockMark(sendMutex);
      sendQueue.push(message);
      TracyPlot("SendQueueSize", int64_t(sendQueue.size()));
    }

    // Clear the line and print formatted message
    std::cout << "\033[F\033[2K"; // Move cursor up and clear the line
    std::cout << "[user-" << userIp << "-" << userPort << "] Me: " << message
              << std::endl;

    sendCV.notify_one(); // Notify send thread that a message is ready
  }
}

void sendThread(SOCKET &sendSocket, sockaddr_in &remoteAddr) {
  tracy::SetThreadName("Send Thread");
  ZoneScopedN("Send Loop");

  while (running || !sendQueue.empty()) {
    std::unique_lock<LockableBase(std::mutex)> lock(sendMutex);
    ZoneScopedN("wait sendCV");
    sendCV.wait(lock, [] {
      return !sendQueue.empty() || !running;
    }); // Wait for new messages or termination
    if (!sendQueue.empty()) {
      ZoneScopedN("sendto");
      // snapshot size for plot while still holding lock (cheap)
      TracyPlot("SendQueueSize", static_cast<int64_t>(sendQueue.size()));
      std::string message = sendQueue.front();
      sendQueue.pop();
      lock.unlock(); // Unlock mutex before sending data
      sendto(sendSocket, message.c_str(), static_cast<int>(message.size()), 0,
             reinterpret_cast<sockaddr *>(&remoteAddr), sizeof(remoteAddr));
    }
  }
}

void receiveThread(SOCKET &recvSocket) {
  tracy::SetThreadName("Receive Thread");
  ZoneScopedN("Recv Loop");

  char buffer[2048];
  sockaddr_in senderAddr;
  int senderAddrLen = sizeof(senderAddr);

  while (running) {
    ZoneScopedN("recvfrom");
    int bytesReceived =
        recvfrom(recvSocket, buffer, sizeof(buffer), 0,
                 reinterpret_cast<sockaddr *>(&senderAddr), &senderAddrLen);
    if (bytesReceived > 0) {
      {
        std::lock_guard<LockableBase(std::mutex)> lock(receiveMutex);
        LockMark(receiveMutex);
        receiveQueue.emplace(buffer, static_cast<size_t>(bytesReceived));
        TracyPlot("RecvQueueSize", int64_t(receiveQueue.size()));
      }
      receiveCV
          .notify_one(); // Notify display thread that a message is received
    }
  }
}

void displayThread(const std::string &otherIp, unsigned short otherPort) {
  tracy::SetThreadName("Display Thread");
  ZoneScopedN("Display Loop");

  while (running || !receiveQueue.empty()) {
    std::unique_lock<LockableBase(std::mutex)> lock(receiveMutex);
    ZoneScopedN("wait receiveCV");
    receiveCV.wait(lock, [] {
      return !receiveQueue.empty() || !running;
    }); // Wait for new messages or termination
    if (!receiveQueue.empty()) {
      ZoneScopedN("print");
      TracyPlot("RecvQueueSize", static_cast<int64_t>(receiveQueue.size()));
      std::string message = receiveQueue.front();
      receiveQueue.pop();
      lock.unlock(); // Unlock mutex while printing data
      std::cout << "[user-" << otherIp << "-" << otherPort
                << "] Received: " << message << std::endl;
    }
  }
}

int main(int argc, char *argv[]) {
  tracy::SetThreadName("Main Thread");
  ZoneScoped;

  if (argc != 4) {
    std::cerr << "Usage: " << argv[0]
              << " <localhostIP> <instancePort> <otherInstancePort>"
              << std::endl;
    return 1;
  }

  std::string local_ip = argv[1];
  unsigned short instance_port =
      static_cast<unsigned short>(std::stoi(argv[2]));
  unsigned short other_instance_port =
      static_cast<unsigned short>(std::stoi(argv[3]));

  // Initialize WinSock
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    std::cerr << "WSAStartup failed!" << std::endl;
    return 1;
  }

  // Create sockets
  SOCKET recvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  SOCKET sendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if (recvSocket == INVALID_SOCKET || sendSocket == INVALID_SOCKET) {
    std::cerr << "Socket creation failed!" << std::endl;
    WSACleanup();
    return 1;
  }

  // Bind receiving socket to local address and port
  sockaddr_in localAddr{};
  localAddr.sin_family = AF_INET;
  if (inet_pton(AF_INET, local_ip.c_str(), &localAddr.sin_addr) <= 0) {
    std::cerr << "Invalid IP address format for local IP: " << local_ip
              << std::endl;
    closesocket(recvSocket);
    closesocket(sendSocket);
    WSACleanup();
    return 1;
  }
  localAddr.sin_port = htons(instance_port);

  if (bind(recvSocket, reinterpret_cast<sockaddr *>(&localAddr),
           sizeof(localAddr)) == SOCKET_ERROR) {
    std::cerr << "Bind failed!" << std::endl;
    closesocket(recvSocket);
    closesocket(sendSocket);
    WSACleanup();
    return 1;
  }

  // Set up remote address for sending
  sockaddr_in remoteAddr{};
  remoteAddr.sin_family = AF_INET;
  if (inet_pton(AF_INET, local_ip.c_str(), &remoteAddr.sin_addr) <= 0) {
    std::cerr << "Invalid IP address format for remote IP: " << local_ip
              << std::endl;
    closesocket(recvSocket);
    closesocket(sendSocket);
    WSACleanup();
    return 1;
  }
  remoteAddr.sin_port = htons(other_instance_port);

  std::jthread input(inputThread, local_ip, instance_port);
  std::jthread sender(sendThread, std::ref(sendSocket), std::ref(remoteAddr));
  std::jthread receiver(receiveThread, std::ref(recvSocket));
  std::jthread display(displayThread, local_ip, other_instance_port);

  while (running) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(100)); // Avoid busy waiting
  }

  // Cleanup
  closesocket(recvSocket);
  closesocket(sendSocket);
  WSACleanup();

  std::cout << "Terminated gracefully. Bye!" << std::endl;

  return 0;
}
