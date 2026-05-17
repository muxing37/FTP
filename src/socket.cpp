#include "socket.h"

TcpSocket::TcpSocket(int sockfd) : sockfd_(sockfd) {}

TcpSocket::~TcpSocket() {
  close(sockfd_);
}

int send_all(int fd,void *buf,size_t len) {
  size_t total=0;
  char *p=(char*)buf;
  while(total<len) {
    int n=send(fd,p+total,len-total,0);
    if(n>0) {
      total=total+n;
    } else if(n==0) {
      return total;
    } else {
      if (errno == EINTR) continue;
      // if (errno == EAGAIN || errno == EWOULDBLOCK) return -1;
      return -1;
    }
  }
  return total;
}

int TcpSocket::sendMsg(std::string msg) {
  uint32_t l=msg.size();
  uint32_t len=htonl(l);

  if(send_all(sockfd_,&len,sizeof(len))!=sizeof(len)) {
    return -1;
  }
  
  if(send_all(sockfd_,msg.data(),msg.size())!=(int)msg.size()) {
    return -1;
  }

  return 0;
}

int recv_all(int fd,void *buf,size_t len) {
  size_t total=0;
  int retry=0;
  char *p=(char*)buf;
  while(total<len) {
    int n=recv(fd,p+total,len-total,0);
    if(n>0) {
      total=total+n;
    } else if(n==0) {
      return total;
    } else {
      // if(errno == EAGAIN || errno == EWOULDBLOCK) return -1;
      if(errno == EINTR) continue;
      return -1;
    }
  }
  return total;
}

int TcpSocket::recvMsg(std::string& msg) {

  msg.clear();
  uint32_t len=0;
  int ret=recv_all(sockfd_,&len,sizeof(len));
  if(ret==0) return -1;
  if(ret!=sizeof(len)) return -1;
  
  uint32_t l=ntohl(len);

  const uint32_t MAX_LEN=100*1024*1024;

  if(l>MAX_LEN) {
    return -1;
  }

  msg.resize(l);

  ret=recv_all(sockfd_,&msg[0],l);

  if(ret!=(int)l) {
    close(sockfd_);
    sockfd_=-1;
    return -1;
  }

  return 0;
}
