#ifndef SFC_ID_TAG_H
#define SFC_ID_TAG_H


#include <ostream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/stats-module.h"
#include "ns3/tag.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"

namespace ns3{
class Sfc_id_tag : public Tag {
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);

  // these are our accessors to our tag structure
  void set_sfc_id (int id);
  int get_sfc_id (void) const;

  void Print (std::ostream &os) const;

private:
  uint8_t m_sfc_id;

  // end class TimestampTag
};

} // namespace ns3
#endif