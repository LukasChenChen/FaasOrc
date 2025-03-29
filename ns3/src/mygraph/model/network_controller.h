#ifndef NETWORK_CONTROLLER_H
#define NETWORK_CONTROLLER_H

#include "ns3/core-module.h"// for log
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

#include "ns3/ipv4-list-routing-helper.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> // for ip address
#include "ns3/simulator.h"
#include "ns3/traffic_gen_helper.h"
// #include "ns3/sfc_routing_helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-nix-vector-helper.h"

#include "sfc_sink.h"
#include "ns3/sfc_sink_helper.h"
// #include "sfc.h"
#include <string>
#include "type.h"

namespace ns3 {
    

typedef std::pair<int, int> Key;

class Network_controller
{
public:
    Network_controller()
    {

    };  

    void set_map(Topology map, ServerlessConfig cfg, std::string rate = "kb/s", std::string link_r = "kbps");

    void create_node();

    void create_network();

    void create_all_p2p_channel();

    bool create_traffic(std::string traffic_rate, Request r, int start_time, int end_time);

    void create_request_traffic(Request r);

    NodeContainer get_NodeContainer();

    void set_max_slot(int t_lengh, int slot_num);

    void create_sink(RequestsMap rm);

    Topology m_topo;

    NodeContainer m_NodeContainer;

    PointToPointHelper m_p2p;

    std::map< Key, NetDeviceContainer> m_NetDeviceContainer_map;

    std::map< Key, Ipv4InterfaceContainer> m_Ipv4InterfaceContainer_map;

    Ipv4ListRoutingHelper m_listHelper;

    InternetStackHelper m_stack;

    Ipv4AddressHelper m_ipv4;

    std::map<int, ApplicationContainer> m_apps_map;

    int m_time_slot_length;

    int m_slot_num;

    ServerlessConfig m_cfg;

    std::string m_rate; //could be Mb/s, kb/s and so on. kb/s is default, sending rate

    std::string m_link_rate; // the bandwidth on links

    requestTag rt;



private:
    void create_p2p_channel(int src_id, int dst_id, std::string data_rate, std::string delay);

    bool set_NetDeviceContainer(int src_id, int dst_id, NetDeviceContainer d);

    // void create_routing_protocol();

    const char* increment_ipv4_address(const char* address_string);

    void create_ipv4_address();

    bool get_Ipv4InterfaceContainer(int src_id, int dst_id, Ipv4InterfaceContainer& i);

};

} //ns3

#endif