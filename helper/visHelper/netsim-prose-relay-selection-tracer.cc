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

#ifdef HAS_NETSIMULYZER

#include "netsim-prose-relay-selection-tracer.h"

#include "ns3/nr-sl-ue-prose.h"

#include <ns3/boolean.h>
#include <ns3/color-palette.h>
#include <ns3/logical-link-helper.h>
#include <ns3/lte-ue-rrc.h>
#include <ns3/nr-ue-net-device.h>
#include <ns3/pointer.h>

namespace ns3
{

using namespace std;

#define DISCONNECTED "Disconnected"
#define RELAY "Relay "

int
NetSimulyzerProseRelaySelectionTracer::EnableVisualizations(
    Ptr<netsimulyzer::Orchestrator> orchestrator,
    NetDeviceContainer remoteNetDevs,
    NetDeviceContainer relayNetDevs)
{
    bool areParamsWrong =
        ((orchestrator == nullptr) || (remoteNetDevs.GetN() * relayNetDevs.GetN() == 0));
    NS_ABORT_MSG_IF(areParamsWrong, "Either orchestrator was null or #remotes or #relays is 0");

    m_orchestrator = orchestrator;
    AddL2Ids(remoteNetDevs);
    AddL2Ids(relayNetDevs);
    MakeGraphs(remoteNetDevs, relayNetDevs);
    LinkTraces(remoteNetDevs, relayNetDevs);
    m_isSetUp = true;
    return 0;
}

void
NetSimulyzerProseRelaySelectionTracer::MakeGraphs(NetDeviceContainer remoteNetDevs,
                                                  NetDeviceContainer relayNetDevs)
{
    MakeRsrpGraphs(remoteNetDevs, relayNetDevs);
    MakeRelaySelectedStateGraphs(remoteNetDevs, relayNetDevs);
    MakeConnectedRemotesHistogram(relayNetDevs);
}

void
NetSimulyzerProseRelaySelectionTracer::LinkTraces(NetDeviceContainer remoteNetDevs,
                                                  NetDeviceContainer relayNetDevs)
{
    for (uint32_t i = 0; i < remoteNetDevs.GetN(); i++)
    {
        auto prose = remoteNetDevs.Get(i)->GetObject<NrSlUeProse>();
        prose->TraceConnectWithoutContext(
            "RelaySelectionTrace",
            MakeCallback(&NetSimulyzerProseRelaySelectionTracer::RelaySelectionTracedCallback,
                         this));
        prose->TraceConnectWithoutContext(
            "RelayRsrpTrace",
            MakeCallback(&NetSimulyzerProseRelaySelectionTracer::RelayRsrpTracedCallback, this));
    }
}

void
NetSimulyzerProseRelaySelectionTracer::AddL2Ids(NetDeviceContainer netDevices)
{
    for (uint32_t i = 0; i < netDevices.GetN(); i++)
    {
        auto node = netDevices.Get(i)->GetNode();
        uint32_t srcL2Id = GetSrcL2Id(netDevices.Get(i));

        m_L2IdToNodePtr.insert(std::pair<uint32_t, Ptr<Node>>(srcL2Id, node));
    }
}

void
NetSimulyzerProseRelaySelectionTracer::MakeRelaySelectedStateGraphs(
    NetDeviceContainer remoteNetDevs,
    NetDeviceContainer relayNetDevs)
{
    std::vector<std::string> categories;

    for (uint32_t i = 0; i < relayNetDevs.GetN(); i++)
    {
        auto relay = relayNetDevs.Get(i)->GetNode();
        categories.push_back(RELAY + std::to_string(relay->GetId()));
    }
    categories.push_back(DISCONNECTED);

    for (uint32_t i = 0; i < remoteNetDevs.GetN(); i++)
    {
        auto remoteNodeId = remoteNetDevs.Get(i)->GetNode()->GetId();

        auto connectedRelayCatSeries =
            CreateObject<netsimulyzer::CategoryValueSeries>(m_orchestrator, categories);
        connectedRelayCatSeries->SetAttribute(
            "Name",
            StringValue("Connected relay vs Time - Remote: " + std::to_string(remoteNodeId)));
        connectedRelayCatSeries->GetXAxis()->SetAttribute("Name", StringValue("Time(s)"));
        connectedRelayCatSeries->GetYAxis()->SetAttribute("Name", StringValue("Connected Relay"));
        // Any remote starts off as disconnected
        connectedRelayCatSeries->Append(DISCONNECTED, 0);

        uint32_t srcL2Id = GetSrcL2Id(remoteNetDevs.Get(i));

        m_relaySelectedCatGraphPerRemote.insert(
            std::pair<uint32_t, Ptr<netsimulyzer::CategoryValueSeries>>(srcL2Id,
                                                                        connectedRelayCatSeries));

        m_prevCategories.insert(std::pair<uint32_t, std::string>(srcL2Id, DISCONNECTED));
    }
}

void
NetSimulyzerProseRelaySelectionTracer::MakeRsrpGraphs(NetDeviceContainer remoteNetDevs,
                                                      NetDeviceContainer relayNetDevs)
{
    PointerValue xAxis;
    PointerValue yAxis;

    for (uint32_t i = 0; i < remoteNetDevs.GetN(); i++)
    {
        uint32_t remoteSrcL2Id = GetSrcL2Id(remoteNetDevs.Get(i));

        auto remoteRsrpCollection = CreateObject<netsimulyzer::SeriesCollection>(m_orchestrator);
        remoteRsrpCollection->SetAttribute(
            "Name",
            StringValue("Rsrp vs Time - Remote " +
                        std::to_string(remoteNetDevs.Get(i)->GetNode()->GetId())));
        remoteRsrpCollection->GetAttribute("XAxis", xAxis);
        remoteRsrpCollection->GetAttribute("YAxis", yAxis);
        xAxis.Get<netsimulyzer::ValueAxis>()->SetAttribute("Name", StringValue("Time (s)"));
        yAxis.Get<netsimulyzer::ValueAxis>()->SetAttribute("Name", StringValue("Rsrp (dBm)"));
        remoteRsrpCollection->SetAttribute("HideAddedSeries", BooleanValue(false));

        for (uint32_t j = 0; j < relayNetDevs.GetN(); j++)
        {
            uint32_t relaySrcL2Id = GetSrcL2Id(relayNetDevs.Get(j));

            auto relayRsrpLine = CreateObject<netsimulyzer::XYSeries>(m_orchestrator);
            relayRsrpLine->SetAttribute(
                "Name",
                StringValue(RELAY + std::to_string(relayNetDevs.Get(j)->GetNode()->GetId())));
            relayRsrpLine->SetAttribute("Visible", BooleanValue(false));
            relayRsrpLine->SetAttribute("Color", netsimulyzer::NextColor3Value());
            remoteRsrpCollection->Add(relayRsrpLine);

            m_rsrpTimeSeriesGraphs.insert(
                std::pair<std::pair<uint32_t, uint32_t>, Ptr<netsimulyzer::XYSeries>>(
                    {remoteSrcL2Id, relaySrcL2Id},
                    relayRsrpLine));
        }

        m_rsrpTimeSeriesGraphCollectionPerUe.insert(
            std::pair<uint32_t, Ptr<netsimulyzer::SeriesCollection>>(remoteSrcL2Id,
                                                                     remoteRsrpCollection));
    }
}

void
NetSimulyzerProseRelaySelectionTracer::MakeConnectedRemotesHistogram(
    NetDeviceContainer relayNetDevs)
{
    PointerValue xAxis;
    PointerValue yAxis;

    m_connectedRemotesHist = CreateObject<netsimulyzer::SeriesCollection>(m_orchestrator);
    m_connectedRemotesHist->SetAttribute(
        "Name",
        StringValue("Connected remotes count Histogram - All relays"));
    m_connectedRemotesHist->GetAttribute("XAxis", xAxis);
    m_connectedRemotesHist->GetAttribute("YAxis", yAxis);
    xAxis.Get<netsimulyzer::ValueAxis>()->SetAttribute("Name", StringValue("Relay Node Id"));
    yAxis.Get<netsimulyzer::ValueAxis>()->SetAttribute("Name", StringValue("Count"));
    m_connectedRemotesHist->SetAttribute("HideAddedSeries", BooleanValue(false));

    for (uint32_t i = 0; i < relayNetDevs.GetN(); i++)
    {
        uint32_t relayNodeId = relayNetDevs.Get(i)->GetNode()->GetId();

        auto connectedRelaysLine = CreateObject<netsimulyzer::XYSeries>(m_orchestrator);
        connectedRelaysLine->SetAttribute("Name",
                                          StringValue("Relay Node " + std::to_string(relayNodeId)));
        connectedRelaysLine->SetAttribute("Connection", StringValue("Line"));
        connectedRelaysLine->SetAttribute("Visible", BooleanValue(false));
        m_connectedRemotesHist->Add(connectedRelaysLine);
        connectedRelaysLine->Append(relayNodeId, 0);

        m_histLines.insert(
            std::pair<uint32_t, Ptr<netsimulyzer::XYSeries>>(relayNodeId, connectedRelaysLine));

        m_connectedRemotesSet.insert(std::pair<uint32_t, std::set<uint32_t>>(relayNodeId, {}));
    }
}

uint32_t
NetSimulyzerProseRelaySelectionTracer::GetSrcL2Id(Ptr<NetDevice> netDev)
{
    Ptr<NrSlUeProse> nodeProse = netDev->GetObject<NrSlUeProse>();
    auto ueRrc = nodeProse->GetObject<NrUeNetDevice>()->GetRrc();
    uint32_t srcL2Id = ueRrc->GetSourceL2Id();

    return srcL2Id;
}

Ptr<Node>
NetSimulyzerProseRelaySelectionTracer::GetNode(uint32_t l2Id)
{
    auto it = m_L2IdToNodePtr.find(l2Id);
    NS_ABORT_MSG_IF(it == m_L2IdToNodePtr.end(), "L2Id: " + std::to_string(l2Id) + "not found");
    return it->second;
}

/**
 * Trace sink for the ns3::NrSlUeProse::RelayRsrpTrace trace source
 *
 * \param relayTrace a pointer to NrSlRelayTrace class
 * \param path trace path
 * \param remoteL2Id remote L2 ID
 * \param relayL2Id relay L2 ID
 * \param rsrpValue RSRP value
 */
void
NetSimulyzerProseRelaySelectionTracer::RelayRsrpTracedCallback(uint32_t remoteL2Id,
                                                               uint32_t relayL2Id,
                                                               double rsrpValue)
{
    std::pair<uint32_t, uint32_t> searchTuple = {remoteL2Id, relayL2Id};
    auto rsrpGraphIt = m_rsrpTimeSeriesGraphs.find(searchTuple);
    NS_ABORT_MSG_IF((rsrpGraphIt == m_rsrpTimeSeriesGraphs.end()), "Unidentified L2Ids");

    rsrpGraphIt->second->Append(Simulator::Now().GetSeconds(), rsrpValue);
}

/**
 * Trace sink for the ns3::NrSlUeProse::RelaySelectionTrace trace source
 *
 * \param relayTrace a pointer to NrSlRelayTrace class
 * \param path trace path
 * \param remoteL2Id remote L2 ID
 * \param currentRelayL2Id current relay L2 ID
 * \param selectedRelayL2id selected relay L2 ID
 * \param relayCode relay service code
 * \param rsrpValue RSRP value
 */
void
NetSimulyzerProseRelaySelectionTracer::RelaySelectionTracedCallback(uint32_t remoteL2Id,
                                                                    uint32_t currentRelayL2Id,
                                                                    uint32_t selectedRelayL2Id,
                                                                    uint32_t relayCode,
                                                                    double rsrpValue)
{
    auto remoteStateGraphIt = m_relaySelectedCatGraphPerRemote.find(remoteL2Id);
    if (remoteStateGraphIt != m_relaySelectedCatGraphPerRemote.end())
    {
        // Add point to say that remote was in prev category until now
        // Done to avoid slanting lines in category series
        remoteStateGraphIt->second->Append(m_prevCategories[remoteL2Id],
                                           Simulator::Now().GetSeconds());
        // Remote became disconnected
        if (selectedRelayL2Id == 0)
        {
            remoteStateGraphIt->second->Append(DISCONNECTED, Simulator::Now().GetSeconds());
            m_prevCategories[remoteL2Id] = DISCONNECTED;
        }
        else
        {
            remoteStateGraphIt->second->Append(
                RELAY + std::to_string(GetNode(selectedRelayL2Id)->GetId()),
                Simulator::Now().GetSeconds());
            m_prevCategories[remoteL2Id] =
                RELAY + std::to_string(GetNode(selectedRelayL2Id)->GetId());
        }
    }
    else
    {
        NS_FATAL_ERROR("Remote L2 id not found");
    }

    // No updation of logical links - stayed disconnected(both 0) or stayed with same relay
    if (selectedRelayL2Id == currentRelayL2Id)
    {
        return;
    }

    std::pair<uint32_t, uint32_t> oldPair = {remoteL2Id, currentRelayL2Id};
    std::pair<uint32_t, uint32_t> newPair = {remoteL2Id, selectedRelayL2Id};

    // Breaking connection with a relay - Toggle off the old link - guarenteed to be different from
    // new due to an earlier check
    if (currentRelayL2Id != 0)
    {
        m_existingLinks[oldPair]->SetAttribute("Active", BooleanValue(false));
    }
    // Became disconnected - no logical links to be added/toggled on
    if (selectedRelayL2Id == 0)
    {
        return;
    }

    // At this point
    // Change in connected relay and did not become disconnected
    // (could have been disconnected and just connected to a relay)
    Ptr<Node> remoteNode = GetNode(remoteL2Id);
    Ptr<Node> newRelayNode = GetNode(selectedRelayL2Id);

    // Add/Toggle new logical link to chosen relay
    auto linkIt = m_existingLinks.find(newPair);
    if (linkIt != m_existingLinks.end())
    {
        linkIt->second->SetAttribute("Active", BooleanValue(true));
    }
    else
    {
        netsimulyzer::LogicalLinkHelper linkHelper(m_orchestrator);
        auto newLink = linkHelper.Link(remoteNode, newRelayNode);

        m_existingLinks.insert(
            std::pair<std::pair<uint32_t, uint32_t>, Ptr<netsimulyzer::LogicalLink>>(newPair,
                                                                                     newLink));
    }

    // Add to connected remotes histogram
    uint32_t relayNodeId = newRelayNode->GetId();
    auto connectedRemotesSetIt = m_connectedRemotesSet.find(relayNodeId);
    NS_ABORT_MSG_IF((connectedRemotesSetIt == m_connectedRemotesSet.end()),
                    "Relay " + std::to_string(relayNodeId) + "not found");

    std::set<uint32_t> connectedRemotesSet = connectedRemotesSetIt->second;
    if (connectedRemotesSet.contains(remoteL2Id) == false)
    {
        connectedRemotesSet.insert(remoteL2Id);
        m_histLines[relayNodeId]->Append(relayNodeId, connectedRemotesSet.size());
    }
}

} /* namespace ns3 */

#endif
