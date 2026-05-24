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

int TcpSocket::sendFile(std::string& path) {
  int fd = open(path.c_str(), O_RDONLY);
  if(fd < 0) {
    return -1;
  }
  struct stat st;
  if(fstat(fd, &st) == -1) {
    close(fd);
    return -1;
  }

  uint64_t filesize = st.st_size;
  uint64_t netSize = htonll(filesize);

  if(send_all(sockfd_,&netSize,sizeof(netSize))!= sizeof(netSize)) {
    close(fd);
    return -1;
  }
  char buf[4096];
  while(true) {
    int n = read(fd,buf,sizeof(buf));
    if(n>0) {
      if(send_all(sockfd_,buf,n) != n) {
        close(fd);
        return -1;
      }
    } else if(n==0) {
      break;
    } else {
      close(fd);
      return -1;
    }
  }

  close(fd);
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

int TcpSocket::recvFile(std::string& path) {
  int fd = open(path.c_str(),O_WRONLY | O_CREAT | O_TRUNC,0644);

  if(fd < 0) {
    return -1;
  }

  uint64_t netSize = 0;
  int ret = recv_all(sockfd_,&netSize,sizeof(netSize));

  if(ret != sizeof(netSize)) {
    close(fd);
    return -1;
  }

  uint64_t filesize = ntohll(netSize);
  uint64_t remain = filesize;

  char buf[4096];
  while(remain > 0) {
    int chunk = remain > sizeof(buf) ? sizeof(buf) : remain;
    int n = recv_all(sockfd_, buf, chunk);

    if(n <= 0) {
      close(fd);
      return -1;
    }

    int written = write(fd, buf, n);
    if(written != n) {
      close(fd);
      return -1;
    }
    remain -= n;
  }
  close(fd);
  return 0;
}
