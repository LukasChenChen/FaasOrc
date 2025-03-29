#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/header.h"
#include <iostream>
#include <vector>
#include "ns3/nstime.h"

#ifndef SFC_HEADER_H
#define SFC_HEADER_H
 
 using namespace ns3;
 
 namespace ns3{

 
 class sfc_header : public Header 
 {
 public:
 
   sfc_header ();
   virtual ~sfc_header ();
 
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;
  void SetTimestamp (Time time);
  Time GetTimestamp (void) const;

  void Print (std::ostream &os) const;
 
 private:
//    uint16_t m_data;
   Time m_timestamp;
 };
 }// namespace ns3

  
 #endif /* SFC_HEADER_H */