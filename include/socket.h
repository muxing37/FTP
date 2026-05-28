#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>

enum class NetResult {
  OK,
  TIMEOUT,
  SEND_ERROR,
  RECV_ERROR,
  FILE_ERROR
};

class TcpSocket {
  public:
  TcpSocket(int sockfd);
  ~TcpSocket();

  NetResult sendMsg(std::string msg);
  NetResult recvMsg(std::string& msg);

  NetResult sendFile(std::string& path);
  NetResult recvFile(std::string& path);

  private:
  int sockfd_;
};
