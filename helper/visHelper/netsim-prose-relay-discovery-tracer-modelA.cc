#ifdef HAS_NETSIMULYZER

#include "netsim-prose-relay-discovery-tracer-modelA.h"
#include "ns3/nr-sl-ue-prose.h"

#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include <ns3/nr-module.h>
#include <ns3/netsimulyzer-module.h>

using namespace ns3;
using namespace std;

NetSimulyzerProseRelayDiscoveryTracerModelA::NetSimulyzerProseRelayDiscoveryTracerModelA()
{
  m_isSetUp = false;
  m_orchestrator = nullptr;
  m_discoveredRelaysHist = nullptr;
}

int
NetSimulyzerProseRelayDiscoveryTracerModelA::SetUp(Ptr<netsimulyzer::Orchestrator> orchestrator, 
                                NodeContainer remoteNodes, NetDeviceContainer remoteNetDevs, 
                                NodeContainer relayNodes, NetDeviceContainer relayNetDevs, std::vector<uint32_t> relayDstL2Ids)
{
  if (m_isSetUp)
  {
    return 0;
  }

  if ((orchestrator == nullptr) || (remoteNodes.GetN() != remoteNetDevs.GetN()) || (relayNodes.GetN() != relayNetDevs.GetN()) ||
          (relayNodes.GetN() != relayDstL2Ids.size()) || (remoteNodes.GetN() * relayNodes.GetN() == 0))
  {
    m_isSetUp = false;
    return -1;
  } 

  m_orchestrator = orchestrator;
  AddNodes(remoteNodes, remoteNetDevs, std::vector<uint32_t>{});
  AddNodes(relayNodes, relayNetDevs, relayDstL2Ids);
  CreateRelaySideGraphs(remoteNodes, remoteNetDevs, relayNodes, relayNetDevs);

  LinkTraces(remoteNetDevs, relayNetDevs);

  m_isSetUp = true;
  return 1;
}

void
NetSimulyzerProseRelayDiscoveryTracerModelA::AddNodes(NodeContainer nodes, NetDeviceContainer netDevs, std::vector<uint32_t> dstL2Ids)
{

  for (uint32_t i = 0; i < nodes.GetN(); i++)
  {
    Ptr<Node> node = nodes.Get(i);
    uint32_t srcL2Id = GetSrcL2Id(netDevs.Get(i));

    m_L2IdToNodePtr.insert(std::pair<uint32_t, Ptr<Node>>
      (srcL2Id, node));

    if (dstL2Ids.size() > 0)
    {
      m_L2IdToNodePtr.insert
      (
        std::pair<uint32_t, Ptr<Node>>
        (dstL2Ids[i], node)
      );

      m_virtualToActualL2Id.insert
      (
        std::pair<uint32_t, uint32_t>
        (dstL2Ids[i], srcL2Id)
      );
    }
    else
    {
      m_remoteL2Ids.insert(srcL2Id);
      //std::cout << "Added remote l2 id: " << std::to_string(srcL2Id) << std::endl;
    }
  }

}

uint32_t
NetSimulyzerProseRelayDiscoveryTracerModelA::GetSrcL2Id(Ptr<NetDevice> netDev)
{
    Ptr<NrSlUeProse> nodeProse = netDev->GetObject<NrSlUeProse>();
    auto ueRrc = nodeProse->GetObject<NrUeNetDevice>()->GetRrc();
    uint32_t srcL2Id = ueRrc->GetSourceL2Id();

    return srcL2Id;
}

void
NetSimulyzerProseRelayDiscoveryTracerModelA::LinkTraces(NetDeviceContainer remoteNetDevs, NetDeviceContainer relayNetDevs)
{
  for (uint32_t i = 0; i < remoteNetDevs.GetN(); i++)
  {
    auto prose = remoteNetDevs.Get(i)->GetObject<NrSlUeProse>();
    prose->TraceConnectWithoutContext("DiscoveryTrace", MakeCallback(&NetSimulyzerProseRelayDiscoveryTracerModelA::DiscoveryTraceCallback, this));
  }

  for (uint32_t i = 0; i < relayNetDevs.GetN(); i++)
  {
    auto prose = relayNetDevs.Get(i)->GetObject<NrSlUeProse>();
    prose->TraceConnectWithoutContext("DiscoveryTrace", MakeCallback(&NetSimulyzerProseRelayDiscoveryTracerModelA::DiscoveryTraceCallback, this));
  }
}

void
NetSimulyzerProseRelayDiscoveryTracerModelA::CreateRelaySideGraphs(NodeContainer remoteNodes, NetDeviceContainer remoteNetDevs, 
                                                          NodeContainer relayNodes, NetDeviceContainer relayNetDevs)
{
  PointerValue xAxis;
  PointerValue yAxis;

  for (uint32_t i = 0; i < relayNodes.GetN(); i++)
  {
    auto relayNode = relayNodes.Get(i);

    auto seriesCollect = CreateObject<netsimulyzer::SeriesCollection>(m_orchestrator);
    seriesCollect->SetAttribute("Name", StringValue("Announcements vs Time - For Relay Node: " + std::to_string(relayNode->GetId())));
    seriesCollect->GetAttribute("XAxis", xAxis);
    seriesCollect->GetAttribute("YAxis", yAxis);
    xAxis.Get<netsimulyzer::ValueAxis>()->SetAttribute("Name", StringValue("Time(s)"));
    yAxis.Get<netsimulyzer::ValueAxis>()->SetAttribute("Name", StringValue("Node Id"));
    seriesCollect->SetAttribute("HideAddedSeries", BooleanValue(false));

    auto announcementsTxXYSeries = CreateObject<netsimulyzer::XYSeries>(m_orchestrator);
    announcementsTxXYSeries->SetAttribute("Name", StringValue("Announcements Tx by Relay Node " + std::to_string(relayNode->GetId())));
    announcementsTxXYSeries->SetAttribute("Connection", StringValue("None"));
    announcementsTxXYSeries->SetAttribute("Color", netsimulyzer::RED_VALUE);
    announcementsTxXYSeries->SetAttribute("Visible", BooleanValue(false)); 
    seriesCollect->Add(announcementsTxXYSeries);

    std::pair<uint32_t, uint32_t> announceTxTuple = {GetSrcL2Id(relayNetDevs.Get(i)), GetSrcL2Id(relayNetDevs.Get(i))};
    m_discoveryMessageSeries.insert
    (
      std::pair<std::pair<uint32_t, uint32_t>, Ptr<netsimulyzer::XYSeries>>
      (announceTxTuple, announcementsTxXYSeries)
    );

    for (uint32_t j = 0; j < remoteNodes.GetN(); j++)
    {
      auto remoteNode = remoteNodes.Get(j);

      auto announcementsRxXYSeries = CreateObject<netsimulyzer::XYSeries>(m_orchestrator);
      announcementsRxXYSeries->SetAttribute("Name", StringValue("Announcements Rx by Remote Node " + std::to_string(remoteNode->GetId())));
      announcementsRxXYSeries->SetAttribute("Connection", StringValue("None"));
      announcementsRxXYSeries->SetAttribute("Visible", BooleanValue(false)); 
      seriesCollect->Add(announcementsRxXYSeries);

      std::pair<uint32_t, uint32_t> announceRxTuple = {GetSrcL2Id(relayNetDevs.Get(i)), GetSrcL2Id(remoteNetDevs.Get(j))};
      m_discoveryMessageSeries.insert
      (
        std::pair<std::pair<uint32_t, uint32_t>, Ptr<netsimulyzer::XYSeries>>
        (announceRxTuple, announcementsRxXYSeries)
      );
    }
  }

}

//void
//NetSimulyzerProseRelayDiscoveryTracerModelA::CreateRelaysDiscoveredCountHistogram(NodeContainer remoteNodes)
//{
//  PointerValue xAxis;
//  PointerValue yAxis;
//
//  //std::map<uint32_t, std::set<uint32_t>> m_discoveredRelays;
//  m_discoveredRelaysHist = CreateObject<netsimulyzer::SeriesCollection>(m_orchestrator);
//  m_discoveredRelaysHist->SetAttribute("Name", StringValue("Histogram - #Relays discovered by each remote"));
//  m_discoveredRelaysHist->GetAttribute("XAxis", xAxis);
//  m_discoveredRelaysHist->GetAttribute("YAxis", yAxis);
//  xAxis.Get<netsimulyzer::ValueAxis>()->SetAttribute("Name", StringValue("Remote Node Id"));
//  yAxis.Get<netsimulyzer::ValueAxis>()->SetAttribute("Name", StringValue("Count"));
//  m_discoveredRelaysHist->SetAttribute("HideAddedSeries", BooleanValue(false));
//
//  for (uint32_t i = 0; i < remoteNodes.GetN(); i++)
//  {
//    uint32_t remoteNodeId = remoteNodes.Get(i)->GetId();
//    uint32_t srcL2Id = GetSrcL2Id(remoteNodes.Get(i)->GetObject<NrUeNetDevice>());
//
//    m_discoveredRelays.insert
//    (
//      std::pair<uint32_t, std::set<uint32_t>>
//      (srcL2Id, {})
//    );
//
//    //Make histogram chart
//  }
//}

void
NetSimulyzerProseRelayDiscoveryTracerModelA::DiscoveryTraceCallback(uint32_t senderL2Id,
                                                                    uint32_t receiverL2Id,
                                                                    bool isTx,
                                                                    NrSlDiscoveryHeader discMsg)
{
  if (!m_isSetUp)
  {
    std::cout << "NOT SETUP" << std::endl;
    return;
  }
  
  std::cout << "isTx: " << std::to_string(isTx) << ", SenderL2Id: " << std::to_string(senderL2Id) << 
    ", ReceiverL2Id: " << std::to_string(receiverL2Id) << std::endl;

  if(isTx)
  {
    // only relays transmit announcements
    std::pair<uint32_t, uint32_t> searchTuple = {senderL2Id, senderL2Id};

    auto it = m_discoveryMessageSeries.find(searchTuple);
    if (it != m_discoveryMessageSeries.end())
    {
      it->second->Append(Simulator::Now().GetSeconds(), m_L2IdToNodePtr[senderL2Id]->GetId());
    }
    else
    {
      std::cout << "\n\nERROR ERROR ERROR ERROR!!!!\n" << std::endl;
      NS_FATAL_ERROR("(" + std::to_string(receiverL2Id) + ", " + std::to_string(senderL2Id) + ") is not a valid (remote, relay) l2id pair.");
    }
  }
  else
  {
    //only remotes receive announcements
    std::pair<uint32_t, uint32_t> searchTuple = {senderL2Id, receiverL2Id};

    auto it = m_discoveryMessageSeries.find(searchTuple);
    if (it != m_discoveryMessageSeries.end())
    {
      it->second->Append(Simulator::Now().GetSeconds(), m_L2IdToNodePtr[receiverL2Id]->GetId());
    }
    else
    {
      std::cout << "\n\nERROR ERROR ERROR ERROR!!!!\n" << std::endl;
      NS_FATAL_ERROR("(" + std::to_string(receiverL2Id) + ", " + std::to_string(senderL2Id) + ") is not a valid (remote, relay) l2id pair.");
    }

    ////TODO: Code for histogram
    //if (m_discoveredRelays[receiverL2Id].contains(senderL2Id) == false)
    //{
    //  m_discoveredRelays[receiverL2Id].insert(senderL2Id);

    //}
  }

}

#endif /* HAS_NETSIMULYZER */
