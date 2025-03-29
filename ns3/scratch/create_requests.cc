//generate requests based on zipf distribution.

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

#include "ns3/zipf.h"
#include "ns3/myAlgorithm.h"



using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("test-my");

//<node_id, reqNum>
void writeFile(std::string fileName, std::map<int, int> req_map){
    std::ofstream outfile;

    outfile.open(fileName, std::ofstream::app);

    if(outfile.is_open())
    { 
     
      for (int i = 1; i <= req_map.size(); i++){
          auto it = req_map.find(i);

          if (it == req_map.end()){
              NS_LOG_ERROR("cannot find the node number " << i);
              break;
          }

          outfile << it->second;
          if(i != req_map.size())
          {
              outfile << ',';
          }
    
      }

      outfile << std::endl;
      outfile.close();
    }
}



int
main (int argc, char *argv[])
{
   LogComponentEnable ("MyAlgorithm", LOG_LEVEL_INFO);
   LogComponentEnable ("Zipf", LOG_LEVEL_INFO);

   int slotNum = 30;//how many slot we want
   

   MyAlgorithm ma;
   ma.loadConfig("config/config.txt");
   ma.loadTopo();
   ma.initFuncMap();
   ma.loadRequest();

   std::string fileName = "config/requests-" + std::to_string(ma.m_cfg.Beta) +".csv";

    // Generate and output zipf random variables
    std::cout << "-----start zipf-----";
    //for different type of requests
    for(auto const& typeRequests : ma.m_req_num){
        NS_LOG_INFO("iterate typeRequests: " << typeRequests.first);
        
        int type = typeRequests.first;
        ReqNumMap reqNumMap = typeRequests.second;

        std::ofstream outfile;

        outfile.open(fileName, std::ofstream::app);

        if(outfile.is_open())
        { 
        outfile << "funcType";
        outfile << ",";
        outfile << typeRequests.first;
        outfile << "\n";
        outfile.close();
        }

        //time slot start from 1
        // for(int i = 1; i <= reqNumMap.size(); i++){
        for(int i = 1; i <= slotNum; i++){

                auto it = reqNumMap.find(i);
                if(it == reqNumMap.end()){
                    NS_LOG_ERROR(" createRequests breaks!!!");
                    break;
                }

                int totalReqNum = it->second;
                
                //number of nodes, beta, and max mum, note the max number
                //get the number of requests for each node
                // std::map<int, int> req_map = ma.genZipfNum(ma.m_map.size(), ma.m_cfg.Beta, totalReqNum/(ma.m_map.size()));

                std::map<int, int> req_map = ma.genZipfNum(ma.m_map.size(), ma.m_cfg.Beta, totalReqNum);

                
                
                NS_LOG_LOGIC(" createRequest---------");
                writeFile(fileName, req_map);
               
        }
        

    }

}