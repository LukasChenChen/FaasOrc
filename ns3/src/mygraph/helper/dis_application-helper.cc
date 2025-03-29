/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "dis_application-helper.h"
#include "ns3/udp-server.h"
#include "ns3/udp-client.h"
#include "ns3/udp-trace-client.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/dis_application.h"


namespace ns3 {
using namespace ns3;
/* ... */
Dis_application_Helper::Dis_application_Helper ()
{
  m_factory.SetTypeId (Dis_application::GetTypeId ());
}

Dis_application_Helper::Dis_application_Helper (uint16_t port)
{
  m_factory.SetTypeId (Dis_application::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void
Dis_application_Helper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
Dis_application_Helper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      m_server = m_factory.Create<Dis_application> ();
      node->AddApplication (m_server);
      apps.Add (m_server);

    }
  return apps;
}

Ptr<Dis_application>
Dis_application_Helper::GetServer (void)
{
  return m_server;
}




}

