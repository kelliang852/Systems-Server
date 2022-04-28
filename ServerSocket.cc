/*
 * Copyright ©2022 Travis McGaha.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Pennsylvania
 * CIT 595 for use solely during Spring Semester 2022 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <cstdio>       // for snprintf()
#include <unistd.h>      // for close(), fcntl()
#include <sys/types.h>   // for socket(), getaddrinfo(), etc.
#include <sys/socket.h>  // for socket(), getaddrinfo(), etc.
#include <arpa/inet.h>   // for inet_ntop()
#include <netdb.h>       // for getaddrinfo()
#include <errno.h>       // for errno, used by strerror()
#include <cstring>      // for memset, strerror()
#include <iostream>      // for std::cerr, etc.

#include <assert.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "./ServerSocket.h"

using std::cout;
using std::endl;
using std::cerr;

namespace searchserver {

ServerSocket::ServerSocket(uint16_t port) {
  port_ = port;
  listen_sock_fd_ = -1;
}

ServerSocket::~ServerSocket() {
  // Close the listening socket if it's not zero.  The rest of this
  // class will make sure to zero out the socket if it is closed
  // elsewhere.
  if (listen_sock_fd_ != -1)
    close(listen_sock_fd_);
  listen_sock_fd_ = -1;
}

bool ServerSocket::bind_and_listen(int ai_family, int *listen_fd) {
  // Use "getaddrinfo," "socket," "bind," and "listen" to
  // create a listening socket on port port_.  Return the
  // listening socket through the output parameter "listen_fd"
  // and set the ServerSocket data member "listen_sock_fd_"

  struct addrinfo hints; 
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_STREAM; 
  hints.ai_flags = AI_PASSIVE;
  hints.ai_flags |= AI_V4MAPPED;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_canonname = nullptr; 
  hints.ai_addr = nullptr; 
  hints.ai_next = nullptr;

  // getting address info 
  struct addrinfo* result; 
  int res = getaddrinfo(nullptr, std::to_string(port_).c_str(), &hints, &result);
  if (res != 0) {
    cerr << "getaddrinfo() failed: "<<gai_strerror(res) << endl;
    return false;
  }

  // creating server socket
  for (struct addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
    *listen_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (*listen_fd == -1) {
      cerr<<"socket() failed: ";
      cerr<<strerror(errno)<<endl;
      *listen_fd = 0;
      continue;
    } 
    // optional reconfiguration 
    int optval = 1; 
    setsockopt(*listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    //binding the socket to the address
    if (bind(*listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
      listen_sock_fd_ = *listen_fd;
      sock_family_ = rp->ai_family; 
      break;
    }

    *listen_fd = -1;
  }

  freeaddrinfo(result);

  // checking if binding is succesful
  if (*listen_fd == -1) {
    cerr<<"no binding"<<endl;
    return false;
  }

  // listening socket 
  if (listen(*listen_fd, SOMAXCONN)!=0) {
    cerr<<"cannot be marked as listneing socket: "<<endl;
    cerr<<strerror(errno)<<endl;
    return false;
  }

  // setting global variable to *listen_fd
  listen_sock_fd_ = *listen_fd;

  // cout<<"*listen_fd = "<<*listen_fd<<endl;
  // cout<<"** listen_sock_fd_ = "<<listen_sock_fd_<<"\n"<<endl;
  
  // sleep(20);
  // sleep(1);
  
  return true;
}

bool ServerSocket::accept_client(int *accepted_fd,
                          std::string *client_addr,
                          uint16_t *client_port,
                          std::string *client_dns_name,
                          std::string *server_addr,
                          std::string *server_dns_name) const {
  // Accept a new connection on the listening socket listen_sock_fd_.
  // (Block until a new connection arrives.)  Return the newly accepted
  // socket, as well as information about both ends of the new connection,
  // through the various output parameters.

  struct sockaddr_storage caddr;
  socklen_t caddr_len = sizeof(caddr);
  int client_fd = accept(listen_sock_fd_,
                        reinterpret_cast<struct sockaddr *>(&caddr),
                        &caddr_len);
  if (client_fd < 0) {
    if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK))
      // continue;
      std::cerr << "Failure on accept: " << strerror(errno) << std::endl;
      // break;
  }

  
  *accepted_fd = client_fd;
  char hostname[1024];  // ought to be big enough.
  if (getnameinfo(reinterpret_cast<struct sockaddr *>(&caddr), 
    caddr_len, hostname, 1024, nullptr, 0, 0) != 0) {
    sprintf(hostname, "[reverse DNS failed]");
  }
  // std::cout << " DNS name: " << hostname << std::endl;
  *client_dns_name = hostname;

  if (reinterpret_cast<struct sockaddr *>(&caddr)->sa_family == AF_INET) {
    // Print out the IPV4 address and port
    char astring[INET_ADDRSTRLEN];
    struct sockaddr_in *in4 = reinterpret_cast<struct sockaddr_in *>(reinterpret_cast<struct sockaddr *>(&caddr));
    inet_ntop(AF_INET, &(in4->sin_addr), astring, INET_ADDRSTRLEN);
    // std::cout << " IPv4 address " << astring;
    // std::cout << " and port " << ntohs(in4->sin_port) << std::endl;
    *client_addr = astring; 
    *client_port = ntohs(in4->sin_port);
  } else if (reinterpret_cast<struct sockaddr *>(&caddr)->sa_family == AF_INET6) {
    // Print out the IPV6 address and port

    char astring[INET6_ADDRSTRLEN];
    struct sockaddr_in6 *in6 = reinterpret_cast<struct sockaddr_in6 *>(reinterpret_cast<struct sockaddr *>(&caddr));
    inet_ntop(AF_INET6, &(in6->sin6_addr), astring, INET6_ADDRSTRLEN);
    // std::cout << " IPv6 address " << astring;
    // std::cout << " and port " << ntohs(in6->sin6_port) << std::endl;
    *client_addr = astring; 
    *client_port = ntohs(in6->sin6_port);
  } else {
    std::cout << " ???? address and port ????" << std::endl;
  }

  char hname[1024];
  hname[0] = '\0';
  // std::cout << "Server side interface is ";
  if (sock_family_ == AF_INET) {
    // The server is using an IPv4 address.
    struct sockaddr_in srvr;
    socklen_t srvrlen = sizeof(srvr);
    char addrbuf[INET_ADDRSTRLEN];
    getsockname(client_fd, (struct sockaddr *) &srvr, &srvrlen);
    inet_ntop(AF_INET, &srvr.sin_addr, addrbuf, INET_ADDRSTRLEN);
    // std::cout << "yello!!! -->" <<addrbuf;
    *server_addr = addrbuf;
    // Get the server's dns name, or return it's IP address as
    // a substitute if the dns lookup fails.
    getnameinfo((const struct sockaddr *) &srvr,
                srvrlen, hname, 1024, nullptr, 0, 0);
    // std::cout << " [" << hname << "]" << std::endl;
    *server_dns_name = hname;
  } else {
    // The server is using an IPv6 address.
    struct sockaddr_in6 srvr;
    socklen_t srvrlen = sizeof(srvr);
    char addrbuf[INET6_ADDRSTRLEN];
    getsockname(client_fd, (struct sockaddr *) &srvr, &srvrlen);
    inet_ntop(AF_INET6, &srvr.sin6_addr, addrbuf, INET6_ADDRSTRLEN);
    // std::cout << "yello!!! -->" <<addrbuf;
    *server_addr = addrbuf;
    // Get the server's dns name, or return it's IP address as
    // a substitute if the dns lookup fails.
    getnameinfo((const struct sockaddr *) &srvr,
                srvrlen, hname, 1024, nullptr, 0, 0);
    // std::cout << " [" << hname << "]" << std::endl;
    *server_dns_name = hname;
  }

  


    // struct sockaddr_in *in4 = reinterpret_cast<struct sockaddr_in *>(addr);
    // struct sockaddr_in6 *in6 = reinterpret_cast<struct sockaddr_in6 *>(addr);

    // if (sock_family_ == AF_INET) { // IPv4
    //   struct sockaddr_in *in4 = reinterpret_cast<struct sockaddr_in *>(addr);
    //   *client_addr = inet_ntop(AF_INET, &(in4->sin_addr), astring, INET_ADDRSTRLEN);
    // } else { // IPv6
    //   struct sockaddr_in6 *in6 = reinterpret_cast<struct sockaddr_in6 *>(addr);
    //   *client_addr = inet_ntop(AF_INET6, &(in6->sin6_addr), astring, INET6_ADDRSTRLEN);
    // }
    
    // *client_port = ntohs(in6->sin6_port);
    // *client_dns_name = getnameinfo(addr, addrlen, hostname, 1024, nullptr, 0, 0);
    // *server_addr = inet_ntop(AF_INET, &srvr.sin_addr, addrbuf, INET_ADDRSTRLEN);
    // *server_dns_name = getnameinfo((const struct sockaddr *) &srvr,
    //             srvrlen, hname, 1024, nullptr, 0, 0);


  return true;
}

}  // namespace searchserver
