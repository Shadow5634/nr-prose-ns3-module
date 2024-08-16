/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * NIST-developed software is provided by NIST as a public
 * service. You may use, copy and distribute copies of the software in
 * any medium, provided that you keep intact this entire notice. You
 * may improve, modify and create derivative works of the software or
 * any portion of the software, and you may copy and distribute such
 * modifications or works. Modified works should carry a notice
 * stating that you changed the software and should note the date and
 * nature of any such change. Please explicitly acknowledge the
 * National Institute of Standards and Technology as the source of the
 * software.
 *
 * NIST-developed software is expressly provided "AS IS." NIST MAKES
 * NO WARRANTY OF ANY KIND, EXPRESS, IMPLIED, IN FACT OR ARISING BY
 * OPERATION OF LAW, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTY OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * NON-INFRINGEMENT AND DATA ACCURACY. NIST NEITHER REPRESENTS NOR
 * WARRANTS THAT THE OPERATION OF THE SOFTWARE WILL BE UNINTERRUPTED
 * OR ERROR-FREE, OR THAT ANY DEFECTS WILL BE CORRECTED. NIST DOES NOT
 * WARRANT OR MAKE ANY REPRESENTATIONS REGARDING THE USE OF THE
 * SOFTWARE OR THE RESULTS THEREOF, INCLUDING BUT NOT LIMITED TO THE
 * CORRECTNESS, ACCURACY, RELIABILITY, OR USEFULNESS OF THE SOFTWARE.
 *
 * You are solely responsible for determining the appropriateness of
 * using and distributing the software and you assume all risks
 * associated with its use, including but not limited to the risks and
 * costs of program errors, compliance with applicable laws, damage to
 * or loss of data, programs or equipment, and the unavailability or
 * interruption of operation. This software is not intended to be used
 * in any situation where a failure could cause risk of injury or
 * damage to property. The software developed by NIST employees is not
 * subject to copyright protection within the United States.
 *
 * Author: Nihar Kapasi <niharkkapasi@gmail.com>
 */

#ifndef NETSIM_PROSE_DIRECT_DISCOVERY_TRACER_H
#define NETSIM_PROSE_DIRECT_DISCOVERY_TRACER_H

#ifdef HAS_NETSIMULYZER

#include "ns3/nr-sl-discovery-header.h"

#include <ns3/netsimulyzer-module.h>
#include <ns3/net-device-container.h>

namespace ns3
{

//TODO: The main difference between model A and model B is the order of the pair to use in maps
//As a result the discovery trace callback will also function slightly differently
class NetSimulyzerProseDirectDiscoveryTracer
{
public:
  NetSimulyzerProseDirectDiscoveryTracer();

  // \return 0 - setup successful
  //         -1 - setup unsuccessful (model!=ModelA/ModelB | orchestrator = nullptr | #netDevs != #nodes | #nodes = 0)
  int SetUp(std::string model, Ptr<netsimulyzer::Orchestrator> orchestrator, NetDeviceContainer netDevs);
  
private:

  // Links to the NrSlUeProse DiscoveryTrace
  // for each of the netDevices that were given during setup
  void LinkTraces(NetDeviceContainer netDevs);

  // Makes the graphs that are mentioned under the charting features header
  void MakeGraphs(NetDeviceContainer netDevs);


  // The trace sink for DiscoveryTrace source of NrSlUeProse
  // Responsible for adding data points to the graphs
  void DiscoveryTraceCallback(  uint32_t senderL2Id,
                                uint32_t receiverL2Id,
                                bool isTx,
                                NrSlDiscoveryHeader discMsg);

  Ptr<Node> GetNode(uint32_t l2Id);

  /*
   * Gets the Src2Id for the given netDevice
   * Uses the LteUeRrc for the same
   * TODO: Update when lte and nr models are separated fully
   */
  uint32_t GetSrcL2Id(Ptr<NetDevice> netDev);
  
  bool m_isSetUp;
  std::string m_model;
  std::map<uint32_t, Ptr<Node>> m_L2IdToNodePtr;
  Ptr<netsimulyzer::Orchestrator> m_orchestrator;

  void MakeDiscoveredUesCountHistogram(std::set<uint32_t> allNodeIds, std::set<uint32_t> allL2Ids);
  Ptr<netsimulyzer::SeriesCollection> m_discoveredUesCountHistogram;
  // maps node ids to its individual histogram line
  std::map<uint32_t, Ptr<netsimulyzer::XYSeries>> m_discoveredUesCountGraphPerNode;
  // maps receiver L2Ids to set of discovered L2Ids
  std::map<uint32_t, std::set<uint32_t>> m_discoveredL2Ids;
  
  ////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////
  
  void MakeDiscoveriesReceivedGraphs(std::set<uint32_t> allL2Ids);
  // maps src L2 ids to its collection of discoveries received from other nodes
  std::map<uint32_t, Ptr<netsimulyzer::SeriesCollection>> m_discoveryMsgCollectionPerNode;
  // maps (receiverL2Id, senderL2id) to graph of discoveries received by sender
  std::map<std::pair<uint32_t, uint32_t>, Ptr<netsimulyzer::XYSeries>> m_discoveryMsgGraphPerL2IdPair;

};

} /* namespace ns3 */

#endif /* HAS_NETSIMULYZER */

#endif /* NETSIM_DIRECT_DISCOVERY_TRACER_H */
