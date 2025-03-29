//----------------------------------------------------------------------
//-- TimestampTag

//examples/stats/"wifi-example-apps.h"
//------------------------------------------------------
#include <ostream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/stats-module.h"
#include "ns3/tag.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "sfc_id_tag.h"

namespace ns3{

TypeId 
Sfc_id_tag::GetTypeId (void)
{
  static TypeId tid = TypeId ("Sfc_id_tag")
    .SetParent<Tag> ()
    .AddConstructor<Sfc_id_tag> ();
  return tid;
}
TypeId 
Sfc_id_tag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t 
Sfc_id_tag::GetSerializedSize (void) const
{
  return 8;
}
void 
Sfc_id_tag::Serialize (TagBuffer i) const
{
  
//   i.Write ((const uint8_t *)&m_sfc_id, 8);
  i.WriteU8(m_sfc_id); 
}
void 
Sfc_id_tag::Deserialize (TagBuffer i)
{
  //i.Read ((const uint8_t *)&m_sfc_id, 8);
  
  m_sfc_id = i.ReadU8();
}

void
Sfc_id_tag::set_sfc_id (int id)
{
  m_sfc_id = (uint8_t)id;
}
int
Sfc_id_tag::get_sfc_id (void) const
{
  return (unsigned int)m_sfc_id;
}

void 
Sfc_id_tag::Print (std::ostream &os) const
{
  os << "sfc id=" << m_sfc_id;
}

} //namespace ns3