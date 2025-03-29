/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 * Author:  Tom Henderson (tomhend@u.washington.edu)
 */
#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"

#include "ns3/boolean.h"

#include "sfc_sink.h"
#include "ns3/nstime.h"
#include "type.h"
#include "sfc_tag.h"
#include "operation.h"
// #include "time_stamp.h"
// #include "sfc_id_tag.h"



namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("serverless_server");

NS_OBJECT_ENSURE_REGISTERED (Sfc_sink);

TypeId 
Sfc_sink::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Sfc_sink")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<Sfc_sink> ()
    .AddAttribute ("Local",
                   "The Address on which to Bind the rx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&Sfc_sink::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("Protocol",
                   "The type id of the protocol to use for the rx socket.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&Sfc_sink::m_tid),
                   MakeTypeIdChecker ())
    .AddAttribute ("EnableSeqTsSizeHeader",
                   "Enable optional header tracing of SeqTsSizeHeader",
                   BooleanValue (false),
                   MakeBooleanAccessor (&Sfc_sink::m_enableSeqTsSizeHeader),
                   MakeBooleanChecker ())
    .AddTraceSource ("Rx",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&Sfc_sink::m_rxTrace),
                     "ns3::Packet::AddressTracedCallback")
    .AddTraceSource ("RxWithAddresses", "A packet has been received",
                     MakeTraceSourceAccessor (&Sfc_sink::m_rxTraceWithAddresses),
                     "ns3::Packet::TwoAddressTracedCallback")
    .AddTraceSource ("RxWithSeqTsSize",
                     "A packet with SeqTsSize header has been received",
                     MakeTraceSourceAccessor (&Sfc_sink::m_rxTraceWithSeqTsSize),
                     "ns3::Sfc_sink::SeqTsSizeCallback")
  ;
  return tid;
}

Sfc_sink::Sfc_sink ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_totalRx = 0;
}

Sfc_sink::~Sfc_sink()
{
  NS_LOG_FUNCTION (this);
}

uint64_t Sfc_sink::GetTotalRx () const
{
  NS_LOG_FUNCTION (this);
  return m_totalRx;
}

Ptr<Socket>
Sfc_sink::GetListeningSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

std::list<Ptr<Socket> >
Sfc_sink::GetAcceptedSockets (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socketList;
}

void Sfc_sink::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_socketList.clear ();

  // chain up
  Application::DoDispose ();
}


// Application Methods
void Sfc_sink::StartApplication ()    // Called at time specified by Start
{ NS_LOG_INFO("start sfc_sink");
  NS_LOG_FUNCTION (this);
  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      if (m_socket->Bind (m_local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      m_socket->Listen ();
      m_socket->ShutdownSend ();
      if (addressUtils::IsMulticast (m_local))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, m_local);
            }
          else
            {
              NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
            }
        }
    }
  
  HandleRead(m_socket);
  m_socket->SetRecvCallback (MakeCallback (&Sfc_sink::HandleRead, this));
  m_socket->SetAcceptCallback (
    MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
    MakeCallback (&Sfc_sink::HandleAccept, this));
  m_socket->SetCloseCallbacks (
    MakeCallback (&Sfc_sink::HandlePeerClose, this),
    MakeCallback (&Sfc_sink::HandlePeerError, this));
}

void Sfc_sink::StopApplication ()     // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
  while(!m_socketList.empty ()) //these are accepted sockets, close them
    {
      Ptr<Socket> acceptedSocket = m_socketList.front ();
      m_socketList.pop_front ();
      acceptedSocket->Close ();
    }
  if (m_socket) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void Sfc_sink::HandleRead (Ptr<Socket> socket)
{ NS_LOG_INFO("accept request");
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  Address localAddress;


  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        { //EOF
          break;
        }
      m_totalRx += packet->GetSize ();
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s packet sink received "
                       <<  packet->GetSize () << " bytes from "
                       << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s packet sink received "
                       <<  packet->GetSize () << " bytes from "
                       << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
                       << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
      socket->GetSockName (localAddress);
      m_rxTrace (packet, from);
      m_rxTraceWithAddresses (packet, from, localAddress);

      if (m_enableSeqTsSizeHeader)
        {
          PacketReceived (packet, from, localAddress);
        }


        NS_LOG_INFO(" sfc_sink:handleread:handle timestamp");
       
    //Read the packet tag, write result to the file
    sfc_tag my_tag;
    NS_LOG_LOGIC ("about to peek packet tag!");
    if (packet->FindFirstMatchingByteTag (my_tag)){
        NS_LOG_LOGIC ("Received packet tag!");
        // packet->PrintByteTags (std::cout);

        //set request tag, i only set id and arrive time.
        my_tag.setRequestTag();
        //caculate the latency
        NS_LOG_DEBUG ("m_request_map size " << m_request_map.size() << " tag arrivetime " <<my_tag.m_rt.arriveTime<< "tag id " << my_tag.m_rt.id);
        Request r = m_request_map.find(my_tag.m_rt.arriveTime, my_tag.m_rt.id);

        //write to file

        // if(r.id == -1){
        //   NS_LOG_ERROR("Cannot find the request id");
        //   return ;
        // }else{
        //   NS_LOG_ERROR("find request << " << r.id << " time slot " << r.arriveTime);
        // }
        std::vector<float> result;
        float totalDelay = 0;

        result.push_back(float(r.arriveTime));
        result.push_back(float(r.id));

        result.push_back(float(r.function.type));

        result.push_back(r.linkDelay);

        totalDelay += totalDelay;
        //conver ms to s
        result.push_back(r.function.processingTime/1000);
        totalDelay += r.function.processingTime/1000;

        if(r.isColdStart == true){
            result.push_back(r.function.coldStartTime);
            result.push_back(1.0);
            totalDelay += r.function.coldStartTime;
        }else{
            
            result.push_back(0.0);
            result.push_back(0.0);
        }
        result.push_back(totalDelay);
        // write_vector_file("result/my-result-test.csv", result);
        


    }else{
        NS_LOG_LOGIC ("NO packet tag!");
    }
    }

     
}

void
Sfc_sink::PacketReceived (const Ptr<Packet> &p, const Address &from,
                            const Address &localAddress)
{
  SeqTsSizeHeader header;
  Ptr<Packet> buffer;

  auto itBuffer = m_buffer.find (from);
  if (itBuffer == m_buffer.end ())
    {
      itBuffer = m_buffer.insert (std::make_pair (from, Create<Packet> (0))).first;
    }

  buffer = itBuffer->second;
  buffer->AddAtEnd (p);
  buffer->PeekHeader (header);

  NS_ABORT_IF (header.GetSize () == 0);

  while (buffer->GetSize () >= header.GetSize ())
    {
      NS_LOG_DEBUG ("Removing packet of size " << header.GetSize () << " from buffer of size " << buffer->GetSize ());
      Ptr<Packet> complete = buffer->CreateFragment (0, static_cast<uint32_t> (header.GetSize ()));
      buffer->RemoveAtStart (static_cast<uint32_t> (header.GetSize ()));

      complete->RemoveHeader (header);

      m_rxTraceWithSeqTsSize (complete, from, localAddress, header);

      if (buffer->GetSize () > header.GetSerializedSize ())
        {
          buffer->PeekHeader (header);
        }
      else
        {
          break;
        }
    }
}

void Sfc_sink::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}
 
void Sfc_sink::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

void Sfc_sink::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  s->SetRecvCallback (MakeCallback (&Sfc_sink::HandleRead, this));
  m_socketList.push_back (s);
}

void Sfc_sink::setResult(RequestsMap rm){
  NS_LOG_DEBUG("setResult size " << rm.size());
  m_request_map = rm;
}
  
  
} // Namespace ns3
