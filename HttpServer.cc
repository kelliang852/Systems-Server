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

#include <boost/algorithm/string.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <sstream>

#include "./FileReader.h"
#include "./HttpConnection.h"
#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpServer.h"


using std::cerr;
using std::cout;
using std::endl;
using std::list;
using std::map;
using std::string;
using std::stringstream;
using std::unique_ptr;
using std::vector;

namespace searchserver {
///////////////////////////////////////////////////////////////////////////////
// Constants, internal helper functions
///////////////////////////////////////////////////////////////////////////////
static const char *kFivegleStr =
  "<html><head><title>595gle</title></head>\n"
  "<body>\n"
  "<center style=\"font-size:500%;\">\n"
  "<span style=\"position:relative;bottom:-0.33em;color:orange;\">5</span>"
    "<span style=\"color:red;\">9</span>"
    "<span style=\"color:gold;\">5</span>"
    "<span style=\"color:blue;\">g</span>"
    "<span style=\"color:green;\">l</span>"
    "<span style=\"color:red;\">e</span>\n"
  "</center>\n"
  "<p>\n"
  "<div style=\"height:20px;\"></div>\n"
  "<center>\n"
  "<form action=\"/query\" method=\"get\">\n"
  "<input type=\"text\" size=30 name=\"terms\" />\n"
  "<input type=\"submit\" value=\"Search\" />\n"
  "</form>\n"
  "</center><p>\n";

// static
const int HttpServer::kNumThreads = 100;

// This is the function that threads are dispatched into
// in order to process new client connections.
static void HttpServer_ThrFn(ThreadPool::Task *t);

// Given a request, produce a response.
static HttpResponse ProcessRequest(const HttpRequest &req,
                            const string &base_dir,
                            WordIndex *indices);

// Process a file request.
static HttpResponse ProcessFileRequest(const string &uri,
                                const string &base_dir);

// Process a query request.
static HttpResponse ProcessQueryRequest(const string &uri,
                                 WordIndex *index);


///////////////////////////////////////////////////////////////////////////////
// HttpServer
///////////////////////////////////////////////////////////////////////////////
bool HttpServer::run(void) {
  // Create the server listening socket.
  int listen_fd;
  cout << "  creating and binding the listening socket..." << endl;
  if (!socket_.bind_and_listen(AF_INET6, &listen_fd)) {
    cerr << endl << "Couldn't bind to the listening socket." << endl;
    return false;
  }

  // Spin, accepting connections and dispatching them.  Use a
  // threadpool to dispatch connections into their own thread.
  cout << "  accepting connections..." << endl << endl;
  ThreadPool tp(kNumThreads);
  while (1) {
    HttpServerTask *hst = new HttpServerTask(HttpServer_ThrFn);
    hst->base_dir = static_file_dir_path_;
    hst->index = index_;
    if (!socket_.accept_client(&hst->client_fd,
                    &hst->c_addr,
                    &hst->c_port,
                    &hst->c_dns,
                    &hst->s_addr,
                    &hst->s_dns)) {
      // The accept failed for some reason, so quit out of the server.
      // (Will happen when kill command is used to shut down the server.)
      break;
    }
    // The accept succeeded; dispatch it.
    tp.dispatch(hst);
  }
  return true;
}

static void HttpServer_ThrFn(ThreadPool::Task *t) {
  // Cast back our HttpServerTask structure with all of our new
  // client's information in it.
  unique_ptr<HttpServerTask> hst(static_cast<HttpServerTask *>(t));
  cout << "  client " << hst->c_dns << ":" << hst->c_port << " "
       << "(IP address " << hst->c_addr << ")" << " connected." << endl;

  // Read in the next request, process it, write the response.

  // Use the HttpConnection class to read and process the next
  // request from our current client, then write out our response.  If
  // the client sends a "Connection: close\r\n" header, then shut down
  // the connection -- we're done.
  //
  // Hint: the client can make multiple requests on our single connection,
  // so we should keep the connection open between requests rather than
  // creating/destroying the same connection repeatedly.

  bool done = false;
  HttpConnection conn(hst->client_fd);

  cout<<"\nclient fd= "<<hst->client_fd<<endl;
  
  while (!done) {
    // cout<<"inside the loop"<<endl;
    HttpRequest req;
    // read in request
    if (conn.next_request(&req)){

      cout<< "uri: " << req.uri()<<endl;
      
      if (req.GetHeaderValue("connection").compare("close") == 0){
        done = true;
      } else {
        // cout<< "!!! triaging right now " << req.uri()<<endl;
        //Process Request
        HttpResponse resp = ProcessRequest(req, hst->base_dir, hst->index); 
        // cout<< "to write: " <<endl;
        // cout<< resp.GenerateResponseString() << endl;

        // Write response
        bool successful_write = conn.write_response(resp);
        if (successful_write){
          cout<< "successfully written" << endl;
          // cout<< resp.GenerateResponseString()<<endl;
        } else {
          cout<< "fatal error for written" << endl;
        }

        // cout<<resp.get_body()<<endl;
        // cout<<"\n number of words in hst->index= "<<hst->index->num_words()<<endl;
        cout<<"\nWrite attempted\n"<<endl;
      }
      
    }
    
  }
  // cout<<"finished the loop"<<endl;
}

static HttpResponse ProcessRequest(const HttpRequest &req,
                            const string &base_dir,
                            WordIndex *index) {
  // Is the user asking for a static file?
  if (req.uri().substr(0, 8) == "/static/") {
    cout<<"file request processing"<<endl;
    return ProcessFileRequest(req.uri(), base_dir);
  }

  // The user must be asking for a query.
  cout<<"query request processing"<<endl;
  return ProcessQueryRequest(req.uri(), index);
}

static HttpResponse ProcessFileRequest(const string &uri,
                                const string &base_dir) {
  // The response we'll build up.
  HttpResponse ret;

  // Steps to follow:
  //  - use the URLParser class to figure out what filename
  //    the user is asking for. Note that we identify a request
  //    as a file request if the URI starts with '/static/'
  //
  //  - use the FileReader class to read the file into memory
  //
  //  - copy the file content into the ret.body
  //
  //  - depending on the file name suffix, set the response
  //    Content-type header as appropriate, e.g.,:
  //      --> for ".html" or ".htm", set to "text/html"
  //      --> for ".jpeg" or ".jpg", set to "image/jpeg"
  //      --> for ".png", set to "image/png"
  //      etc.
  //    You should support the file types mentioned above,
  //    as well as ".txt", ".js", ".css", ".xml", ".gif",
  //    and any other extensions to get bikeapalooza
  //    to match the solution server.
  //
  // be sure to set the response code, protocol, and message
  // in the HttpResponse as well.

  // cout<<" ~~~You have reached inside FILE request"<<endl;

  string file_name = "";
  string file_last_name = "";
  URLParser up; 
  up.parse(uri);
  file_name += (up.path()).substr(8); 


  vector<string> file_dir; 
  boost::split(file_dir, uri, boost::is_any_of("/"));
  file_last_name = file_dir[file_dir.size()-1]; 

  string fileContent;

  // get the type after "." by splitting 
  vector<string> splitted; 
  boost::split(splitted, file_last_name, boost::is_any_of("."));
  string type = splitted.back(); 


  if (type.compare("html") == 0 || type.compare("htm") == 0) {
    ret.set_content_type("text/html");
  } else if (type.compare("jpeg") == 0 || type.compare("jpg") == 0) {
    ret.set_content_type("image/jpeg");
  } else if (type.compare("png") == 0) {
    ret.set_content_type("image/png");
  // } else if (type.compare("txt") == 0){
  //   //--------------------------- TO FILL IN ----------------------------------------
  // } else if (type.compare("js") == 0){
  //   //--------------------------- TO FILL IN ----------------------------------------
  } else if (type.compare("css") == 0){
    ret.set_content_type("text/css");
  } else if (type.compare("xml") == 0){
    ret.set_content_type("application/xml");
  } else if (type.compare("gif") == 0){
    ret.set_content_type("image/gif");
  } else if (type.compare("svg") == 0){
    ret.set_content_type("image/svg+xml");
  } else if (type.compare("js") == 0){
    ret.set_content_type("application/json");
  } else if (splitted.size() == 1) {
    ret.set_content_type("text/javascript");
  } else { //--------------------------- MORE DATA TYPES  --------------------------------
    ret.set_content_type("text/plain");
  } 

  // cout << type << endl;
  // cout<<"content-type: " << ret.get_content_type() << endl;

  cout<<"file_name = "<<file_name<<endl;

  FileReader f(file_name);
  if (f.read_file(&fileContent)) {
    // if (ret.get_content_type().string::find("image") != string::npos || 
    //     ret.get_content_type().string::find("application") != string::npos){ //text
    //   cout<<"image file"<<endl;
    //   // copying content to the body of html
      ret.AppendToBody(fileContent);
    // } else { // anything else
    //   cout<<"text file"<<endl;
    //   // ret.AppendToBody("<pre style=\"word-wrap: break-word; white-space: pre-wrap;\">\n");
    //   // copying content to the body of html
    //   ret.AppendToBody(fileContent);
    //   // ret.AppendToBody("\n</pre>");
    // }

    // setting the rest of the stuff
    ret.set_protocol("HTTP/1.1");
    ret.set_response_code(200);
    ret.set_message("OK");
    return ret; 
  }

  cout<<" \n*** read_file returns false ***\n"<<endl;
  
  // TODO: Implement

  // If you couldn't find the file, return an HTTP 404 error.
  ret.set_protocol("HTTP/1.1");
  ret.set_response_code(404);
  ret.set_message("Not Found");
  ret.AppendToBody("Couldn't find file \""
                   + escape_html(file_name)
                   + "\"\n");
  return ret;
}

static HttpResponse ProcessQueryRequest(const string &uri,
                                 WordIndex *index) {
  // The response we're building up.
  HttpResponse ret;

  // Your job here is to figure out how to present the user with
  // the same query interface as our solution_binaries/httpd server.
  // A couple of notes:
  //
  //  - no matter what, you need to present the 595gle logo and the
  //    search box/button
  //
  //  - if the user sent in a search query, you also
  //    need to display the search results. You can run the solution binaries to see how these should look
  //
  //  - you'll want to use the URLParser to parse the uri and extract
  //    search terms from a typed-in search query.  convert them
  //    to lower case.
  //
  //  - Use the specified index to generate the query results

  // TODO: implement

  // cout<<" ~~~You have reached inside QUERY request"<<endl;
  URLParser u;
  u.parse(uri);
  string uri_ = u.path();
  std::map<std::string, std::string> args_ = u.args();
  list<Result> indexRes;
  vector<string> queries;
  if (args_.size() != 0){
    string q = args_.at("terms");
    // cout<<"queries: " <<q<<endl;
    boost::split(queries, q, boost::is_any_of(" "), boost::token_compress_on);
    for (auto& i :queries) {
      boost::to_lower(i);
      cout<< "query: " <<i<<endl;
    }
    indexRes = index->lookup_query(queries);
  }
  ret.set_protocol("HTTP/1.1");
  ret.set_response_code(200);
  ret.set_message("OK");
  ret.AppendToBody(kFivegleStr);

  // cout<<" ~~~Done setting up basic stuff"<<endl;

  // if search is pressed with nothing inside to query, do not change interface
  if (args_.size() == 0){
    // cout<<"HttpServer::QueryRequest= Nothing inside the query "<<endl;;
    return ret;
  }

  if (indexRes.size() == 0) {
// ret.AppendToBody("<p>\n Welcome! </p>\n <p> - Dana & Kelly</p></body></html>");
    // ret.AppendToBody("<p>\n No Results </p></body></html>");
    ret.AppendToBody("<p>\n <br>");
    ret.AppendToBody(" No results found for <b>");
    string tmp;
    for (auto& i : queries) {
      tmp += i + " ";
    }
    ret.AppendToBody(tmp);
    ret.AppendToBody("</b>\n </p>\n <p></p>\n<p></p>\n</body>");
    // cout<<" !!! No result"<<endl;
    // cout<<" ~~~You have reached the END QUERY request\n"<<endl;

  } else {

    // ret.AppendToBody("<p>\n YES RESULT </p>\n</body>");
    // cout<<"there are results"<<endl;
    ret.AppendToBody("<p>\n <br>\n");
    ret.AppendToBody(std::to_string(indexRes.size()));
    ret.AppendToBody(" result for <b>");
    string tmp;
    for (auto& i : queries) {
      tmp += i+" ";
    }
    ret.AppendToBody(tmp);
    ret.AppendToBody("</b>\n </p>\n <p></p>\n<ul>\n");
    for (auto& i :indexRes) {
      ret.AppendToBody("<li>");
      ret.AppendToBody("<a href =\"");
      ret.AppendToBody("/static/./");
      ret.AppendToBody(i.doc_name);
      ret.AppendToBody("\">");
      ret.AppendToBody(i.doc_name);
      ret.AppendToBody("</a> [");
      ret.AppendToBody(std::to_string(i.rank));
      ret.AppendToBody("]\n<br>\n</li>\n");
    }
    ret.AppendToBody("</ul>\n</body>\n</html>");
    // cout<<" !!! YES result"<<endl;
    // cout<<" ~~~You have reached the END QUERY request\n"<<endl;
    
  }
  return ret;
}

}  // namespace searchserver
