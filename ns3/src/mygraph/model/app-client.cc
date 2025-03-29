/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Magister Solutions
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
 * Author: Budiarto Herman <budiarto.herman@magister.fi>
 *
 */

#include "app-client.h"

#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/callback.h>
#include <ns3/pointer.h>
#include <ns3/uinteger.h>
#include <ns3/double.h>
#include <ns3/three-gpp-http-variables.h>
#include <ns3/packet.h>
#include <ns3/socket.h>
#include <ns3/tcp-socket-factory.h>
#include <ns3/inet-socket-address.h>
#include <ns3/inet6-socket-address.h>
#include <ns3/unused.h>
// #include <ns3/sfc_tag.h>

#include <string>

NS_LOG_COMPONENT_DEFINE ("AppClient");


namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (AppClient);


AppClient::AppClient ()
  : m_state (NOT_STARTED),
  m_socket (0),
  m_objectBytesToBeReceived (0),
  m_objectClientTs (MilliSeconds (0)),
  m_objectServerTs (MilliSeconds (0)),
  m_embeddedObjectsToBeRequested (0),
  m_httpVariables (CreateObject<ThreeGppHttpVariables> ())
{
  NS_LOG_FUNCTION (this);
}


// static
TypeId
AppClient::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::AppClient")
    .SetParent<Application> ()
    .AddConstructor<AppClient> ()
    .AddAttribute ("Variables",
                   "Variable collection, which is used to control e.g. timing and HTTP request size.",
                   PointerValue (),
                   MakePointerAccessor (&AppClient::m_httpVariables),
                   MakePointerChecker<ThreeGppHttpVariables> ())
    .AddAttribute ("RemoteServerAddress",
                   "The address of the destination server.",
                   AddressValue (),
                   MakeAddressAccessor (&AppClient::m_remoteServerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemoteServerPort",
                   "The destination port of the outbound packets.",
                   UintegerValue (80), // the default HTTP port
                   MakeUintegerAccessor (&AppClient::m_remoteServerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddTraceSource ("ConnectionEstablished",
                     "Connection to the destination web server has been established.",
                     MakeTraceSourceAccessor (&AppClient::m_connectionEstablishedTrace),
                     "ns3::AppClient::TracedCallback")
    .AddTraceSource ("ConnectionClosed",
                     "Connection to the destination web server is closed.",
                     MakeTraceSourceAccessor (&AppClient::m_connectionClosedTrace),
                     "ns3::AppClient::TracedCallback")
    .AddTraceSource ("Tx",
                     "General trace for sending a packet of any kind.",
                     MakeTraceSourceAccessor (&AppClient::m_txTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("TxMainObjectRequest",
                     "Sent a request for a main object.",
                     MakeTraceSourceAccessor (&AppClient::m_txMainObjectRequestTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("TxEmbeddedObjectRequest",
                     "Sent a request for an embedded object.",
                     MakeTraceSourceAccessor (&AppClient::m_txEmbeddedObjectRequestTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("RxMainObjectPacket",
                     "A packet of main object has been received.",
                     MakeTraceSourceAccessor (&AppClient::m_rxMainObjectPacketTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("RxMainObject",
                     "Received a whole main object. Header is included.",
                     MakeTraceSourceAccessor (&AppClient::m_rxMainObjectTrace),
                     "ns3::AppClient::TracedCallback")
    .AddTraceSource ("RxEmbeddedObjectPacket",
                     "A packet of embedded object has been received.",
                     MakeTraceSourceAccessor (&AppClient::m_rxEmbeddedObjectPacketTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("RxEmbeddedObject",
                     "Received a whole embedded object. Header is included.",
                     MakeTraceSourceAccessor (&AppClient::m_rxEmbeddedObjectTrace),
                     "ns3::AppClient::TracedCallback")
    .AddTraceSource ("Rx",
                     "General trace for receiving a packet of any kind.",
                     MakeTraceSourceAccessor (&AppClient::m_rxTrace),
                     "ns3::Packet::PacketAddressTracedCallback")
    .AddTraceSource ("RxDelay",
                     "General trace of delay for receiving a complete object.",
                     MakeTraceSourceAccessor (&AppClient::m_rxDelayTrace),
                     "ns3::Application::DelayAddressCallback")
    .AddTraceSource ("RxRtt",
                     "General trace of round trip delay time for receiving a complete object.",
                     MakeTraceSourceAccessor (&AppClient::m_rxRttTrace),
                     "ns3::Application::DelayAddressCallback")
    .AddTraceSource ("StateTransition",
                     "Trace fired upon every HTTP client state transition.",
                     MakeTraceSourceAccessor (&AppClient::m_stateTransitionTrace),
                     "ns3::Application::StateTransitionCallback")
  ;
  return tid;
}


Ptr<Socket>
AppClient::GetSocket () const
{
  return m_socket;
}


AppClient::State_t
AppClient::GetState () const
{
  return m_state;
}


std::string
AppClient::GetStateString () const
{
  return GetStateString (m_state);
}

// static
std::string
AppClient::GetStateString (AppClient::State_t state)
{
  switch (state)
    {
    case NOT_STARTED:
      return "NOT_STARTED";
      break;
    case CONNECTING:
      return "CONNECTING";
      break;
    case EXPECTING_MAIN_OBJECT:
      return "EXPECTING_MAIN_OBJECT";
      break;
    case PARSING_MAIN_OBJECT:
      return "PARSING_MAIN_OBJECT";
      break;
    case EXPECTING_EMBEDDED_OBJECT:
      return "EXPECTING_EMBEDDED_OBJECT";
      break;
    case READING:
      return "READING";
      break;
    case STOPPED:
      return "STOPPED";
      break;
    default:
      NS_FATAL_ERROR ("Unknown state");
      return "FATAL_ERROR";
      break;
    }
}


void
AppClient::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  if (!Simulator::IsFinished ())
    {
      StopApplication ();
    }

  Application::DoDispose (); // Chain up.
}


void
AppClient::StartApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_state == NOT_STARTED)
    {
      m_httpVariables->Initialize ();
      OpenConnection ();
    }
  else
    {
      NS_FATAL_ERROR ("Invalid state " << GetStateString ()
                                       << " for StartApplication().");
    }
}


void
AppClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  SwitchToState (STOPPED);
  CancelAllPendingEvents ();
  m_socket->Close ();
  m_socket->SetConnectCallback (MakeNullCallback<void, Ptr<Socket> > (),
                                MakeNullCallback<void, Ptr<Socket> > ());
  m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
}

//TODO:
void
AppClient::ConnectionSucceededCallback (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  if (m_state == CONNECTING)
    {
      NS_ASSERT_MSG (m_socket == socket, "Invalid socket.");
      m_connectionEstablishedTrace (this);
      socket->SetRecvCallback (MakeCallback (&AppClient::ReceivedDataCallback,
                                             this));
      NS_ASSERT (m_embeddedObjectsToBeRequested == 0);
      m_eventRequestMainObject = Simulator::ScheduleNow (
          &AppClient::RequestMainObject, this);
    }
  else
    {
      NS_FATAL_ERROR ("Invalid state " << GetStateString ()
                                       << " for ConnectionSucceeded().");
    }
}


void
AppClient::ConnectionFailedCallback (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  if (m_state == CONNECTING)
    {
      NS_LOG_ERROR ("Client failed to connect"
                    << " to remote address " << m_remoteServerAddress
                    << " port " << m_remoteServerPort << ".");
    }
  else
    {
      NS_FATAL_ERROR ("Invalid state " << GetStateString ()
                                       << " for ConnectionFailed().");
    }
}


void
AppClient::NormalCloseCallback (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  CancelAllPendingEvents ();

  if (socket->GetErrno () != Socket::ERROR_NOTERROR)
    {
      NS_LOG_ERROR (this << " Connection has been terminated,"
                         << " error code: " << socket->GetErrno () << ".");
    }

  m_socket->SetCloseCallbacks (MakeNullCallback<void, Ptr<Socket> > (),
                               MakeNullCallback<void, Ptr<Socket> > ());

  m_connectionClosedTrace (this);
}


void
AppClient::ErrorCloseCallback (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  CancelAllPendingEvents ();
  if (socket->GetErrno () != Socket::ERROR_NOTERROR)
    {
      NS_LOG_ERROR (this << " Connection has been terminated,"
                         << " error code: " << socket->GetErrno () << ".");
    }

  m_connectionClosedTrace (this);
}


void
AppClient::ReceivedDataCallback (Ptr<Socket> socket)
{
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
                            << " / " << InetSocketAddress::ConvertFrom (from) << ".");
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO (this << " A packet of " << packet->GetSize () << " bytes"
                            << " received from " << Inet6SocketAddress::ConvertFrom (from).GetIpv6 ()
                            << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ()
                            << " / " << Inet6SocketAddress::ConvertFrom (from) << ".");
        }
#endif /* NS3_LOG_ENABLE */

      m_rxTrace (packet, from);

      switch (m_state)
        {
        case EXPECTING_MAIN_OBJECT:
          ReceiveMainObject (packet, from);
          break;
        case EXPECTING_EMBEDDED_OBJECT:
          ReceiveEmbeddedObject (packet, from);
          break;
        default:
          NS_FATAL_ERROR ("Invalid state " << GetStateString ()
                                           << " for ReceivedData().");
          break;
        }

    } // end of `while ((packet = socket->RecvFrom (from)))`

} // end of `void ReceivedDataCallback (Ptr<Socket> socket)`


void
AppClient::OpenConnection ()
{
  NS_LOG_FUNCTION (this);

  if (m_state == NOT_STARTED || m_state == EXPECTING_EMBEDDED_OBJECT
      || m_state == PARSING_MAIN_OBJECT || m_state == READING)
    {
      m_socket = Socket::CreateSocket (GetNode (),
                                       TcpSocketFactory::GetTypeId ());

      int ret;

      if (Ipv4Address::IsMatchingType (m_remoteServerAddress))
        {
          ret = m_socket->Bind ();
          NS_LOG_DEBUG (this << " Bind() return value= " << ret
                             << " GetErrNo= " << m_socket->GetErrno () << ".");

          Ipv4Address ipv4 = Ipv4Address::ConvertFrom (m_remoteServerAddress);
          InetSocketAddress inetSocket = InetSocketAddress (ipv4,
                                                            m_remoteServerPort);
          NS_LOG_INFO (this << " Connecting to " << ipv4
                            << " port " << m_remoteServerPort
                            << " / " << inetSocket << ".");
          ret = m_socket->Connect (inetSocket);
          NS_LOG_DEBUG (this << " Connect() return value= " << ret
                             << " GetErrNo= " << m_socket->GetErrno () << ".");
        }
      else if (Ipv6Address::IsMatchingType (m_remoteServerAddress))
        {
          ret = m_socket->Bind6 ();
          NS_LOG_DEBUG (this << " Bind6() return value= " << ret
                             << " GetErrNo= " << m_socket->GetErrno () << ".");

          Ipv6Address ipv6 = Ipv6Address::ConvertFrom (m_remoteServerAddress);
          Inet6SocketAddress inet6Socket = Inet6SocketAddress (ipv6,
                                                               m_remoteServerPort);
          NS_LOG_INFO (this << " connecting to " << ipv6
                            << " port " << m_remoteServerPort
                            << " / " << inet6Socket << ".");
          ret = m_socket->Connect (inet6Socket);
          NS_LOG_DEBUG (this << " Connect() return value= " << ret
                             << " GetErrNo= " << m_socket->GetErrno () << ".");
        }

      NS_UNUSED (ret); // Mute compiler warning.
      NS_ASSERT_MSG (m_socket != 0, "Failed creating socket.");

      SwitchToState (CONNECTING);

      m_socket->SetConnectCallback (MakeCallback (&AppClient::ConnectionSucceededCallback,
                                                  this),
                                    MakeCallback (&AppClient::ConnectionFailedCallback,
                                                  this));
      m_socket->SetCloseCallbacks (MakeCallback (&AppClient::NormalCloseCallback,
                                                 this),
                                   MakeCallback (&AppClient::ErrorCloseCallback,
                                                 this));
      m_socket->SetRecvCallback (MakeCallback (&AppClient::ReceivedDataCallback,
                                               this));
      m_socket->SetAttribute ("MaxSegLifetime", DoubleValue (0.02)); // 20 ms.

    } // end of `if (m_state == {NOT_STARTED, EXPECTING_EMBEDDED_OBJECT, PARSING_MAIN_OBJECT, READING})`
  else
    {
      NS_FATAL_ERROR ("Invalid state " << GetStateString ()
                                       << " for OpenConnection().");
    }

} // end of `void OpenConnection ()`

//TODO: Modify there to add contents
void
AppClient::RequestMainObject ()
{
  NS_LOG_FUNCTION (this);

  if (m_state == CONNECTING || m_state == READING)
    {
      ThreeGppHttpHeader header;
      header.SetContentLength (0); // Request does not need any content length.
      header.SetContentType (ThreeGppHttpHeader::MAIN_OBJECT);
      header.SetClientTs (Simulator::Now ());

      // header.SetSeq (m_seq++);
      // header.SetSize (100);
      // packet = Create<Packet> (100 - header.GetSerializedSize ());

      const uint32_t requestSize = m_httpVariables->GetRequestSize ();

       //add string to packet
      // std::string str = "hello world";
      // uint8_t buffer[255];
      // uint32_t len=str.length();
      // for(uint32_t i=0; i<len;i++){
      //   buffer[i] = str[i];
      // }
      // buffer[len] = '\0';
      uint8_t data[4] = {(uint8_t)9,(uint8_t)8,(uint8_t)7,(uint8_t)6};
      Ptr<Packet> packet = Create<Packet> (data, sizeof(data));


      // Ptr<Packet> packet = Create<Packet> (requestSize);
      packet->AddHeader (header);

      const uint32_t packetSize = packet->GetSize ();
      m_txMainObjectRequestTrace (packet);
      m_txTrace (packet);

      //  //add tag
      // sfc_tag my_tag; 
      // my_tag.SetData (m_rt);
      // packet->AddPacketTag(my_tag);
      // NS_LOG_ERROR ("Client add tag" );
      // packet->PrintPacketTags (std::cout);


     
       //check the data
      uint8_t check[4];
      packet->CopyData(check, sizeof(check));


      NS_LOG_ERROR ("client send data size " << sizeof(check) );

      for (int i = 0; i < 4; i++) {

          std::cout << "client data" << check[i] + 0 << std::endl;
      }


      const int actualBytes = m_socket->Send (packet);
      
      NS_LOG_DEBUG (this << " Send() packet " << packet
                         << " of " << packet->GetSize () << " bytes,"
                         << " return value= " << actualBytes << ".");
      if (actualBytes != static_cast<int> (packetSize))
        {
          NS_LOG_ERROR (this << " Failed to send request for embedded object,"
                             << " GetErrNo= " << m_socket->GetErrno () << ","
                             << " waiting for another Tx opportunity.");
        }
      else
        {
          SwitchToState (EXPECTING_MAIN_OBJECT);
        }
    }
  else
    {
      NS_FATAL_ERROR ("Invalid state " << GetStateString ()
                                       << " for RequestMainObject().");
    }

} // end of `void RequestMainObject ()`


void
AppClient::RequestEmbeddedObject ()
{
  NS_LOG_FUNCTION (this);

  if (m_state == CONNECTING || m_state == PARSING_MAIN_OBJECT
      || m_state == EXPECTING_EMBEDDED_OBJECT)
    {
      if (m_embeddedObjectsToBeRequested > 0)
        {
          ThreeGppHttpHeader header;
          header.SetContentLength (0); // Request does not need any content length.
          header.SetContentType (ThreeGppHttpHeader::EMBEDDED_OBJECT);
          header.SetClientTs (Simulator::Now ());

          const uint32_t requestSize = m_httpVariables->GetRequestSize ();

          //add string to packet
          uint8_t data[4] = {9,8,7,6};
          // uint8_t buffer[255];
          // uint32_t len = str.length();
          // for(uint32_t i=0; i<sizeof(data);i++){
          //   buffer[i] =data[i];
          // }
          // buffer[len] = '\0';

          Ptr<Packet> packet = Create<Packet> (data, sizeof(data));

          // Ptr<Packet> packet = Create<Packet> (requestSize);
          NS_LOG_ERROR ("BEOFRE TAG PACKET SIZE" << packet->GetSize ());
          packet->AddHeader (header);

        
          
          const uint32_t packetSize = packet->GetSize ();
          NS_LOG_ERROR ("PACKET SIZE" << packet->GetSize ());
          m_txEmbeddedObjectRequestTrace (packet);
          m_txTrace (packet);


           //TODO:
          //add tag
          // sfc_tag my_tag;
          // my_tag.SetData (m_rt);
          // packet->AddPacketTag(my_tag);
          // NS_LOG_ERROR ("AFTER TAG PACKET SIZE" << packet->GetSize ());
          // NS_LOG_ERROR ("Client add tag" );
          // packet->PrintPacketTags (std::cout);


          const int actualBytes = m_socket->Send (packet);
          NS_LOG_DEBUG (this << " Send() packet " << packet
                             << " of " << packet->GetSize () << " bytes,"
                             << " return value= " << actualBytes << ".");

          if (actualBytes != static_cast<int> (packetSize))
            {
              NS_LOG_ERROR (this << " Failed to send request for embedded object,"
                                 << " GetErrNo= " << m_socket->GetErrno () << ","
                                 << " waiting for another Tx opportunity.");
            }
          else
            {
              m_embeddedObjectsToBeRequested--;
              SwitchToState (EXPECTING_EMBEDDED_OBJECT);
            }
        }
      else
        {
          NS_LOG_WARN (this << " No embedded object to be requested.");
        }
    }
  else
    {
      NS_FATAL_ERROR ("Invalid state " << GetStateString ()
                                       << " for RequestEmbeddedObject().");
    }

} // end of `void RequestEmbeddedObject ()`


void
AppClient::ReceiveMainObject (Ptr<Packet> packet, const Address &from)
{
  NS_LOG_FUNCTION (this << packet << from);

  if (m_state == EXPECTING_MAIN_OBJECT)
    {
      /*
       * In the following call to Receive(), #m_objectBytesToBeReceived *will*
       * be updated. #m_objectClientTs and #m_objectServerTs *may* be updated.
       * ThreeGppHttpHeader will be removed from the packet, if it is the first
       * packet of the object to be received; the header will be available in
       * #m_constructedPacketHeader.
       * #m_constructedPacket will also be updated.
       */
      Receive (packet);
      m_rxMainObjectPacketTrace (packet);

      if (m_objectBytesToBeReceived > 0)
        {
          /*
           * There are more packets of this main object, so just stay still
           * and wait until they arrive.
           */
          NS_LOG_INFO (this << " " << m_objectBytesToBeReceived << " byte(s)"
                            << " remains from this chunk of main object.");
        }
      else
        {
          /*
           * This is the last packet of this main object. Acknowledge the
           * reception of a whole main object
           */
          NS_LOG_INFO (this << " Finished receiving a main object.");
          m_rxMainObjectTrace (this, m_constructedPacket);

          if (!m_objectServerTs.IsZero ())
            {
              m_rxDelayTrace (Simulator::Now () - m_objectServerTs, from);
              m_objectServerTs = MilliSeconds (0); // Reset back to zero.
            }

          if (!m_objectClientTs.IsZero ())
            {
              m_rxRttTrace (Simulator::Now () - m_objectClientTs, from);
              m_objectClientTs = MilliSeconds (0); // Reset back to zero.
            }

          EnterParsingTime ();

        } // end of else of `if (m_objectBytesToBeReceived > 0)`

    } // end of `if (m_state == EXPECTING_MAIN_OBJECT)`
  else
    {
      NS_FATAL_ERROR ("Invalid state " << GetStateString ()
                                       << " for ReceiveMainObject().");
    }

} // end of `void ReceiveMainObject (Ptr<Packet> packet)`


void
AppClient::ReceiveEmbeddedObject (Ptr<Packet> packet, const Address &from)
{
  NS_LOG_FUNCTION (this << packet << from);

  if (m_state == EXPECTING_EMBEDDED_OBJECT)
    {
      /*
       * In the following call to Receive(), #m_objectBytesToBeReceived *will*
       * be updated. #m_objectClientTs and #m_objectServerTs *may* be updated.
       * ThreeGppHttpHeader will be removed from the packet, if it is the first
       * packet of the object to be received; the header will be available in
       * #m_constructedPacket, which will also be updated.
       */
      Receive (packet);
      m_rxEmbeddedObjectPacketTrace (packet);

      if (m_objectBytesToBeReceived > 0)
        {
          /*
           * There are more packets of this embedded object, so just stay
           * still and wait until they arrive.
           */
          NS_LOG_INFO (this << " " << m_objectBytesToBeReceived << " byte(s)"
                            << " remains from this chunk of embedded object");
        }
      else
        {
          /*
           * This is the last packet of this embedded object. Acknowledge
           * the reception of a whole embedded object
           */
          NS_LOG_INFO (this << " Finished receiving an embedded object.");
          m_rxEmbeddedObjectTrace (this, m_constructedPacket);

          if (!m_objectServerTs.IsZero ())
            {
              m_rxDelayTrace (Simulator::Now () - m_objectServerTs, from);
              m_objectServerTs = MilliSeconds (0); // Reset back to zero.
            }

          if (!m_objectClientTs.IsZero ())
            {
              m_rxRttTrace (Simulator::Now () - m_objectClientTs, from);
              m_objectClientTs = MilliSeconds (0); // Reset back to zero.
            }

          if (m_embeddedObjectsToBeRequested > 0)
            {
              NS_LOG_INFO (this << " " << m_embeddedObjectsToBeRequested
                                << " more embedded object(s) to be requested.");
              // Immediately request another using the existing connection.
              m_eventRequestEmbeddedObject = Simulator::ScheduleNow (
                  &AppClient::RequestEmbeddedObject, this);
            }
          else
            {
              /*
               * There is no more embedded object, the web page has been
               * downloaded completely. Now is the time to read it.
               */
              NS_LOG_INFO (this << " Finished receiving a web page.");
              EnterReadingTime ();
            }

        } // end of else of `if (m_objectBytesToBeReceived > 0)`

    } // end of `if (m_state == EXPECTING_EMBEDDED_OBJECT)`
  else
    {
      NS_FATAL_ERROR ("Invalid state " << GetStateString ()
                                       << " for ReceiveEmbeddedObject().");
    }

} // end of `void ReceiveEmbeddedObject (Ptr<Packet> packet)`


void
AppClient::Receive (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);

  /* In a "real" HTTP message the message size is coded differently. The use of a header
   * is to avoid the burden of doing a real message parser.
   */
  bool firstPacket = false;

  if (m_objectBytesToBeReceived == 0)
    {
      // This is the first packet of the object.
      firstPacket = true;

      // Remove the header in order to calculate remaining data to be received.
      ThreeGppHttpHeader httpHeader;
      packet->RemoveHeader (httpHeader);

      m_objectBytesToBeReceived = httpHeader.GetContentLength ();
      m_objectClientTs = httpHeader.GetClientTs ();
      m_objectServerTs = httpHeader.GetServerTs ();

      // Take a copy for constructed packet trace. Note that header is included.
      m_constructedPacket = packet->Copy ();
      m_constructedPacket->AddHeader (httpHeader);
    }
  uint32_t contentSize = packet->GetSize ();

  /* Note that the packet does not contain header at this point.
   * The content is purely raw data, which was the only intended data to be received.
   */
  if (m_objectBytesToBeReceived < contentSize)
    {
      NS_LOG_WARN (this << " The received packet"
                        << " (" << contentSize << " bytes of content)"
                        << " is larger than"
                        << " the content that we expected to receive"
                        << " (" << m_objectBytesToBeReceived << " bytes).");
      // Stop expecting any more packet of this object.
      m_objectBytesToBeReceived = 0;
      m_constructedPacket = NULL;
    }
  else
    {
      m_objectBytesToBeReceived -= contentSize;
      if (!firstPacket)
        {
          Ptr<Packet> packetCopy = packet->Copy ();
          m_constructedPacket->AddAtEnd (packetCopy);
        }
    }

} // end of `void Receive (packet)`


void
AppClient::EnterParsingTime ()
{
  NS_LOG_FUNCTION (this);

  if (m_state == EXPECTING_MAIN_OBJECT)
    {
      const Time parsingTime = m_httpVariables->GetParsingTime ();
      NS_LOG_INFO (this << " The parsing of this main object"
                        << " will complete in "
                        << parsingTime.GetSeconds () << " seconds.");
      m_eventParseMainObject = Simulator::Schedule (
          parsingTime, &AppClient::ParseMainObject, this);
      SwitchToState (PARSING_MAIN_OBJECT);
    }
  else
    {
      NS_FATAL_ERROR ("Invalid state " << GetStateString ()
                                       << " for EnterParsingTime().");
    }
}


void
AppClient::ParseMainObject ()
{
  NS_LOG_FUNCTION (this);

  if (m_state == PARSING_MAIN_OBJECT)
    {
      m_embeddedObjectsToBeRequested = m_httpVariables->GetNumOfEmbeddedObjects ();
      NS_LOG_INFO (this << " Parsing has determined "
                        << m_embeddedObjectsToBeRequested
                        << " embedded object(s) in the main object.");

      if (m_embeddedObjectsToBeRequested > 0)
        {
          /*
           * Immediately request the first embedded object using the
           * existing connection.
           */
          m_eventRequestEmbeddedObject = Simulator::ScheduleNow (
              &AppClient::RequestEmbeddedObject, this);
        }
      else
        {
          /*
           * There is no embedded object in the main object. So sit back and
           * enjoy the plain web page.
           */
          NS_LOG_INFO (this << " Finished receiving a web page.");
          EnterReadingTime ();
        }

    }
  else
    {
      NS_FATAL_ERROR ("Invalid state " << GetStateString ()
                                       << " for ParseMainObject().");
    }

} // end of `void ParseMainObject ()`


void
AppClient::EnterReadingTime ()
{
  NS_LOG_FUNCTION (this);

  if (m_state == EXPECTING_EMBEDDED_OBJECT || m_state == PARSING_MAIN_OBJECT)
    {
      const Time readingTime = m_httpVariables->GetReadingTime ();
      NS_LOG_INFO (this << " Client will finish reading this web page in "
                        << readingTime.GetSeconds () << " seconds.");

      // Schedule a request of another main object once the reading time expires.
      m_eventRequestMainObject = Simulator::Schedule (
          readingTime, &AppClient::RequestMainObject, this);
      SwitchToState (READING);
    }
  else
    {
      NS_FATAL_ERROR ("Invalid state " << GetStateString ()
                                       << " for EnterReadingTime().");
    }
}


void
AppClient::CancelAllPendingEvents ()
{
  NS_LOG_FUNCTION (this);

  if (!Simulator::IsExpired (m_eventRequestMainObject))
    {
      NS_LOG_INFO (this << " Canceling RequestMainObject() which is due in "
                        << Simulator::GetDelayLeft (m_eventRequestMainObject).GetSeconds ()
                        << " seconds.");
      Simulator::Cancel (m_eventRequestMainObject);
    }

  if (!Simulator::IsExpired (m_eventRequestEmbeddedObject))
    {
      NS_LOG_INFO (this << " Canceling RequestEmbeddedObject() which is due in "
                        << Simulator::GetDelayLeft (m_eventRequestEmbeddedObject).GetSeconds ()
                        << " seconds.");
      Simulator::Cancel (m_eventRequestEmbeddedObject);
    }

  if (!Simulator::IsExpired (m_eventParseMainObject))
    {
      NS_LOG_INFO (this << " Canceling ParseMainObject() which is due in "
                        << Simulator::GetDelayLeft (m_eventParseMainObject).GetSeconds ()
                        << " seconds.");
      Simulator::Cancel (m_eventParseMainObject);
    }
}


void
AppClient::SwitchToState (AppClient::State_t state)
{
  const std::string oldState = GetStateString ();
  const std::string newState = GetStateString (state);
  NS_LOG_FUNCTION (this << oldState << newState);

  if ((state == EXPECTING_MAIN_OBJECT) || (state == EXPECTING_EMBEDDED_OBJECT))
    {
      if (m_objectBytesToBeReceived > 0)
        {
          NS_FATAL_ERROR ("Cannot start a new receiving session"
                          << " if the previous object"
                          << " (" << m_objectBytesToBeReceived << " bytes)"
                          << " is not completely received yet.");
        }
    }

  m_state = state;
  NS_LOG_INFO (this << " HttpClient " << oldState
                    << " --> " << newState << ".");
  m_stateTransitionTrace (oldState, newState);
}


void 
AppClient::SetTag(requestTag rt){
  m_rt =rt;
}

} // end of `namespace ns3`

