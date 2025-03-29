/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DIS_APPLICATION_HELPER_H
#define DIS_APPLICATION_HELPER_H


#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/udp-server.h"
#include "ns3/udp-client.h"
#include "ns3/dis_application.h"


namespace ns3 {
using namespace ns3;
/* ... */
class Dis_application_Helper
{
public:
  /**
   * Create UdpServerHelper which will make life easier for people trying
   * to set up simulations with udp-client-server application.
   *
   */
  Dis_application_Helper ();

  /**
   * Create UdpServerHelper which will make life easier for people trying
   * to set up simulations with udp-client-server application.
   *
   * \param port The port the server will wait on for incoming packets
   */
  Dis_application_Helper (uint16_t port);

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Create one UDP server application on each of the Nodes in the
   * NodeContainer.
   *
   * \param c The nodes on which to create the Applications.  The nodes
   *          are specified by a NodeContainer.
   * \returns The applications created, one Application per Node in the
   *          NodeContainer.
   */
  ApplicationContainer Install (NodeContainer c);

  /**
   * \brief Return the last created server.
   *
   * This function is mainly used for testing.
   *
   * \returns a Ptr to the last created server application
   */
  Ptr<Dis_application> GetServer (void);
private:
  ObjectFactory m_factory; //!< Object factory.
  Ptr<Dis_application> m_server; //!< The last created server application
};



}

#endif /* DIS_APPLICATION_HELPER_H */

