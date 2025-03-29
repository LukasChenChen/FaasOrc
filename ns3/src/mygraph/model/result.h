#ifndef RESULT_H
#define RESULT_H
#include <vector>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp> // is_any_of
#include "mygraph.h"

namespace ns3 {

//the result for one request
class Result{
    
    private:
        int m_id; // sfc_id
        Path m_path;  // traversed path
        std::vector<int> m_host_node;  // hosted node in order


    public:
        Result(){
        };

        int get_id(){
            return m_id;
        };

        Path get_path(){

            return m_path;
        };

        std::vector<int> get_host_node(){

            return m_host_node;
        };

        void set_id(int id){
            m_id = id;
        };

        void set_path(Path p){
            m_path = p;
        };

        void set_host_node(std::vector<int> h){

            m_host_node = h;
        }
       
};



}

#endif