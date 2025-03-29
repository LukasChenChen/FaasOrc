/*
When add to activefunctions, change the freq.
When leaving the cache map, reduce the freq.
Everything in m_afs will be moved into cache because since it can be created,
memory is enough.
when insert function to cache map, the phynodeid must be updated.
Everything add to m_afs, the priority will be refreshed, the m_clock and m_functionfreq will be refreshed.
*/

#include "ns3/core-module.h"
#include "flame.h"
#include "ns3/log.h"
#include <boost/algorithm/string/split.hpp> // boost:split()
#include <boost/algorithm/string.hpp> // boost::is_any_of()
#include "ns3/zipf.h"
#include "ns3/operation.h"
#include "ns3/network_controller.h"

// #include "ns3/network_controller.h"

namespace ns3 {
  NS_LOG_COMPONENT_DEFINE ("Flame");

  Flame::Flame (){
    NS_LOG_FUNCTION (this);
  }

  Flame::~Flame(){
    NS_LOG_FUNCTION (this);
  }

  bool Flame::loadConfig(std::string filename){

    std::string line;
    std::ifstream myfile (filename);

    // int inter_latency, intra_latency, inter_bw, intra_bw, domain_num, processing_cap;

    std::string TopoName, RequestFile;
    float LatencyPara, MemCap, Beta, ReduFactor;
    int NodeNum, SlotNum;

    NS_LOG_INFO("-----read config-----");
    
    if (myfile.is_open()) {
        
        while (getline (myfile, line)) {
            
            size_t pos = line.find("=");
            if (pos == std::string::npos){

                std::cout << "cant find =" <<std::endl;
                return false;
            }
                

            std::string tmp_key = line.substr(0, pos);

            
            if (tmp_key == "TopoName")
            {
                TopoName = line.substr(pos+1);
                std::cout<< "TopoName " << TopoName << std::endl;
            }
            else if (tmp_key == "RequestFile")
            {
                RequestFile = line.substr(pos+1);
                std::cout<< "RequestFile " << RequestFile << std::endl;
            }
            else if (tmp_key == "LatencyPara")
            {
                LatencyPara = stof(line.substr(pos+1));
                std::cout<< "LatencyPara " << LatencyPara << std::endl;
            }
            else if (tmp_key == "MemCap")
            {
                MemCap = stof(line.substr(pos+1));
                std::cout<< "MemCap " << MemCap << std::endl;
            }
            else if (tmp_key == "NodeNum")
            {
                NodeNum = stoi(line.substr(pos+1));
                std::cout<< "NodeNum " << NodeNum << std::endl;
            }
            else if (tmp_key == "Beta")
            {
                Beta = stof(line.substr(pos+1));
                std::cout<< "Beta " << Beta << std::endl;
            }
            else if (tmp_key == "SlotNum")
            {
                SlotNum = stoi(line.substr(pos+1));
                std::cout<< "SlotNum " << SlotNum << std::endl;
            }
            else if (tmp_key == "ReduFactor")
            {
                ReduFactor = stof(line.substr(pos+1));
                std::cout<< "ReduFactor " << ReduFactor << std::endl;
                break;
            }
           
        
        
            
        }


        myfile.close();
    } 
    else 
    { 
        NS_LOG_ERROR("Unable to open file Scaling::read_config"); 
        return false;
    }

    
    
    m_cfg.TopoName = TopoName;
    m_cfg.RequestFile = RequestFile;
    m_cfg.LatencyPara = LatencyPara;
    m_cfg.MemCap = MemCap;
    m_cfg.NodeNum = NodeNum;
    m_cfg.Beta =Beta;
    m_cfg.SlotNum =SlotNum;
    m_cfg.ReduFactor =ReduFactor;
    
    
    return true;

}

void Flame::initFuncMap(){
    //processing time in ms, cold start time in seconds, size
    //processing time in ms, cold start time in seconds, size
    FunctionInfo fi = {350, 1, 55};
    m_funcInfoMap.add(1,fi);

    fi = {630, 2.1, 158};
    m_funcInfoMap.add(2,fi);

    fi = {200, 2.2, 332};
    m_funcInfoMap.add(3,fi);

    fi = {207, 2.3, 92};
    m_funcInfoMap.add(4,fi);

    fi = {500, 2.4, 30};
    m_funcInfoMap.add(5,fi);

    fi = {100, 2.5, 40};
    m_funcInfoMap.add(6,fi);

    fi = {150, 2.6, 50};
    m_funcInfoMap.add(7,fi);

    fi = {200, 2.7, 60};
    m_funcInfoMap.add(8,fi);

    fi = {175,2.8, 70};
    m_funcInfoMap.add(9,fi);

    fi = {125, 2.9, 80};
    m_funcInfoMap.add(10,fi);

}
  
void Flame::loadTopo(){
    NS_LOG_FUNCTION("this");

    std::string line;

    std::ifstream myfile(m_cfg.TopoName);

    NS_LOG_INFO("-----read topo file-----");

    int nodeID = 0;
        
    if (myfile.is_open()) {
        
        while (getline (myfile, line)) 
        {
           
            std::string node_s = line;

            std::vector<std::string> split_node_v;

            boost::split(split_node_v, node_s, boost::is_any_of(","), boost::token_compress_on);
            
            int count = 1;

            std::string LATITUDE_S, LONGITUDE_S;
            
            //skip first row, it is the title
            if(nodeID == 0){
                nodeID++;
                continue;
            }

            for (std::vector<std::string>::iterator it = split_node_v.begin(); it  != split_node_v.end(); it++)
            {

                int i = count;
             
                if(i == 2)
                {
                    LATITUDE_S = *it;

                    NS_LOG_LOGIC(" LATITUDE_S " << LATITUDE_S);
                    
                }else if(i == 3)
                {
                    LONGITUDE_S = *it;

                    NS_LOG_LOGIC("LONGITUDE_S " << LONGITUDE_S);
                    
                }
                count ++;
            }

            float latitude = stof(LATITUDE_S); 

            float longitude = stof(LONGITUDE_S);

            Location loc;
            loc.init(latitude, longitude);

            m_map.insert({nodeID, loc});

            PhyNode f = {
                nodeID, //id
                latitude, //latitude
                longitude, //longitude
                m_cfg.MemCap, //mem
            };

            m_topo.add(nodeID, f);
             

            NS_LOG_LOGIC("add node " << nodeID << "lat " << loc.latitude << "long " << loc.latitude);
            nodeID++; 
        }
    }

    myfile.close();

}

//notice  the request file is row 2, 4, 6, 8...
void Flame::loadRequest(){
    NS_LOG_FUNCTION("this");

    std::string line;

    std::ifstream myfile(m_cfg.RequestFile);

    NS_LOG_INFO("-----read Request file-----");

    int req_num = 0;

    int rowNum = 1;
        
    if (myfile.is_open()) {
        
        while (getline (myfile, line)) 
        {   
            //get rid of odd row
            if(rowNum % 2 != 0 ){
               rowNum++;
               continue;
            }

            std::string req_num_s = line;

            std::vector<std::string> split_req_v;
            
            //split req_num_s into several strings
            boost::split(split_req_v, req_num_s, boost::is_any_of(","), boost::token_compress_on);
            
            int count = 1;

            ReqNumMap reqNumMap;

            for (std::vector<std::string>::iterator it = split_req_v.begin(); it  != split_req_v.end(); it++)
            {

                int i = count;
             
                req_num = stoi(*it);

                NS_LOG_LOGIC("time slot " << i << "req_num: " << req_num);

                reqNumMap.insert({i, req_num});
                    
                count ++;
            }
            NS_LOG_LOGIC("insert function type " << rowNum/2);
            m_req_num.insert({ rowNum/2 ,reqNumMap});

            rowNum++;
           
        }
    }

    myfile.close();
}

//given the num_values -> node num, aplha, and max_num -> num of requests/num of nodes
std::map<int, int> Flame::genZipfNum(int num_values, float alpha, int max_num){
  NS_LOG_FUNCTION("this");
//   NS_LOG_INFO("num_values " << num_values);
  int zipf_rv = 0;
  //for
  Zipf_generator zipf;
  int seed = 1;
  zipf.rand_val(seed);
  
  std::map<int, int> req_map;// <node id, request_num>;
  for (int i=1; i<= num_values; i++)
  { 
    NS_LOG_LOGIC("num_values " << num_values);
    zipf_rv = zipf.zipf(alpha, max_num);
    NS_LOG_LOGIC("zipf value " << zipf_rv);
    req_map.insert({i, zipf_rv});
  }

  return req_map;

}

bool Flame::createRequest(int timeSlot, int funcType, int ingressID, Request &r){
    NS_LOG_FUNCTION(this);

    auto it = m_map.find(ingressID);

    if (it != m_map.end()){
    
        PhyNode pNode = {
            ingressID,            //int     id;
            it->second.latitude,  //float   latitude;
            it->second.longitude, //float   longitude;
            m_cfg.MemCap,         //float   mem;
        };

        PhyNode pn;//empty

        FunctionInfo fInfo;

        Function f;
        f.type = funcType;

        NS_ASSERT_MSG(funcType<=10 && funcType >= 1 ,"create request cannot find func type " << funcType);
        
        FunctionInfo fi;
        if (m_funcInfoMap.get(funcType, fi)){
           f.processingTime   = fi.processingTime;
           f.coldStartTime   = fi.coldStartTime;
           f.size   = fi.size;

           Request request = {
           m_req_count, //id
           f,           //Function function;
           pNode,       //PhyNode  ingress;
           timeSlot,           //arriveTime;
           false,       // served
           pn,           // deployNode
           false,       //isColdStart
           };

           r = request;

           m_req_count++;
           return true;
        }
        else{
            NS_LOG_ERROR("cannot find funcType :" << funcType);
            NS_ASSERT_MSG(funcType <= 10 && funcType >= 1 ,"create request cannot find func type " << funcType);
            return false;
        }

    }//end it != m_map.end()
    else{
        NS_LOG_ERROR("cannot find ingress id: " << ingressID);
        return false;
    }
}

//create the requests for a single time slot
void Flame::createRequestInSlot(int timeSlot, int funcType, ReqOnNodes ron){
    NS_LOG_FUNCTION(this);
    //order does not matter
    //create for different node
    for(int i = 0; i < ron.numVector.size(); i++){
        //node id starts from 1
        int nodeID = i+1;
        int rNum = ron.numVector[i];

        rNum = (int)(rNum/m_cfg.ReduFactor);
         //create rNum requests
        for (int i = 0; i < rNum; i++){

            Request r;
            createRequest(timeSlot, funcType, nodeID, r);

            m_request_map.add(timeSlot, r);

            NS_ASSERT_MSG(r.function.type >= 1 && r.function.type <= 10, "error: add request functype wrong " << r.function.type);
        }
    }

}

//create all requests from m_rontt
void Flame::createRequests(){

    for (auto it = m_rontt.numMap.begin(); it != m_rontt.numMap.end(); it++){
        int funcType = it->first;
        ReqOnNodesTime ront = it->second;
        //iterate over all time slot
        for(int i = 0; i < ront.numVector.size(); i++){
            //time slot start from 1
            createRequestInSlot(i+1, funcType, ront.numVector[i]);
        }
    }

}


//CREATE serverless requests based on m_req_num
// void Flame::createRequests(){

//     // Generate and output zipf random variables
    

//     NS_LOG_FUNCTION(this);
//     m_req_count = 0;
//     //for different type of requests
//     for(auto const& typeRequests : m_req_num){
//         NS_LOG_LOGIC("iterate typeRequests: " << typeRequests.first);
        
//         int type = typeRequests.first;
//         ReqNumMap reqNumMap = typeRequests.second;

//         //time slot start from 1
//         // for(int i = 1; i <= reqNumMap.size(); i++){
//         for(int i = 1; i <= 1; i++){

//                 auto it = reqNumMap.find(i);
//                 if(it == reqNumMap.end()){
//                     NS_LOG_ERROR(" createRequests breaks!!!");
//                     break;
//                 }

//                 int totalReqNum = it->second;
                
//                 //number of nodes, beta, and max mum
//                 //get the number of requests for each node
//                 std::map<int, int> req_map = genZipfNum(m_map.size(), m_cfg.Beta, totalReqNum/(m_map.size()));

//                 // std::map<int, int> req_map = genZipfNum(125, 0.5, 519);
                
//                 NS_LOG_LOGIC(" createRequest---------");
//                 //create request for  this time slot
//                 createRequestInSlot(i, typeRequests.first, req_map);
//         }
        

//     }


// }

//read the request from the file, reduce by the factor
void Flame::readRequests(){
    // std::map<int, int> req_map = genZipfNum(125, 0.5, 519);

    NS_LOG_FUNCTION("this");

    std::string line;

    std::ifstream myfile("config/requests-" + std::to_string(m_cfg.Beta) +".csv");

    NS_LOG_INFO("-----read zipf requests file-----");

    int req_num = 0;

    int funcType = 0;

    ReqOnNodesTime ront;

    if (myfile.is_open()) {
        
        while (getline (myfile, line)) 
        {   
            //get rid of odd row
            if(line.find("funcType") != std::string::npos ){
               NS_LOG_INFO("found " << line);
               if(funcType == 0){
                   funcType++;
                   continue; //skip the first line of the file
               }

               NS_LOG_LOGIC("add functype " << funcType);
               //add to this member variable
               m_rontt.add(funcType, ront);
               ront.clear();
               funcType++;
               continue; //go to the data line
            }

            std::string req_num_s = line;

            std::vector<std::string> split_req_v;
            
            //split req_num_s into several strings
            boost::split(split_req_v, req_num_s, boost::is_any_of(","), boost::token_compress_on);
            
            int count = 1;


            ReqOnNodes ron;

            //get the number of each nodes
            for (std::vector<std::string>::iterator it = split_req_v.begin(); it  != split_req_v.end(); it++)
            {
                NS_LOG_LOGIC("node id " << count << "req_num: " << req_num);
                
                req_num = stoi(*it);

                ron.add(req_num);
                count++;
            }
           
            ront.add(ron);
           
        }//end while
         //when all finish add type 4
        if (!getline (myfile, line)){
            NS_LOG_LOGIC("add functype " << funcType);
            m_rontt.add(funcType, ront);
        }
    }//end if

    myfile.close();

    return;        
}

//return meter
float Flame::distance(int node_1, int node_2){
    NS_LOG_FUNCTION(this);
    auto it_1 = m_map.find(node_1);
    auto it_2 = m_map.find(node_2);

    if(it_1 == m_map.end() || it_2 == m_map.end()){
        NS_LOG_ERROR(" cannot find node id : " << node_1 << " or " << node_2);
        return 0;
    }

    Location loc1 = it_1->second;
    Location loc2 = it_2->second;

    return CalcGPSDistance(loc1.latitude, loc1.longitude, loc2.latitude, loc2.longitude)/1000;

}

//sort in ascending order of distance
DistSlice Flame::sortPhyNodes(Request r){
    NS_LOG_FUNCTION(this);
    NS_LOG_LOGIC("Calculate distance -------");

    if (r.ingress.id > m_map.size()){
        NS_LOG_ERROR( "Wrong ingress id "<< r.ingress.id);
    }
    DistSlice ds;
    for(auto it = m_map.begin(); it != m_map.end(); it++){
        int nodeId = it->first;
        //in kilometers
        float dist = distance(r.ingress.id, nodeId);
        NS_LOG_LOGIC("distance is " << dist);

        Distance dis;
        dis.phyNodeID = it->first;
        dis.distance = dist;
        ds.add(dis);
        NS_LOG_LOGIC("dsitance ->>> " << dist);
    }
    NS_LOG_LOGIC("displaying nodes -------");
    // ds.show();
    NS_LOG_LOGIC("Sorting nodes -------");
    //sort ds.
    std::sort(ds.vec.begin(), ds.vec.end());

    // ds.show();

    return ds;
}

//index = -1 means not successful
Function Flame::getIdleFunction(int phynodeID, int funcType, int &index){
    NS_LOG_FUNCTION(this);

    Function f = m_cm.getIdleFunction(phynodeID, funcType, index);

    return f;

}


//place to selected node by creating new
void Flame::createNew(Request &r, int phyNodeID){
    NS_LOG_FUNCTION(this);
    bool succFlag = true;
    Function f;
    int count = 0;
    r.function.calcHotscore(m_functionfreq);
    NS_LOG_INFO("------phyNodeID-----" << phyNodeID);
    NS_LOG_INFO("------mem node-----" << m_topo.get(phyNodeID).mem);
    if(r.function.size > m_topo.get(phyNodeID).mem){
        while (count < 1) {
            count++;

            float p2 = m_cm.getLowestHotscore(phyNodeID);

            if (p2 < r.function.hotscore){
                NS_LOG_INFO("------lowest hotscore------");
                succFlag = m_cm.deleteLowestHotscore(phyNodeID, f, m_functionfreq);

                if(succFlag == true){
                    //if success terminate the container
                   // terminate(f); end the traffic application

                    m_topo.update("add", phyNodeID, f.size);
                }
                
                //keep check the memory, if sufficient
                if (r.function.size <= m_topo.get(phyNodeID).mem){

                    //create containersphyNodeID
                    PhyNode p = m_topo.get(phyNodeID);

                    if(p.id == 0){
                        return;
                    }
                    
                    r.function.phyNode = p;

                    NS_ASSERT_MSG( phyNodeID <= 125 && phyNodeID > 0 , "phyNodeID smaller than 125");

                    m_afs.add(r.function, phyNodeID, m_functionfreq, m_clock);

                    m_topo.update("minus", phyNodeID, r.function.size);

                    r.update(r.function, p, true);

                    return;

                }

                //no space to delete
                if (succFlag == false){

                    return;
                }
           }
           else{
            NS_LOG_INFO("------no other option------");
                return;
           }//endif
       }//end while
    }
    else{

        NS_LOG_INFO("------create hotscore------");
        r.function.calcHotscore(m_functionfreq);

        NS_ASSERT_MSG(phyNodeID <= 125 && phyNodeID >0, "phyNodeID smaller than 125");

         //create containers
         PhyNode p = m_topo.get(phyNodeID);

         if(p.id == 0){
             return;
         }
         
        r.function.phyNode = p;
        
        //put the function in active list
        m_afs.add(r.function, phyNodeID, m_functionfreq, m_clock);

        m_topo.update("minus", phyNodeID, r.function.size);

        r.update(r.function, p, true);

        return;
    }

    return;

}

void Flame::deployRoundRobin(Request &r){
    NS_LOG_FUNCTION(this);


    int node_num = m_topo.size();
    //if iterate over one round, init
    if(m_current_node_index > node_num){
        m_current_node_index = 1;
    }
    int j = m_current_node_index;

    NS_ASSERT_MSG(j >= 1 , "j smaller than 1");

    int index = -1;

    Function f = getIdleFunction(j, r.function.type, index);

    if (index != -1){
        //there is cached container
        NS_LOG_INFO("-----placeToCurrent------");
        placeToCurrent(r, f, index, j);
      
    }else{
        //place to this node
        NS_LOG_INFO("-----createNew------");
        createNew(r, j);
      
    }


    m_current_node_index++;
    
    return;
}

void Flame::createToCurrent(Request &r){
    NS_LOG_FUNCTION(this);
    bool succFlag = true;
    int count = 0; //just incase infinitely loop
    Function f;
    //if memory not sufficient and this request priority is higher than the cached one
    if(r.function.size > m_topo.get(r.ingress.id).mem){
        //clear the cache map
        while (count < 1000) {
            count++;

            float p2 = m_cm.getLowestPriority(r.ingress.id);

            if (p2 < r.function.priority){
                succFlag = m_cm.deleteLowestPriority(r.ingress.id, f, m_functionfreq);

                if(succFlag == true){
                    //if success terminate the container
                   // terminate(f); end the traffic application

                    m_topo.update("add", r.ingress.id, f.size);
                }
                
                //keep check the memory, if sufficient
                if (r.function.size <= m_topo.get(r.ingress.id).mem){

                    //create containers
                    
                    r.function.phyNode = r.ingress;

                    m_afs.add(r.function, r.ingress.id, m_functionfreq, m_clock);

                    m_topo.update("minus", r.ingress.id, f.size);

                    r.update(r.function, r.ingress, true);

                    return;

                }
                //no space to delete
                if (succFlag == false){

                    return;
                }
            }//end if
            else{
                //priority too low, just leave it.
                return;
            }

        }//end while
    }//end if
    else{
        //space sufficient
        //create containers

        //TODO: DO priority?

        r.function.phyNode = r.ingress;
        
        m_afs.add(r.function, r.ingress.id, m_functionfreq, m_clock);

        m_topo.update("minus", r.ingress.id, r.function.size);

        r.update(r.function, r.ingress, true);

        return;
    }

    return;
}
//place to cached container
void Flame::placeToCurrent(Request &r, Function f, int index, int phyNodeID){
    NS_LOG_FUNCTION(this);
    //update the freq
    f.calcHotscore(m_functionfreq);

    NS_ASSERT_MSG(phyNodeID <= 125 && phyNodeID > 0 , "phyNodeID excceed");
     //create containers
     PhyNode p = m_topo.get(phyNodeID);

     if(p.id == 0){
         return;
     }
     
    f.phyNode = p;

    m_afs.add(f, phyNodeID, m_functionfreq, m_clock);

    m_cm.remove(phyNodeID, index, m_functionfreq);

    r.update(f, p, false);

}

//TODO: round robin
void Flame::deployRequest(Request &r){
    NS_LOG_FUNCTION(this);

    deployRoundRobin(r);

}

void Flame::updateCache(){
    NS_LOG_FUNCTION(this);

    //iterate over active function, add to cachemap, delete the active functions, because we assume every container finish in one slot
    //it_afs nodefunctions
    for(auto it_afs = m_afs.m.begin(); it_afs != m_afs.m.end(); it_afs++){
        //it_nfs std::map <int, Functions>
        for(auto it_nfs = it_afs->second.functions.begin(); it_nfs != it_afs->second.functions.end(); it_nfs++){
            for(int i = 0; i < it_nfs->second.size(); i++){
                Function f = it_nfs->second.get(i);
                m_cm.add(f, m_functionfreq);//add to cache map
            }//end for

        }// end for
    }//end for

    //clear the activemaop
    m_afs.clear();

    m_cm.sortHotScore();



}

void Flame::deployRequests(int timeslotnum){
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("m_request_map size " << m_request_map.size());

    // float memoryLeft = m_topo.getTotalMem();

    // m_memoryLeft.push_back(memoryLeft);
    //iterate over time slot

    NS_ASSERT_MSG(timeslotnum <= m_request_map.size(), " timeslotnum exceeds the request map size");

    //this only iterate
    // for(int i=1; i<= m_request_map.size(); i++){
    for(int i=1; i<= timeslotnum; i++){
        if(m_request_map.rsMap.find(i) != m_request_map.rsMap.end()){
            auto it_map = m_request_map.rsMap.find(i);
            // Requests rs = it_map->second;

            for(auto it = it_map->second.rs.begin(); it != it_map->second.rs.end(); it++){
                deployRequest(*it);

                m_total_req_num += 1;
            }
        }

        m_clock += 1;

        NS_LOG_INFO("activelist size " << m_afs.size());

        updateCache();

        NS_LOG_INFO("Cache size " << m_cm.size());

        // m_cm.print();

        float memoryLeft = m_topo.getTotalMem();

        m_memoryLeft.push_back(memoryLeft);


    }//end for

   
}


void Flame::init(){

    m_clock = 1;

    m_cold_req_num = 0;

    m_total_req_num = 0;

    m_served_req_num = 0;

    m_req_count = 0; //count to create request

    m_current_node_index = 1;
}

//calculate the avg mem usage
void Flame::calculateMem(){

    float memoryUsage = 0;
    float initMemory = 0;

    for (auto it = m_memoryLeft.begin(); it != m_memoryLeft.end(); it++){
        if (it == m_memoryLeft.begin()){
            initMemory = *it;
            continue;
        }

        memoryUsage = ((initMemory - (*it))/initMemory)*100;

        NS_LOG_INFO("memory usage" << memoryUsage );

        m_memUsage.push_back(memoryUsage);
    }

}


void Flame::printResult(std::string filename){
    NS_LOG_FUNCTION(this);
    //result [time slot, id, type, linkdelay, processingdelay, coldstartdelay, iscoldstart, total delay]
    std::vector<float> result;
    float totalDelay = 0;
    float avg_delay = 0;


    for(int i=1; i<= m_request_map.size(); i++){
        if(m_request_map.rsMap.find(i) != m_request_map.rsMap.end()){
            auto it_map = m_request_map.rsMap.find(i);
            // Requests rs = it_map->second;

            for(auto it = it_map->second.rs.begin(); it != it_map->second.rs.end(); it++){
                //skip if unserved
                if((*it).served == false){
                    continue;
                }

                NS_LOG_LOGIC("request served");

                m_served_req_num += 1;
                
                result.push_back(i);
                result.push_back(float((*it).id));

                result.push_back(float((*it).function.type));

                float linkDelay = distance((*it).ingress.id, (*it).deployNode.id);

                result.push_back(linkDelay);

                (*it).calcLinkDelay(linkDelay);

                totalDelay += linkDelay;
                //conver ms to s
                result.push_back((*it).function.processingTime/1000);
                totalDelay += (*it).function.processingTime/1000;

                if((*it).isColdStart == true){
                    result.push_back((*it).function.coldStartTime);
                    result.push_back(1.0);
                    totalDelay += (*it).function.coldStartTime;

                    m_cold_req_num++;

                }else{
                    
                    result.push_back(0.0);
                    result.push_back(0.0);
                }
                result.push_back(totalDelay);
                avg_delay += totalDelay;
                write_vector_file(filename, result);
                result.clear();
                totalDelay = 0;
            }
        }
    }//end for
    calculateMem();
    //log total numbers:
    write_result_title(filename);
    std::vector<float> numbers;
    numbers.push_back(float(m_cold_req_num));
    numbers.push_back(float(m_served_req_num));
    numbers.push_back(float(m_total_req_num));
    float coldstartfreq = float(m_cold_req_num)/float(m_served_req_num);
    numbers.push_back(coldstartfreq);
    avg_delay = float(avg_delay)/float(m_served_req_num);
    numbers.push_back(avg_delay);
    write_vector_file(filename, numbers);

    write_vector_file(filename, m_memUsage);
    

}

//print result without slot 1
void Flame::printResult_no_1(std::string filename){
    NS_LOG_FUNCTION(this);
    //result [time slot, id, type, linkdelay, processingdelay, coldstartdelay, iscoldstart, total delay]
    std::vector<float> result;
    float totalDelay = 0;

    float cold_req_num_no_1 = 0;
    float served_req_num_no_1 = 0;
    float total_req_num_no_1 = 0;
    float avg_delay = 0;


    for(int i=2; i<= m_request_map.size(); i++){
        if(m_request_map.rsMap.find(i) != m_request_map.rsMap.end()){
            auto it_map = m_request_map.rsMap.find(i);
            // Requests rs = it_map->second;

            for(auto it = it_map->second.rs.begin(); it != it_map->second.rs.end(); it++){

                total_req_num_no_1 += 1;
                //skip if unserved
                if((*it).served == false){
                    continue;
                }

                NS_LOG_LOGIC("request served");

                
                served_req_num_no_1 += 1;

                
                result.push_back(i);
                result.push_back(float((*it).id));
                
                int functype = float((*it).function.type);
                result.push_back(functype);

                NS_ASSERT_MSG(functype <= 10 && functype >= 1,"result push wrong function type " << functype);

                float linkDelay = distance((*it).ingress.id, (*it).deployNode.id); //inseconds

                result.push_back(linkDelay);

                (*it).calcLinkDelay(linkDelay);

                totalDelay += linkDelay;
                //conver ms to s
                result.push_back((*it).function.processingTime/1000);
                totalDelay += (*it).function.processingTime/1000;

                if((*it).isColdStart == true){
                    result.push_back((*it).function.coldStartTime);
                    result.push_back(1.0);
                    totalDelay += (*it).function.coldStartTime;

                    
                    cold_req_num_no_1 += 1;
                    
                    
                }else{
                    
                    result.push_back(0.0);
                    result.push_back(0.0);
                }
                result.push_back(totalDelay);
                avg_delay += totalDelay;
                write_vector_file(filename, result);
                result.clear();
                totalDelay = 0;
            }
        }
    }//end for
   
    //log total numbers:
    write_result_title(filename);
    std::vector<float> numbers;
    numbers.push_back(cold_req_num_no_1);
    numbers.push_back(served_req_num_no_1);
    numbers.push_back(total_req_num_no_1);
    float coldstartfreq = cold_req_num_no_1/served_req_num_no_1;
    numbers.push_back(coldstartfreq);
    avg_delay = avg_delay/served_req_num_no_1;
    numbers.push_back(avg_delay);
    write_vector_file(filename, numbers);
    calculateMem();
    write_vector_file(filename, m_memUsage);
    
    

}

void Flame::genTraffic(int time_slot, int time_slot_num){
    NS_LOG_FUNCTION(this);

    Network_controller nc = Network_controller();
    //data rate can not be too small otherwise the packet will not arrive
    nc.set_map(m_topo, m_cfg, "1kBps", "1kbps");

    nc.set_max_slot(time_slot, time_slot_num);

    nc.create_network();


    //for loop request

    //iterate over time slot
    for(int i=1; i<= m_request_map.size(); i++){
        if(m_request_map.rsMap.find(i) != m_request_map.rsMap.end()){
            auto it_map = m_request_map.rsMap.find(i);
            // Requests rs = it_map->second;

            for(auto it = it_map->second.rs.begin(); it != it_map->second.rs.end(); it++){
                nc.create_request_traffic(*it);

            }
        }


    }//end for
    NS_LOG_DEBUG("create sink m_request_map size " << m_request_map.size());
    nc.create_sink(m_request_map);

}

void Flame::scheduleRequests(float beta_input, float reduFactor_input){
    loadConfig("config/config.txt");

    m_cfg.Beta = beta_input;
    m_cfg.ReduFactor = reduFactor_input;

    loadTopo();
    init();
    initFuncMap();
    float totalMem = m_topo.getTotalMem(); 

    m_memoryLeft.push_back(totalMem);

    readRequests();

    createRequests();

    deployRequests(30);

    //output data
    std::string beta, reducefactor;
    beta = std::to_string(m_cfg.Beta);
    reducefactor = std::to_string(m_cfg.ReduFactor);
    beta.erase(beta.find_last_not_of('0')+1, std::string::npos);
    beta.erase(beta.find_last_not_of('.')+1, std::string::npos);
    reducefactor.erase(beta.find_last_not_of('0')+1,  std::string::npos);
    reducefactor.erase(beta.find_last_not_of('.')+ 1, std::string::npos);
    std::string filename = "result/flame-result-"+ beta +"-" + reducefactor + ".csv";
    write_time(filename, m_cfg);
    printResult_no_1(filename);
    printResult(filename);
    //end output data
    
    //TODO: Traffic and record result

    NS_LOG_INFO("turn on printResult() this will show");

    NS_LOG_INFO("Total Cold start Num served<< " << m_cold_req_num);

    NS_LOG_INFO("Total Request Num served<< " << m_served_req_num);

    NS_LOG_INFO("Total Request Num << " << m_total_req_num);

    // genTraffic(1, 30);
}

}//end ns3