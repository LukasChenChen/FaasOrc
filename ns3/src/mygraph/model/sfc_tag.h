#include "ns3/tag.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include <iostream>
#include <vector>
// #include <type.h>
#include "ns3/type.h"
 
 using namespace ns3;
 
 namespace ns3 {
 class sfc_tag : public Tag
 {
  public:
   static  TypeId GetTypeId (void);
   virtual TypeId GetInstanceTypeId (void) const;
   virtual uint32_t GetSerializedSize (void) const;
   virtual void Serialize (TagBuffer i) const;
   virtual void Deserialize (TagBuffer i);
   virtual void Print (std::ostream &os) const;

   void setRequestTag (); 
   // these are our accessors to our tag structure
 
   bool SetData (requestTag rt);
   void show (); 
   std::vector<uint8_t> GetData (void) const;
   std::vector<uint8_t> m_tag; 
   
   int m_buffer_size;

   requestTag m_rt;
 };
 
 }//namespace ns3