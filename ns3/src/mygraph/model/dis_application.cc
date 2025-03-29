/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "dis_application.h"
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

#include "ns3/seq-ts-header.h"
#include <cstdlib>
#include <cstdio>
#include <cmath>

#include "sfc.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/properties.hpp>


namespace ns3 {

/* ... */
NS_LOG_COMPONENT_DEFINE ("Dis_application");

NS_OBJECT_ENSURE_REGISTERED (Dis_application);

TypeId
Dis_application::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Dis_application")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<Dis_application> ()
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&Dis_application::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval",
                   "The time to wait between packets", TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&Dis_application::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&Dis_application::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort", "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&Dis_application::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize",
                   "Size of packets generated. The minimum packet size is 12 bytes which is the size of the header carrying the sequence number and the time stamp.",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&Dis_application::m_size),
                   MakeUintegerChecker<uint32_t> (12,65507))
  ;
  return tid;
}

Dis_application::Dis_application ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_socket = 0;
  m_sendEvent = EventId ();

}

Dis_application::~Dis_application ()
{
  NS_LOG_FUNCTION (this);
}

void
Dis_application::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
Dis_application::SetRemote (Address addr)
{
  NS_LOG_FUNCTION (this << addr);
  m_peerAddress = addr;
}

void
Dis_application::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
Dis_application::StartApplication (void)
{
  
}

void Dis_application::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_sendEvent);
}

void Dis_application::Send (void)
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
      m_sendEvent = Simulator::Schedule (m_interval, &Dis_application::Send, this);
    }
}



 void Dis_application::init(int time, sfc::Request r, MyGraph g, int k_num)
 {
  m_time = time;
  m_request = r;
  m_flows = m_request.m_flows;
  m_graph = g;
  m_request_instance_map = r.m_map;
  m_k_num = k_num;

 }


//if some flows was not deployed, just skip
FLOW_STATE Dis_application::check_flow(sfc::Flow f)
{
  //check if need release
  if(f.m_end_time == m_time && f.m_is_deployed == true && f.m_is_gone == false)
  {
    return NEED_RELEASE;
  }
  else if (f.m_start_time == m_time)
  {

    return NEED_DEPLOY;
  }
  else
  {
    return SKIP;
  }

}


void Dis_application::get_total_traffic()
{ 

  int total_traffic = 0;

  NS_LOG_DEBUG("----get total traffic----");

  if(m_flows_to_deploy.get_size() == 0)
  {

    m_total_traffic =  0;
    return;

  }

  NS_LOG_DEBUG("----get total traffic loop---- flows size " <<m_flows_to_deploy.get_size() );

  for(std::vector<sfc::Flow>::iterator it = m_flows_to_deploy.m_flows.begin(); it != m_flows_to_deploy.m_flows.end(); it++)
  {

    total_traffic += (*it).m_traffic_rate;

    NS_LOG_DEBUG("----get total traffic total_traffic----  " <<total_traffic );
    NS_LOG_DEBUG("----get total traffic flow id----  " <<(*it).m_id );
    NS_LOG_DEBUG("----get total traffic flow rate----  " <<(*it).m_traffic_rate );

  }

  NS_LOG_DEBUG("----get total traffic loop finish----");

  m_total_traffic = total_traffic;

  return;

}


bool Dis_application::scale_up(Path p, sfc::Vnf n, int vnf_order_id)
{ 
  NS_LOG_DEBUG("-----scale up-----function order id " << vnf_order_id );
  
  int tmp_processing_cap  = m_total_traffic;

  for(Path::iterator it = p.begin(); it != p.end(); it++)
  {

    int node_cap  = m_tmp_graph.get_node_cap(*it);

    if(tmp_processing_cap <= node_cap )
    {
      
      //initial the sf instance
      // create a instance first, instance id is set to 1.  used capacity will used in steer traffic
      sfc::Sf_instance sf = {m_request.m_id, vnf_order_id, 1, *it,  tmp_processing_cap, 0};

      int tmp_instance_id = 0; 

      m_tmp_request_instance_map.insert_instance(vnf_order_id, tmp_instance_id, sf);// flow modify this one

      m_slot_instance_map.insert_instance_slot(vnf_order_id, tmp_instance_id, sf);//all flow choose instance from this one

      //update tmp resource map
      NS_LOG_DEBUG("minus node cap " << *it << " " << tmp_processing_cap);

      m_tmp_graph.minus_node_cap(*it, tmp_processing_cap);


      return true;
    }
  }

  return false;
}


bool Dis_application::scale_out(Path p, sfc::Vnf n, int vnf_order_id)
{ 
  NS_LOG_DEBUG("-----scale out-----function order id " << vnf_order_id  << "traffic rate " << m_total_traffic);
  

  int instance_num = ceil(double(m_total_traffic)/double(n.m_cap));

  int flow_num = m_flows_to_deploy.get_size();

  if(flow_num > instance_num)
  {
    instance_num = flow_num;
  }

  NS_LOG_WARN(" ----scale out ---- instance num " << instance_num);

  int num_count = 0;
  for(Path::iterator it = p.begin(); it != p.end(); it++)
  { 

    if(num_count >= instance_num)
    {
      break;
    }

    int node_cap  = m_tmp_graph.get_node_cap(*it);

     if(n.m_cap <= node_cap && num_count <= instance_num)
    {
      sfc::Sf_instance sf = {m_request.m_id, vnf_order_id, 0, *it, n.m_cap, 0};

      int tmp_instance_id = 0;

      m_tmp_request_instance_map.insert_instance(vnf_order_id, tmp_instance_id, sf);

      m_slot_instance_map.insert_instance_slot(vnf_order_id, tmp_instance_id, sf);

      NS_LOG_DEBUG("minus node cap " << *it << " " << n.m_cap);

      m_tmp_graph.minus_node_cap(*it, n.m_cap);

      num_count += 1;

    }
  }

  if(num_count >= instance_num)
  {
      return true;
  }
  else
  {
    return false;
  }
}





bool Dis_application::place(Path p,sfc::Vnf n, int vnf_order_id)
{ 

  if(n.m_type == sfc::non_io_bound)
  {
    // try scaleup
    if(scale_up(p, n, vnf_order_id) == false)
    { 
      
      if(scale_out(p, n, vnf_order_id) == true)
      {
        return true;
      }
      else
      { 

        return false;
      }

    }
    else
    {

      return true;

    }

  }
  else if (n.m_type == sfc::io_bound)
  {

    // try scale out
    if (scale_out(p, n, vnf_order_id) == false)
    {

      return false;

    }
    else
    {
      return true;
    }

  }

  return false;;

}


//deploy functions based on traffic
bool Dis_application::deploy_functions()
{
 
  std::vector<Path> k_short_path;
  bool k_flag = m_graph.ksp(m_request.m_source, m_request.m_target, k_short_path, m_k_num);

  
  for(std::vector<Path>::iterator it = k_short_path.begin(); it != k_short_path.end(); it++)
  {
    //trace the vnf index
    int vnf_order_id = 1;

    bool all_vnf_placed = true;

    // initial all the temp copy.

    m_tmp_request_instance_map = m_request_instance_map;

    m_slot_instance_map.clear();

    m_tmp_graph = m_graph;

    //deploy functions on a single path
    for(sfc::Chain::iterator it_chain = m_request.m_chain.begin(); it_chain != m_request.m_chain.end(); it_chain++)
    {
        //call place_function
        if (place(*it, *it_chain, vnf_order_id )== false)
        {
          all_vnf_placed == false;
          break;
        }

        vnf_order_id += 1;
    }

    if(all_vnf_placed == true)
    { 
      NS_LOG_INFO("deploy functions succ");
      return true;
    }

  }

  NS_LOG_INFO("deploy functions failed");
  return false;

}

//find the least used instance among one vnf
int Dis_application::get_least_used_instance(int vnf_order_id)
{ 
  NS_LOG_FUNCTION(" start " << m_slot_instance_map.m_map.size());

  //TODO: this place leak, can't find the vnf_order_id
  std::map<int, sfc::Instance_map>::iterator it =  m_slot_instance_map.get_instance_map(vnf_order_id);

  if(it == m_slot_instance_map.m_map.end()){
    return 1;
  }

  sfc::Instance_map im = it->second;

  double min_usage_rate = 100.0;

  int least_instance_id = 1;

  NS_LOG_DEBUG("m_slot_instance_map size " << im.m_instance_map.size() << " vnf id " << vnf_order_id);

  for(auto it_1 = im.m_instance_map.begin(); it_1 != im.m_instance_map.end(); it_1++)
  { 
    
    NS_LOG_FUNCTION("ENTER LOOP" );

    int used_cap = it_1->second.m_used_capacity;
    int cap  = it_1->second.m_capacity;
    NS_LOG_DEBUG("instance  " << it_1->second.m_id << " used cap " << used_cap << " cap "  << cap);


    double tmp_usage_rate = 100*double(used_cap)/double(cap);

    NS_LOG_DEBUG("tmp usage vs least " << tmp_usage_rate << " " << min_usage_rate );

    if(tmp_usage_rate <= min_usage_rate)
    {
      min_usage_rate = tmp_usage_rate;

      least_instance_id = it_1->second.m_id;

    }

  }

  NS_LOG_DEBUG("least used instance id " << least_instance_id );

  return least_instance_id;

}

//get least used instances, if least used dont work, others wont work, because all instance capacity is same
std::vector<int> Dis_application::get_all_least_used_instance()
{ 
  std::vector<int> instance_id_v;

  for(int i = 1; i <= m_request.m_chain.size(); i++)
  {
    int least_instance_id = get_least_used_instance(i);

    NS_LOG_FUNCTION( " loop number " << i);

    instance_id_v.push_back(least_instance_id);
  }

  NS_LOG_FUNCTION(" finish");
  return instance_id_v;
}

//get the src and dst node id between the instance and previous node/instance.
std::pair<int,int> Dis_application::get_pair_instance(int vnf_order_id, int instance_id, int previous_instance_id)
{
  NS_LOG_DEBUG("get pair instance " << vnf_order_id <<" " << instance_id << " " <<  previous_instance_id);
  sfc::Sf_instance sf = m_tmp_request_instance_map.get_instance(vnf_order_id, instance_id);

  int dst = sf.m_host_node;

  int src = 0;

  if(previous_instance_id == 0)
  {
    src = m_request.m_source;
  }
  else if(previous_instance_id == -1) // that means get the last instance to dst
  {
    src = sf.m_host_node;

    dst = m_request.m_target;

  }
  else 
  {
    sfc::Sf_instance sf_1 = m_tmp_request_instance_map.get_instance(vnf_order_id-1, previous_instance_id);

    src = sf_1.m_host_node;

  }

  return std::make_pair(src, dst);
}


//given the instance host node, find a path to accomodate the traffic
bool Dis_application::select_flow_path(sfc::Flow& f, int vnf_order_id, int instance_id, int previous_instance_id )
{ 

  std::pair<int,int> pair = get_pair_instance(vnf_order_id, instance_id, previous_instance_id);

  // the same node, then push back the node
  if(pair.first == pair.second)
  { 
    std::vector<int> tmp_v;

    tmp_v.push_back(pair.first);

    f.m_routing_segments.push_back(tmp_v);//record the segment

    NS_LOG_DEBUG("Flow " << f.m_id << " Vnf " << vnf_order_id << " " << pair.first << " -> " << pair.second);

    return true;
  }

  std::vector<Path> p_v; // get k_short_path

  NS_LOG_DEBUG("Flow " << f.m_id << " Vnf " << vnf_order_id << " " << pair.first << " -> " << pair.second);

  m_tmp_graph.ksp(pair.first, pair.second, p_v ,m_k_num);

  for(auto it = p_v.begin(); it != p_v.end(); it++)
  {
    if(m_tmp_graph.check_path_bw(*it, f.m_traffic_rate)== true)
    {

      m_tmp_graph.minus_path_bw(*it, f.m_traffic_rate);

      f.m_routing_segments.push_back(*it);//record the segment

      return true;
    }

  }

  return false;
}







// steer one flow
bool Dis_application::steer_traffic_one_flow(sfc::Flow& f, std::vector<int> instance_id_v)
{

  int vnf_order_id = 1;

  sfc::Request_instance_map tmp_map = m_tmp_request_instance_map;

  MyGraph tmp_graph = m_tmp_graph;

  //iterate over all instances
  for(std::vector<int>::iterator it = instance_id_v.begin(); it != instance_id_v.end(); it++)
  { 
    NS_LOG_DEBUG("------fetch vnf order id " << vnf_order_id << " instance id " << *it);

    sfc::Sf_instance sf = m_tmp_request_instance_map.get_instance(vnf_order_id, *it);

    if(f.m_traffic_rate * m_request.get_change_factor(vnf_order_id) > sf.m_capacity - sf.m_used_capacity)
    { 
      m_tmp_request_instance_map = tmp_map; // resume

      m_tmp_graph = tmp_graph;

      f.m_routing_segments.clear();//record the segment

      NS_LOG_DEBUG(" steer traffic processing capacity not enough " << sf.m_capacity - sf.m_used_capacity << " " << f.m_traffic_rate * m_request.get_change_factor(vnf_order_id));

      return false;
    }

    // modify the instance used cap
    sf.m_used_capacity += f.m_traffic_rate * m_request.get_change_factor(vnf_order_id);

    NS_LOG_DEBUG("modify sf id " << sf.m_id << " used cap "<< sf.m_used_capacity);

    // modify the instance used cap
    m_tmp_request_instance_map.modify_instance(vnf_order_id, *it, sf);

    m_slot_instance_map.modify_instance(vnf_order_id, *it, sf);

    //modify the tmp_flow
    f.m_host_node.push_back(sf.m_host_node);

    // modify m_tmp_graph 
    // shortest between instance id and previous node
    int instance_id = 0;

    // if first vnf then, previous instance id = 0 i.e. src of request
    if(vnf_order_id != 1)
    {
      instance_id = instance_id_v.at(vnf_order_id-2); // minus 2 here because this is previous instance id and vnf_order starts from 1
    }
    
    

    // find a path, modify bw, record the path
    if(select_flow_path(f, vnf_order_id, *it, instance_id) == false)
    {  
      m_tmp_request_instance_map = tmp_map;

      m_tmp_graph = tmp_graph;

      f.m_routing_segments.clear();//record the segment

      NS_LOG_DEBUG(" select flow path fail");

      return false;
    }
    
    // calculate the last instance to dst traffic
    if(vnf_order_id == m_request.m_chain.size())
    { 
      instance_id = -1;
      // find a path, modify bw, record the path
      if(select_flow_path(f, vnf_order_id, *it, instance_id) == false)
      {  
        m_tmp_request_instance_map = tmp_map;

        m_tmp_graph = tmp_graph;

        f.m_routing_segments.clear();//record the segment

        return false;
      }
    }




    //record the path
    vnf_order_id += 1;
  }

  //assemble the routing path

  //modify the flow host instance
  f.m_host_instance = instance_id_v;

  f.m_is_deployed = true;

  return true;

}

// steer the traffic of all flows
bool Dis_application::steer_traffic()
{ 

  //iterate all flows
  for(std::vector<sfc::Flow>::iterator it = m_flows_to_deploy.m_flows.begin(); it != m_flows_to_deploy.m_flows.end(); it++)
  {
    //get least used instance

    std::vector<int> instance_id_v = get_all_least_used_instance();

    bool ret = steer_traffic_one_flow(*it, instance_id_v);


    // if not succ try other instance
    if(ret == false)
    {
      return false;
    }

    
  }
  return true;

}

// remove the instance from m_request_instance_map
bool Dis_application::release_instance(int vnf_order_id, int instance_id)
{
  return m_request_instance_map.remove_instance(vnf_order_id, instance_id);
}


// release instance if it is not being used any more.
void Dis_application::release_node( sfc::Sf_instance sf)
{
  int node_id = sf.m_host_node;

  int cap = sf.m_capacity;

  //modify tmp graph, copy at last
  m_graph.release_node_cap(node_id, cap);
}



//release a flow from one instance
//if the used capa = 0
//release
// update m_graph
void Dis_application::release_flow_instance(sfc::Flow f, int vnf_order_id, int instance_id)
{
  sfc::Sf_instance sf = m_request_instance_map.get_instance(vnf_order_id, instance_id);

  NS_LOG_DEBUG(" m_used_capacity " << sf.m_used_capacity);

  sf.m_used_capacity  = sf.m_used_capacity - f.m_traffic_rate * m_request.get_change_factor(vnf_order_id);

  NS_LOG_DEBUG(" minus sf id " << sf.m_id << " released cap " << f.m_traffic_rate * m_request.get_change_factor(vnf_order_id));

  NS_LOG_DEBUG(" release flow instance sf " << sf.m_id << " used_cap " << sf.m_used_capacity);

  m_request_instance_map.modify_instance(vnf_order_id, instance_id, sf);

  //this number be set to 10 because sometimes due to decimal remaining
  if(sf.m_used_capacity < 10)
  {
    release_node(sf);

    release_instance(vnf_order_id, instance_id);
  }

}

void Dis_application::release_flow_bw(sfc::Flow f)
{ 
  Path route_path = f.m_routing_path;

  m_graph.release_path_bw(route_path, f.m_traffic_rate);
}


void Dis_application::release_flow(sfc::Flow& f)
{

  // release instance
  int vnf_order_id = 1;
  for(std::vector<int>::iterator it = f.m_host_instance.begin(); it != f.m_host_instance.end(); it++)
  {
    NS_LOG_DEBUG("release flow : " << f.m_id << " vnf_order_id " << vnf_order_id << "instance id " <<  *it);
    //release a instance
    release_flow_instance(f, vnf_order_id , *it);
    vnf_order_id += 1;
  }


  //release bw
  release_flow_bw(f);

  // the flow has been released
  f.m_is_gone = true;

}


void Dis_application::release_flows()
{
  for(auto it = m_flows_to_release.m_flows.begin(); it != m_flows_to_release.m_flows.end(); it++)
  {
    release_flow(*it);
  }
}


// check flows need to be placed.
bool Dis_application::check_flows()
{ 
  sfc::Flow f;
  m_flows_to_deploy.clear();
  m_flows_to_release.clear();

  for(int i=0; i < m_flows.get_size(); i++)
  {

    f = m_flows.get_flow(i);

    if(check_flow(f) == NEED_DEPLOY)
    {
      //to deploy
      m_flows_to_deploy.add_flow(f);

      NS_LOG_DEBUG("need_deploy flow " << m_flows_to_deploy.m_flows.back().m_id << " traffic rate " << m_flows_to_deploy.m_flows.back().m_traffic_rate);

    }
    else if(check_flow(f) == NEED_RELEASE)
    {
      //to release
      m_flows_to_release.add_flow(f);

      NS_LOG_DEBUG("need_release flow " << f.m_id << " traffic rate " << f.m_traffic_rate);

    }

  }
  // if there is no flow to deal with, then return false;
  if(m_flows_to_deploy.get_size() ==0 && m_flows_to_release.get_size() == 0)
  {
    return false;
  }

  for(auto it = m_flows_to_deploy.m_flows.begin(); it != m_flows_to_deploy.m_flows.end(); it ++)
  {
    NS_LOG_DEBUG("iterate m flows to deploy " << (*it).m_id << " " << (*it).m_traffic_rate);

  }



  return true;
}



//modify the flows in the request by using m_flows_to_deploy
void Dis_application::modify_flows()
{ 
 
  for(auto it = m_flows_to_deploy.m_flows.begin(); it != m_flows_to_deploy.m_flows.end(); it++)
  { 
    //assemble the segments
    (*it).create_result_path();

    (*it).print_routing_path();

    m_request.replace_flow(*it);
  }

}

// save  the result to a global variable
void Dis_application::save(MyGraph& g, sfc::Request& r)
{
  g = m_graph;

  modify_flows();

  m_request.m_map = m_request_instance_map;

  r = m_request;

  NS_LOG_DEBUG(" dis_application save " );
}


void Dis_application::show_flows_path()
{
  for(auto it = m_flows_to_deploy.m_flows.begin(); it != m_flows_to_deploy.m_flows.end(); it++)
  { 

    std::vector<int> p = (*it).m_routing_path;
    std::vector<int> h = (*it).m_host_node;

    std::cout << "----routing path is ---- " << std::endl;
    for(auto it_1 = p.begin(); it_1 != p.end(); it_1++)
    {
      std::cout << *it_1 << "->";
    }
    std::cout << std::endl;


    std::cout << "----host node path is ---- " << std::endl;
    for(auto it_2 = h.begin(); it_2 != h.end(); it_2++)
    {
      std::cout << *it_2 << "->";
    }
    std::cout << std::endl;


  }
}

// init the flow deployment info
void Dis_application::init_flow(sfc::Flow& f)
{
  f.m_is_deployed = false;
  f.m_is_gone = false;
  f.m_routing_path.clear();
  f.m_routing_segments.clear();
  f.m_host_instance.clear();
  f.m_host_node.clear();
}


void Dis_application::init_flows()
{
  for(auto it = m_request.m_flows.m_flows.begin(); it != m_request.m_flows.m_flows.end(); it++)
  {
    init_flow(*it);
  }

}

//get the current number of instance
//iterate over m_request_instance_map
double Dis_application::get_instance_num()
{
  double num_instance = 0;

  for(auto it = m_request_instance_map.m_map.begin(); it != m_request_instance_map.m_map.end(); it++)
  {
      num_instance += (it->second).m_instance_map.size();
  }

  return num_instance;
}

//get the average utilization of 
double Dis_application::get_mean_instance_utilization()
{ 
  NS_LOG_FUNCTION("---start---");
  int cap, used_cap, num_instance;

  num_instance = 0;

  double tmp_usage_rate = 0.0;

  NS_LOG_FUNCTION("---m request instance map---" << m_request_instance_map.m_map.size());

  for(auto it = m_request_instance_map.m_map.begin(); it != m_request_instance_map.m_map.end(); it++)
  {
    for(auto it_im = (it->second).m_instance_map.begin(); it_im != (it->second).m_instance_map.end(); it_im++)
    {
      cap = (it_im->second).m_capacity;

      used_cap = (it_im->second).m_used_capacity;

      tmp_usage_rate += double(used_cap)/double(cap);

      NS_LOG_DEBUG("USED CAP " << used_cap );

      NS_LOG_DEBUG(  "TOTAL CAP" << cap);

      NS_LOG_DEBUG("tmp usage rate  " << tmp_usage_rate );

      if(used_cap >=0 ){
        std::cout << "USED CAP ok" ;
      }
      else{
        std::cout <<"USED CAP wrong" ;
      }

      if(cap > 0 ){
        std::cout <<"total CAP ok" ;
      }
      else if(cap = 0 )
      {
       std::cout <<"total CAP 0" ;
      }
      else{
        std::cout <<"total CAP wrong" ;
      }

      num_instance += 1;

    }
  }
  NS_LOG_DEBUG("usage rate  " << 100*tmp_usage_rate/num_instance );

  if(100*tmp_usage_rate/num_instance >0 && 100*tmp_usage_rate/num_instance <= 100){
    return 100*tmp_usage_rate/num_instance; // in percentage form
  }
  else{
    return 0.0;
  }
}

double Dis_application::get_mean_node_util()
{
  return m_graph.get_mean_node_util();
}

double Dis_application::get_mean_edge_util()
{
  return m_graph.get_mean_edge_util();
}

std::vector<double> Dis_application::get_graph_bw(){

  std::vector<int> intVec = m_graph.get_graph_bw();
  std::vector<double> doubleVec(intVec.begin(), intVec.end());

  return doubleVec;
}

//process one request one time
REQUEST_STATE Dis_application::main(int time, sfc::Request& r, MyGraph& g, int k_num)
{ 
  //if one flow fails previously, then no need to keep checking it.
  if(r.m_succ == false)
  {
    return NONE;
  }

  NS_LOG_DEBUG("------Dis_application main");
  init(time,  r, g, k_num);


  NS_LOG_DEBUG("------Dis_application check_flows");
  //no release no deployment
  bool TO_PROCESS = check_flows();

  if(TO_PROCESS == false)
  {
    return NONE;
  }
  else if(TO_PROCESS == true)
  { 

    NS_LOG_DEBUG("------Dis_application release_flows");

    release_flows();

    NS_LOG_DEBUG("------Dis_application get total traffic");

    get_total_traffic();

    NS_LOG_DEBUG("------Dis_application get_size");

    NS_LOG_DEBUG("------Dis_application start copy maps");

    m_tmp_request_instance_map = m_request_instance_map;

    NS_LOG_DEBUG("------m_tmp_request_instance_map ini size " << m_tmp_request_instance_map.get_size());

    m_tmp_graph = m_graph;

    m_slot_instance_map.clear();

    if(m_flows_to_deploy.get_size() != 0)
    {
    
      NS_LOG_DEBUG("------Dis_application deploy functions");
      // deploy functions
      if(deploy_functions()== false)
      { 
        r.m_succ = false;
        return ERROR;
      }


      NS_LOG_DEBUG("------Dis_application steer traffic");
      if(steer_traffic()==false)
      { 
        r.m_succ = false;
        init_flows();
        // remove the flow deployment info
        NS_LOG_WARN(" steer traffic fails");
        return ERROR;
      }
    }

    m_graph = m_tmp_graph;

    m_request_instance_map = m_tmp_request_instance_map;

    NS_LOG_DEBUG("------m_tmp_request_instance_map final size " << m_tmp_request_instance_map.get_size());

    save(g, r);

    //show_flows_path();

    //m_graph.print_graph();

    return SUCC;

  }

  return SUCC;

}

void Dis_application::print_hello()
{

  std::cout << "application running";
}



//process all request one-off without time series
REQUEST_STATE Dis_application::main_offline( sfc::Request& r, MyGraph& g, int k_num )
{ 
  int time = 0;

  NS_LOG_DEBUG("------Dis_application main");

  init(time,  r, g, k_num);

  m_flows_to_deploy.clear();

  for(auto it = r.m_flows.m_flows.begin(); it != r.m_flows.m_flows.end(); it++){

    m_flows_to_deploy.add_flow(*it);

  }

  get_total_traffic();

  NS_LOG_DEBUG("------Dis_application get_size");

  NS_LOG_DEBUG("------Dis_application start copy maps");

  m_tmp_request_instance_map = m_request_instance_map;

  NS_LOG_DEBUG("------m_tmp_request_instance_map ini size " << m_tmp_request_instance_map.get_size());

  m_tmp_graph = m_graph;

  m_slot_instance_map.clear();

    
  NS_LOG_DEBUG("------Dis_application deploy functions");
  // deploy functions
  if(deploy_functions()== false)
  { 
    r.m_succ = false;
    return ERROR;
  }


  NS_LOG_DEBUG("------Dis_application steer traffic");
  if(steer_traffic()==false)
  { 
    r.m_succ = false;
    init_flows();
    // remove the flow deployment info
    NS_LOG_WARN(" steer traffic fails");
    return ERROR;
  }
  

  m_graph = m_tmp_graph;

  m_request_instance_map = m_tmp_request_instance_map;

  NS_LOG_DEBUG("------m_tmp_request_instance_map final size " << m_tmp_request_instance_map.get_size());

  save(g, r);

  //show_flows_path();

  //m_graph.print_graph();

  return SUCC;

}

double Dis_application::get_functional_delay(){

  sfc::Request r = m_request;

  double delay = 0;

  for(auto it = r.m_chain.begin(); it != r.m_chain.end(); it++){
      sfc::Vnf n = *it;

      delay += n.m_processing_time;
  }
  return delay;
}

//calculate the throughput, delay, hops, 

double Dis_application::get_bound_delay(bool enable_function){
  double delay = 0;

  int src = m_request.m_source;

  int dst = m_request.m_target;

  delay = m_graph.get_short_path_delay(src, dst);

  if(enable_function == true){

    delay += get_functional_delay();

  }

  return delay;

}

double Dis_application::get_bound_throughput(){

  double throughput = 0;

  for(auto it = m_request.m_flows.m_flows.begin(); it != m_request.m_flows.m_flows.end(); it++){

    throughput += (*it).m_traffic_rate;
  }

  return throughput;

}

double Dis_application::get_bound_hops(){
  double hops = 0;

  int src = m_request.m_source;

  int dst = m_request.m_target;

  std::vector<int> p = m_graph.get_short_path(src, dst);

  hops = p.size();

  return hops;

}


  







}//end namespace dis_application

