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
#include "ns3/log.h"
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
#include "serverlessServer.h"

#include "ns3/sfc_tag.h"
#include "ns3/seq-ts-size-header.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("serverlessServer");

NS_OBJECT_ENSURE_REGISTERED (serverlessServer);

TypeId 
serverlessServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::serverlessServer")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<serverlessServer> ()
    .AddAttribute ("Local",
                   "The Address on which to Bind the rx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&serverlessServer::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("Protocol",
                   "The type id of the protocol to use for the rx socket.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&serverlessServer::m_tid),
                   MakeTypeIdChecker ())
    .AddTraceSource ("Rx",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&serverlessServer::m_rxTrace),
                     "ns3::Packet::AddressTracedCallback")
    .AddTraceSource ("RxWithAddresses", "A packet has been received",
                     MakeTraceSourceAccessor (&serverlessServer::m_rxTraceWithAddresses),
                     "ns3::Packet::TwoAddressTracedCallback")
  ;
  return tid;
}

serverlessServer::serverlessServer ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_totalRx = 0;
}

serverlessServer::~serverlessServer()
{
  NS_LOG_FUNCTION (this);
}

uint64_t serverlessServer::GetTotalRx () const
{
  NS_LOG_FUNCTION (this);
  return m_totalRx;
}

Ptr<Socket>
serverlessServer::GetListeningSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

std::list<Ptr<Socket> >
serverlessServer::GetAcceptedSockets (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socketList;
}

void serverlessServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_socketList.clear ();

  // chain up
  Application::DoDispose ();
}


// Application Methods
void serverlessServer::StartApplication ()    // Called at time specified by Start
{
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
  NS_LOG_INFO ("About to receive data");
  m_socket->SetRecvCallback (MakeCallback (&serverlessServer::ReceivedDataCallback,
                                                      this));

  // m_socket->SetRecvCallback (MakeCallback (&serverlessServer::HandleRead, this));
  m_socket->SetAcceptCallback (
    MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
    MakeCallback (&serverlessServer::HandleAccept, this));

  sendResponse();

  m_socket->SetCloseCallbacks (
    MakeCallback (&serverlessServer::HandlePeerClose, this),
    MakeCallback (&serverlessServer::HandlePeerError, this));
}

void serverlessServer::StopApplication ()     // Called at time specified by Stop
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

void serverlessServer::HandleRead (Ptr<Socket> socket)
{
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

      sfc_tag my_tag;
      NS_LOG_ERROR ("about to peek packet tag!");
      if (packet->FindFirstMatchingByteTag (my_tag)){
        NS_LOG_INFO ("Received packet tag!");
        packet->PrintByteTags (std::cout);
      }else{
        NS_LOG_INFO ("NO packet tag!");
      }

      socket->GetSockName (localAddress);
      m_rxTrace (packet, from);
      m_rxTraceWithAddresses (packet, from, localAddress);
    }
}


void serverlessServer::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}
 
void serverlessServer::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}
 

void serverlessServer::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  s->SetRecvCallback (MakeCallback (&serverlessServer::HandleRead, this));
  m_socketList.push_back (s);
}


void serverlessServer::ReceivedDataCallback (Ptr<Socket> socket)
{ 
  NS_LOG_INFO ("START TO RECEIVE DATA");
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  Address from;

  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
      {
        break; // EOF
      }

      #ifdef NS3_LOG_ENABLE
            // Some log messages.
            if (InetSocketAddress::IsMatchingType (from))
              {
                NS_LOG_INFO (this << " A packet of " << packet->GetSize () << " bytes"
                                  << " received from " << InetSocketAddress::ConvertFrom (from).GetIpv4 ()
                                  << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                                  << " / " << InetSocketAddress::ConvertFrom (from));
              }
            else if (Inet6SocketAddress::IsMatchingType (from))
              {
                NS_LOG_INFO (this << " A packet of " << packet->GetSize () << " bytes"
                                  << " received from " << Inet6SocketAddress::ConvertFrom (from).GetIpv6 ()
                                  << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ()
                                  << " / " << Inet6SocketAddress::ConvertFrom (from));
              }
      #endif /* NS3_LOG_ENABLE */

      // Check the header. No need to remove it, since it is not a "real" header.
      sfc_tag my_tag;

      if (packet->FindFirstMatchingByteTag (my_tag)){
        NS_LOG_ERROR ("Received packet tag!");
        packet->PrintByteTags (std::cout);
      }

      

    } // end of `while ((packet = socket->RecvFrom (from)))`

} // end of `void ReceivedDataCallback (Ptr<Socket> socket)`


void serverlessServer::sendResponse(){
      NS_LOG_INFO ("Send response");
      sfc_tag my_tag; // sfc header 
      struct requestTag rt = {9, 9, 9, 9, 9,99};

      my_tag.SetData(rt);
  
      m_pktSize = 512;
      Ptr<Packet> packet;
      
      Address from, to;
      m_socket->GetSockName (from);
      m_socket->GetPeerName (to);
      SeqTsSizeHeader header;
      header.SetSeq (m_seq++);
      header.SetSize (m_pktSize);
      NS_ABORT_IF (m_pktSize < header.GetSerializedSize ());
      packet = Create<Packet> (m_pktSize - header.GetSerializedSize ());
      // Trace before adding header, for consistency with PacketSink
      // m_txTraceWithSeqTsSize (packet, from, to, header);
      packet->AddHeader (header);

      packet->AddByteTag(my_tag);
      packet->PrintByteTags(std::cout);

      m_socket->Send (packet);
    
}


} // Namespace ns3