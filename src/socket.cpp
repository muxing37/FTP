#include "socket.h"
#include "ftpls.h"

TcpSocket::TcpSocket(int sockfd) : sockfd_(sockfd) {}

TcpSocket::~TcpSocket() {
  close(sockfd_);
}

uint64_t htonll(uint64_t num) {
  uint32_t high = htonl((uint32_t)(num >> 32));
  uint32_t low  = htonl((uint32_t)(num & 0xFFFFFFFF));

  return ((uint64_t)low << 32) | high;
}

uint64_t ntohll(uint64_t num) {
  uint32_t high = ntohl((uint32_t)(num >> 32));
  uint32_t low  = ntohl((uint32_t)(num & 0xFFFFFFFF));

  return ((uint64_t)low << 32) | high;
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

NetResult TcpSocket::sendMsg(std::string msg) {
  uint32_t l=msg.size();
  uint32_t len=htonl(l);

  if(send_all(sockfd_,&len,sizeof(len))!=sizeof(len)) {
    return NetResult::SEND_ERROR;
  }
  
  if(send_all(sockfd_,msg.data(),msg.size())!=(int)msg.size()) {
    return NetResult::SEND_ERROR;
  }
  return NetResult::OK;
}

NetResult TcpSocket::sendFile(std::string& path) {
  int fd = open(path.c_str(), O_RDONLY);
  if(fd < 0) {
    return NetResult::FILE_ERROR;
  }
  struct stat st;
  if(fstat(fd, &st) == -1) {
    close(fd);
    return NetResult::FILE_ERROR;
  }

  uint64_t filesize = st.st_size;
  uint64_t netSize = htonll(filesize);

  if(send_all(sockfd_,&netSize,sizeof(netSize))!= sizeof(netSize)) {
    close(fd);
    return NetResult::SEND_ERROR;
  }
  char buf[4096];
  while(true) {
    int n = read(fd,buf,sizeof(buf));
    if(n>0) {
      if(send_all(sockfd_,buf,n) != n) {
        close(fd);
        return NetResult::SEND_ERROR;
      }
    } else if(n==0) {
      break;
    } else {
      close(fd);
      return NetResult::SEND_ERROR;
    }
  }

  close(fd);
  return NetResult::OK;
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

NetResult TcpSocket::recvMsg(std::string& msg) {

  msg.clear();
  uint32_t len=0;
  int ret=recv_all(sockfd_,&len,sizeof(len));
  if(ret==0) return NetResult::RECV_ERROR;
  if(ret!=sizeof(len)) return NetResult::RECV_ERROR;
  
  uint32_t l=ntohl(len);

  const uint32_t MAX_LEN=100*1024*1024;

  if(l>MAX_LEN) {
    return NetResult::RECV_ERROR;
  }

  msg.resize(l);

  ret=recv_all(sockfd_,&msg[0],l);

  if(ret!=(int)l) {
    close(sockfd_);
    sockfd_=-1;
    return NetResult::RECV_ERROR;
  }

  return NetResult::OK;
}

NetResult TcpSocket::recvFile(std::string& path) {
  int fd = open(path.c_str(),O_WRONLY | O_CREAT | O_TRUNC,0644);

  if(fd < 0) {
    return NetResult::RECV_ERROR;
  }

  uint64_t netSize = 0;
  int ret = recv_all(sockfd_,&netSize,sizeof(netSize));

  if(ret != sizeof(netSize)) {
    close(fd);
    return NetResult::RECV_ERROR;
  }

  uint64_t filesize = ntohll(netSize);
  uint64_t remain = filesize;

  char buf[4096];
  while(remain > 0) {
    int chunk = remain > sizeof(buf) ? sizeof(buf) : remain;
    int n = recv_all(sockfd_, buf, chunk);

    if(n <= 0) {
      close(fd);
      return NetResult::RECV_ERROR;
    }

    int written = write(fd, buf, n);
    if(written != n) {
      close(fd);
      return NetResult::RECV_ERROR;
    }
    remain -= n;
  }
  close(fd);
  return NetResult::OK;
}
