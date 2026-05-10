#pragma once
#include "socket.h"
#include <memory>

class TcpServer {
  public:
  TcpServer();
  ~TcpServer();

  TcpSocket* getSocket() const { return socket_.get(); }

  bool setListen(unsigned short port);

  bool acceptConn();

  private:
  int listenfd_;
  std::unique_ptr<TcpSocket> socket_;
};
