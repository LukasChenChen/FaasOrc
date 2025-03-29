/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Original work author (from packet-sink-helper.cc):
 *   - Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *
 * Converted to HTTP web browsing traffic models by:
 *   - Budiarto Herman <budiarto.herman@magister.fi>
 *
 */

#ifndef APP_HELPER_H
#define APP_HELPER_H

#include <ns3/object-factory.h>
#include <ns3/node-container.h>
#include <ns3/application-container.h>


namespace ns3 {


/**
 * \ingroup applications
 * Helper to make it easier to instantiate an AppClient on a set of nodes.
 */
class AppClientHelper
{
public:
  /**
   * Create a ThreeGppHttpClientHelper to make it easier to work with ThreeGppHttpClient
   * applications.
   * \param address The address of the remote server node to send traffic to.
   */
  AppClientHelper (const Address &address);

  /**
   * Helper function used to set the underlying application attributes, but
   * *not* the socket attributes.
   * \param name The name of the application attribute to set.
   * \param value The value of the application attribute to set.
   */
  void SetAttribute (const std::string &name,
                     const AttributeValue &value);

  /**
   * Install a ThreeGppHttpClient on each node of the input container configured with
   * all the attributes set with SetAttribute().
   * \param c NodeContainer of the set of nodes on which an ThreeGppHttpClient
   *          will be installed.
   * \return Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (NodeContainer c) const;

  /**
   * Install a ThreeGppHttpClient on each node of the input container
   * configured with all the attributes set with SetAttribute().
   * \param node The node on which an ThreeGppHttpClient will be installed.
   * \return Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (Ptr<Node> node) const;

  /**
   * Install a ThreeGppHttpClient on each node of the input container
   * configured with all the attributes set with SetAttribute().
   * \param nodeName The name of the node on which an ThreeGppHttpClient
   *                 will be installed.
   * \return Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (const std::string &nodeName) const;

private:
  /**
   * \internal
   * Install a ThreeGppHttpClient on the node configured with all the
   * attributes set with SetAttribute().
   * \param node The node on which an ThreeGppHttpClient will be installed.
   * \return Ptr to the application installed.
   */
  Ptr<Application> InstallPriv (Ptr<Node> node) const;

  /// Used to instantiate an ThreeGppHttpClient instance.
  ObjectFactory m_factory;

}; // end of `class ThreeGppHttpClientHelper`



/**
 * \ingroup http
 * Helper to make it easier to instantiate an AppServer on a set of nodes.
 */
class AppServerHelper
{
public:
  /**
   * Create a AppServerHelper to make it easier to work with
   * AppServer applications.
   * \param address The address of the server.
   */
  AppServerHelper (const Address &address);

  /**
   * Helper function used to set the underlying application attributes, but
   * *not* the socket attributes.
   * \param name The name of the application attribute to set.
   * \param value The value of the application attribute to set.
   */
  void SetAttribute (const std::string &name,
                     const AttributeValue &value);

  /**
   * Install an ThreeGppHttpServer on each node of the input container
   * configured with all the attributes set with SetAttribute().
   * \param c NodeContainer of the set of nodes on which an ThreeGppHttpServer
   *              will be installed.
   * \return Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (NodeContainer c) const;

  /**
   * Install an ThreeGppHttpServer on each node of the input container
   * configured with all the attributes set with SetAttribute().
   * \param node The node on which an ThreeGppHttpServer will be installed.
   * \return Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (Ptr<Node> node) const;

  /**
   * Install an AppServer on each node of the input container
   * configured with all the attributes set with SetAttribute().
   * \param nodeName The name of the node on which an AppServer
   *                             will be installed.
   * \return Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (const std::string &nodeName) const;

private:
  /**
   * \internal
   * Install an AppServer on the node configured with all the
   * attributes set with SetAttribute().
   * \param node The node on which an AppServerHelper will be installed.
   * \return Ptr to the application installed.
   */
  Ptr<Application> InstallPriv (Ptr<Node> node) const;

  /// Used to instantiate a AppServerHelper instance.
  ObjectFactory m_factory;

}; // end of `class AppServerHelper`


} // end of `namespace ns3`


#endif /* APP_HELPER_H */
