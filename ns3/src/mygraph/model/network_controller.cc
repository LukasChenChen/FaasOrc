#include "network_controller.h"
#include "ns3/core-module.h"// for log
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "sfc.h"

#include "ns3/traffic_gen_helper.h"
#include "ns3/sfc_routing_helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-nix-vector-helper.h"

#include "ns3/traffic_gen_helper.h"
#include "sfc_sink.h"
#include "ns3/sfc_sink_helper.h"


namespace ns3 
{
NS_LOG_COMPONENT_DEFINE("network_controller");
void Network_controller::set_map (Topology topo, ServerlessConfig cfg, std::string rate, std::string link_r)
{
    NS_LOG_FUNCTION (this);
    m_topo = topo;

    m_cfg = cfg;

    m_rate = rate;

    m_link_rate = link_r;
   
}

bool Network_controller::set_NetDeviceContainer(int src_id, int dst_id, NetDeviceContainer d){
    NS_LOG_FUNCTION (this);
     Key key;
     key = std::make_pair(src_id, dst_id);

     auto it = m_NetDeviceContainer_map.find(key);

     if(it == m_NetDeviceContainer_map.end()){

         m_NetDeviceContainer_map.insert(std::make_pair(key, d));
         return true;

     }
     else
     {
         NS_LOG_LOGIC(this << " key exists");
         return false;
     }
     
 }

//create link between two nodes
void Network_controller::create_p2p_channel(int src_id, int dst_id, std::string data_rate, std::string delay){
    NS_LOG_FUNCTION (this);

    NS_LOG_LOGIC ("src " << src_id << " dst " << dst_id);
    m_p2p.SetDeviceAttribute ("DataRate", StringValue (data_rate));
    m_p2p.SetChannelAttribute ("Delay", StringValue (delay));
    //container index starts from 0 but our node id starts from 1 
    NodeContainer tmp_c = NodeContainer (m_NodeContainer.Get (src_id-1), m_NodeContainer.Get (dst_id-1));

    NetDeviceContainer tmp_d = m_p2p.Install (tmp_c);

    set_NetDeviceContainer(src_id, dst_id ,tmp_d);

 }


void Network_controller::create_node(){
    NS_LOG_FUNCTION (this);

    int node_num = m_topo.size();

    m_NodeContainer.Create(node_num);//generate nodes
   
}

void Network_controller::create_all_p2p_channel()
{    
    NS_LOG_FUNCTION (this);
   
    std::pair<int,int> tmp_key;

    std::string data_rate = "1Mbps";
    std::string delay = "1ms";

    for (auto it = m_topo.m.begin(); it != m_topo.m.end(); it++)
    {   
        int src = it->first;

        auto nx = std::next(it,1);

        int dst = nx->first;

        if(it == std::prev(m_topo.m.end())){
            src = it->first;
            dst = m_topo.m.begin()->first;
        }
        
        create_p2p_channel(src, dst, data_rate, delay);
    }
    
      
}




// void Network_controller::create_routing_protocol()
// {
//     // sfc Helper to install sfc routing
//     sfc_routing_helper sfcRoutingHelper;
//     // Ipv4GlobalRoutingHelper globalRoutingHelper;
//     Ipv4NixVectorHelper nixVectorHelper;

//     m_listHelper.Add(sfcRoutingHelper, 0x0009); // higher priority
//     m_listHelper.Add(nixVectorHelper, 0x0008);
//     // m_listHelper.Add(globalRoutingHelper, 0x0007);

    
//     m_stack.SetRoutingHelper (m_listHelper); // has effect on the next Install ()
//     m_stack.Install (m_NodeContainer);


// }


const char* Network_controller::increment_ipv4_address(const char* address_string){

    //const char* address_string= "10.1.1.0";
    in_addr_t address = inet_addr(address_string);
    address = ntohl(address);
    address = address+256; // every time from 10.1.1.0 to 10.1.2.0
    address = htonl(address);
    struct in_addr address_struct;
    address_struct.s_addr = address;
    const char* str = inet_ntoa(address_struct);
    std::cout << str;
    return str;

}



void Network_controller::create_ipv4_address()
{
    NS_LOG_LOGIC ("Assign IP Addresses.");
    
    const char* address_string= "10.1.1.0";

    for (auto it = m_NetDeviceContainer_map.begin(); it != m_NetDeviceContainer_map.end(); it++)
    {   
        
        NetDeviceContainer d = it->second;
        
        Key key = it->first;

        
        NS_LOG_LOGIC(" node id " << key.first << " " << key.second );

        //so maximum 255 link
        if (it != m_NetDeviceContainer_map.begin())
        {
            address_string = increment_ipv4_address(address_string);
        }
        
        // NS_LOG_INFO("address_string" << address_string  );
        m_ipv4.SetBase (address_string, "255.255.255.0");   
        Ipv4InterfaceContainer tmp_i = m_ipv4.Assign (d);

        //update the ipv4interfacecontainer map
        m_Ipv4InterfaceContainer_map.insert(std::pair<Key, Ipv4InterfaceContainer> (key, tmp_i) );
    }
    

}


bool Network_controller::get_Ipv4InterfaceContainer(int src_id, int dst_id, Ipv4InterfaceContainer& i)
{
    std::pair<int,int> key;
    key = std::make_pair(src_id, dst_id);
    auto it = m_Ipv4InterfaceContainer_map.find(key);

    if(it != m_Ipv4InterfaceContainer_map.end()){

        i =  it->second;
        return true;
    }
    else
    {  //reverse the key and find
    
        return false;
    }
         

}

void Network_controller::create_network()
{
    create_node(); //create node and sink apps
    create_all_p2p_channel(); //create p2p channel and store the netdevice container

    m_stack.Install (m_NodeContainer);
    // create_routing_protocol();
    create_ipv4_address();

}

//TODO: MODIFY TO requestTag
//application installed on src node
//send to the dst node, get the address from dst and dst -1;
bool Network_controller::create_traffic(std::string traffic_rate, Request r, int start_time, int end_time)
{

    int src = r.ingress.id;
    int dst = r.deployNode.id;

    Ipv4InterfaceContainer i;
    Ipv4Address addr;

    // NS_LOG_INFO ("get_Ipv4InterfaceContainer.");
    if(get_Ipv4InterfaceContainer(dst-1,dst,i)){
        addr = i.GetAddress (1);

    }
    else if (get_Ipv4InterfaceContainer(dst,dst-1,i))
    {
        addr = i.GetAddress (0);
    }
    else
    {
        // NS_LOG_ERROR(" Network_controller::create_sfc_traffic cant find ipv4container");
        return false;
    }

    // std::string str = std::to_string(traffic_rate);

    // str = str+m_rate;

    NS_LOG_DEBUG( "cadendate str is " << traffic_rate);

    uint16_t port = 8080;   
    traffic_gen_helper onoff ("ns3::TcpSocketFactory", 
                        Address (InetSocketAddress (addr, port)));

    NS_LOG_DEBUG("gen traffic " << src << "-> " <<dst);

    //default packet size 512
    onoff.SetConstantRate (DataRate (traffic_rate) );

    ApplicationContainer apps_network = onoff.Install (m_NodeContainer.Get (src-1));

    m_apps_map.insert(std::make_pair(src, apps_network));

    Ptr<traffic_gen_application> app_t = apps_network.Get(0)->GetObject<traffic_gen_application> ();
    
    requestTag rt;
    rt.id = r.id;
    // rt.ingress = 99;
    rt.arriveTime = r.arriveTime;
    app_t->set_requestTag(rt);// set the byte tag

    apps_network.Start (Seconds(start_time * m_time_slot_length));
    apps_network.Stop (Seconds (end_time* m_time_slot_length));

    return true;
}

void Network_controller::create_request_traffic(Request r)
{   
    if(r.served == false)
    {
        return;
    }
    //we assume all reuqest are finished in one sec
    create_traffic( m_rate, r , r.arriveTime , r.arriveTime + 1);

    NS_LOG_LOGIC(" create traffic " << r.id);
   
    
}


void Network_controller::create_sink(RequestsMap rm){

    NS_LOG_DEBUG( "create on sink");

    for(NodeContainer::Iterator it = m_NodeContainer.Begin(); it != m_NodeContainer.End(); it++)
    {
        // Create a packet sink to receive these packets
        Sfc_sink_Helper sink ("ns3::TcpSocketFactory",
                            Address (InetSocketAddress (Ipv4Address::GetAny (), 8080)));
        ApplicationContainer sink_apps;
        

        sink_apps = sink.Install (*it);

        for(auto index = 0; index < sink_apps.GetN(); index++){
            // sink_apps.Get(index)

            Ptr<Sfc_sink> app_t = sink_apps.Get(index)->GetObject<Sfc_sink> ();
            app_t->setResult(rm);
        } 

        sink_apps.Start (Seconds(0));
        sink_apps.Stop (Seconds(m_slot_num * m_time_slot_length));  

    } 
}

//set up the length of each slot and the number of slot
void Network_controller::set_max_slot(int t_lengh, int slot_num)
{
    m_time_slot_length = t_lengh;
    m_slot_num = slot_num;
}
//for flowmonitor
NodeContainer Network_controller::get_NodeContainer(){
    return m_NodeContainer;
}





}