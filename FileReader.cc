/*
 * Copyright ©2022 Travis McGaha.  All rights reserved.  Permission is
 * hereby granted to the students registered for University of Pennsylvania
 * CIT 595 for use solely during Spring Semester 2022 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <stdio.h>
#include <string>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include <unistd.h>
#include <fcntl.h>

#include "./HttpUtils.h"
#include "./FileReader.h"

using std::string;
using std::cout; 
using std::endl;
using std::cerr; 

namespace searchserver {

bool FileReader::read_file(string *str) {

  *str = "";

  // // Read the file into memory, and store the file contents in the
  // // output parameter "str."  Be careful to handle binary data
  // // correctly; i.e., you probably want to use the two-argument
  // // constructor to std::string (the one that includes a length as a
  // // second argument).

  // char buff[200]; // buffer for holding what's read in
  // ssize_t rdin; // holder for how many was read in
  
  // // attempt to open file
  // // if file descriptor is -1, file-opening failed, RET
  // int fd_ = open(fname_.c_str(), O_RDONLY); 
  // if (fd_ == -1) { 
  //   // cerr<<"!!! error in opening file !!!"<<endl;
  //   return false; 
  // } 

  // // now file has been successfully opened
  // // as long as we have not reached the end of file, 
  // // continue to read into the buffer
  // while (true) {
  //   rdin = read(fd_, buff, 200);
  //   // if readin count = -1, error in reading, RET
  //   if (rdin == -1) { 
  //     // cerr<<"!!! error in reading file !!!"<<endl;
  //     return false; 
  //   } 
  //   // reading is successful, add characters to *str
  //   for (int i = 0; i < rdin; i ++) {
  //     *str += buff[i];
  //   }
  //   // reached end of file
  //   // break from loop and return
  //   if (rdin == 0) { 
  //     // cout<<"*** end of file reached ***"<<endl;
  //     break;
  //   }
  // }
  // close(fd_);
  // return true;

  FILE* f;
  long f_size;
  char* buffer;
  size_t result;
  size_t read = 0;
  f = fopen(fname_.c_str(), "r");
  if (f == NULL){
    return false;
  }
  fseek(f , 0 , SEEK_END);
  f_size = ftell(f);
  // std::cout<<"size: "<< f_size<< std::endl;
  rewind(f);
  buffer = (char*) malloc(sizeof(char)*f_size);
  while ((ssize_t)read < f_size){
    result = fread(buffer, 1, f_size, f);
    // std::cout<<"read size: "<< result<< std::endl;
    for (size_t i = 0; i < result; i++){
      *str += buffer[i];
    }
    read = str->length();
    // std::cout<<"string size: "<< read<< std::endl;
  }
  // std::cout<<"final string size: "<< str->length()<< std::endl;
  free(buffer);
  fclose(f);
  if (result != (size_t)(f_size)){

    cout<<"\n file did not read in correctly\n"<<endl;


    return false;
  } else {
    return true;
  }

}

}  // namespace searchserver
