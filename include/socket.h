#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>

class TcpSocket {
  public:
  TcpSocket(int sockfd);
  ~TcpSocket();

  int sendMsg(std::string msg);
  int recvMsg(std::string& msg);

  private:
  int sockfd_;
};
