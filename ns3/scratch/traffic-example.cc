/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Magister Solutions
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
 * Author: Lauri Sormunen <lauri.sormunen@magister.fi>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic_gen_helper.h"
//  #include "ns3/applications-module.h"
// #include "ns3/mygraph-module.h"
#include "ns3/sfc_tag.h"
#include "ns3/type.h"
#include "ns3/serverlessServer-helper.h"



using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TrafficExample");



int
main (int argc, char *argv[])
{
  double simTimeSec = 300;
  CommandLine cmd;
  cmd.AddValue ("SimulationTime", "Length of simulation in seconds.", simTimeSec);
  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);
  LogComponentEnableAll (LOG_PREFIX_TIME);
  //LogComponentEnableAll (LOG_PREFIX_FUNC);
  //LogComponentEnable ("ThreeGppHttpClient", LOG_INFO);
  ///LogComponentEnable ("ThreeGppHttpServer", LOG_INFO);
  LogComponentEnable ("TrafficExample", LOG_INFO);

  LogComponentEnable ("serverlessServer", LOG_LEVEL_INFO);

  LogComponentEnable ("traffic_gen", LOG_LEVEL_INFO);
 

  // Setup two nodes
  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  // Ipv4GlobalRoutingHelper globalRoutingHelper;
  // Ipv4ListRoutingHelper listHelper;
  // listHelper.Add(globalRoutingHelper, 0x0007);
  // InternetStackHelper stack;
  // stack.SetRoutingHelper (listHelper); // has effect on the next Install ()
  // stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  Ipv4Address serverAddress = interfaces.GetAddress (1);

  struct requestTag rt = {1, 10, 1, 1, 0,99};
  uint16_t port = 8080;   
  traffic_gen_helper onoff ("ns3::TcpSocketFactory", 
                        Address (InetSocketAddress (serverAddress, port)));

  Ptr<Node> src = nodes.Get (0);

//   NS_LOG_DEBUG("gen traffic " << src << "-> " <<dst);

  //default packet size 512
    
  onoff.SetConstantRate (DataRate(10000));

  ApplicationContainer apps_network = onoff.Install (nodes.Get (0));

//   m_apps_map.insert(std::make_pair(src, apps_network));
  Ptr<traffic_gen_application> app_t = apps_network.Get(0)->GetObject<traffic_gen_application> ();

  app_t->set_requestTag(rt);// set the packet tag

  apps_network.Start (Seconds(0));
  apps_network.Stop (Seconds (1));


  // Create a packet sink to receive these packets
  ServerlessServerHelper sink ("ns3::TcpSocketFactory",
                        Address (InetSocketAddress (Ipv4Address::GetAny (), 8080)));
  ApplicationContainer sink_apps;
    

  sink_apps = sink.Install (nodes);
  sink_apps.Start (Seconds(0));
  sink_apps.Stop (Seconds(1));

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}