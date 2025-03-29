/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef SCALING_H
#define SCALING_H


#include "ns3/core-module.h"
#include "myGraph.h"
#include "sfc.h"
#include "dis_application.h"





namespace ns3{

using namespace ns3;





class Scaling{

    private:

    bool read_config();

    std::string m_graph_file;
    std::string m_request_file;
    std::string m_config_file;
    unsigned m_r_num;

    

    

    

    public:
    Scaling()
    {

    };

    void init(std::string gml_file, std::string request_file, std::string config_file, int r_num, double sfc_factor=1.0);

    void deploy_all_request();

    Cfg m_cfg;
    MyGraph m_graph;
    std::vector<sfc::Request> m_request_vec;


};

}  /* SCALING_H*/

#endif