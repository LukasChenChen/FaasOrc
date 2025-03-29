/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DIS_APPLICATION_H
#define DIS_APPLICATION_H


#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include <string>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/properties.hpp>

#include "sfc.h"
#include "myGraph.h"


using namespace ns3;


namespace ns3 {

enum FLOW_STATE{NEED_DEPLOY, NEED_RELEASE, SKIP };

enum REQUEST_STATE{SUCC, NONE, ERROR}; // NEED DEPLOY, NOTHING, ERROR




/* ... */

class Socket;
class Packet;

/**
 * 
 *
 * \brief A distributed sfc deploy application, deploy a single request.
    \brief input a time slot count to simulate time slot manner. if hit, starts to deploy or release.
 *
 */

class Dis_application : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  Dis_application ();

  virtual ~Dis_application ();

  /**
   * \brief set the remote address and port
   * \param ip remote IP address
   * \param port remote port
   */
  void SetRemote (Address ip, uint16_t port);
  /**
   * \brief set the remote address
   * \param addr remote address
   */
  void SetRemote (Address addr);
  //------------------------------------

  //initialize the application, get the state of the graph
  void init(int time, sfc::Request r, MyGraph g, int k_num);

  // check the request in the request
  // bool check_request(); 

  // check the flow in the request
  FLOW_STATE check_flow(sfc::Flow f); 

  bool deploy_flow_single_path(Path p,sfc::Vnf n);

  bool deploy_flow(sfc::Flow f);

  void get_total_traffic();

  bool check_flows();

  bool scale_up(Path p,sfc::Vnf n, int vnf_order_id);

  bool scale_out(Path p, sfc::Vnf n, int vnf_order_id);

  bool place(Path p,sfc::Vnf n, int vnf_order_id);

  int get_least_used_instance(int vnf_order_id);

  std::vector<int> get_all_least_used_instance();

  bool steer_traffic_one_flow(sfc::Flow& f, std::vector<int> instance_id_v);

  bool steer_traffic();

  void release_node( sfc::Sf_instance sf);

  bool release_instance(int vnf_order_id, int instance_id);

  void release_flow_instance(sfc::Flow f, int vnf_order_id, int instance_id);

  void release_flow_bw(sfc::Flow f);

  void release_flow(sfc::Flow& f);

  void release_flows();

  void init_flow(sfc::Flow& f);

  void init_flows();

  bool deploy_functions();

  std::pair<int,int> get_pair_instance(int vnf_order_id, int instance_id, int previous_instance_id);

  bool select_flow_path(sfc::Flow& f, int vnf_order_id, int instance_id, int previous_instance_id );

  void save(MyGraph& g, sfc::Request& r);

  void modify_flows();

  double get_instance_num();

  double get_mean_instance_utilization();

  double get_mean_node_util();

  double get_mean_edge_util();

  std::vector<double> get_graph_bw();

  REQUEST_STATE main(int time, sfc::Request& r, MyGraph& g, int k_num);

  REQUEST_STATE main_offline( sfc::Request& r, MyGraph& g, int k_num );

  void print_hello();

  void show_flows_path();

  double get_functional_delay();

  double get_bound_delay(bool enable_function);

  double get_bound_throughput();

  double get_bound_hops();

  





  
  



protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * \brief Send a packet
   */
  void Send (void);

  uint32_t m_count; //!< Maximum number of packets the application will send
  Time m_interval; //!< Packet inter-send time
  uint32_t m_size; //!< Size of the sent packet (including the SeqTsHeader)

  uint32_t m_sent; //!< Counter for sent packets
  Ptr<Socket> m_socket; //!< Socket
  Address m_peerAddress; //!< Remote peer address
  uint16_t m_peerPort; //!< Remote peer port
  EventId m_sendEvent; //!< Event to send the next packet

  int m_time;

  sfc::Request m_request;

  MyGraph m_graph;// 

  MyGraph m_tmp_graph; //modify this graph first as tmp data structure

  sfc::Flows m_flows;

  sfc::Flows m_flows_to_deploy;

  sfc::Flows m_flows_to_release;

  std::vector<Path> m_k_shortest_path;

  int m_total_traffic;

  //RESULT for the request
  sfc::Request_instance_map m_request_instance_map;

  //a copy of result, modify this first
  sfc::Request_instance_map m_tmp_request_instance_map;

  //Result for one time plot, only contains result for a single time slot
  sfc::Request_instance_map m_slot_instance_map;

  int m_k_num;



};

}

#endif /* DIS_APPLICATION_H */

