#ifndef FAASCACHE_H
#define FAASCACHE_H

#include "ns3/core-module.h"
#include "type.h"
using namespace ns3;

namespace ns3 {

class Faascache
{
public:
  Faascache ();

  virtual ~Faascache();

  bool loadConfig(std::string filename);

  void loadTopo();

  void loadRequest();

  std::map<int, int> genZipfNum(int num_values, float alpha, int max_num);

  void init();

  void initFuncMap();

  bool createRequest(int timeSlot, int funcType, int ingressID, Request &r);

  void createRequestInSlot(int timeSlot, int funcType, ReqOnNodes ron);

  void createRequests();

  void readRequests();

  float distance(int node_1, int node_2);

  DistSlice sortPhyNodes(Request r);

  void placeToNeighbour(Request &r, Function function, int index, int phynodeID);

  bool deployToNeighbour(DistSlice ds, Request &r);

  void createToCurrent(Request &r);

  void placeToCurrent(Request &r, Function f, int index, int phynodeID);

  void deployRequest(Request &r);

  void deployRequests(int timeslonum);

  void scheduleRequests(float beta_input, float reduFactor_input);

  Function getIdleFunction(int phynodeID, int funcType, int &index);

  void updateCache();

  void printResult(std::string filename);

  void printResult_no_1(std::string filename);

  void genTraffic(int time_slot, int time_slot_num);

  void createNew(Request &r, int phyNodeID);

  void deployRoundRobin(Request &r);

  void calculateMem();

  ServerlessConfig m_cfg;

  
  
  //the map for nodes <nodeID, Location>
  std::map<int, Location> m_map;
  
  // <function type, reqNumMap>
  std::map<int, ReqNumMap> m_req_num;

  RequestsMap m_request_map; //all the request here

  int m_req_count;
  
  //function map stores function info
  FunctionInfoMap m_funcInfoMap;

  ReqOnNodesTimeType m_rontt;

  CacheMap m_cm;

  ActiveFunctions m_afs;

  Topology m_topo;

  int m_clock;

  FunctionFreq m_functionfreq;

  int m_cold_req_num;

  int m_total_req_num;

  int m_served_req_num;

  int m_current_node_index;

  float m_unused_mem;

  std::vector<float> m_memoryLeft; // first one is the total memory

  std::vector<float> m_memUsage; // first one is the total memorys


};//END class Faascache



} //end ns3 namespace

#endif /* FAASCACHE_H */