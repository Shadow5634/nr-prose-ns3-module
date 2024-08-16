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

#ifndef NETSIM_PROSE_RELAY_SELECTION_TRACER_H
#define NETSIM_PROSE_RELAY_SELECTION_TRACER_H

#ifdef HAS_NETSIMULYZER

#include <ns3/category-value-series.h>
#include <ns3/logical-link.h>
#include <ns3/net-device-container.h>
#include <ns3/node.h>
#include <ns3/orchestrator.h>
#include <ns3/ptr.h>
#include <ns3/series-collection.h>
#include <ns3/xy-series.h>

namespace ns3
{

/**
 * Tracer for relay selection related netsim aspects - FOR REMOTE UEs
 * ASSUMPTION: L2Id == 0 means invalid (disconnection)
 * Visualization features
 *  - Maintain logical links between a remote node and its chosen relay (and update based on
 * switches)
 *  - IMPORTANT: Does not delete links it makes - just toggles the active feature
 * Charting features
 *  - Rsrp vs time XYSeries for remote ue. Rsrp is plotted as negative value
 *  - Relay selected CategorySeries for remote ue
 */
class NetSimulyzerProseRelaySelectionTracer
{
  public:
    NetSimulyzerProseRelaySelectionTracer() = default;

    /*
     * ASSUMPTIONS: none of the netDevs share a node
     * \returns 1 on successful setup
     *          0 on recalling setup after a successful setup (setup is skipped, returning
     * immediately) -1  on error (orchestrator = nullptr | remoteNetDevs = 0 | relayNetDevs = 0)
     */
    int EnableVisualizations(Ptr<netsimulyzer::Orchestrator> orchestrator,
                             NetDeviceContainer remoteNetDevs,
                             NetDeviceContainer relayNetDevs);

  private:
    void AddL2Ids(NetDeviceContainer netDevices);
    void MakeGraphs(NetDeviceContainer remoteNetDevs, NetDeviceContainer relayNetDevs);
    void MakeRsrpGraphs(NetDeviceContainer remoteNetDevs, NetDeviceContainer relayNetDevs);
    void MakeRelaySelectedStateGraphs(NetDeviceContainer remoteNetDevs,
                                      NetDeviceContainer relayNetDevs);
    void MakeConnectedRemotesHistogram(NetDeviceContainer relayNetDevs);

    void LinkTraces(NetDeviceContainer remoteNetDevs, NetDeviceContainer relayNetDevs);

    Ptr<Node> GetNode(uint32_t l2Id);
    /*
     * Gets the Src2Id for the given netDevice
     * Uses the LteUeRrc for the same
     * TODO: Update when lte and nr models are separated fully
     */
    uint32_t GetSrcL2Id(Ptr<NetDevice> netDev);
    void RelaySelectionTracedCallback(uint32_t remoteL2Id,
                                      uint32_t currentRelayL2Id,
                                      uint32_t selectedRelayL2Id,
                                      uint32_t relayCode,
                                      double rsrpValue);

    void RelayRsrpTracedCallback(uint32_t remoteL2Id, uint32_t relayL2Id, double rsrpValue);

    // Maps the l2Id to the corresponding node pointer - for making and breaking logical links
    std::map<uint32_t, Ptr<Node>> m_L2IdToNodePtr;

    // Maps (remoteL2Id, relayL2Id) to its corresponding logical link ptr if it exists
    std::map<std::pair<uint32_t, uint32_t>, Ptr<netsimulyzer::LogicalLink>> m_existingLinks;

    // Maps remoteL2Id to the XYSeries of RsRp values against time
    std::map<uint32_t, Ptr<netsimulyzer::SeriesCollection>> m_rsrpTimeSeriesGraphCollectionPerUe;
    std::map<std::pair<uint32_t, uint32_t>, Ptr<netsimulyzer::XYSeries>> m_rsrpTimeSeriesGraphs;
    std::map<uint32_t, Ptr<netsimulyzer::CategoryValueSeries>> m_relaySelectedCatGraphPerRemote;

    Ptr<netsimulyzer::Orchestrator> m_orchestrator;
    bool m_isSetUp = false;

    Ptr<netsimulyzer::SeriesCollection> m_connectedRemotesHist;
    // maps relay node ids to its xy-series line in the histogram
    std::map<uint32_t, Ptr<netsimulyzer::XYSeries>> m_histLines;
    // maps relay node ids to its set of connected remotes L2 ids
    std::map<uint32_t, std::set<uint32_t>> m_connectedRemotesSet;
    std::map<uint32_t, std::string> m_prevCategories;
};

} /* namespace ns3 */

#endif /* HAS_NETSIMULYZER */

#endif /* NETSIM_PROSE_RELAY_SELECTION_TRACER_H */
