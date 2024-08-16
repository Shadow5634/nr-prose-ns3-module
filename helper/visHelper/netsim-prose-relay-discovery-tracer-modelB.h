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

#ifndef NETSIM_PROSE_RELAY_DISCOVERY_TRACER_MB_H
#define NETSIM_PROSE_RELAY_DISCOVERY_TRACER_MB_H

#ifdef HAS_NETSIMULYZER

#include "ns3/nr-sl-discovery-header.h"

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/netsimulyzer-module.h>

namespace ns3
{

using namespace std;

struct TxRxMessageSeries
{
  Ptr<netsimulyzer::XYSeries> txSeries;
  Ptr<netsimulyzer::XYSeries> rxSeries;
};

/* NetSimulyzer tracer that generates graphs for U2N relay discovery for Model B
 * ASSUMPTION: Each virtual L2 id is associated with only 1 node
 *
 * Charting features - naming according to node ids
 * 1) Series Collection for each remote UE 
 *   - For each relay a horizontal line of points of Solicitation transmissions vs time and a similar line for Response received below it
 *   - Thus for a Solicitation Tx point no close response point below it likely corresponds to a packet loss in the discovery
 *   - Does not give an indication if the loss occured in reception of the Solicitation or reception of the Response
 * 2) Series Collection for each relay UE
 *   - For each remote a horizontal line of points of Solicitation requests received
 *   - Helps to identify start and end of solicitation requests
 *
 * IMP: Since the charts are SeriesCollections the names of the individual XY Series will not have significance outside of its Series Collection
 */
class NetSimulyzerProseRelayDiscoveryTracerModelB
{
public:
  /*
   * \param relayDestL2Ids - the virtual l2Ids used by the relays
   *
   * \returns 0 - setup successful
   *          -1 - setup unsuccessful (orchestrator=NULL | #nodes != #netDevs != #relayDestL2Ids | #nodes = 0)
   *          If unsuccessful the track sink will return immediately on trigger
   */
  int SetUp(Ptr<netsimulyzer::Orchestrator> orchestrator, 
                                NodeContainer remoteNodes, NetDeviceContainer remoteNetDevs, 
                                NodeContainer relayNodes, NetDeviceContainer relayNetDevs, std::vector<uint32_t> relayDstL2Ids);
  
  NetSimulyzerProseRelayDiscoveryTracerModelB();

private:
    /*
     * Gets the Src2Id for the given netDevice
     * Uses the LteUeRrc for the same
     * TODO: Update when lte and nr models are separated fully
     */
    uint32_t GetSrcL2Id(Ptr<NetDevice> netDev);

  /*
   * Add the nodes, netDevices, destination L2 Ids to the maps being maintained
   *
   * \param dstL2Ids The virtual L2 ids being used by specified nodes
   *                 Only used for relay nodes (empty vector for remotes)
   */
  void AddNodes(NodeContainer nodes, NetDeviceContainer netDevs, std::vector<uint32_t> dstL2Ids);

  /*
   * Creates the graphs for the remote ues
   */
  void CreateGraphs(NodeContainer remoteNodes, NetDeviceContainer remoteNetDevs, 
                                                          NodeContainer relayNodes, NetDeviceContainer relayNetDevs);

  /*
   * Creates the graphs for the U2N relay ues
   */
  void CreateRelaySideGraphs(NodeContainer remoteNodes, NetDeviceContainer remoteNetDevs, 
                                                          NodeContainer relayNodes, NetDeviceContainer relayNetDevs);
  
  /*
   * Hooks to the DiscoveryTrace of NrSlUeProse for relay and remote ues
   */
  void LinkTraces(NetDeviceContainer remoteNetDevs, NetDeviceContainer relayNetDevs);

  /*
   * Trace Sink for the DiscoveryTrace of NrSlUeProse for relay and remote ues
   * Adds to the charts
   */
  void DiscoveryTraceCallback(uint32_t senderL2Id,
                              uint32_t receiverL2Id,
                              bool isTx,
                              NrSlDiscoveryHeader discMsg);

  // status variable to denote if setup was successful
  // Public methods and trace sinks will return immediately if false
  bool m_isSetUp;
  // Orchestrator to be used for chart creation
  Ptr<netsimulyzer::Orchestrator> m_orchestrator;
  
  // Maps source L2Ids and virtual L2Ids to their respective node ptrs
  // Used to extract the node id from the L2 id
  std::map<uint32_t, Ptr<Node>> m_L2IdToNodePtr;
  // Maps virtualL2 ids to the actual L2 ids
  // Used to make the L2 Id pairs to access the correspondings charts
  std::map<uint32_t, uint32_t> m_virtualToActualL2Id;
  // Set of the all the remoteL2Ids being considered
  // Used to filter unwanted data during trace sink callback
  std::set<uint32_t> m_allRemoteL2Ids;
  std::set<uint32_t> m_allRelayL2Ids;

  //Maps (remote L2Id, relay L2Id) to the graphs for the remote of Solicitations sent and Responses received vs time
  std::map<std::pair<uint32_t, uint32_t>, TxRxMessageSeries> m_discoveryMessageSeries;

  //Maps (relay L2Id, remote L2Id) to graphs for relay  of solicitation requests received vs time
  std::map<std::pair<uint32_t, uint32_t>, TxRxMessageSeries> m_relaySolicitationRequestsSeries;

};

} /* namespace ns3 */

#endif /* HAS_NETSIMULYZER */

#endif /* NETSIM_PROSE_RELAY_DISCOVERY_TRACER_MB_H */

// TODO:
//  - Remote the virtual to actual L2 id mapping and instead have maps use node id pairs instead of L2 id pairs?
//  - No neet to input nodes? Get nodes from the devices?
//  - 
