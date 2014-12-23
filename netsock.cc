#include "netsock.hh"

#if !__WIN32__
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#endif

using namespace Net;

#if __WIN32__
#define last_error WSAGetLastError()
#else
#define last_error errno
#endif

static const char* error_string(int err = last_error) {
#if __WIN32__
  static thread_local char buf[256];
#ifdef _UNICODE
  WCHAR wide_buf[sizeof(buf)];
  FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, 0, err, 0, wide_buf,
                 elementcount(wide_buf), 0);
  WideCharToMultiByte(CP_UTF8, 0, wide_buf, -1, buf, sizeof(buf), NULL, NULL);
#else
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, err, 0, buf, sizeof(buf), 0);
#endif
  return buf;
#else
  return strerror(err);
#endif
}

static bool have_inited_sockets = false;
#ifdef __WIN32__
static WSADATA wsaData;
#endif

static void init_sockets() {
  have_inited_sockets = true;
#ifdef __WIN32__
  int fail;
  if((fail = WSAStartup(MAKEWORD(1,1),&wsaData)))
    die("WSAStartup failed with error code %i", fail);
  dprintf("WinSock version %i.%i in use (%i.%i max)\nDescription: %s\nStatus: %s\n", wsaData.wVersion & 255, wsaData.wVersion >> 8, wsaData.wHighVersion & 255, wsaData.wHighVersion >> 8, wsaData.szDescription, wsaData.szSystemStatus);
#else
  signal(SIGPIPE, SIG_IGN);
#endif
}

Sock::Sock() : sock(INVALID_SOCKET) {}
Sock::~Sock() { if(Valid()) Close(); }

bool Sock::Init(std::string& error_out, int domain, int type, bool blocking) {
  if(!have_inited_sockets) init_sockets();
  int sock = socket(domain, type, 0);
  if(sock == INVALID_SOCKET) {
    error_out = std::string("Could not create socket: ") + error_string();
    return false;
  }
  Become(sock, blocking);
  return true;
}

void Sock::Become(SOCKET sock, bool blocking) {
  if(!have_inited_sockets) init_sockets();
#if !__WIN32__
  if(sock >= FD_SETSIZE)
    die("Too many sockets! (Local FD_SETSIZE=%i)", FD_SETSIZE);
  /* WinSock fd_set is an array-list, not a bitset. Don't bother checking; TEG
     games aren't going to be developed on Windows first, after all. */
#endif
  if(Valid()) Close();
  this->sock = sock;
  SetBlocking(blocking);
}

void Sock::SetBlocking(bool blocking) {
#if __WIN32__
  u_long nonblocking = !blocking;
  if(ioctlsocket(sock, FIONBIO, &nonblocking) < 0)
#else
  if(fcntl(sock, F_SETFL, blocking ? 0 : O_NONBLOCK) < 0)
#endif
  {
    fprintf(stderr, "WARNING: Could not set blocking status of socket! (Reason given for failure: %s)\n", error_string());
  }
}

bool Sock::HasError(std::string& error_out) {
  if(!Valid()) {
    error_out = "Socket is not valid";
    return true;
  }
  int err = 0;
  unsigned int len = sizeof(err);
  getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &len);
  if(err) {
    error_out = error_string(err);
    return true;
  }
  else return false;
}

void Sock::Close() {
  if(!Valid()) return;
#if __WIN32__
  closesocket(sock);
#else
  close(sock);
#endif
  sock = INVALID_SOCKET;
}

bool Sock::GetPeerName(Address& out) {
  out.faceless.sa_family = AF_UNSPEC;
  if(!Valid()) return false;
  unsigned int len = sizeof(out);
  int err = getpeername(sock, &out.faceless, &len);
  return err == 0;
}

Address& Address::operator=(const struct sockaddr* src) {
  switch(src->sa_family) {
  case AF_INET:
    memcpy(&in, (const struct sockaddr_in*)src, sizeof(struct sockaddr_in));
    break;
  case AF_INET6:
    memcpy(&in6, (const struct sockaddr_in6*)src, sizeof(struct sockaddr_in6));
    break;
  default:
    faceless.sa_family = AF_UNSPEC;
  }
  return *this;
}

size_t Address::Length() const {
  switch(faceless.sa_family) {
  case AF_INET: return sizeof(in);
  case AF_INET6: return sizeof(in6);
  default: return sizeof(storage);
  }
}

bool Address::operator==(const Address& other) const {
  if(other.faceless.sa_family != faceless.sa_family) return false;
  switch(faceless.sa_family) {
  case AF_INET:
    return other.in.sin_addr.s_addr == in.sin_addr.s_addr
      && other.in.sin_port == in.sin_port;
  case AF_INET6:
    return !memcmp(other.in6.sin6_addr.s6_addr,
                   in6.sin6_addr.s6_addr, sizeof(in6.sin6_addr.s6_addr))
      && other.in6.sin6_port == in6.sin6_port;
  default:
    die("Internal error! Uninitialized/invalid Net::Address!");
  }
  return true;
}

bool Address::operator<(const Address& other) const {
#define PIVOT_COMPARE(wat) \
  if(wat < other.wat) return true; \
  else if(wat > other.wat) return false
  PIVOT_COMPARE(faceless.sa_family);
  switch(faceless.sa_family) {
  case AF_INET:
    PIVOT_COMPARE(in.sin_addr.s_addr);
    PIVOT_COMPARE(in.sin_port);
    return false;
  case AF_INET6:
    {
      int c = memcmp(in6.sin6_addr.s6_addr, other.in6.sin6_addr.s6_addr,
                     sizeof(in6.sin6_addr.s6_addr));
      if(c < 0) return true;
      else if(c > 0) return false;
    }
    PIVOT_COMPARE(in6.sin6_port);
    return false;
  default:
    die("Internal error! Uninitialized/invalid Net::Address!");
  }
  return false;
#undef PIVOT_COMPARE
}

std::string Address::ToString() const {
  switch(faceless.sa_family) {
  case AF_INET:
    {
      char ip[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &in.sin_addr, ip, sizeof(ip));
      return std::string(ip);
    }
  case AF_INET6:
    {
      char ip[INET6_ADDRSTRLEN];
      inet_ntop(AF_INET6, &in6.sin6_addr, ip, sizeof(ip));
      return std::string(ip);
    }
  default:
    return "(Uninitialized/invalid Net::Address)";
  }
}

std::string Address::ToLongString() const {
  switch(faceless.sa_family) {
  case AF_INET:
    {
      char ip[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &in.sin_addr, ip, sizeof(ip));
      return TEG::format("%s:%u", ip, ntohs(in.sin_port));
    }
  case AF_INET6:
    {
      char ip[INET6_ADDRSTRLEN];
      inet_ntop(AF_INET6, &in6.sin6_addr, ip, sizeof(ip));
      return TEG::format("[%s]:%u", ip, ntohs(in6.sin6_port));
    }
  default:
    return "(Uninitialized/invalid Net::Address)";
  }
}

IOResult SockStream::Connect(std::string& error_out, Address& target_address,
                             bool initially_blocking) {
  if(!Init(error_out, target_address.faceless.sa_family, SOCK_STREAM, initially_blocking))
    return IOResult::ERROR;
  if(connect(sock, &target_address.faceless, target_address.Length())) {
    auto err = last_error;
    Close();
    switch(err) {
    case EINPROGRESS: return IOResult::WOULD_BLOCK;
    default:
      error_out = std::string("Could not connect to ")
        + target_address.ToLongString() + ": " + error_string(err);
      return err == ECONNREFUSED ? IOResult::CONNECTION_CLOSED : IOResult::ERROR;
    }
  }
  return IOResult::OKAY;
}

IOResult SockStream::Receive(std::string& error_out,
                             void* buf, size_t& len_inout) {
  if(!Valid()) {
    error_out = "Socket not valid";
    return IOResult::ERROR;
  }
 intr_retry:
  ssize_t result = recv(sock, buf, len_inout, 0);
  if(result < 0) {
    switch(last_error) {
    case EINTR: goto intr_retry;
    case EAGAIN:
#if EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:
#endif
      return IOResult::WOULD_BLOCK;
    default:
      auto err = last_error;
      error_out = std::string("Could not receive: ") + error_string(err);
      return err == ECONNREFUSED ? IOResult::CONNECTION_CLOSED : IOResult::ERROR;
    }
  }
  else if(result == 0) {
    error_out = std::string("Could not receive: Connection closed");
    return IOResult::CONNECTION_CLOSED;
  }
  else {
    len_inout = result;
    return IOResult::OKAY;
  }
}

IOResult SockStream::Send(std::string& error_out,
                         const void* buf, size_t& len_inout) {
  if(!Valid()) {
    error_out = "Socket not valid";
    return IOResult::ERROR;
  }
 intr_retry:
  ssize_t result = send(sock, buf, len_inout, 0);
  if(result < 0) {
    switch(last_error) {
    case EINTR: goto intr_retry;
    case EAGAIN:
#if EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:
#endif
      return IOResult::WOULD_BLOCK;
    default:
      auto err = last_error;
      error_out = std::string("Could not send: ") + error_string(err);
      return (err == ECONNREFUSED || err == EPIPE) ? IOResult::CONNECTION_CLOSED : IOResult::ERROR;
    }
  }
  else {
    len_inout = result;
    return IOResult::OKAY;
  }
}

void SockStream::ShutdownSend() {
  if(!Valid()) return;
  shutdown(sock, SHUT_WR);
}
 
void SockStream::ShutdownReceive() {
  if(!Valid()) return;
  shutdown(sock, SHUT_RD);
}
 
void SockStream::ShutdownBoth() {
  if(!Valid()) return;
  shutdown(sock, SHUT_RDWR);
}
 
IOResult SockDgram::Connect(std::string& error_out, Address& target_address) {
  if(!Init(error_out, target_address.faceless.sa_family, SOCK_DGRAM))
    return IOResult::ERROR;
  if(connect(sock, &target_address.faceless, target_address.Length())) {
    switch(last_error) {
    case EINPROGRESS: return IOResult::WOULD_BLOCK;
    default:
      auto err = last_error;
      error_out = std::string("Could not connect to ")
        + target_address.ToLongString() + ": " + error_string(err);
      return err == ECONNREFUSED ? IOResult::CONNECTION_CLOSED : IOResult::ERROR;
    }
  }
  return IOResult::OKAY;
}

IOResult SockDgram::Receive(std::string& error_out,
                            void* buf, size_t& len_inout) {
  if(!Valid()) {
    error_out = "Socket not valid";
    return IOResult::ERROR;
  }
 intr_retry:
  ssize_t result = recv(sock, buf, len_inout, 0);
  if(result < 0) {
    switch(last_error) {
    case EINTR: goto intr_retry;
    case EAGAIN:
#if EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:
#endif
      return IOResult::WOULD_BLOCK;
    default:
      auto err = last_error;
      error_out = std::string("Could not receive: ") + error_string(err);
      return err == ECONNREFUSED ? IOResult::CONNECTION_CLOSED : IOResult::ERROR;
    }
  }
  else {
    len_inout = result;
    return IOResult::OKAY;
  }
}

IOResult SockDgram::Send(std::string& error_out,
                         const void* buf, size_t len) {
  if(!Valid()) {
    error_out = "Socket not valid";
    return IOResult::ERROR;
  }
 intr_retry:
  ssize_t result = send(sock, buf, len, 0);
  if(result < 0) {
    switch(last_error) {
    case EINTR: goto intr_retry;
    case EAGAIN:
#if EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:
#endif
      return IOResult::WOULD_BLOCK;
    case EMSGSIZE:
      return IOResult::MSGSIZE;
    default:
      auto err = last_error;
      error_out = std::string("Could not send: ") + error_string(err);
      return (err == ECONNREFUSED || err == EPIPE) ? IOResult::CONNECTION_CLOSED : IOResult::ERROR;
    }
  }
  else if((size_t)result != len) {
    error_out = std::string("Could not send: Message size too long (and it was"
                            " truncated illegally at the OS level)");
    return IOResult::ERROR;
  }
  else
    return IOResult::OKAY;
}
 
bool ServerSock::SubBind(std::string& error_out, const char* bind_address,
                         uint16_t port, IPVersion v, int type) {
  if(!Init(error_out, (int)v, type))
    return false;
  Address addr;
  addr.faceless.sa_family = (int)v;
  int e;
  if(bind_address) {
    switch(v) {
    case IPVersion::V4: e = inet_pton((int)v, bind_address, &addr.in.sin_addr); break;
    case IPVersion::V6: e = inet_pton((int)v, bind_address, &addr.in6.sin6_addr); break;
    }
    if(e == 0) {
      error_out = "Invalid address";
      Close();
      return false;
    }
    else if(e == -1) {
      error_out = "Unsupported IP version";
      Close();
      return false;
    }
  }
  else {
    switch(v) {
    case IPVersion::V4: addr.in.sin_addr.s_addr = INADDR_ANY; break;
    case IPVersion::V6: addr.in6.sin6_addr = in6addr_any; break;
    }
  }
  switch(v) {
  case IPVersion::V4: addr.in.sin_port = htons(port); break;
  case IPVersion::V6: addr.in6.sin6_port = htons(port); break;
  }
  if(bind(sock, &addr.faceless, addr.Length())) {
    error_out = std::string("Unable to bind: ") + error_string();
    Close();
    return false;
  }
  return true;
}

bool ServerSockStream::Bind(std::string& error_out, const char* bind_address,
                            uint16_t port, IPVersion v, int backlog) {
  if(!SubBind(error_out, bind_address, port, v, SOCK_STREAM)) return false;
  if(listen(sock, backlog)) {
    error_out = std::string("Unable to listen: ") + error_string();
    Close();
    return false;
  }
  return true;
}

bool ServerSockStream::Accept(SockStream& sock_out, Address& address_out) {
  unsigned address_len = sizeof(address_out);
  SOCKET sock = accept(this->sock, &address_out.faceless, &address_len);
  if(sock != INVALID_SOCKET) {
    sock_out.Become(sock);
    return true;
  }
  else return false;
}

bool ServerSockDgram::Bind(std::string& error_out, const char* bind_address,
                           uint16_t port, IPVersion v) {
  if(!SubBind(error_out, bind_address, port, v, SOCK_DGRAM)) return false;
  return true;
}

IOResult ServerSockDgram::Receive(std::string& error_out,
                                  void* buf, size_t& len_inout,
                                  Address& address_out) {
  if(!Valid()) {
    error_out = "Socket not valid";
    return IOResult::ERROR;
  }
  unsigned addrlen = sizeof(address_out.storage);
 intr_retry:
  ssize_t result = recvfrom(sock, buf, len_inout, 0,
                            &address_out.faceless, &addrlen);
  if(result < 0) {
    switch(last_error) {
    case EINTR: goto intr_retry;
    case EAGAIN:
#if EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:
#endif
      return IOResult::WOULD_BLOCK;
    default:
      error_out = std::string("Could not receive: ") + error_string();
      return IOResult::ERROR;
    }
  }
  else {
    len_inout = result;
    return IOResult::OKAY;
  }
}

IOResult ServerSockDgram::Send(std::string& error_out,
                               const void* buf, size_t len,
                               const Address& address) {
  if(!Valid()) {
    error_out = "Socket not valid";
    return IOResult::ERROR;
  }
 intr_retry:
  ssize_t result = sendto(sock, buf, len, 0,
                          &address.faceless, address.Length());
  if(result < 0) {
    switch(last_error) {
    case EINTR: goto intr_retry;
    case EAGAIN:
#if EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:
#endif
      return IOResult::WOULD_BLOCK;
    case EMSGSIZE:
      return IOResult::MSGSIZE;
    default:
      auto err = last_error;
      error_out = std::string("Could not send to ") + address.ToLongString()
        + ": " + error_string(err);
      return (err == ECONNREFUSED || err == EPIPE) ? IOResult::CONNECTION_CLOSED : IOResult::ERROR;
    }
  }
  else if((size_t)result != len) {
    error_out = std::string("Could not send to ") + address.ToLongString()
      + ": Message size too long (and it was truncated illegally at the OS"
      " level)";
    return IOResult::ERROR;
  }
  else
    return IOResult::OKAY;
}
 
Select::Select(const std::forward_list<ServerSockStream*>* read_ss,
               const std::forward_list<ServerSockDgram*>* read_sd,
               const std::forward_list<ServerSockDgram*>* write_sd,
               const std::forward_list<SockStream*>* read_s,
               const std::forward_list<SockStream*>* write_s,
               const std::forward_list<SockDgram*>* read_d,
               const std::forward_list<SockDgram*>* write_d,
               size_t max_timeout_us) {
  fd_set readfds, writefds;
  SOCKET nfds = 0;
  FD_ZERO(&readfds);
  FD_ZERO(&writefds);
#define SOCK_INTO_SET(socklist, set) \
  if(socklist) for(auto sock : *socklist) { FD_SET(sock->sock, &set); if(sock->sock + 1 > nfds) nfds = sock->sock + 1; }
  SOCK_INTO_SET(read_ss, readfds);
  SOCK_INTO_SET(read_sd, readfds);
  SOCK_INTO_SET(write_sd, writefds);
  SOCK_INTO_SET(read_s, readfds);
  SOCK_INTO_SET(write_s, writefds);
  SOCK_INTO_SET(read_d, readfds);
  SOCK_INTO_SET(write_d, writefds);
#undef SOCK_INTO_SET
  struct timeval timeout;
  timeout.tv_sec = max_timeout_us / 1000000;
  timeout.tv_usec = max_timeout_us % 1000000;
 intr_retry:
  int nset = select(nfds, &readfds, &writefds, NULL, &timeout);
  if(nset < 0) {
    switch(last_error) {
    case EINTR:
      timeout.tv_sec = 0;
      timeout.tv_usec = 0;
      goto intr_retry;
    default:
      die("select() error: %s", error_string());
    }
  }
#define SOCK_INTO_LIST(srclist, dstlist, set) \
  if(srclist) { \
    for(auto sock : *srclist) { \
      if(FD_ISSET(sock->sock, set)) { \
        dstlist.emplace_front(sock); \
        if(--nset <= 0) return; \
      } \
    } \
  }
  SOCK_INTO_LIST(read_ss, readable_ss, &readfds);
  SOCK_INTO_LIST(read_sd, readable_sd, &readfds);
  SOCK_INTO_LIST(write_sd, writable_sd, &writefds);
  SOCK_INTO_LIST(read_s, readable_s, &readfds);
  SOCK_INTO_LIST(write_s, writable_s, &writefds);
  SOCK_INTO_LIST(read_d, readable_d, &readfds);
  SOCK_INTO_LIST(write_d, writable_d, &writefds);
#undef SOCK_INTO_LIST
}

bool Net::ResolveHost(std::string& error_out, std::forward_list<Address>& ret,
                      const char* host, uint16_t port, bool v4only) {
  struct addrinfo hints;
  struct addrinfo* head;
  hints.ai_family = v4only ? AF_INET : AF_UNSPEC;
  hints.ai_socktype = 0;
  hints.ai_protocol = 0;
  hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;
  int status = getaddrinfo(host, NULL, &hints, &head);
  if(status != 0) {
    error_out = TEG::format("Could not resolve %s: %s",
                            host, gai_strerror(status));
    return false;
  }
  else if(head == NULL) {
    error_out = TEG::format("Could not resolve %s: NULL response from getaddrinfo", host);
    return false;
  }
  auto it = ret.before_begin();
  for(struct addrinfo* p = head; p; p = p->ai_next) {
    switch(p->ai_family) {
    case AF_INET6:
      if(v4only) continue;
      ret.emplace_after(it, p->ai_addr);
      (++it)->in6.sin6_port = htons(port);
      break;
    case AF_INET:
      ret.emplace_after(it, p->ai_addr);
      (++it)->in.sin_port = htons(port);
      break;
    default:
      continue;
    }
  }
  freeaddrinfo(head);
  return true;
}
