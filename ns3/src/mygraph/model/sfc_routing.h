
 
 #include "ns3/channel.h"
 #include "ns3/node-container.h"
 #include "ns3/node-list.h"
 #include "ns3/net-device-container.h"
 #include "ns3/ipv4-routing-protocol.h"
 #include "ns3/ipv4-route.h"
 #include "ns3/nix-vector.h"
 #include "ns3/bridge-net-device.h"
 #include "ns3/nstime.h"
 
 #include <map>
 #include <unordered_map>
 #include "ns3/sfc_header.h"
  #include "ns3/ipv4-global-routing.h"
#include "ns3/ipv4-nix-vector-routing.h"
 

#ifndef SFC_ROUTING_H
#define SFC_ROUTING_H
 
 namespace ns3 {
 
 typedef std::map<Ipv4Address, Ptr<NixVector> > NixMap_t;
 typedef std::map<Ipv4Address, Ptr<Ipv4Route> > Ipv4RouteMap_t;
 
 class sfc_routing : public Ipv4GlobalRouting
 {
 public:
   sfc_routing ();
   ~sfc_routing ();
   static TypeId GetTypeId (void);
   void SetNode (Ptr<Node> node);
 
   void FlushGlobalNixRoutingCache (void) const;
 
 private:
 
   void FlushNixCache (void) const;
 
   void FlushIpv4RouteCache (void) const;
 
   void ResetTotalNeighbors (void);
 
 
   Ptr<Ipv4Route> GetIpv4RouteInCache (Ipv4Address address);
 
   void GetAdjacentNetDevices (Ptr<NetDevice> netDevice, Ptr<Channel> channel, NetDeviceContainer & netDeviceContainer);
 
   Ptr<Node> GetNodeByIp (Ipv4Address dest);
 
   uint32_t FindTotalNeighbors (void);
 
   Ptr<BridgeNetDevice> NetDeviceIsBridged (Ptr<NetDevice> nd) const;
 
 
   uint32_t FindNetDeviceForNixIndex (uint32_t nodeIndex, Ipv4Address & gatewayIp);

 
   void DoDispose (void);
 
   /* From Ipv4RoutingProtocol */
   //use this
   virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
   virtual bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                            UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                            LocalDeliverCallback lcb, ErrorCallback ecb);  // use this
   virtual void NotifyInterfaceUp (uint32_t interface);
   virtual void NotifyInterfaceDown (uint32_t interface);
   virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
   virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
   virtual void SetIpv4 (Ptr<Ipv4> ipv4);
   virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;
  
   void CheckCacheStateAndFlush (void) const;
 
   void BuildIpv4AddressToNodeMap (void);

    bool get_node_index (uint8_t node_id_next, uint32_t& nodeIndex);
 
   static bool g_isCacheDirty;
 
   mutable NixMap_t m_nixCache;
 
   mutable Ipv4RouteMap_t m_ipv4RouteCache;
 
   Ptr<Ipv4> m_ipv4; 
   Ptr<Node> m_node; 
 
   uint32_t m_totalNeighbors;
 
 
   struct Ipv4AddressHash
   {
     size_t operator() (const Ipv4Address &address) const
     {
       return std::hash<uint32_t>()(address.Get ());
     }
   };
 
   typedef std::unordered_map<Ipv4Address, ns3::Ptr<ns3::Node>, Ipv4AddressHash > Ipv4AddressToNodeMap;
   static Ipv4AddressToNodeMap g_ipv4AddressToNodeMap;
 };
 } // namespace ns3
 
 #endif /* SFC_ROUTING_H */