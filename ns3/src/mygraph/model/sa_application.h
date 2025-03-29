/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
Simulated annealing as a centralised approach to solve the sfc scaling problem.
author: chen chen
date: 04.10.2021

*/
#ifndef SA_APPLICATION_H
#define SA_APPLICATION_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include <string>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/properties.hpp>

#include "sfc.h"
#include "myGraph.h"

#include <cstdlib>
#include <ctime>


using namespace ns3;


namespace ns3 {





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
class Vnf_host{
    public:
    Vnf_host(){

    };

    void create_random_placement(int max_instance, int max_node){

        srand(time(0));

        m_array[0] = 1 + rand() % max_instance;  // [1, max_instance]

        m_array[1] = 1 + rand() % max_node; // [1, max_node]

    };

    void assign_placement(int instance_id, int node_id){

        m_array[0] = instance_id;

        m_array[1] = node_id;
    };

    void assign_host(int node_id){
        m_array[1] = node_id;
    }

    int get_instance_id(){

        return m_array[0];
    };

    int get_host(){

        return m_array[1];
    };

    // two numbers, for example [1,2] means use the first instance on node v.
    int m_array[2];
};

class Vnf_host_map{
    public:
    Vnf_host_map(){

    };

    void insert(int vnf_order_id,  Vnf_host v_h)
    {
        m_array_map.insert(std::pair<int, Vnf_host>(vnf_order_id, v_h));//starts from 0
    };


    int get_size(){
        return m_array_map.size();
    };

    Vnf_host get_map(int vnf_order_id){

        Vnf_host v_h;

        auto it =  m_array_map.find(vnf_order_id);

        if(it != m_array_map.end()){

            v_h =  it->second;
        }

        return v_h;
    };

    std::map <int, Vnf_host>::iterator find(int vnf_order_id){

        auto it =  m_array_map.find(vnf_order_id);

        return it;
    };

    //get the host nodes
    std::vector<int> get_host_nodes(){

        std::vector<int> host_node_v;

        for(auto it = m_array_map.begin(); it != m_array_map.end(); it++){

            int host_id = (it->second).get_host();

            host_node_v.push_back(host_id);
        }

        return host_node_v;

    };

    void set_host_nodes(std::vector<int> host_node_v){
        
        int host_id = 0;

        int i = 0;

        for(auto it = m_array_map.begin(); it != m_array_map.end(); it++){

            host_id = host_node_v.at(i);

            (it->second).assign_host(host_id);

            i++;
        }


    }

    


    // int is the id of vnf_order_id
    std::map <int, Vnf_host> m_array_map;
};

class Flow_map{
    public:
    Flow_map(){

    };

    void insert(int flow_id, Vnf_host_map v_h_map)
    {
        m_flow_map.insert(std::pair<int, Vnf_host_map>(flow_id, v_h_map));
    };

    int get_size(){
        return m_flow_map.size();
    }

    void clear(){
        m_flow_map.clear();
    }

    Vnf_host_map get_map(int flow_id){

        Vnf_host_map v_h_map;

        auto it =  m_flow_map.find(flow_id);

        if(it != m_flow_map.end()){

            v_h_map =  it->second;
        }

        return v_h_map;
    }

    std::map<int, Vnf_host_map>::iterator find(int flow_id){

        auto it =  m_flow_map.find(flow_id);

        return it;

    }

    // int is the id of  flow
    std::map <int, Vnf_host_map> m_flow_map;
};



class SA_result
{
    public:
    SA_result(){

    };

    void insert(int request_id, Flow_map f_map){
        m_request_map.insert(std::pair<int, Flow_map>(request_id, f_map));
    };

    void clear(){
        m_request_map.clear();
    };

    int get_size(){
        return m_request_map.size();
    };

    Flow_map get_map(int request_id){

        Flow_map f_map;

        auto it =  m_request_map.find(request_id);

        if(it != m_request_map.end()){

            f_map =  it->second;
        }

        return f_map;
    };

    void update(int request_id, int flow_id, int vnf_order_id, int new_host_id){

        auto it = m_request_map.find(request_id);

        auto it_f = (it->second).find(flow_id);

        auto it_vnf = it_f->second.find(vnf_order_id);

        (it_vnf->second).assign_host(new_host_id);

    };

    std::pair<int, int> get_instance_host(int request_id, int flow_id, int vnf_order_id){

        auto it = m_request_map.find(request_id);

        auto it_f = (it->second).find(flow_id);

        auto it_vnf = it_f->second.find(vnf_order_id);

        int host_id = (it_vnf->second).get_host();

        int instance_id = (it_vnf->second).get_instance_id();

        return std::make_pair(instance_id, host_id);

    };
    
    // int is the request id
    std::map<int, Flow_map> m_request_map;
       
};



class SA_application : public Application{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  SA_application ();

  virtual ~SA_application ();

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
  void init(std::vector<sfc::Request> r_v, MyGraph g);
  
  void init(sfc::Sfc_vec r_v, MyGraph g);

  int get_max_instance(sfc::Vnf n, int flow_num, int total_traffic);

  Flow_map create_flows_placement(sfc::Request r);

  Flow_map create_optimal_flows_placement(sfc::Request r);

  void create_init_solution();

  int get_host_nodes_delay( std::vector<int> host_node_v);

  int get_flow_link_delay(Vnf_host_map v_h_map, int src, int dst);

  double get_flow_function_delay(sfc::Request r);

  double get_request_delay(Flow_map f_map, sfc::Request r);

  double get_average_delay(SA_result sa_result);

  double get_dE( sfc::Request r, Vnf_host_map v_h_map_old, Vnf_host_map v_h_map_new);

  double move_to_neighbour();

  double move_to_short();

  double ran_double();

  std::vector<int> get_path(std::vector<int> host_node_v, sfc::Request r);

  bool check_request_resource(Flow_map f_map, sfc::Request r, int& fail_edge_num);

  bool check_solution(SA_result sa_result, int& fail_edge_num );

  void get_flow_result(Flow_map f_map, sfc::Request& r);

  void get_all_result();

  double get_instance_num();

  double get_mean_instance_utilization();

  double get_mean_node_util();

  double get_mean_edge_util();

  std::vector<double> get_every_edge_util();

  void update_graph(MyGraph& g);

  void main(double temp, double coolingRate, int kNum, std::string filename, std::string is_test = "non-test");

//   std::pair<int,int> get_overload_edge(int src, int dst, int required_bw);

//   void get_overload_path(std::vector<int> routing_path, int required_bw);

//   void remove_edges_tmp();

  void move_overloaded_requests(int k_num);

  bool minus_request_resource(Flow_map f_map, sfc::Request r);

  bool move_flow_map(Flow_map& f_map, sfc::Request r, int k_num, int request_id);

  double get_penalty(double p_factor, int fail_edge_num);

  void update_request_succ();

  void write_vector_file( std::string filename, std::string s, std::vector<int> data = std::vector<int>());

  void print_result(std::string filename);

  void write_count( std::string filename, std::string s, double count );

  double get_throughput();

  void write_time(std::string filename);

  bool move_flow_to_path(Vnf_host_map& v_h_map, std::vector<int> p);

  bool move_request_to_path(int k_num);

  void release_request_bw(Flow_map f_map , sfc::Request r);

  void create_test_solution();

  Flow_map create_test_flows_placement(sfc::Request r, std::vector<int> host_node_v);

  bool check_nodes_cap(std::vector<int> host_node_v , sfc::Request r);

  void release_nodes_cap(std::vector<int> host_node_v , sfc::Request r);

  void release_request_cap(Flow_map f_map , sfc::Request r);

  bool minus_nodes_cap(std::vector<int> host_node_v , sfc::Request r, MyGraph& g);

  bool check_if_release(sfc::Request& r, int time);

  bool check_if_deploy(sfc::Request& r, int time);

  void check_requests(int time);

  void release_flow_bw(sfc::Flow f);

  void release_flow(sfc::Flow f);

  void release_requests();

  void main_online(int time, double temp, double coolingRate, int kNum, std::string filename, std::string is_test= "non-test");

  double get_instance_num_all();

  double get_instance_util(int time);

  double get_mean_hops(int time);

  std::vector<sfc::Request> m_result_request_v;//store the result of one offline result

  std::vector<sfc::Request> m_request_v; // for online, this is the result to be deployed, for offline, this is all request

  std::vector<sfc::Request> m_request_v_all; //all request

  std::vector<sfc::Request> m_request_release;

  std::vector<sfc::Request> m_result_request_v_all;//store the result of all time
  



protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * \brief Send a packet
   */
  void Send (void);

  void create_optimal_solution();

  uint32_t m_count; //!< Maximum number of packets the application will send
  Time m_interval; //!< Packet inter-send time
  uint32_t m_size; //!< Size of the sent packet (including the SeqTsHeader)

  uint32_t m_sent; //!< Counter for sent packets
  Ptr<Socket> m_socket; //!< Socket
  Address m_peerAddress; //!< Remote peer address
  uint16_t m_peerPort; //!< Remote peer port
  EventId m_sendEvent; //!< Event to send the next packet


  

  double m_Fmin;//best value in sa

  double m_new_delay;

  int m_flow_num; //numbef of all flows

  MyGraph m_graph;// 

  MyGraph m_tmp_graph; //modify this graph first as tmp data structure

  MyGraph m_min_graph; //keep this copy as the f_min status

  SA_result m_sa_result;

  SA_result m_tmp_sa_result; // keep the new solution

  MyGraph m_new_graph; // keep the status of the new solution

  std::vector<int> m_overloaded_r; // the sfc id that fail check_solution()

  std::vector<int> m_min_overloaded_r; // keep this copy as f_min status

  
  std::vector<std::pair<int,int>> m_overload_edges; // all the overloaded edges



  

 

  //RESULT for the request
  sfc::Request_instance_map m_request_instance_map;

  //a copy of result, modify this first
  sfc::Request_instance_map m_tmp_request_instance_map;

  //Result for one time plot, only contains result for a single time slot
  sfc::Request_instance_map m_slot_instance_map;

  //count
  int m_minus_bw_fail;

  int m_check_r_bw_fail;

  int m_request_num_count;

  //store the request that be modified at once
  std::map<int, int> m_modified_count;
  
  //store how many request has been modified
  int m_loop_count;

  double m_instance_num; // the overall instance num for online


};

}

#endif /* SA_APPLICATION_H */

