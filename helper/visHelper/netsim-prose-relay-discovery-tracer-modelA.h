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

#ifndef NETSIM_PROSE_RELAY_DISCOVERY_TRACER_MA_H
#define NETSIM_PROSE_RELAY_DISCOVERY_TRACER_MA_H

#ifdef HAS_NETSIMULYZER

#include "ns3/nr-sl-discovery-header.h"

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/netsimulyzer-module.h>

namespace ns3
{

/*
 * Charting features
 * - For each receiver a total distinct discoveries received vs time graph
 * - For each receiver a SeriesCollection (hist) of # discoveries received from each L2Id
 * - For each receiver a SeriesCollection of # total discoveries received vs time for each sender L2Id 
 */
class NetSimulyzerProseRelayDiscoveryTracerModelA
{
public:
  NetSimulyzerProseRelayDiscoveryTracerModelA();

  /*
   * \param relayDestL2Ids - the virtual l2Ids used by the relays
   *
   * \returns 0 - setup successful
   *          1 - setup unsuccessful (orchestrator=NNULL | #nodes != #netDevs != relayDestL2Ids | #nodes = 0)
   *          If unsuccessful the track sink will return immediately on trigger
   */
  int SetUp(Ptr<netsimulyzer::Orchestrator> orchestrator, 
                                NodeContainer remoteNodes, NetDeviceContainer remoteNetDevs, 
                                NodeContainer relayNodes, NetDeviceContainer relayNetDevs, std::vector<uint32_t> relayDstL2Ids);

  void DiscoveryTraceCallback(uint32_t senderL2Id,
                              uint32_t receiverL2Id,
                              bool isTx,
                              NrSlDiscoveryHeader discMsg);
  
private:
  void LinkTraces(NetDeviceContainer remoteNetDevs, NetDeviceContainer relayNetDevs);

    /*
     * Gets the Src2Id for the given netDevice
     * Uses the LteUeRrc for the same
     * TODO: Update when lte and nr models are separated fully
     */
    uint32_t GetSrcL2Id(Ptr<NetDevice> netDev);

  void AddNodes(NodeContainer nodes, NetDeviceContainer netDevs, std::vector<uint32_t> dstL2Ids);
  void CreateRelaySideGraphs(NodeContainer remoteNodes, NetDeviceContainer remoteNetDevs, 
                                                          NodeContainer relayNodes, NetDeviceContainer relayNetDevs);

  //void CreateRelaysDiscoveredCountHistogram(NodeContainer remoteNodes);

  bool m_isSetUp;
  Ptr<netsimulyzer::Orchestrator> m_orchestrator;

  // Actual and virtual L2Ids to their respective node ptrs
  std::map<uint32_t, Ptr<Node>> m_L2IdToNodePtr;
  std::map<uint32_t, uint32_t> m_virtualToActualL2Id;
  std::set<uint32_t> m_remoteL2Ids;

  //Maps (relay L2Id, remote L2Id) to the corresponding XYSeries graph of announcements received
  std::map<std::pair<uint32_t, uint32_t>, Ptr<netsimulyzer::XYSeries>> m_discoveryMessageSeries;

  // Maps remote L2Id to set of discovered relay L2Ids set
  std::map<uint32_t, std::set<uint32_t>> m_discoveredRelays;
  Ptr<netsimulyzer::SeriesCollection> m_discoveredRelaysHist;

};

} /* namespace ns3 */

#endif /* HAS_NETSIMULYZER */

#endif /* NETSIM_PROSE_RELAY_DISCOVERY_TRACER_MA_H */

//TODO: Add histo of #relays discovered for each remote
