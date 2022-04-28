
#include <unordered_map>
#include <list>
#include <vector>
#include <string>
#include <map>
#include <iostream>

#include <map>

#include "./WordIndex.h"

using std::endl;
using std::cout; 
using std::cerr; 

using std::string;
using std::list;
using std::vector;
using std::unordered_map;
using std::map;
using std::pair;
using std::find;


// implement look up table with 3 columns in it 
// with words, documents, counts
// keep track of number of times each word appears in each documents
// in how many documents it has appeared, individual number
// choose the appropriate stl container to implement the look up table

namespace searchserver {

WordIndex::WordIndex() {
}

WordIndex::~WordIndex() {
}

size_t WordIndex::num_words() {
  return master.size();
}

void WordIndex::record(const string& word, const string& doc_name) {
  auto search = master.find(word);
  
  if (search != master.end()){  
    // (A) master map has the word in it!
    // now find the current list that is attached to the word 
    map<string, int> &currList = search -> second;

    // looping through the list and see if the doc_name is 
    // in the list as one of the pair --> determine if word + doc 
    // or word x doc to go to one of the two following scenarios 
    bool in = false; 
    auto searchIn = currList.find(doc_name);
    if (searchIn != currList.end()) {
      in = true;
    }

    // (1) master map has the word but not the document in the list 
    // create a new pair of doc_name + count and insert into the currList
    // replace the list in the master map
    if (!in) {
      // cout<<"contains word but x doc"<<endl;

      currList.insert(pair<string, int>(doc_name, 1)); 

    } else {
    // (2) master map has the word and has the document in the list
    // loop through the list to find the element
    // simply increment the second element of the pair by 1
 
      // int old = searchIn -> second;
      // cout<<word<<"--"<<searchIn->first<<": old check= "<<old<<endl;
      // currList.erase(doc_name);
      // currList.insert(pair<string, int>(doc_name, old+1)); 
      currList[doc_name] ++;
    }
  } else {
    // master map does not have the word
    // 1. create a new list
    // 2. create a new pair and insert into the list
    // 3. insert the list to the master map
    // cout<<"x word"<<endl;

    // map<string, int> tmp;
    // tmp.insert(pair<string, int> (doc_name, 1));
    // master.insert(pair<string, map<string, int>>(word, tmp));

    master[word][doc_name] = 1;

  }

}

list<Result> WordIndex::lookup_word(const string& word) {
  list<Result> result;

  // getting the appropriate list attached to word from the master map
  auto search = master.find(word);
  if (search != master.end()) {
    // cout<<" !!! found the word !!!"<<endl;
    map<string, int> &currList = search -> second;
    
    for (auto& i : currList) {
      Result r (i.first, i.second);
      result.push_back(r);
    }
  }
  result.sort();
  return result;
}

list<Result> WordIndex::lookup_query(const vector<string>& query) {
  
  list<Result> results;

  // map as temporary storage between the document and the count
  map<string, int> tmp;
  map<string, int> docCount;
  // for each of the query word in the vector, do lookup_word
  // store result in a temporary list smResult
  // for every Result pair in smResult, see if doc_name in map 
  // if in map --> increment the count; else insert new pair
  for (size_t i = 0; i < query.size(); i ++) {
    list<Result> smResult = lookup_word(query.at(i));

    // cout<<"smResult size= "<<smResult.size()<<endl;

    for (auto& i : smResult) {
      
      auto docInd = docCount.find(i.doc_name);
      if (docInd != docCount.end()) {
        // cout<<"oldCount "<<docInd->second<<endl;
        int k = docInd->second+1;
        docCount.erase(i.doc_name);
        docCount.insert(pair<string, int>(i.doc_name, k));
      }

      auto mapInd = tmp.find(i.doc_name); 
      if (mapInd != tmp.end()) {
        int old = mapInd->second;
        int newVal = old + i.rank;
        // cout<<"old = "<<old<<endl;
        // cout<<"new = "<<newVal<<endl;
        string doc = mapInd->first;
        tmp.erase(doc);
        tmp.insert(pair<string, int>(doc, newVal));
        // cout<<"replaced"<<endl;
        // cout<<doc<<" = "<<tmp.at(doc)<<endl;
        // cout<<endl;
      } else {
        tmp.insert(pair<string, int>(i.doc_name, i.rank));
        docCount.insert(pair<string, int>(i.doc_name, 1));
      }
    }
  }

  // find the list of valid document that has all the words in the query in it
  int queryCount = query.size(); 
  list<string> validDoc;
  for (auto& i : docCount) {
    // cout<<"** i.second = "<<i.second<<endl;
    if (i.second == queryCount) {
      validDoc.push_back(i.first);
    }
  }
  // cout<<"query size= "<<query.size()<<endl;
  // cout<<"validDoc size = "<<validDoc.size()<<endl;
  // cout<<"docCount size = "<<docCount.size()<<endl;
  // cout<<"tmp size = "<<tmp.size()<<endl;
  
  for (auto& i : tmp) {
    bool rec = false;
    for (auto& d : validDoc) {
      if (i.first == d) {
        // cout<<i.first<<endl;
        rec = true;
      }
    }
    if (rec) {
      Result r (i.first, i.second);
      results.push_back(r);
    }
  }

  //TODO: add up the occurence!!! 
  results.sort();
  return results;
}

}  // namespace searchserver
