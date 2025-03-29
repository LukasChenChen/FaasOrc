/*
Simulated annealing approach,
use horizontal scaling/placement
author:chen chen
17.10.2021
*/
#include "sa_application.h"
#include <ctime>
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include <time.h>

#include "ns3/seq-ts-header.h"
#include <cstdlib>
#include <cstdio>
#include <cmath>

#include "sfc.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/properties.hpp>

#include <chrono>  // chrono::system_clock
#include <ctime>   // localtime
#include <iomanip> // put_time

#define EPISON 1e-7

namespace ns3 {

/* ... */
NS_LOG_COMPONENT_DEFINE ("SA_application");

NS_OBJECT_ENSURE_REGISTERED (SA_application);

TypeId
SA_application::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SA_application")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<SA_application> ()
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&SA_application::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval",
                   "The time to wait between packets", TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&SA_application::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&SA_application::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort", "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&SA_application::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize",
                   "Size of packets generated. The minimum packet size is 12 bytes which is the size of the header carrying the sequence number and the time stamp.",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&SA_application::m_size),
                   MakeUintegerChecker<uint32_t> (12,65507))
  ;
  return tid;
}

SA_application::SA_application ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_socket = 0;
  m_sendEvent = EventId ();

}

SA_application::~SA_application ()
{
  NS_LOG_FUNCTION (this);
}

void
SA_application::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
SA_application::SetRemote (Address addr)
{
  NS_LOG_FUNCTION (this << addr);
  m_peerAddress = addr;
}

void
SA_application::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
SA_application::StartApplication (void)
{
  
}

void SA_application::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_sendEvent);
}

void SA_application::Send (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());
  SeqTsHeader seqTs;
  seqTs.SetSeq (m_sent);
  Ptr<Packet> p = Create<Packet> (m_size-(8+4)); // 8+4 : the size of the seqTs header
  p->AddHeader (seqTs);

  std::stringstream peerAddressStringStream;
  if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
      peerAddressStringStream << Ipv4Address::ConvertFrom (m_peerAddress);
    }
  else if (Ipv6Address::IsMatchingType (m_peerAddress))
    {
      peerAddressStringStream << Ipv6Address::ConvertFrom (m_peerAddress);
    }

  if ((m_socket->Send (p)) >= 0)
    {
      ++m_sent;
      NS_LOG_INFO ("TraceDelay TX " << m_size << " bytes to "
                                    << peerAddressStringStream.str () << " Uid: "
                                    << p->GetUid () << " Time: "
                                    << (Simulator::Now ()).GetSeconds ());

    }
  else
    {
      NS_LOG_INFO ("Error while sending " << m_size << " bytes to "
                                          << peerAddressStringStream.str ());
    }

  if (m_sent < m_count)
    {
      m_sendEvent = Simulator::Schedule (m_interval, &SA_application::Send, this);
    }
}



void SA_application::init(std::vector<sfc::Request> r_v, MyGraph g)
{
m_request_v_all = r_v;
m_request_v = r_v;
m_graph = g;
m_tmp_graph = g;
m_instance_num = 0;

}

// for a vnf n , create a random solution
int SA_application::get_max_instance(sfc::Vnf n, int flow_num, int total_traffic)
{
  int instance_num = ceil(double(total_traffic)/double(n.m_cap));

  if(flow_num > instance_num)
  {
      instance_num = flow_num;
  }

  return instance_num;

}


//randomly create a placement for all flows in one request
Flow_map SA_application::create_flows_placement(sfc::Request r)
{   
    // srand(time(NULL));

    int max_node = m_graph.get_max_node();

    int instance_num = 0;

    Vnf_host v_h = Vnf_host();

    int flow_id = 0;

    Flow_map f_map = Flow_map();

    //random a solution for each flow, dont overlap.    
    for(auto it_f = r.m_flows.m_flows.begin(); it_f != r.m_flows.m_flows.end(); it_f++)
    {   

      int vnf_order_id = 0;

      Vnf_host_map v_h_map = Vnf_host_map();

    for(auto it = r.m_chain.begin(); it != r.m_chain.end(); it++)
    {   
      Vnf_host v_h = Vnf_host();

      instance_num = get_max_instance(*it, r.m_flows.get_size(), r.get_total_traffic());

      if(flow_id >= instance_num){

          NS_LOG_ERROR("create flow placement flow id exceed");
      }

        int node_id = 1 + rand() % max_node;

        int count = 0;

        while(m_tmp_graph.check_node_cap( *it,node_id) == false && count < 100 )
        {
            node_id = 1 + rand() % max_node; // try again
            count++;
        }

        // m_tmp_graph.minus_node_cap(node_id, (*it).m_cap);

      

        // assign to the first instance then second and so on, flow id is smaller than instance number anyway
        v_h.assign_placement(flow_id, node_id);

        //check the node capacity

        v_h_map.insert(vnf_order_id, v_h);

        NS_LOG_FUNCTION("v_h_map insert vnf_order_id ->" << vnf_order_id << "instance id->"<< flow_id <<"node_id ->" << node_id);

        vnf_order_id++;
    }

    f_map.insert(flow_id, v_h_map);

    NS_LOG_FUNCTION(" insert -> flow_id -> " << flow_id );

    flow_id++;

  }
  NS_LOG_FUNCTION(" f_map size -> " << f_map.get_size() );

  return f_map;


}

//randomly create a placement for all flows in one request based on shortest path
Flow_map SA_application::create_optimal_flows_placement(sfc::Request r)
{   

    std::vector<int> p =  m_graph.get_short_path( r.m_source, r.m_target);

    int instance_num = 0;

    Vnf_host v_h = Vnf_host();

    int flow_id = 0;

    Flow_map f_map = Flow_map();

    //random a solution for each flow, dont overlap.    
    for(auto it_f = r.m_flows.m_flows.begin(); it_f != r.m_flows.m_flows.end(); it_f++)
    {   

        int vnf_order_id = 0;

        Vnf_host_map v_h_map = Vnf_host_map();

        int node_count = 0;

        for(auto it = r.m_chain.begin(); it != r.m_chain.end(); it++)
        {   
            Vnf_host v_h = Vnf_host();

            instance_num = get_max_instance(*it, r.m_flows.get_size(), r.get_total_traffic());

            if(flow_id >= instance_num){

                NS_LOG_ERROR("create flow placement flow id exceed");
            }

            int node_id;
            
            if(node_count < p.size()){

              node_id  = p.at(node_count);

            }
            else
            {
              node_id  = p.back();
            }
            
            node_count++;   

            // assign to the first instance then second and so on, flow id is smaller than instance number anyway
            v_h.assign_placement(flow_id, node_id);

            //check the node capacity

            v_h_map.insert(vnf_order_id, v_h);

            NS_LOG_FUNCTION("v_h_map insert vnf_order_id ->" << vnf_order_id << "instance id->"<< flow_id <<"node_id ->" << node_id);

            vnf_order_id++;
        }

        f_map.insert(flow_id, v_h_map);

        NS_LOG_FUNCTION(" insert -> flow_id -> " << flow_id );

        flow_id++;

    }
    NS_LOG_FUNCTION(" f_map size -> " << f_map.get_size() );

    return f_map;


}


//randomly create a placement for all request
void SA_application::create_init_solution()
{   
  m_sa_result.clear();

  int request_id = 0;

  for(auto it = m_request_v.begin(); it != m_request_v.end(); it++){

      Flow_map f_map = create_flows_placement(*it);

      m_sa_result.insert(request_id, f_map);

      NS_LOG_FUNCTION("f_map size: " << f_map.get_size());

      request_id++;
  }

}

//create solution for 2 request
Flow_map SA_application::create_test_flows_placement(sfc::Request r, std::vector<int> host_node_v)
{   
    // srand(time(NULL));

    int max_node = m_graph.get_max_node();

    int instance_num = 0;

    Vnf_host v_h = Vnf_host();

    int flow_id = 0;

    Flow_map f_map = Flow_map();

    //random a solution for each flow, dont overlap.    
    for(auto it_f = r.m_flows.m_flows.begin(); it_f != r.m_flows.m_flows.end(); it_f++)
    {   

        int vnf_order_id = 0;

        Vnf_host_map v_h_map = Vnf_host_map();


        for(auto it = r.m_chain.begin(); it != r.m_chain.end(); it++)
        {   
            Vnf_host v_h = Vnf_host();

            instance_num = get_max_instance(*it, r.m_flows.get_size(), r.get_total_traffic());

            if(flow_id >= instance_num){

                NS_LOG_ERROR("create flow placement flow id exceed");
            }

            int node_id = host_node_v.at(vnf_order_id);

            int count = 0;
        
            // assign to the first instance then second and so on, flow id is smaller than instance number anyway
            v_h.assign_placement(flow_id, node_id);

            //check the node capacity

            v_h_map.insert(vnf_order_id, v_h);

            NS_LOG_FUNCTION("v_h_map insert vnf_order_id ->" << vnf_order_id << "instance id->"<< flow_id <<"node_id ->" << node_id);

            vnf_order_id++;
        }

        f_map.insert(flow_id, v_h_map);

        NS_LOG_FUNCTION(" insert -> flow_id -> " << flow_id );

        flow_id++;

    }
    NS_LOG_FUNCTION(" f_map size -> " << f_map.get_size() );

    return f_map;


}

//create two solutionbs, one can be deployed one cannot.
void SA_application::create_test_solution()
{   
    int request_id = 0;

    std::vector<int> host_node_v = {10, 32 , 47};

    for(auto it = m_request_v.begin(); it != m_request_v.end(); it++){

        Flow_map f_map = create_test_flows_placement(*it, host_node_v);

        m_sa_result.insert(request_id, f_map);

        NS_LOG_FUNCTION("f_map size: " << f_map.get_size());

        request_id++;
    }

}

void SA_application::create_optimal_solution()
{   
    int request_id = 0;

    for(auto it = m_request_v.begin(); it != m_request_v.end(); it++){

        Flow_map f_map = create_optimal_flows_placement(*it);

        m_sa_result.insert(request_id, f_map);

        NS_LOG_FUNCTION("f_map size: " << f_map.get_size());

        request_id++;
    }

}



//get delay traverses these nodes by using shortest path
int  SA_application::get_host_nodes_delay( std::vector<int> host_node_v)
{ 
  int total_delay = 0;

  if(host_node_v.size() < 2)
  {
    return total_delay;
  }

  int src,dst;

  for(auto it = host_node_v.begin(); it != host_node_v.end()-1; it++ )
  {
    src = *it;

    dst = *(it+1);

    total_delay += m_graph.get_short_path_delay(src, dst);
  }

  NS_LOG_FUNCTION("link delay -> " << total_delay);

  return total_delay;
}

//delay for a single flow
int SA_application::get_flow_link_delay(Vnf_host_map v_h_map, int src, int dst)
{ 
  int total_delay = 0;

  std::vector<int> host_node_v;

  host_node_v.push_back(src);

  for(int i = 0; i < v_h_map.m_array_map.size(); i++){

      auto it = v_h_map.m_array_map.find(i);

      if(it != v_h_map.m_array_map.end()){

          Vnf_host v_h = it->second;

          int host_node = v_h.get_host();

          host_node_v.push_back(host_node);
      }
  }

  host_node_v.push_back(dst);


  return get_host_nodes_delay(host_node_v);

}
//fucntion delay one flow
double SA_application::get_flow_function_delay(sfc::Request r)
{ 
  double total_function_delay = 0;

  for(auto it = r.m_chain.begin(); it != r.m_chain.end(); it++)
  {
    total_function_delay += (*it).m_processing_time;
  }

  NS_LOG_FUNCTION("function_delay -> " << total_function_delay/1000);

  if(std::isnan(total_function_delay))
  {
    total_function_delay = 0;
  }

  return total_function_delay/1000; //in miliseconds
}

double SA_application::get_request_delay(Flow_map f_map, sfc::Request r)
{
  int src = r.m_source;

  int dst = r.m_target;

  double total_delay = 0;

  NS_LOG_FUNCTION("f_map size: " << f_map.get_size());

  for(auto it = f_map.m_flow_map.begin(); it != f_map.m_flow_map.end(); it++)
  {
    total_delay += get_flow_link_delay(it->second, src, dst);

    //get functional delay
    total_delay += get_flow_function_delay(r);

  }

  NS_LOG_FUNCTION("request delay: " << total_delay);

  return total_delay;
}

//get the objective value, in our case, the average delay of each flow
double SA_application::get_average_delay(SA_result sa_result)
{ 
  int request_id = 0;

  double total_delay = 0;

  int flow_num = 0;

  NS_LOG_FUNCTION("m sa result size: " << sa_result.get_size() );

  for(auto it = sa_result.m_request_map.begin(); it != sa_result.m_request_map.end(); it++)
  {

    NS_LOG_FUNCTION("enter loop");

    NS_LOG_FUNCTION("m sa result size: " << sa_result.get_size() );


    Flow_map f_map = it->second;

    NS_LOG_FUNCTION("f_map size: " << f_map.get_size() << "request id " << it->first);

    sfc::Request r = m_request_v.at(request_id);  

    //if it is not in the overloaded request.
    if(std::find(m_overloaded_r.begin(), m_overloaded_r.end(), request_id) == m_overloaded_r.end()){

      total_delay += get_request_delay(f_map, r);

      flow_num += r.get_flow_num();
    }

    request_id++;

    
  }

  NS_LOG_FUNCTION("Average flow delay is : " <<  total_delay/double(flow_num));

  m_flow_num = flow_num;

  return total_delay/double(flow_num);
  

}

//get the difference value by only calculate the difference
//de = Fmin - new_delay;
//new delay = old_delay + (-original flow delay + new flow_delay)/number of flows
double SA_application::get_dE( sfc::Request r, Vnf_host_map v_h_map_old, Vnf_host_map v_h_map_new){

// dont need this actually, because function delay is the same
//get_flow_function_delay(r);

double old_link_delay = get_flow_link_delay(v_h_map_old, r.m_source, r.m_target);
double new_link_delay = get_flow_link_delay(v_h_map_new, r.m_source, r.m_target);

double F_new = m_Fmin + (new_link_delay - old_link_delay)/m_flow_num;

double dE = m_Fmin - F_new;

m_new_delay = F_new;


return dE;

}


// move instance to its neighbour node
// random a reuqest id, random get a flow, random get a vnf_order_id, change the host node to its neighbour
double SA_application::move_to_neighbour(){

  // srand(time(0));

  int ran_r_id =  rand()%(m_request_v.size()); // [0, request num]

  m_tmp_sa_result = m_sa_result; // cop the current solution

  Flow_map f_map = m_tmp_sa_result.get_map(ran_r_id);

  int f_map_size = f_map.get_size();

  int ran_f_id =  rand() % f_map_size; // [0, f_map_size-1]

  Vnf_host_map v_h_map = f_map.get_map(ran_f_id); // get aq random flow mapping

  int host_map_size = v_h_map.get_size();

  int rand_vnf_id =  rand() % host_map_size; // [0, f_map_size-1]

  Vnf_host v_h = v_h_map.get_map(rand_vnf_id);

  //now move to a neighbour

  int host_id = v_h.get_host();

  std::vector<int> neighbours = m_graph.get_neighbours(host_id);


  //random get a neighbour from it.

  int ran_id =  rand() % neighbours.size(); // [0, f_map_size-1]

  int new_host_id = neighbours.at(ran_id);


  // assign this new host to the tmp sa result.

  m_tmp_sa_result.update(ran_r_id, ran_f_id,  rand_vnf_id, new_host_id);

  NS_LOG_INFO(ran_r_id << " -> " << ran_f_id << " -> " << rand_vnf_id << " -> " << new_host_id << " OLD HOST ID " << host_id);


  NS_LOG_FUNCTION(" check the map host -> " << m_tmp_sa_result.get_instance_host(ran_r_id, ran_f_id,  rand_vnf_id).second);

  Flow_map f_map_new = m_tmp_sa_result.get_map(ran_r_id);

  Vnf_host_map v_h_map_new = f_map_new.get_map(ran_f_id);

  double dE = get_dE(m_request_v.at(ran_r_id) ,v_h_map, v_h_map_new);

  return dE;
}

//move the flow to a random node
// void SA_application::move_flow_to_ran(std::vector<int>& host_node_v, int required_bw){

//   int ran_r_id = 0;

//   int ran_host_id = 0;

//   for(auto it = host_node_v.begin(); it != host_node_v.end(); it++){

//     ran_r_id =  rand()%(host_node_v.size()); 

//     ran_host_id = 1+rand()%(m_tmp_graph.get_max_node());

//     host_node_v.at(ran_r_id) = ran_host_id;

//     m_tmp_graph.check_path_bw(host_node_v, required_bw);

//   }

  

// }

//  move first then check bw, then go next
//move flow to a path, need check bw
bool SA_application::move_flow_to_path(Vnf_host_map& v_h_map, std::vector<int> p){

  Vnf_host_map v_h_map_copy = v_h_map;

  auto it = p.begin();

  auto it_h = v_h_map_copy.m_array_map.begin();

  while(it != p.end() && it_h != v_h_map_copy.m_array_map.end()){

    (it_h->second).assign_host(*it);

    it++;

    it_h++;

    if(it_h == v_h_map_copy.m_array_map.end()){

      v_h_map = v_h_map_copy;

      return true;
    }

  }

  return false;

}


void SA_application::release_request_bw(Flow_map f_map , sfc::Request r){

  for(auto it = f_map.m_flow_map.begin(); it != f_map.m_flow_map.end(); it++){

    std::vector<int> host_node_v = it->second.get_host_nodes();

    std::vector<int> routing_path = get_path(host_node_v, r);

    sfc::Flow f = r.m_flows.m_flows.at(it->first);

    m_tmp_graph.release_path_bw(routing_path, f.m_traffic_rate);

  }
      
}

//check all the host nodes cap
bool SA_application::check_nodes_cap(std::vector<int> host_node_v , sfc::Request r){

  int index = 0;

  for(auto it = r.m_chain.begin(); it != r.m_chain.end(); it++){

    if(index >= host_node_v.size()){

      return false;

    }

    if(m_tmp_graph.check_node_cap( *it,host_node_v.at(index)) == false){
      return false;
    }

    index++;
  }

  return true;

}

//release nodes in one request
void  SA_application::release_nodes_cap(std::vector<int> host_node_v , sfc::Request r){

  int index = 0;

  for(auto it = r.m_chain.begin(); it != r.m_chain.end(); it++){

    if(index >= host_node_v.size()){

      return;

    }

    m_tmp_graph.release_node_cap( host_node_v.at(index), (*it).m_cap);
    
    index++;
  }

}

bool  SA_application::minus_nodes_cap(std::vector<int> host_node_v , sfc::Request r, MyGraph& g){

  int index = 0;

  for(auto it = r.m_chain.begin(); it != r.m_chain.end(); it++){

    if(index >= host_node_v.size()){

      return false;

    }

    if(g.minus_node_cap( host_node_v.at(index), (*it).m_cap) == false){
      return false;
    }
    
    index++;
  }
  return true;

}


void SA_application::release_request_cap(Flow_map f_map , sfc::Request r){

  for(auto it = f_map.m_flow_map.begin(); it != f_map.m_flow_map.end(); it++){

    std::vector<int> host_node_v = it->second.get_host_nodes();

    release_nodes_cap(host_node_v, r);

  }
}

//random move flow map one by one  and check
bool SA_application::move_flow_map(Flow_map& f_map, sfc::Request r, int k_num, int request_id){

  NS_LOG_FUNCTION(" start ");

  Flow_map f_map_copy = f_map;

  std::vector<int> routing_path;

  std::vector<Path> short_path_v;

  m_tmp_graph.ksp(r.m_source, r.m_target, short_path_v, k_num);

  for(auto it = f_map.m_flow_map.begin(); it != f_map.m_flow_map.end(); it++){

    for(auto it_p = short_path_v.begin(); it_p != short_path_v.end(); it_p++){

      if(move_flow_to_path(it->second, *it_p) == true){

        std::vector<int> host_node_v = it->second.get_host_nodes();

        std::vector<int> routing_path = get_path(host_node_v, r);

        sfc::Flow f = r.m_flows.m_flows.at(it->first);

        if(m_tmp_graph.check_path_bw(routing_path, f.m_traffic_rate) == true ){
          // check node 
          // if(check_nodes_cap(host_node_v, r) == true)
          // {

          // }
          break;

        }
      }

    }
  }

  NS_LOG_FUNCTION("finish");

  int fail_edge_num = 0;

  if(check_request_resource(f_map , r, fail_edge_num) == true)
  { 
    // if it is not overloaded, cant find here

    std::vector<int>::iterator it = std::find(m_overloaded_r.begin(), m_overloaded_r.end(), request_id);
   
    

    //minus the new bw
    if(minus_request_resource(f_map, r) == false)
    {
      return false;
    }

     if(it == m_overloaded_r.end())
    {    //release the old bw
      release_request_bw(f_map_copy, r);

      //release nodes
      // release_request_cap(f_map_copy , r);
    }
    else
    {
      //if it is not deployed, remove it from m_overloaded_r
      m_overloaded_r.erase(it);

    }

    return true;

  }
  else
  {

    return false;
  }

  
}


//move a random request to a path
bool SA_application::move_request_to_path(int k_num){

  m_tmp_sa_result = m_sa_result;

  int request_id = 0;

  //if not all request been moved at least once
  if(m_loop_count < m_request_v.size()){

    request_id = m_loop_count;
  }
  else{

    request_id = rand()%(m_request_v.size());
  }

  //record this has been modified
  m_modified_count.insert(std::make_pair(request_id, 1));

  NS_LOG_INFO("move request to path r id " << request_id);

  auto it = m_tmp_sa_result.m_request_map.find(request_id);

  if(it == m_tmp_sa_result.m_request_map.end()){

    return false;
  }

  m_loop_count++;

  if(move_flow_map(it->second, m_request_v.at(request_id), k_num, request_id) == false){
    return false;
  }

  return true;

  

}


//move the overloaded request to another place
void SA_application::move_overloaded_requests(int k_num){

  for(auto it = m_overloaded_r.begin(); it != m_overloaded_r.end(); it++){

    auto it_f_map = m_tmp_sa_result.m_request_map.find(*it);

    if(it_f_map == m_tmp_sa_result.m_request_map.end()){

      continue;
    }

    Flow_map f_map = it_f_map->second;

    if(*it >= m_request_v.size()){
      break;
    }

    if(move_flow_map(f_map, m_request_v.at(*it), k_num, *it) == true){

      NS_LOG_FUNCTION("f_map");

      it_f_map->second = f_map;

      NS_LOG_FUNCTION("erase finish");
    }

    NS_LOG_FUNCTION("finish");


  }


}




// 3 digit random number between [0 and 1]
double SA_application::ran_double(){

  int N = 999;

  double ran_num = rand()%(N+1)/(double)(N+1);

  return ran_num;
}


//get the routing path, include the request src and dst
std::vector<int> SA_application::get_path(std::vector<int> host_node_v, sfc::Request r){

  std::vector<int> routing_path;
    
  int src = r.m_source;

  int dst = r.m_target;

  host_node_v.insert(host_node_v.begin(), src);

  host_node_v.push_back(dst);

  for(auto it = host_node_v.begin(); it != host_node_v.end()-1; it++ )
  {
    src = *it;

    dst = *(it+1);

    std::vector<int> tmp_path = m_graph.get_short_path(src, dst);

    // first round
    if(routing_path.size() == 0){

      routing_path.insert(routing_path.end(), tmp_path.begin(), tmp_path.end());

    } // if it is on the same node, the size is 1.
    else{
      //remove the duplicate tail and head
      if(routing_path.back() == tmp_path.front() && tmp_path.size() > 1){

        routing_path.insert(routing_path.end(), tmp_path.begin()+1, tmp_path.end());

      }

    }
  }

  return routing_path;


}

//get the overloaded path
// std::pair<int,int,> SA_application::get_overload_edge(int src, int dst, int required_bw){

//   std::pair<int,int> overload_edge = std::make_pair(-1,-1);

//   if(m_tmp_graph.check_edge_bw( src, dst, required_bw) == false){

//     overload_edge = std::make_pair(src,dst);

//   }

//   return overload_edge;
// }

//get all the edges overloaded
// void SA_application::get_overload_path(std::vector<int> routing_path, int required_bw){

//   for(auto it = routing_path.begin(); it != routing_path.end()-1; it++)
//   {

//     std::pair<int,int> overload_edge = get_overload_edge(*it, *(it+1), required_bw);

//     if(overload_edge.first == -1){
//       continue;
//     }

//     m_overload_path.push_back(overload_edge);
       
//   }


// }

//remove the edges from m_tmp_graph
// void SA_application::remove_edges_tmp(){

//   if(m_overload_path.size() == 0){
//     return;
//   }

//   for(auto it = m_overload_path.begin(); it != m_overload_path.end(); it++){

//     boost::remove_edge((*it).first, (*it).second, m_tmp_graph);

//   }

// }

//check if the request placement is ok. fail_edge_num is another output
bool SA_application::check_request_resource(Flow_map f_map, sfc::Request r, int& fail_edge_num){

  int src = r.m_source;

  int dst = r.m_target;

  int f_count = 0;

  bool succ_flag = true;


  for(auto it = f_map.m_flow_map.begin(); it != f_map.m_flow_map.end(); it++)
  {

    Vnf_host_map v_h_map = it->second;

    std::vector<int> host_node_v = v_h_map.get_host_nodes();

    std::vector<int> routing_path = get_path( host_node_v, r);

    sfc::Flow f = r.m_flows.m_flows.at(f_count);

    //minus bw
    if(m_tmp_graph.check_path_bw(routing_path, f.m_traffic_rate) ==false)
    { 

      succ_flag = false;

      fail_edge_num += m_tmp_graph.check_fail_edge_num(routing_path, f.m_traffic_rate);

    }

    // if(check_nodes_cap(host_node_v, r) ==false)
    // { 
    //   succ_flag = false;
    // }
    

    f_count++;

  }

  //if successful, minus the map
  if(succ_flag == true){

    return true;

  }
  else{

    return false;

  }
 

}

//check if the request placement is ok.
bool SA_application::minus_request_resource(Flow_map f_map, sfc::Request r){

  int src = r.m_source;

  int dst = r.m_target;

  int f_count = 0;

  bool succ_flag = true;

  MyGraph m_tmp_graph_copy =  m_tmp_graph;

  for(auto it = f_map.m_flow_map.begin(); it != f_map.m_flow_map.end(); it++)
  {

    Vnf_host_map v_h_map = it->second;

    std::vector<int> host_node_v = v_h_map.get_host_nodes();

    std::vector<int> routing_path = get_path( host_node_v, r);

    sfc::Flow f = r.m_flows.m_flows.at(f_count);

    //minus bw
    if(m_tmp_graph_copy.minus_path_bw(routing_path, f.m_traffic_rate) ==false)
    { 
      succ_flag = false;
    }

    //  if(minus_nodes_cap(host_node_v, r, m_tmp_graph_copy) ==false)
    // { 
    //   succ_flag = false;
    // }


    f_count++;

  }

  //if successful, minus the map
  if(succ_flag == true){

    m_tmp_graph = m_tmp_graph_copy;

    return true;

  }
  else{

    return false;

  }
 

}


//check the bw  of one solution
bool SA_application::check_solution(SA_result sa_result, int& fail_edge_num){

  fail_edge_num = 0;

  //init it
  m_overloaded_r.clear();

  //roll back to the initial status;
  m_tmp_graph = m_graph;

  int request_id = 0;

  double total_delay = 0;

  int flow_num = 0;

  bool succ_flag = true;

  m_minus_bw_fail = 0;

  m_check_r_bw_fail = 0;

  m_request_num_count = 0;

  for(auto it = sa_result.m_request_map.begin(); it != sa_result.m_request_map.end(); it++)
  {

    Flow_map f_map = it->second;

    NS_LOG_FUNCTION("f_map size: " << f_map.get_size() << "request id " << it->first);

    sfc::Request r = m_request_v.at(request_id);  

    // if any deployment fail, record it. allow some requests fail
    if (check_request_resource(f_map, r, fail_edge_num) == false){

      NS_LOG_INFO("request fails -> " << r.m_id);

      m_overloaded_r.push_back(request_id); // overloaded r here, use re

      succ_flag = false;

      m_check_r_bw_fail++;

    }
    else
    {
      if(minus_request_resource(f_map, r) == false)
      {
        m_minus_bw_fail++;

        m_overloaded_r.push_back(request_id);

        succ_flag = false;
      }
    }

    request_id++;

  }

  m_request_num_count = request_id;
 

  if(succ_flag == true){

    return true;
  }
  else{
    return false; 
  }

}


//be in line with dis approach, put all results in request, flow
void SA_application::get_flow_result(Flow_map f_map, sfc::Request& r){

  int f_id = 0;

  for(auto it = f_map.m_flow_map.begin(); it != f_map.m_flow_map.end(); it++){

    Vnf_host_map v_h_map = it->second;

    std::vector<int> host_node_v = v_h_map.get_host_nodes();

    std::vector<int> routing_path = get_path( host_node_v, r);

    (r.m_flows.m_flows.at(f_id)).m_host_node = host_node_v;

    (r.m_flows.m_flows.at(f_id)).m_routing_path = routing_path;

    f_id++;

  }

}


//modify all the flows in all requests, so the result is stored.
void SA_application::get_all_result(){

  int request_id = 0;

  //init here
  m_result_request_v.clear();

  for(auto it = m_sa_result.m_request_map.begin(); it != m_sa_result.m_request_map.end(); it++)
  {

    Flow_map f_map = it->second;

    sfc::Request r = m_request_v.at(request_id);

    r.m_finish = true;  

    // if any deployment fail, record it. allow some requests fail
    get_flow_result(f_map, r);

    m_result_request_v.push_back(r);

    request_id++;

  }

}

//random select a node from shortest path
double SA_application::move_to_short(){

  //random a request
  int ran_r_id =  rand()%(m_request_v.size()); // [0, request num]

  sfc::Request r = m_request_v.at(ran_r_id);
  //get shortest path
  std::vector<int> s_path = m_graph.get_short_path(r.m_source,  r.m_target);

  m_tmp_sa_result = m_sa_result; // cop the current solution

  Flow_map f_map = m_tmp_sa_result.get_map(ran_r_id);

  int f_map_size = f_map.get_size();

  int ran_f_id =  rand() % f_map_size; // [0, f_map_size-1]

  Vnf_host_map v_h_map = f_map.get_map(ran_f_id); // get aq random flow mapping

  int host_map_size = v_h_map.get_size();

  int rand_vnf_id =  rand() % host_map_size; // [0, f_map_size-1]

  Vnf_host v_h = v_h_map.get_map(rand_vnf_id);

  //now move to a neighbour

  int host_id = v_h.get_host();

  //random get a neighbour from it.

  int ran_id =  rand() % s_path.size(); // [0, f_map_size-1]

  int new_host_id = s_path.at(ran_id);


  // assign this new host to the tmp sa result.

  m_tmp_sa_result.update(ran_r_id, ran_f_id,  rand_vnf_id, new_host_id);

  NS_LOG_INFO(ran_r_id << " -> " << ran_f_id << " -> " << rand_vnf_id << " -> " << new_host_id << " OLD HOST ID " << host_id);


  NS_LOG_FUNCTION(" check the map host -> " << m_tmp_sa_result.get_instance_host(ran_r_id, ran_f_id,  rand_vnf_id).second);

  Flow_map f_map_new = m_tmp_sa_result.get_map(ran_r_id);

  Vnf_host_map v_h_map_new = f_map_new.get_map(ran_f_id);

  double dE = get_dE(m_request_v.at(ran_r_id) ,v_h_map, v_h_map_new);

  return dE;
}

//instance num for current round
double SA_application::get_instance_num(){

  double num_instance = 0;

  int request_id = 0;

  for(auto it = m_sa_result.m_request_map.begin(); it != m_sa_result.m_request_map.end(); it++)
  {   
    if(std::find(m_overloaded_r.begin(), m_overloaded_r.end(), request_id) != m_overloaded_r.end()){

      request_id++;

      continue;

    }
  
    for (auto it_f =  (it->second).m_flow_map.begin(); it_f != (it->second).m_flow_map.end(); it_f++){

      num_instance += (it_f->second).get_size();

    }

    request_id++;
  }

  return num_instance;

}

//always call get_instance_util first.
double SA_application::get_instance_num_all(){

  return m_instance_num;
}

//get mean hops for a time slot
double SA_application::get_mean_hops(int time){

  double num_flows = 0;

  double total_hops = 0;

  for(auto it = m_result_request_v_all.begin(); it != m_result_request_v_all.end(); it++)
  { 
    for(auto it_f = (*it).m_flows.m_flows.begin(); it_f != (*it).m_flows.m_flows.end(); it_f++)
    {
      //make sure the request is alive
      if((*it_f).m_start_time <= time && (*it_f).m_end_time > time)
      {

        total_hops += (*it_f).get_flows_hops();

        num_flows += 1;
      }

    }
  }

  return total_hops/num_flows;

}


//get the utilization rate of a time slot, online
double SA_application::get_instance_util(int time){

  int cap, used_cap, num_instance;

  // double usage_rate = 0 ;

  num_instance = 0;

  double tmp_usage_rate = 0.0;

  for(auto it = m_result_request_v_all.begin(); it != m_result_request_v_all.end(); it++)
  { 
    for(auto it_f = (*it).m_flows.m_flows.begin(); it_f != (*it).m_flows.m_flows.end(); it_f++)
    {
      //make sure the request is alive
      if((*it_f).m_start_time <= time && (*it_f).m_end_time > time)
      {

        for(auto it_n = (*it).m_chain.begin(); it_n != (*it).m_chain.end(); it_n++)
        {
          cap = (*it_n).m_cap;//the vnf capacity

          used_cap = (*it_f).m_traffic_rate;

          tmp_usage_rate += double(used_cap)/double(cap);

          num_instance += 1;

        }

      }

    }
    NS_LOG_DEBUG("usage rate  " << 100*tmp_usage_rate/num_instance );

  }

  m_instance_num = num_instance;

  return 100*tmp_usage_rate/num_instance;


}

//get the mean for offline
double SA_application::get_mean_instance_utilization(){

  int cap, used_cap, num_instance;

  // double usage_rate = 0 ;

  num_instance = 0;

  double tmp_usage_rate = 0.0;
  for(auto it = m_result_request_v.begin(); it != m_result_request_v.end(); it++)
  { 
    for(auto it_f = (*it).m_flows.m_flows.begin(); it_f != (*it).m_flows.m_flows.end(); it_f++)
    {
      for(auto it_n = (*it).m_chain.begin(); it_n != (*it).m_chain.end(); it_n++)
      {
        cap = (*it_n).m_cap;//the vnf capacity

        used_cap = (*it_f).m_traffic_rate;

        tmp_usage_rate += double(used_cap)/double(cap);

        num_instance += 1;


      }
    }
    NS_LOG_DEBUG("usage rate  " << 100*tmp_usage_rate/num_instance );

    

  }

  return 100*tmp_usage_rate/num_instance;


}

double SA_application:: get_mean_node_util(){
  return m_graph.get_mean_node_util();
}

double SA_application::get_mean_edge_util(){
  return m_graph.get_mean_edge_util();
}

std::vector<double> SA_application::get_every_edge_util(){

  return m_graph.get_every_edge_util();
}


//update m_graph according to m_Sa_result
void SA_application::update_graph(MyGraph& g){

  int host_id, request_id, vnf_order_id;

  //minus node cap first
  for(auto it = m_sa_result.m_request_map.begin(); it != m_sa_result.m_request_map.end(); it++){

    for(auto it_r = (it->second).m_flow_map.begin(); it_r != (it->second).m_flow_map.end(); it_r++){

        for(auto it_im = (it_r->second).m_array_map.begin(); it_im != (it_r->second).m_array_map.end(); it_im++){

          Vnf_host v_h = it_im->second;

          host_id = v_h.get_host();

          request_id = it->first;

          vnf_order_id = it_im->first;

          //get the request, id starts from 0
          sfc::Request r = m_request_v.at(request_id);

          sfc::Vnf n = r.get_vnf(vnf_order_id);

          g.minus_node_cap( host_id,n.m_cap);

        }
    }
  }
  //minus edge cap then
  for(auto it = m_result_request_v.begin(); it != m_result_request_v.end(); it++)
  {   
          sfc::Request_result r_result = (*it).get_flows_routing_path();

          for(auto it_f = r_result.begin(); it_f != r_result.end(); it_f++){

            g.minus_path_bw((*it_f).second, (*it_f).first);

            NS_LOG_DEBUG("MINUS BW" << (*it_f).first);
          }   
      
  }

  
}

//penalty the overloaded request by a factor
double SA_application::get_penalty(double p_factor, int fail_edge_num){

  // int overloaded_num = m_overloaded_r.size();

  // NS_LOG_DEBUG(" penalty num is " << overloaded_num);

  return p_factor*fail_edge_num;
}

//finally decide if the request is succ
void SA_application::update_request_succ(){

  int r_id = 0;

  for(auto it = m_overloaded_r.begin(); it != m_overloaded_r.end(); it++){

    r_id = *it;

    if(r_id >= m_request_v.size())
    {
      continue;
    }

    m_request_v.at(r_id).m_succ = false;
  }

}

double SA_application::get_throughput(){

  double throughput = 0;

  for(auto it = m_request_v.begin(); it != m_request_v.end(); it++){

    throughput += (*it).get_total_traffic();
  }

  return throughput;

}


//offline algorithm
void SA_application::main(double temp, double coolingRate, int kNum, std::string filename, std::string is_test){
  
  if(m_request_v.empty()){

    return;
  }

  NS_LOG_INFO("SA main START");
  clock_t start,end;

  start = clock();

  srand(time(0));

  //improve the SA by record a global best, and roll back to it finally if the last result is bad.
  double Fbest = 0;

  //after a certain number if no improvement, then stop it.
  int no_im_count = 0;

  SA_result best_sa_result;

  double penalty = 0;

  std::vector<int> bw_v = m_tmp_graph.get_graph_bw();

  write_time("scratch/bw-status.csv");

  write_vector_file("scratch/bw-status.csv", "bw status ", bw_v );

  if(is_test != "test"){

    create_init_solution();
  }
  else{

    create_test_solution();

  }

  //create solution based on shortest path
  // create_optimal_solution();

  int fail_edge_num = 0;

  if(check_solution(m_sa_result, fail_edge_num) == false){
      
      //get a penalty value based on overloaded requests

      // penalty = get_penalty(p_factor, fail_edge_num);

  }

  //save more info in test
  

  bw_v = m_tmp_graph.get_graph_bw();

  write_vector_file("scratch/bw-status.csv", "initial bw status ", bw_v );

  double total_traffic_rate = get_throughput();

  write_count("scratch/bw-status.csv", " theoretical throughput ", total_traffic_rate);

  write_count("scratch/bw-status.csv", " check requests bw fails ", m_check_r_bw_fail);

  write_count("scratch/bw-status.csv", " minus requests bw fails ", m_minus_bw_fail);

  write_count("scratch/bw-status.csv", " checked reuqest num ", m_request_num_count);

  

  //calculate average delay, i.e. the objective
  m_Fmin = get_average_delay(m_sa_result);

  m_min_graph = m_tmp_graph;

  m_min_overloaded_r = m_overloaded_r;

  Fbest = m_Fmin;

  best_sa_result = m_sa_result;

  NS_LOG_DEBUG("initial delay " << m_Fmin);
  
  get_all_result();

  write_time("scratch/routing-result.csv");

  print_result("scratch/routing-result.csv");

  m_loop_count = 0;

  double dE = 0;

  while( temp > 1){

    NS_LOG_DEBUG("New round");

    // if(no_im_count >=50)
    // {
    //   NS_LOG_DEBUG("break due to no improvement");
    //   break;
    // }

    m_tmp_sa_result = m_sa_result;

    m_tmp_graph = m_min_graph;

    m_overloaded_r = m_min_overloaded_r;

    if(std::isnan(m_Fmin)){
      NS_LOG_DEBUG("find a nan temperature is " << temp);

      return;
    }

    if(is_test == "test"){

      //record for test
      bw_v = m_tmp_graph.get_graph_bw();

      write_vector_file("scratch/bw-status.csv", "loop bw status ", bw_v );

      get_all_result();

      print_result("scratch/routing-result.csv");

      write_count("scratch/bw-status.csv", " check requests bw fails ", m_check_r_bw_fail);

      write_count("scratch/bw-status.csv", " minus requests bw fails ", m_minus_bw_fail);

      write_count("scratch/bw-status.csv", " checked reuqest num ", m_request_num_count);

    }

    //move a request to a path
    //should check and minus the bw
    if(move_request_to_path(kNum) == false){
      // no_im_count++;
      continue;
    }

    //get delay
    m_new_delay = get_average_delay(m_tmp_sa_result);

    NS_LOG_DEBUG("m_new_delay" << m_new_delay);

    dE = m_Fmin - m_new_delay;

    NS_LOG_DEBUG(" dE is " << dE);

    // if(std::isnan(dE)){
    //   NS_LOG_DEBUG("find a nan temperature is " << temp);

    //   return;
    // }

    
    //dE < 0, no improvement dE = old - new;
    if(dE < EPISON){

      no_im_count++;

    }
    else
    {
      no_im_count = 0;
    }

    //consistently no improvement then finish.
    if(no_im_count >=50)
    {
      break;
    }


    //accept if smaller delay
     double random_threshold = ran_double();
    if(dE >= EPISON){

      m_Fmin = m_new_delay;

      m_sa_result = m_tmp_sa_result;

      m_min_graph = m_tmp_graph;

      m_min_overloaded_r = m_overloaded_r;

      NS_LOG_DEBUG("use new status, new delay is " << m_Fmin);
    }
    //else take probability
    // exp(dE/T) is between (0,1)
    else if(exp(dE/temp) > random_threshold)
    {
      m_Fmin = m_new_delay;

      m_sa_result = m_tmp_sa_result;

      m_min_graph = m_tmp_graph;

      m_min_overloaded_r = m_overloaded_r;

      NS_LOG_DEBUG("use new status by prob, new delay is " << m_Fmin << "  random threshold "<<random_threshold);
    }

    //record the best of all time, this is an improvement
    if(Fbest > m_Fmin){

      Fbest = m_Fmin;

      best_sa_result = m_sa_result;
    }

    temp *= 1 - coolingRate;

    
  }//end while

  //Finally the result is not good then roll back
  if(Fbest < m_Fmin){

    m_Fmin = Fbest;

    m_sa_result = best_sa_result;

  }

  //update the m_succ in request
  //this or tmp
  check_solution(m_sa_result, fail_edge_num);

  move_overloaded_requests(kNum);

  check_solution(m_sa_result, fail_edge_num);

  bw_v = m_tmp_graph.get_graph_bw();

  write_vector_file(filename, "final bw status ", bw_v );

  write_count(filename, " check requests bw fails ", m_check_r_bw_fail);

  write_count(filename, " minus requests bw fails ", m_minus_bw_fail);

  write_count("scratch/bw-status.csv", " failed edge num ", fail_edge_num);

  write_count("scratch/bw-status.csv", " checked reuqest num ", m_request_num_count);

  update_request_succ();

  //store result in the requests. 
  get_all_result();

  print_result("scratch/routing-result.csv");

  write_count(filename, " m_Fmin ", m_Fmin);

  write_count(filename, " fail num ", m_overloaded_r.size());

  write_count(filename, "modified request num ", m_modified_count.size());

  NS_LOG_DEBUG("Final average delay " << m_Fmin);
   
  //NS_LOG_DEBUG("shortest link delay"<< m_graph.get_short_path_delay(5, 0));

  end = clock();

  NS_LOG_DEBUG("Running Time: " << (double)(end-start)/CLOCKS_PER_SEC);

  //update graph
  update_graph(m_graph);
  //record value to .csv



  NS_LOG_INFO("SA main FINSIH");
}

//check if the request need release
bool SA_application::check_if_release(sfc::Request& r, int time){

  for(auto it = r.m_flows.m_flows.begin(); it != r.m_flows.m_flows.end(); it++)
  {

    if((*it).m_end_time == time && (*it).m_is_deployed == true)
    {

      return true;

    }

  }

  return false;

}

//check if the request need to be deployed
//if one flow need to be deployed, it is true
bool SA_application::check_if_deploy(sfc::Request& r, int time){

  for(auto it = r.m_flows.m_flows.begin(); it != r.m_flows.m_flows.end(); it++)
  {

    if((*it).m_start_time == time)
    {

      return true;

    }

  }

  return false;

}

//check if the request need  release or deploy
void SA_application::check_requests(int time){

  m_request_release.clear();

  m_request_v.clear();

  for(auto it = m_request_v_all.begin(); it != m_request_v_all.end(); it++){

    if((*it).m_finish == true){
      continue;
    }

    if((*it).m_succ == true){
      //check if need release
      if(check_if_release(*it, time) ==true){

        m_request_release.push_back(*it);

        continue;
      }

      

    }

    //check if need deploy
    if(check_if_deploy(*it, time) == true){

      m_request_v.push_back(*it);

    }
  }

}

void SA_application::release_flow_bw(sfc::Flow f)
{ 
  Path route_path = f.m_routing_path;

  m_graph.release_path_bw(route_path, f.m_traffic_rate);
}

void SA_application::release_flow(sfc::Flow f)
{

  // release instance
  // int vnf_order_id = 1;
  // for(std::vector<int>::iterator it = f.m_host_instance.begin(); it != f.m_host_instance.end(); it++)
  // {
  //   NS_LOG_DEBUG("release flow : " << f.m_id << " vnf_order_id " << vnf_order_id << "instance id " <<  *it);
  //   //release a instance
  //   release_flow_instance(f, vnf_order_id , *it);
  //   vnf_order_id += 1;
  // }


  //release bw
  release_flow_bw(f);

  // the flow has been released
  // f.m_is_gone = true;

}

void SA_application::release_requests()
{ 
  for(auto it = m_request_release.begin(); it != m_request_release.end(); it++)
  {
      for(auto it_f = (*it).m_flows.m_flows.begin(); it_f != (*it).m_flows.m_flows.end(); it_f++)
      {
        release_flow(*it_f);
      }
  }
}

//for each time slot,
//release previous succ request if time ends
//check to be deployed request
//init
//call main
void SA_application::main_online(int time, double temp, double coolingRate, int kNum, std::string filename, std::string is_test){
  
  NS_LOG_INFO("main_online : enter");
  
  //check to released request
  //check the to be deployed request
  check_requests(time);

  NS_LOG_INFO("main_online : finish check_requests");

  //release
  release_requests();

  NS_LOG_INFO("main_online : finish release_requests");

  m_tmp_graph = m_graph;

  //no deployment this round
  if(m_request_v.empty()){
    return;
  }

  //deploy the request
  //may need clean some class attributes
  main(temp, coolingRate, kNum, filename,  is_test);
  
  //append result
  m_result_request_v_all.insert(m_result_request_v_all.end(), m_result_request_v.begin(), m_result_request_v.end());
   
  NS_LOG_DEBUG("overall request num " << m_result_request_v_all.size());
}


void SA_application::write_vector_file( std::string filename, std::string s, std::vector<int> data )
{
    std::ofstream outfile;

    int count = 0;

    outfile.open(filename, std::ofstream::app);

    
    if (outfile.is_open())
    {   
        //no data 
        if(data == std::vector<int>())
        {   outfile << std::endl;
            outfile<< s << std::endl;

           
        
            outfile.close();
        }
        else
        {
            outfile << std::endl;
            outfile<< s << std::endl;
            for(auto it = data.begin(); it != data.end(); it++)
            {
                if(count%1000 == 0){
                    outfile <<std::endl;
                }
                outfile <<*it<< ",";
                count++;
            }
            outfile<<std::endl;
            
            outfile.close();
        }
    }
    else
    {
       std::cout<< "failes to open file " << filename << std::endl;
    }
}

void SA_application::write_count( std::string filename, std::string s, double count )
{
    std::ofstream outfile;

    outfile.open(filename, std::ofstream::app);

    
    if (outfile.is_open())
    {   
      
       
      outfile << std::endl;

      outfile<< s << " " << count << std::endl;
      
      outfile.close();
        
    }
    else
    {
       std::cout<< "failes to open file " << filename << std::endl;
    }
}

// save result to a file
void SA_application::print_result(std::string filename){

  std::vector<int> request_id;

  for(auto it = m_result_request_v.begin(); it != m_result_request_v.end(); it++)
  { 

    request_id.clear();

    request_id.push_back((*it).m_id);

    write_vector_file( filename, "request id " ,request_id);

    if((*it).m_succ == true){

      write_vector_file( filename, "succ " );

    }
    else
    {
      write_vector_file( filename, "failed " );

    }


    for(auto it_f = (*it).m_flows.m_flows.begin(); it_f != (*it).m_flows.m_flows.end(); it_f++){

      auto routing_path = (*it_f).m_routing_path;

      auto host_node = (*it_f).m_host_node;
      
      write_vector_file( filename, "routing path " ,routing_path);

      write_vector_file( filename, "host node " , host_node);

    }

  }

}


void SA_application::write_time(std::string filename){

    std::ofstream outfile;
    outfile.open(filename, std::ofstream::app);
    if (outfile.is_open())
    {
        outfile <<std::endl;
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

        std::stringstream ss;
        

        outfile <<std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X") << std::endl;

        outfile.close();
    }
     else
    {
        std::cout<< "failes to open file " << filename << std::endl;
    }
}



}//end  SA_application