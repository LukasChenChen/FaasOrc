/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
 /*
  * Copyright (c) 2009 The Georgia Institute of Technology 
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License version 2 as
  * published by the Free Software Foundation;
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  *
  * Authors: Josh Pelkey <jpelkey@gatech.edu>
  */

 
#include <queue>
#include <iomanip>

#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/names.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/loopback-net-device.h"

#include "sfc_routing.h"
#include "ns3/sfc_header.h"
#include "ns3/sfc_tag.h"
#include "ns3/ipv4-global-routing.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/nstime.h"
#include "ns3/time_stamp.h"
// #include "ns3/wifi-example-apps.h"

NS_LOG_COMPONENT_DEFINE ("sfc_routing");
 namespace ns3 {
 

 
 NS_OBJECT_ENSURE_REGISTERED (sfc_routing);
 
 bool sfc_routing::g_isCacheDirty = false;
 sfc_routing::Ipv4AddressToNodeMap sfc_routing::g_ipv4AddressToNodeMap;
 
 TypeId 
 sfc_routing::GetTypeId (void)
 {
   static TypeId tid = TypeId ("ns3::sfc_routing")
     .SetParent<Ipv4GlobalRouting> ()
     .SetGroupName ("sfc_routing")
     .AddConstructor<sfc_routing> ()
   ;
   return tid;
 }
 
 sfc_routing::sfc_routing ()
   : m_totalNeighbors (0)
 {
   NS_LOG_FUNCTION_NOARGS ();
 }
 
 sfc_routing::~sfc_routing ()
 {
   NS_LOG_FUNCTION_NOARGS ();
 }
 
 void
 sfc_routing::SetIpv4 (Ptr<Ipv4> ipv4)
 {
   NS_ASSERT (ipv4 != 0);
   NS_ASSERT (m_ipv4 == 0);
   NS_LOG_DEBUG ("Created Ipv4SFCProtocol");
 
   m_ipv4 = ipv4;
 }
 
 void 
 sfc_routing::DoDispose ()
 {
   NS_LOG_FUNCTION_NOARGS ();
 
   m_node = 0;
   m_ipv4 = 0;
 
   Ipv4RoutingProtocol::DoDispose ();
 }
 
 
 void
 sfc_routing::SetNode (Ptr<Node> node)
 {
   NS_LOG_FUNCTION_NOARGS ();
 
   m_node = node;
 }
 
 void
 sfc_routing::FlushGlobalNixRoutingCache (void) const
 {
   NS_LOG_FUNCTION_NOARGS ();
   NodeList::Iterator listEnd = NodeList::End ();
   for (NodeList::Iterator i = NodeList::Begin (); i != listEnd; i++)
     {
       Ptr<Node> node = *i;
       Ptr<sfc_routing> rp = node->GetObject<sfc_routing> ();
       if (!rp)
         {
           continue;
         }
       NS_LOG_LOGIC ("Flushing Nix caches.");
       rp->FlushNixCache ();
       rp->FlushIpv4RouteCache ();
     }
 
   // IPv4 address to node mapping is potentially invalid so clear it.
   // Will be repopulated in lazy evaluation when mapping is needed.
   g_ipv4AddressToNodeMap.clear ();
 }
 
 void
 sfc_routing::FlushNixCache (void) const
 {
   NS_LOG_FUNCTION_NOARGS ();
   m_nixCache.clear ();
 }
 
 void
 sfc_routing::FlushIpv4RouteCache (void) const
 {
   NS_LOG_FUNCTION_NOARGS ();
   m_ipv4RouteCache.clear ();
 }
 
 Ptr<Ipv4Route>
 sfc_routing::GetIpv4RouteInCache (Ipv4Address address)
 {
   NS_LOG_FUNCTION_NOARGS ();
 
   CheckCacheStateAndFlush ();
 
   Ipv4RouteMap_t::iterator iter = m_ipv4RouteCache.find (address);
   if (iter != m_ipv4RouteCache.end ())
     {
       NS_LOG_LOGIC ("Found Ipv4Route in cache.");
       return iter->second;
     }
 
   // not in cache
   return 0;
 }
 
 void
 sfc_routing::GetAdjacentNetDevices (Ptr<NetDevice> netDevice, Ptr<Channel> channel, NetDeviceContainer & netDeviceContainer)
 {
   NS_LOG_FUNCTION_NOARGS ();
 
   for (std::size_t i = 0; i < channel->GetNDevices (); i++)
     {
       Ptr<NetDevice> remoteDevice = channel->GetDevice (i);
       if (remoteDevice != netDevice)
         {
           Ptr<BridgeNetDevice> bd = NetDeviceIsBridged (remoteDevice);
           
           // we have a bridged device, we need to add all 
           // bridged devices
           if (bd)
             {
               NS_LOG_LOGIC ("Looking through bridge ports of bridge net device " << bd);
               for (uint32_t j = 0; j < bd->GetNBridgePorts (); ++j)
                 {
                   Ptr<NetDevice> ndBridged = bd->GetBridgePort (j);
                   if (ndBridged == remoteDevice)
                     {
                       NS_LOG_LOGIC ("That bridge port is me, don't walk backward");
                       continue;
                     }
                   Ptr<Channel> chBridged = ndBridged->GetChannel ();
                   if (chBridged == 0)
                     {
                       continue;
                     }
                   GetAdjacentNetDevices (ndBridged, chBridged, netDeviceContainer);
                 }
             }
           else
             {
               netDeviceContainer.Add (channel->GetDevice (i));
             }
         }
     }
 }
 
 void
 sfc_routing::BuildIpv4AddressToNodeMap (void)
 {
   NS_LOG_FUNCTION_NOARGS ();
 
   for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
     {
       Ptr<Node> node = *it;
       Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
 
       if(ipv4)
         {
           uint32_t numberOfDevices = node->GetNDevices ();
 
           for (uint32_t deviceId = 0; deviceId < numberOfDevices; deviceId++)
             {
               Ptr<NetDevice> device = node->GetDevice (deviceId);
 
               // If this is not a loopback device add the IPv4 address to the map
               if ( !DynamicCast<LoopbackNetDevice>(device) )
                 {
                   int32_t interfaceIndex = (ipv4)->GetInterfaceForDevice (node->GetDevice (deviceId));
                   if (interfaceIndex != -1)
                     {
                       Ipv4InterfaceAddress ifAddr = ipv4->GetAddress (interfaceIndex, 0);
                       Ipv4Address addr = ifAddr.GetLocal ();
 
                       NS_ABORT_MSG_IF (g_ipv4AddressToNodeMap.count (addr),
                                        "Duplicate IPv4 address (" << addr << ") found during NIX Vector map construction for node " << node->GetId ());
 
                       NS_LOG_LOGIC ("Adding IPv4 address " << addr << " for node " << node->GetId () << " to NIX Vector IPv4 address to node map");
                       g_ipv4AddressToNodeMap[addr] = node;
                     }
                 }
             }
         }
     }
 }
 
 Ptr<Node>
 sfc_routing::GetNodeByIp (Ipv4Address dest)
 {
   NS_LOG_FUNCTION_NOARGS ();
 
   // Populate lookup table if is empty.
   if ( g_ipv4AddressToNodeMap.empty () )
     {
       BuildIpv4AddressToNodeMap ();
     }
 
   Ptr<Node> destNode;
 
   Ipv4AddressToNodeMap::iterator iter = g_ipv4AddressToNodeMap.find(dest);
 
   if(iter == g_ipv4AddressToNodeMap.end ())
     {
       NS_LOG_ERROR ("Couldn't find dest node given the IP" << dest);
       destNode = 0;
     }
   else
     {
       destNode = iter -> second;
     }
 
   return destNode;
 }
 
 uint32_t
 sfc_routing::FindTotalNeighbors (void)
 {
   uint32_t numberOfDevices = m_node->GetNDevices ();
   uint32_t totalNeighbors = 0;
 
   // scan through the net devices on the parent node
   // and then look at the nodes adjacent to them
   for (uint32_t i = 0; i < numberOfDevices; i++)
     {
       // Get a net device from the node
       // as well as the channel, and figure
       // out the adjacent net devices
       Ptr<NetDevice> localNetDevice = m_node->GetDevice (i);
       Ptr<Channel> channel = localNetDevice->GetChannel ();
       if (channel == 0)
         {
           continue;
         }
 
       // this function takes in the local net dev, and channel, and
       // writes to the netDeviceContainer the adjacent net devs
       NetDeviceContainer netDeviceContainer;
       GetAdjacentNetDevices (localNetDevice, channel, netDeviceContainer);
 
       totalNeighbors += netDeviceContainer.GetN ();
     }
 
   return totalNeighbors;
 }
 
 Ptr<BridgeNetDevice>
 sfc_routing::NetDeviceIsBridged (Ptr<NetDevice> nd) const
 {
   NS_LOG_FUNCTION (nd);
   NS_LOG_FUNCTION (nd->GetAddress());
   NS_LOG_FUNCTION ((nd->GetNode())->GetId());
 
   Ptr<Node> node = nd->GetNode ();
   uint32_t nDevices = node->GetNDevices ();
 
   //
   // There is no bit on a net device that says it is being bridged, so we have
   // to look for bridges on the node to which the device is attached.  If we
   // find a bridge, we need to look through its bridge ports (the devices it
   // bridges) to see if we find the device in question.
   //
   for (uint32_t i = 0; i < nDevices; ++i)
     {
       Ptr<NetDevice> ndTest = node->GetDevice (i);
       NS_LOG_LOGIC ("Examine device " << i << " " << ndTest);
 
       if (ndTest->IsBridge ())
         {
           NS_LOG_LOGIC ("device " << i << " is a bridge net device");
           Ptr<BridgeNetDevice> bnd = ndTest->GetObject<BridgeNetDevice> ();
           NS_ABORT_MSG_UNLESS (bnd, "Ipv4NixVectorRouting::NetDeviceIsBridged (): GetObject for <BridgeNetDevice> failed");
 
           for (uint32_t j = 0; j < bnd->GetNBridgePorts (); ++j)
             {
               NS_LOG_LOGIC ("Examine bridge port " << j << " " << bnd->GetBridgePort (j));
               if (bnd->GetBridgePort (j) == nd)
                 {
                   NS_LOG_LOGIC ("Net device " << nd << " is bridged by " << bnd);
                   return bnd;
                 }
             }
         }
     }
   NS_LOG_LOGIC ("Net device " << nd << " is not bridged");
   return 0;
 }
 
 uint32_t
 sfc_routing::FindNetDeviceForNixIndex (uint32_t nodeIndex, Ipv4Address & gatewayIp)
 {
   uint32_t numberOfDevices = m_node->GetNDevices ();
   uint32_t index = 0;
   uint32_t totalNeighbors = 0;
 
   // scan through the net devices on the parent node
   // and then look at the nodes adjacent to them
   for (uint32_t i = 0; i < numberOfDevices; i++)
     {
       // Get a net device from the node
       // as well as the channel, and figure
       // out the adjacent net devices
       Ptr<NetDevice> localNetDevice = m_node->GetDevice (i);
       Ptr<Channel> channel = localNetDevice->GetChannel ();
       if (channel == 0)
         {
           continue;
         }
 
       // this function takes in the local net dev, and channel, and
       // writes to the netDeviceContainer the adjacent net devs
       NetDeviceContainer netDeviceContainer;
       GetAdjacentNetDevices (localNetDevice, channel, netDeviceContainer);
 
       // check how many neighbors we have
       if (nodeIndex < (totalNeighbors + netDeviceContainer.GetN ()))
         {
           // found the proper net device
           index = i;
           Ptr<NetDevice> gatewayDevice = netDeviceContainer.Get (nodeIndex-totalNeighbors);
           Ptr<Node> gatewayNode = gatewayDevice->GetNode ();
           Ptr<Ipv4> ipv4 = gatewayNode->GetObject<Ipv4> ();
 
           uint32_t interfaceIndex = (ipv4)->GetInterfaceForDevice (gatewayDevice);
           Ipv4InterfaceAddress ifAddr = ipv4->GetAddress (interfaceIndex, 0);
           gatewayIp = ifAddr.GetLocal ();
           break;
         }
       totalNeighbors += netDeviceContainer.GetN ();
     }
 
   return index;
 }
 

 //for packets that generated by this node
 Ptr<Ipv4Route> 
 sfc_routing::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
 {
   NS_LOG_FUNCTION_NOARGS ();
   NS_LOG_DEBUG(" RouteOutput start:");
  
   Ptr<Ipv4Route> rtentry;

    if (p == NULL)
   {
     return rtentry;
     std::cout << "rtentry" << std::endl;
   }

   
  NS_LOG_DEBUG ("Dest IP from header: " << header.GetDestination ());
  //extract from tag
  uint32_t nodeIndex;
    
   
  sfc_tag my_tag;
   
  Ptr<Packet> aCopy = p->Copy();
  
  if(aCopy->PeekPacketTag(my_tag)){

    NS_LOG_DEBUG ("get my_tag:" );

    // aCopy->PrintPacketTags();

    std::vector<uint8_t> v = my_tag.GetData();

    // std::vector<uint8_t> new_v;

    uint32_t next_id = 0xFFFF;

    int find_flag = 0;

    for (auto it = v.begin(); it != v.end()-1; it++)
    {
      // if(find_flag == 1){

      //   new_v.push_back(*it);

      //   if(it== v.end()-2){

      //       new_v.push_back(*(it+1));
      //   }

      //   continue;
      // }


      if (*it == m_node->GetId()) // find the current node, then find next.
      {
    
        next_id = (uint32_t)*(it+1);
        find_flag = 1;
        NS_LOG_DEBUG ("find flag 1" );
        NS_LOG_DEBUG ("CURRENT id " << (unsigned)*it);
        NS_LOG_DEBUG ("next id " << (unsigned)next_id);

        break;


        
        
      
      }
      NS_LOG_DEBUG ("tag path " <<(unsigned)*it);

        
    }

    if (find_flag == 0)
    {
      NS_LOG_DEBUG( "cant find current node " << m_node->GetId());

      return rtentry;
    }
      
    get_node_index(next_id ,nodeIndex);

    // if(new_v.size() > 0){
  
  
    //   sfc_tag new_tag;

    //   new_tag.SetData(new_v);
    //   //remove the current node id
    //   if(p->ReplacePacketTag(new_tag)){
    //     NS_LOG_INFO(" Routeoutput find old tag");
    //   }

    //   NS_LOG_INFO(" Routeoutput print modified tag");

    //   for (auto it = new_v.begin(); it != new_v.end(); it++){
    //     NS_LOG_INFO( (unsigned)*it);
        
    //   }

    // }
  }
  else
  {
    return rtentry;
  }

  // Search here in a cache for this node index 
  // and look for a Ipv4Route
  rtentry = GetIpv4RouteInCache (header.GetDestination ());

  if (!rtentry || !(rtentry->GetOutputDevice () == oif))
    {
      // not in cache or a different specified output
      // device is to be used

      // first, make sure we erase existing (incorrect)
      // rtentry from the map
      if (rtentry)
        {
          m_ipv4RouteCache.erase (header.GetDestination ());
        }


      NS_LOG_DEBUG("Ipv4Route not in cache, build: ");
      Ipv4Address gatewayIp;
      uint32_t index = FindNetDeviceForNixIndex (nodeIndex, gatewayIp);
      int32_t interfaceIndex = 0;

      if (!oif)
        {
          interfaceIndex = (m_ipv4)->GetInterfaceForDevice (m_node->GetDevice (index));
        }
      else
        {
          interfaceIndex = (m_ipv4)->GetInterfaceForDevice (oif);
        }

      NS_ASSERT_MSG (interfaceIndex != -1, "Interface index not found for device");

      Ipv4InterfaceAddress ifAddr = m_ipv4->GetAddress (interfaceIndex, 0);

      // start filling in the Ipv4Route info
      rtentry = Create<Ipv4Route> ();
      rtentry->SetSource (ifAddr.GetLocal ());

      rtentry->SetGateway (gatewayIp);
      rtentry->SetDestination (header.GetDestination ());

      if (!oif)
        {
          rtentry->SetOutputDevice (m_ipv4->GetNetDevice (interfaceIndex));
        }
      else
        {
          rtentry->SetOutputDevice (oif);
        }

      sockerr = Socket::ERROR_NOTERROR;

      // add rtentry to cache
     // m_ipv4RouteCache.insert (Ipv4RouteMap_t::value_type (header.GetDestination (), rtentry));
    }
    else
    {
      NS_LOG_ERROR ("No path to the dest: " << header.GetDestination ());
      sockerr = Socket::ERROR_NOROUTETOHOST;
    }
    
   return rtentry;
 }
 
 //for packets received by this node
 bool 
 sfc_routing::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                                   UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                                   LocalDeliverCallback lcb, ErrorCallback ecb)
 {
   NS_LOG_FUNCTION_NOARGS ();
 
   //CheckCacheStateAndFlush ();

   NS_LOG_INFO("protocol id " <<(unsigned)header.GetProtocol());
   NS_LOG_INFO("payload " <<(unsigned)header.GetPayloadSize());
  //  NS_LOG_INFO("Uid " << (unsigned)p->GetUid());
  
 
   NS_ASSERT (m_ipv4 != 0);
   // Check if input device supports IP
   NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);
   uint32_t iif = m_ipv4->GetInterfaceForDevice (idev);
   
 
   // Local delivery
   if (m_ipv4->IsDestinationAddress (header.GetDestination (), iif))
     {   NS_LOG_INFO(" Local delivery");
       if (!lcb.IsNull ())
         {
           NS_LOG_LOGIC ("Local delivery to " << header.GetDestination ());
           lcb (p, header, iif);
           return true;
         }
       else
         {
           // The local delivery callback is null.  This may be a multicast
           // or broadcast packet, so return false so that another
           // multicast routing protocol can handle it.  It should be possible
           // to extend this to explicitly check whether it is a unicast
           // packet, and invoke the error callback if so
           return false;
         }
     }
 
   Ptr<Ipv4Route> rtentry;
   

   //extract the routing path
   sfc_tag my_tag;
   Ptr<Packet> aCopy = p->Copy ();
   uint32_t nodeIndex;
   if(aCopy->PeekPacketTag (my_tag)){
      std::vector<uint8_t> v = my_tag.GetData();
      NS_LOG_INFO(" Route input print tag");
      for (auto it = v.begin(); it != v.end(); it++){
        NS_LOG_INFO( (unsigned)*(it));
        
      }



      uint32_t next_id = 0xFFFE;
      std::vector<uint8_t> new_v;


      int find_flag = 0;

      NS_LOG_INFO(" route input: curr node : " <<(unsigned) m_node->GetId());

      for (auto it = v.begin(); it != v.end()-1; it++)
      { //std::cout << " sfc my_tag is " << (unsigned)*it;
        //TODO:

        //REMOVE THE CURRENT ONE
        if(find_flag == 1){

              new_v.push_back(*it);

              if(it== v.end()-2){

                  new_v.push_back(*(it+1));
              }

              continue;
        }


        if (*it == m_node->GetId())
        { 
          next_id = (uint32_t)*(it+1);

          NS_LOG_INFO(" route input: this node :" << m_node->GetId() << "next hop " <<(uint32_t)*(it+1) );
          
          find_flag = 1;
          
        }
        // NS_LOG_INFO(" route input: next hop: " << (unsigned)*(it+1));
        // if (it == v.end()-2)
        // {
        //   NS_LOG_INFO(" route input: route path: " << (unsigned)*(it+1));
        // }
        
      }
      std::cout <<std::endl;

      if (next_id == 0x00FF||next_id == 0xFFFF)
      {
        std::cout << "THIS DEVICE IS THE LAST " << std::endl;
        if (!lcb.IsNull ())
            {
              NS_LOG_LOGIC ("Local delivery to " << header.GetDestination ());
              lcb (p, header, iif);
              return true;
            }
      }
     

      
      get_node_index(next_id ,nodeIndex);

      if(new_v.size() > 0){
  
        sfc_tag new_tag;

        new_tag.SetData(new_v);
        //remove the current node id
        if(aCopy->ReplacePacketTag(new_tag)){
          NS_LOG_INFO(" route input find old tag");
        }

        NS_LOG_INFO(" route input print modified tag");

        for (auto it = new_v.begin(); it != new_v.end(); it++){
          NS_LOG_INFO( (unsigned)*it);
          
        }

      }  

    }
    else
    {
      return false;
    }
   
   rtentry = GetIpv4RouteInCache (header.GetDestination ());
   // not in cache
   if (!rtentry)
     {
       NS_LOG_LOGIC ("Ipv4Route not in cache, build: ");
       Ipv4Address gatewayIp;

       // nodeIdex point to the node of next hop
       uint32_t index = FindNetDeviceForNixIndex (nodeIndex, gatewayIp);
       uint32_t interfaceIndex = (m_ipv4)->GetInterfaceForDevice (m_node->GetDevice (index));
       Ipv4InterfaceAddress ifAddr = m_ipv4->GetAddress (interfaceIndex, 0);
 
       // start filling in the Ipv4Route info
       rtentry = Create<Ipv4Route> ();
       rtentry->SetSource (ifAddr.GetLocal ());
 
       rtentry->SetGateway (gatewayIp);
       rtentry->SetDestination (header.GetDestination ());
       rtentry->SetOutputDevice (m_ipv4->GetNetDevice (interfaceIndex));
       
 
       // add rtentry to cache
      // m_ipv4RouteCache.insert (Ipv4RouteMap_t::value_type (header.GetDestination (), rtentry));
     }
 
   // call the unicast callback
   // local deliver is handled by Ipv4StaticRoutingImpl
   // so this code is never even called if the packet is
   // destined for this node.
   //Modified use aCopy here
   ucb (rtentry, aCopy, header);



 
 
   return true;
 }
 
 void
 sfc_routing::PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
 {
 
   CheckCacheStateAndFlush ();
 
   std::ostream* os = stream->GetStream ();
 
   *os << "Node: " << m_ipv4->GetObject<Node> ()->GetId ()
       << ", Time: " << Now().As (unit)
       << ", Local time: " << GetObject<Node> ()->GetLocalTime ().As (unit)
       << ", Nix Routing" << std::endl;
 
   *os << "NixCache:" << std::endl;
   if (m_nixCache.size () > 0)
     {
       *os << "Destination     NixVector" << std::endl;
       for (NixMap_t::const_iterator it = m_nixCache.begin (); it != m_nixCache.end (); it++)
         {
           std::ostringstream dest;
           dest << it->first;
           *os << std::setiosflags (std::ios::left) << std::setw (16) << dest.str ();
           *os << *(it->second) << std::endl;
         }
     }
   *os << "Ipv4RouteCache:" << std::endl;
   if (m_ipv4RouteCache.size () > 0)
     {
       *os << "Destination     Gateway         Source            OutputDevice" << std::endl;
       for (Ipv4RouteMap_t::const_iterator it = m_ipv4RouteCache.begin (); it != m_ipv4RouteCache.end (); it++)
         {
           std::ostringstream dest, gw, src;
           dest << it->second->GetDestination ();
           *os << std::setiosflags (std::ios::left) << std::setw (16) << dest.str ();
           gw << it->second->GetGateway ();
           *os << std::setiosflags (std::ios::left) << std::setw (16) << gw.str ();
           src << it->second->GetSource ();
           *os << std::setiosflags (std::ios::left) << std::setw (16) << src.str ();
           *os << "  ";
           if (Names::FindName (it->second->GetOutputDevice ()) != "")
             {
               *os << Names::FindName (it->second->GetOutputDevice ());
             }
           else
             {
               *os << it->second->GetOutputDevice ()->GetIfIndex ();
             }
           *os << std::endl;
         }
     }
   *os << std::endl;
 }
 
 // virtual functions from Ipv4RoutingProtocol 
 void
 sfc_routing::NotifyInterfaceUp (uint32_t i)
 {
   g_isCacheDirty = true;
 }
 void
 sfc_routing::NotifyInterfaceDown (uint32_t i)
 {
   g_isCacheDirty = true;
 }
 void
 sfc_routing::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address)
 {
   g_isCacheDirty = true;
 }
 void
 sfc_routing::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address)
 {
   g_isCacheDirty = true;
 }
 
 
 void 
 sfc_routing::CheckCacheStateAndFlush (void) const
 {
   if (g_isCacheDirty)
     {
       FlushGlobalNixRoutingCache ();
       g_isCacheDirty = false;
     }
 }


// map the node id to nodeIdex which corresponds to the output interface
 bool
 sfc_routing::get_node_index (uint8_t node_id_next, uint32_t& nodeIndex)
 {
    uint32_t numberOfDevices = m_node->GetNDevices ();
    uint32_t totalNeighbors = 0;
 
    // scan through the net devices on the parent node
    // and then look at the nodes adjacent to them
    for (uint32_t i = 0; i < numberOfDevices; i++)
      {
        // Get a net device from the node
        // as well as the channel, and figure
        // out the adjacent net devices
        Ptr<NetDevice> localNetDevice = m_node->GetDevice (i);
        Ptr<Channel> channel = localNetDevice->GetChannel ();
        if (channel == 0)
          {
            continue;
          }
  
        // this function takes in the local net dev, and channel, and
        // writes to the netDeviceContainer the adjacent net devs
        NetDeviceContainer netDeviceContainer;
        GetAdjacentNetDevices (localNetDevice, channel, netDeviceContainer);

        // loop over the netdevice container
        uint32_t tmp_index = 0;
        for (auto it = netDeviceContainer.Begin(); it != netDeviceContainer.End(); it++)
        {
          Ptr<NetDevice> gatewayDevice = *it;
          Ptr<Node> gatewayNode = gatewayDevice->GetNode ();
          uint32_t next_id = gatewayNode->GetId();

          if (next_id == node_id_next )// found the next hop
          {
            nodeIndex = totalNeighbors+tmp_index;

            return true;
          }

          tmp_index++;
          
        }

        totalNeighbors += netDeviceContainer.GetN ();
        
    
      }
 
   return false;
 }
 
 } // namespace ns3


