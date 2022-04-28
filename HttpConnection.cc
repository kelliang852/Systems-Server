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

#include <cstdint>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include<regex>

#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpConnection.h"
#include "HttpUtils.h"

using std::map;
using std::string;
using std::vector;
using std::regex;
using std::cout; 
using std::endl;

namespace searchserver {

static const char *kHeaderEnd = "\r\n\r\n";
static const int kHeaderEndLen = 4;

bool HttpConnection::next_request(HttpRequest *request) {
  // Use "wrapped_read" to read data into the buffer_
  // instance variable.  Keep reading data until either the
  // connection drops or you see a "\r\n\r\n" that demarcates
  // the end of the request header.
  //
  // Once you've seen the request header, use parse_request()
  // to parse the header into the *request argument.
  //
  // Very tricky part:  clients can send back-to-back requests
  // on the same socket.  So, you need to preserve everything
  // after the "\r\n\r\n" in buffer_ for the next time the
  // caller invokes next_request()!

  // TODO: implement

  // std::cout<<"old buffer is: " << buffer_ <<std::endl;
  int read = 2;
  // read next request from file descripter fd_  
  while (buffer_.string::find(kHeaderEnd) == string::npos && read != -1 && read != 0){
      string temp_buffer;
    // std::cout<<"fd: " << fd_ <<std::endl;
    read = wrapped_read(fd_, &temp_buffer);
    // std::cout<<"\nHttpConnection::next_request:: read count= " <<read<<"\n" <<std::endl;
    buffer_ += temp_buffer;
  }
  
  // if line.size() > 1 -> store line[size-1] into buffer_
  string req;
  int start = 0;
  size_t end = buffer_.find(kHeaderEnd, start);;
  if (end != string::npos){
    req = buffer_.string::substr(start, end);
    buffer_ = buffer_.string::substr(end + kHeaderEndLen);
  } else {
    req = buffer_.string::substr(start, end);
    buffer_ = "";
  }
  // parse request and store the state in the output parameter "request"
  bool ret = parse_request(req, request);
  // std::cout<<"buffer is: " << buffer_ <<std::endl;

  return ret;  
}

bool HttpConnection::write_response(const HttpResponse &response) {
  // Implement so that the response is converted to a string
  // and written out to the socket for this connection
  
  // TODO: implement
  string write_out = response.GenerateResponseString();
  // std::cout<< "HttpConnection::write_response check= :"<<write_out<< std::endl;
  size_t written = wrapped_write(fd_, write_out);

  // if successfully written -> true
  if (write_out.length() == written){
    cout<<"HttpConnection::write_response= succesul write\n"<<endl;
    return true;
  } else {
    cout<<"HttpConnection::write_response= write failure\n"<<endl;
    return false;
  }
}

bool HttpConnection::parse_request(const string &request, HttpRequest* out) {
  HttpRequest req("/");  // by default, get "/".

  // Split the request into lines.  Extract the URI from the first line
  // and store it in req.URI.  For each additional line beyond the
  // first, extract out the header name and value and store them in
  // req.headers_ (i.e., HttpRequest::AddHeader).  You should look
  // at HttpRequest.h for details about the HTTP header format that
  // you need to parse.
  //
  // You'll probably want to look up boost functions for (a) splitting
  // a string into lines on a "\r\n" delimiter, (b) trimming
  // whitespace from the end of a string, and (c) converting a string
  // to lowercase.
  //
  // If a request is malfrormed, return false, otherwise true and 
  // the parsed request is retrned via *out
  
  // TODO: implement
  vector<string> lines;
  int start = 0;
  int end = 0;
  while ((size_t)(end) != string::npos){
    end = request.find("\r\n", start);
    lines.push_back(request.string::substr(start, end - start));
    start = end + 2;
  }
  for (size_t i = 0; i < lines.size(); i++){
    if (i == 0){ // get uri- split by space
      vector<string> m;
      boost::split(m, lines[i], boost::is_any_of(" "), boost::token_compress_on);
      if (m.size() != 3 || m[0].compare("GET") != 0){
        return false;
      }
      boost::algorithm::trim(m[1]);
      // boost::algorithm::to_lower(m[1]);
      out->set_uri(m[1]);
    } else { // split by ":"
      vector<string> m;
      boost::algorithm::to_lower(lines[i]);
      boost::split(m, lines[i], boost::is_any_of(":"), boost::token_compress_on);
      string val;
      if (m.size() == 1){
        return false;
      }
      for (size_t i = 0; i < m.size(); i++){
        boost::trim(m[i]);
        if (i != 0){
          val += m[i];
        }
      }
      // std::cout<< "key: \"" << m[0] << "\", val: \"" << val << "\"" <<std::endl;
      out->AddHeader(m[0], val);
    }
  }
	// std::cout << "header size: " << out->GetHeaderCount() << std::endl;
  return true;
}

}  // namespace searchserver
