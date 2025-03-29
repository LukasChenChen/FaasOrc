#include "ns3/tag.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"
#include <iostream>
#include "sfc_tag.h"
#include <vector>
 
NS_LOG_COMPONENT_DEFINE ("sfc_tag");
namespace ns3{


TypeId 
sfc_tag::GetTypeId (void)
{
static TypeId tid = TypeId ("ns3::sfc_tag")
    .SetParent<Tag> ()
    .AddConstructor<sfc_tag> ()
;
return tid;
}
TypeId 
sfc_tag::GetInstanceTypeId (void) const
{
return GetTypeId ();
}
uint32_t 
sfc_tag::GetSerializedSize (void) const
{
return 20; 

}
void 
sfc_tag::Serialize (TagBuffer i) const
{

    if (m_tag.size() > GetSerializedSize())
    {   
        std::cout << "Serialize TOO BIG" << std::endl;
        return;
    }
    
//std::cout << " my tag Serialize";
   auto it_p = m_tag.begin();
   for (int j = 0; j < 20; j++)
   {    
       
       if (it_p != m_tag.end())
       {    
           //std::cout << " Serialize " << (unsigned)*it_p << std::endl;
           i.WriteU8(*it_p);  // write the index of the node
           it_p++;
       }
       else
       {   //std::cout << " Serialize 255"  << std::endl;
           i.WriteU8(0xFF); // fill with 0xFF, means invalid.
       }
       
    }

    // std::cout << "Serialize--------------------------" << std::endl;

}
void 
sfc_tag::Deserialize (TagBuffer i)
{
  // we can deserialize 20 bytes from the start of the buffer.
   // we read them in network byte order and store them
   // in host byte order.
  // std::cout << " my tag deserialize";
   std::vector<uint8_t> p;
   for (int j = 0; j < 20; j++)
   {
      int data = i.ReadU8();
      if (data != 255)
      {
        m_tag.push_back(data);
      }
      
     
    //   std::cout << " Deserialize " << data << std::endl;
   }
   
 
   // we return the number of bytes effectively read.
  
    
}
void 
sfc_tag::Print (std::ostream &os) const
{
    //os << "v=" << (uint32_t)m_simpleValue;
    for (auto it = m_tag.begin(); it != m_tag.end(); it++)
    {
        os << "v=" << (uint32_t)*it;
    }

    std::cout << "print" << std::endl;

}

bool 
sfc_tag::SetData (requestTag rt)
{   
    m_rt = rt;
    std::vector<uint8_t> p;
    p.push_back(m_rt.id);
    p.push_back(m_rt.ingress);
    p.push_back(m_rt.type);
    p.push_back(m_rt.isColdStart);
    p.push_back(m_rt.served);
    p.push_back(m_rt.hostNode);
    p.push_back(m_rt.arriveTime);

    if (p.size() == 0 || p.size() > 20)
    {
        return false;
    }

    m_tag = p;
}

std::vector<uint8_t> 
sfc_tag::GetData (void) const
{
    return m_tag;
}


void sfc_tag::show () 
{   
    std::cout << "show sfc_tag: " << std::endl;
   
    for (auto it = m_tag.begin(); it != m_tag.end(); it++)
    {
        std::cout << *it + ","<<std::endl;
    }
}

void sfc_tag::setRequestTag () 
{   
  
    auto it  = m_tag.begin();
    m_rt.id = (uint32_t)*it;

    it = m_tag.end()-1;
    m_rt.arriveTime = (uint32_t)*it;

    return;
}
 


} // namespace ns3