#ifndef NETSOCKHH
#define NETSOCKHH

#include "teg.hh"
#include <forward_list>

#if __WIN32__
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#endif

/*
  Some socket classes. Yay!
  Sockets are always non-blocking.
  Socket member functions that return "bool" return true for success and false
  for failure. Some have an error_out parameter into which they deposit an
  error message on failure.
  Bool-returning socket setup functions (i.e. everything but Send/Receive) also
  implicitly Close the socket on failure, rendering it an uninitialized socket
  once more. Socks are never left "half-initialized".
  Sock::Hash returns a well-distributed hash value that is constant as long as
  the Sock remains valid. Closing / reinitializing a Sock changes its Hash. The
  onus is on you to ensure that everything you use this hash value for forgets
  a Sock's hash when the Sock gets closed.
 */

namespace Net {
#if !__WIN32__
  typedef int SOCKET;
#define INVALID_SOCKET -1
#endif
  enum class IPVersion : int { V4 = PF_INET, V6 = PF_INET6 };
  union Address {
  private:
    struct sockaddr faceless;
    typeof(faceless.sa_family) family;
    struct sockaddr_in in;
    struct sockaddr_in6 in6;
    struct sockaddr_storage storage;
    friend class Sock;
    friend class SockStream;
    friend class SockDgram;
    friend class ServerSock;
    friend class ServerSockStream;
    friend class ServerSockDgram;
    friend class Select;
    friend bool Net::ResolveHost(std::string&, std::forward_list<Address>&,
                                 const char*, uint16_t, bool);
    size_t Length() const;
  public:
    inline Address() : family(AF_UNSPEC) {}
    Address(const struct sockaddr* src);
    Address& operator=(const struct sockaddr* src);
    bool operator==(const Address& other) const;
    bool operator<(const Address& other) const;
    std::string ToString() const;
    std::string ToLongString() const;
    inline size_t Hash() const {
      switch(family) {
      case AF_INET6:
        {
          uint32_t* punned_addr_p = (uint32_t*)in6.sin6_addr.s6_addr;
          return (size_t)punned_addr_p[0] * 0x36CCC1D3U ^
            (size_t)punned_addr_p[1] * 0xABD43619U ^
            (size_t)punned_addr_p[2] * 0x482A7F83U ^
            (size_t)punned_addr_p[3] * 0xBF8A54BBU ^
            (size_t)in6.sin6_port * 0x6D20E3F1U;
        }
      default:
      case AF_INET:
        return (size_t)in.sin_addr.s_addr * 0x506C1BBBU ^
          (size_t)in.sin_port * 0x2D535737U;
      }
    }
  };
  enum class IOResult {
    /*
      WOULD_BLOCK: IO would block. When returned from Connect(), you should
        wait until this socket shows up as "writable" from select(), then check
        HasError() to see whether the connection succeeded, and if it failed,
        retrieve an explanatory error message.
      OKAY: IO succeeded. (A zero return here means an empty, valid IO actually
        occurred.)
      CONNECTION_CLOSED: The connection was closed at the far end. (Or outright
        refused, in some cases.)
      ERROR: A bad error occurred and the socket should be killed.
    */
    WOULD_BLOCK, OKAY, CONNECTION_CLOSED, ERROR
  };
  class Sock {
  protected:
    friend class ServerSockStream;
    friend class Select;
    SOCKET sock;
    Sock();
    ~Sock();
    Sock(const Sock&) = delete;
    bool Init(std::string& error_out, int domain, int type);
    void Become(SOCKET sock);
  public:
    inline Sock(Sock&& other) {
      if(&other == this) return;
      sock = other.sock;
      other.sock = INVALID_SOCKET;
    }
    inline bool Valid() { return sock != INVALID_SOCKET; }
    inline bool operator==(const Sock& other) const { return &other == this; }
    inline bool operator<(const Sock& other) const { return &other < this; }
    /* you should pretty much only use this for dealing with Connect */
    bool HasError(std::string& error_out);
    /* safe to call this on an invalid socket */
    void Close();
    inline size_t Hash() const {
      assert(Valid());
      return (size_t)sock * 0x97E6461BU;
    }
  };
  class SockStream : public Sock {
  public:
    IOResult Connect(std::string& error_out, Address& target_address);
    IOResult Receive(std::string& error_out,
                     void* buf, size_t& len_inout);
    IOResult Send(std::string& error_out,
                  const void* buf, size_t& len_inout);
  };
  class SockDgram : public Sock {
  public:
    IOResult Connect(std::string& error_out, Address& target_address);
    IOResult Receive(std::string& error_out,
                     void* buf, size_t& len_inout);
    IOResult Send(std::string& error_out,
                  const void* buf, size_t len);
  };
  class ServerSock : public Sock {
  protected:
    bool SubBind(std::string& error_out, const char* bind_address,
                 uint16_t port, IPVersion v, int type);
  };
  class ServerSockStream : public ServerSock {
  public:
    bool Bind(std::string& error_out, const char* bind_address,
              uint16_t port, IPVersion v, int backlog = 5);
    bool Accept(SockStream& sock_out, Address& address_out);
  };
  class ServerSockDgram : public ServerSock {
  public:
    bool Bind(std::string& error_out, const char* bind_address,
              uint16_t port, IPVersion v);
    IOResult Receive(std::string& error_out,
                     void* buf, size_t& len_inout,
                     Address& address_out);
    IOResult Send(std::string& error_out,
                  const void* buf, size_t len,
                  const Address& address);
  };
  class Select {
    std::forward_list<ServerSockStream*> readable_ss;
    std::forward_list<ServerSockDgram*> readable_sd;
    std::forward_list<ServerSockDgram*> writable_sd;
    std::forward_list<SockStream*> readable_s;
    std::forward_list<SockStream*> writable_s;
    std::forward_list<SockDgram*> readable_d;
    std::forward_list<SockDgram*> writable_d;
    Select(const Select&) = delete;
    Select(Select&&) = delete;
  public:
    Select(const std::forward_list<ServerSockStream*>* read_ss,
           const std::forward_list<ServerSockDgram*>* read_sd,
           const std::forward_list<ServerSockDgram*>* write_sd,
           const std::forward_list<SockStream*>* read_s,
           const std::forward_list<SockStream*>* write_s,
           const std::forward_list<SockDgram*>* read_d,
           const std::forward_list<SockDgram*>* write_d,
           size_t max_timeout_us = 0);
    inline const std::forward_list<ServerSockStream*>&
    GetReadableServerSockStreams() { return readable_ss; }
    inline const std::forward_list<ServerSockDgram*>&
    GetReadableServerSockDgrams() { return readable_sd; }
    inline const std::forward_list<ServerSockDgram*>&
    GetWritableServerSockDgrams() { return writable_sd; }
    inline const std::forward_list<SockStream*>&
    GetReadableSockStreams() { return readable_s; }
    inline const std::forward_list<SockStream*>&
    GetWritableSockStreams() { return writable_s; }
    inline const std::forward_list<SockDgram*>&
    GetReadableSockDgrams() { return readable_d; }
    inline const std::forward_list<SockDgram*>&
    GetWritableSockDgrams() { return writable_d; }
  };
  /* ret is *not* implicitly cleared!
     this blocks; if you want asynchronous resolution, use a thread */
  bool ResolveHost(std::string& error_out, std::forward_list<Address>& ret,
                   const char* host, uint16_t port, bool v4only = false);
}

namespace std {
  template<> struct hash<Net::Address> {
    size_t operator()(const Net::Address& wat) const { return wat.Hash(); }
  };
  /* Warning: Socks are mutable! */
  template<> struct hash<Net::Sock> {
    size_t operator()(const Net::Sock& wat) const { return wat.Hash(); }
  };
};

#endif
