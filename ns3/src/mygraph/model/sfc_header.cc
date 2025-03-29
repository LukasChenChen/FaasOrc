/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
 #include "ns3/ptr.h"
 #include "ns3/packet.h"
 #include "ns3/header.h"
 #include <iostream>
 #include <vector>

#include "ns3/log.h"
 #include "sfc_header.h"
 #include "ns3/nstime.h"
 

 using namespace ns3;
 

 namespace ns3{
 NS_LOG_COMPONENT_DEFINE ("sfc_header");

 sfc_header::sfc_header ()
 {
   // we must provide a public default constructor, 
   // implicit or explicit, but never private.
 }
 sfc_header::~sfc_header ()
 {
 }
 
 TypeId
 sfc_header::GetTypeId (void)
 {
   static TypeId tid = TypeId ("ns3::sfc_header")
     .SetParent<Header> ()
     .AddConstructor<sfc_header> ()
   ;
   return tid;
 }
 TypeId
 sfc_header::GetInstanceTypeId (void) const
 {
   return GetTypeId ();
 }
 
 uint32_t
 sfc_header::GetSerializedSize (void) const
 {
   // we reserve 20 bytes for our header. which is the routing path for the packet.
   return 8;
 }
 void
 sfc_header::Serialize (Buffer::Iterator start) const
 {
   // we can serialize two bytes at the start of the buffer.
   // we write them in network byte order.
  Buffer::Iterator it = start;
  int64_t t = m_timestamp.GetNanoSeconds ();
  it.Write ((const uint8_t *)&t, 8);
   
 }
 uint32_t
 sfc_header::Deserialize (Buffer::Iterator start)
 {
   // we can deserialize two bytes from the start of the buffer.
   // we read them in network byte order and store them
   // in host byte order.
    Buffer::Iterator it = start;
    int64_t t;
    it.Read ((uint8_t *)&t, 8);
    m_timestamp = NanoSeconds (t);
   // we return the number of bytes effectively read.
   return 8;
 }
 
void 
 sfc_header::SetTimestamp (Time time)
 {
  m_timestamp = time;
 }
Time
sfc_header::GetTimestamp (void) const
{
  return m_timestamp;
}

void 
sfc_header::Print (std::ostream &os) const
{
  os << "t=" << m_timestamp;
}

 
}