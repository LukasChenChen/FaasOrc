#include "myGraph.h"
#include "sfc.h"
#include "ns3/core-module.h"
#include "scaling.h"

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <random>

NS_LOG_COMPONENT_DEFINE("scaling");
using namespace ns3;

namespace ns3{



bool Scaling::read_config(){
        std::string line;
        std::ifstream myfile ("scratch/" + m_config_file);

        int inter_latency, intra_latency, inter_bw, intra_bw, domain_num, processing_cap;

        std::vector<int> cpu_price_v, mem_price_v, cpu_v, mem_v; // different for different domains.
        std::cout <<"-----read config-----"<< std::endl;
       
        if (myfile.is_open()) {
            
            while (getline (myfile, line)) {
               
                size_t pos = line.find("=");
                if (pos == std::string::npos){

                    std::cout << "cant find = read_config" <<std::endl;
                    return false;
                }
                  

                std::string tmp_key = line.substr(0, pos);

                
                if (tmp_key == "inter_latency")
                {
                    inter_latency = stoi(line.substr(pos+1));
                    std::cout<< "inter_latency " << inter_latency << std::endl;
                }
                else if (tmp_key == "intra_latency")
                {
                    intra_latency = stoi(line.substr(pos+1));
                    std::cout<< "intra_latency " << intra_latency << std::endl;
                }
                else if (tmp_key == "inter_bw")
                {
                    inter_bw = stoi(line.substr(pos+1));
                    std::cout<< "inter_bw " << inter_bw << std::endl;
                }
                else if (tmp_key == "intra_bw")
                {
                    intra_bw = stoi(line.substr(pos+1));
                    std::cout<< "intra_bw " << intra_bw << std::endl;
                }
               
                else if (tmp_key == "domain_num")
                {
                    domain_num = stoi(line.substr(pos+1));
                    std::cout<< "domain_num " << domain_num << std::endl;
                }
                 else if (tmp_key == "processing_cap")
                {
                    processing_cap = stoi(line.substr(pos+1));
                    std::cout<< "processing_cap " << processing_cap << std::endl;
                }
            
                
            }


            myfile.close();
        } 
        else 
        { 
            std::cout << "Unable to open file Scaling::read_config"; 
            return false;
        }

      
       
        m_cfg.inter_latency = inter_latency;
        m_cfg.intra_latency = intra_latency;
        m_cfg.inter_bw = inter_bw;
        m_cfg.intra_bw = intra_bw;
        m_cfg.domain_num = domain_num;
        m_cfg.processing_cap =processing_cap;
        

      
        
        return true;
    

}

void Scaling::init(std::string gml_file, std::string request_file, std::string config_file, int r_num, double sfc_factor)
{   

    m_graph_file = gml_file;
    m_request_file = request_file;
    m_config_file = config_file;
    m_r_num = r_num;


    //read sfc
    sfc::read_sfc(m_request_file, m_request_vec, m_r_num, sfc_factor); 

    //read config
    read_config();

    //generate graph
    m_graph = MyGraph();

    NS_LOG_DEBUG("graph file name " << m_graph_file);

    m_graph.init(m_graph_file, m_cfg);

    m_graph.create_graph();

}




} // end ns3

