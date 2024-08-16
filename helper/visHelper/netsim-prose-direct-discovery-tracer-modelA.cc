#ifdef HAS_NETSIMULYZER

#include "ns3/antenna-module.h"
#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/nr-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/stats-module.h"

#include "ns3/nr-sl-ue-prose.h"

#include "netsim-prose-direct-discovery-tracer-modelA.h"
#include "netsim-prose-tracer-helper.h"
#include "no-tracings.h"

using namespace ns3;
using namespace std;

NetSimulyzerProseDirectDiscoveryTracerModelA::NetSimulyzerProseDirectDiscoveryTracerModelA()
{
  m_isSetUp = false;
  m_orchestrator = nullptr;
}

int
NetSimulyzerProseDirectDiscoveryTracerModelA::SetUp(Ptr<netsimulyzer::Orchestrator> orchestrator, NodeContainer nodes, NetDeviceContainer netDevs)
{
  if (m_isSetUp)
  {
    return 0;
  }
  
  if ((orchestrator == nullptr) || (netDevs.GetN() != nodes.GetN()) || (nodes.GetN() == 0))
  {
    m_isSetUp = false;
    return -1;
  }

  m_orchestrator = orchestrator; 

  std::set<uint32_t> allL2Ids;
  for (uint32_t i = 0; i < netDevs.GetN(); i++)
  {
    auto node = nodes.Get(i);
    uint32_t srcL2Id = GetSrcL2Id(netDevs.Get(i));
    allL2Ids.insert(srcL2Id);
    
    m_L2IdToNodePtr.insert
    (
      std::pair<uint32_t, Ptr<Node>>
      (srcL2Id, node)
    );
  }

  MakeGraphs(nodes, netDevs);
  LinkTraces(netDevs);
  m_isSetUp = true;

  return 1;
}

void
NetSimulyzerProseDirectDiscoveryTracerModelA::MakeGraphs(NodeContainer nodes, NetDeviceContainer netDevs)
{
  std::set<uint32_t> allL2Ids;
  std::set<uint32_t> allNodeIds;

  for (uint32_t i = 0; i < netDevs.GetN(); i++)
  {
    auto node = nodes.Get(i);
    uint32_t srcL2Id = GetSrcL2Id(netDevs.Get(i));
    
    allL2Ids.insert(srcL2Id);
    allNodeIds.insert(node->GetId());
    
    m_L2IdToNodePtr.insert
    (
      std::pair<uint32_t, Ptr<Node>>
      (srcL2Id, node)
    );
  }

  MakeDiscoveredUesCountHistogram(allNodeIds, allL2Ids);
  MakeDiscoveriesReceivedGraphs(allL2Ids);
}

void
NetSimulyzerProseDirectDiscoveryTracerModelA::MakeDiscoveredUesCountHistogram(std::set<uint32_t> allNodeIds, std::set<uint32_t> allL2Ids)
{

  PointerValue xAxis;
  PointerValue yAxis;

  m_discoveredUesCountHistogram = CreateObject<netsimulyzer::SeriesCollection>(m_orchestrator);
  m_discoveredUesCountHistogram->SetAttribute("Name", StringValue("#Discovered Ues Histogram - All Nodes"));
  m_discoveredUesCountHistogram->GetAttribute("XAxis", xAxis);
  m_discoveredUesCountHistogram->GetAttribute("YAxis", yAxis);
  yAxis.Get<netsimulyzer::ValueAxis>()->SetAttribute("Name", StringValue("Discovery Count (#)"));
  xAxis.Get<netsimulyzer::ValueAxis>()->SetAttribute("Name", StringValue("Node Id"));
  m_discoveredUesCountHistogram->SetAttribute("HideAddedSeries", BooleanValue(false)); 

  for (uint32_t nodeId : allNodeIds)
  {
    auto xySeries = CreateObject<netsimulyzer::XYSeries>(m_orchestrator);
    xySeries->SetAttribute("Name", StringValue("Node " + std::to_string(nodeId)));
    xySeries->SetAttribute("Color", netsimulyzer::DARK_ORANGE_VALUE);
    xySeries->SetAttribute("Visible", BooleanValue(false));
    xySeries->GetYAxis()->SetAttribute("Name", StringValue("Discovery Count (#)"));
    m_discoveredUesCountHistogram->Add(xySeries);
    xySeries->Append(nodeId, 0);

    m_discoveredUesCountGraphPerNode.insert
    (
      std::pair<uint32_t, Ptr<netsimulyzer::XYSeries>>
      (nodeId, xySeries)
    );
  }

  for (uint32_t l2Id : allL2Ids)
  {
    m_discoveredL2Ids.insert
    (
      std::pair<uint32_t, std::set<uint32_t>>
      (l2Id, std::set<uint32_t>{})
    );
  }
}

void
NetSimulyzerProseDirectDiscoveryTracerModelA::MakeDiscoveriesReceivedGraphs(std::set<uint32_t> allL2Ids)
{
  PointerValue xAxis;
  PointerValue yAxis;

  for (uint32_t senderL2Id : allL2Ids)
  {
    auto senderNode = this->GetNode(senderL2Id);
    auto seriesCollect = CreateObject<netsimulyzer::SeriesCollection>(m_orchestrator);
    seriesCollect->SetAttribute("Name", StringValue("Discoveries vs Time (All senders) - For Sender Node: " + std::to_string(senderNode->GetId())));
    seriesCollect->GetAttribute("XAxis", xAxis);
    seriesCollect->GetAttribute("YAxis", yAxis);
    xAxis.Get<netsimulyzer::ValueAxis>()->SetAttribute("Name", StringValue("Time(s)"));
    yAxis.Get<netsimulyzer::ValueAxis>()->SetAttribute("Name", StringValue("Node Id"));
    seriesCollect->SetAttribute("HideAddedSeries", BooleanValue(false));

    m_discoveriesReceivedCollectionPerNode.insert
    (
      std::pair<uint32_t, Ptr<netsimulyzer::SeriesCollection>>
      (senderL2Id, seriesCollect)
    );

    for (uint32_t receiverL2Id : allL2Ids)
    {
      std::pair<uint32_t, uint32_t> searchTuple = {senderL2Id, receiverL2Id};
      auto receiverNode = this->GetNode(senderL2Id);

      if (senderL2Id != receiverL2Id)
      {
        auto announcementsRxSeries = CreateObject<netsimulyzer::XYSeries>(m_orchestrator);
        announcementsRxSeries->SetAttribute("Name", StringValue("Announcements Rx by Node: " + std::to_string(receiverNode->GetId())));
        announcementsRxSeries->SetAttribute("Connection", StringValue("None"));
        announcementsRxSeries->SetAttribute("Visible", BooleanValue(false));
        announcementsRxSeries->GetXAxis()->SetAttribute("Name", StringValue("Time(s)"));
        seriesCollect->Add(announcementsRxSeries);

        m_discoveriesReceivedGraphPerL2IdPair.insert
        (
          std::pair<std::pair<uint32_t, uint32_t>, Ptr<netsimulyzer::XYSeries>>
          (searchTuple, announcementsRxSeries)
        );
      }
      else
      {
        auto announcementsTxSeries = CreateObject<netsimulyzer::XYSeries>(m_orchestrator);
        announcementsTxSeries->SetAttribute("Name", StringValue("Announcements Tx by Node: " + std::to_string(senderNode->GetId())));
        announcementsTxSeries->SetAttribute("Connection", StringValue("None"));
        announcementsTxSeries->SetAttribute("Color", netsimulyzer::RED_VALUE);
        announcementsTxSeries->SetAttribute("Visible", BooleanValue(false));
        announcementsTxSeries->GetXAxis()->SetAttribute("Name", StringValue("Time(s)"));
        seriesCollect->Add(announcementsTxSeries);

        m_discoveriesReceivedGraphPerL2IdPair.insert
        (
          std::pair<std::pair<uint32_t, uint32_t>, Ptr<netsimulyzer::XYSeries>>
          (searchTuple, announcementsTxSeries)
        );
      }
    }
  }
}

void
NetSimulyzerProseDirectDiscoveryTracerModelA::LinkTraces(NetDeviceContainer netDevs)
{
  for (uint32_t i = 0; i < netDevs.GetN(); i++)
  {
    auto prose = netDevs.Get(i)->GetObject<NrSlUeProse>();
    prose->TraceConnectWithoutContext("DiscoveryTrace", MakeCallback(&NetSimulyzerProseDirectDiscoveryTracerModelA::DiscoveryTraceCallback, this));
  }
}

Ptr<Node> 
NetSimulyzerProseDirectDiscoveryTracerModelA::GetNode(uint32_t l2Id)
{
  auto it = m_L2IdToNodePtr.find(l2Id);
  if (it != m_L2IdToNodePtr.end())
  {
    return it->second;
  }
  else
  {
    NS_FATAL_ERROR("Node with specified l2 id does not exist: " + std::to_string(l2Id));
  }

  return nullptr;
}

/**
 * Trace sink for the ns3::NrSlUeProse::DiscoveryTrace trace source
 * 
 * \param discoveryTrace
 * \param path trace path
 * \param SenderL2Id sender L2 ID
 * \param ReceiverL2Id receiver L2 ID
 * \param isTx True if the UE is transmitting and False receiving a discovery message
 * \param discMsg the discovery header storing the NR SL discovery header information
 */
void 
NetSimulyzerProseDirectDiscoveryTracerModelA::DiscoveryTraceCallback(uint32_t senderL2Id,
                                                                      uint32_t receiverL2Id,
                                                                      bool isTx,
                                                                      NrSlDiscoveryHeader discMsg)
{
  if(!m_isSetUp)
  {return;}

  if (isTx)
  {
    uint32_t senderNodeId = this->GetNode(senderL2Id)->GetId();
    std::pair<uint32_t, uint32_t> searchTuple = {senderL2Id, senderL2Id};
    auto discMsgTxIt = m_discoveriesReceivedGraphPerL2IdPair.find(searchTuple);

    if (discMsgTxIt != m_discoveriesReceivedGraphPerL2IdPair.end())
    {
      discMsgTxIt->second->Append(Simulator::Now().GetSeconds(), senderNodeId);
    }
  }
  else
  {
    // Add to histogram
    uint32_t receiverNodeId = this->GetNode(receiverL2Id)->GetId(); 
    auto discL2IdsIt = m_discoveredL2Ids.find(receiverL2Id);

    if (discL2IdsIt != m_discoveredL2Ids.end())
    {
      if (!(discL2IdsIt->second.contains(senderL2Id)))
      {
        discL2IdsIt->second.insert(senderL2Id);

        auto discoveredHistLine = m_discoveredUesCountGraphPerNode[receiverNodeId];
        discoveredHistLine->Append(receiverNodeId, discL2IdsIt->second.size());
      }
    }
    else
    {
      NS_FATAL_ERROR("L2Id not found");
    }
      
    // Add to discoveries series
    std::pair<uint32_t, uint32_t> searchTuple = {senderL2Id, receiverL2Id};
    auto discMsgRxIt = m_discoveriesReceivedGraphPerL2IdPair.find(searchTuple);

    if (discMsgRxIt != m_discoveriesReceivedGraphPerL2IdPair.end())
    {
      discMsgRxIt->second->Append(Simulator::Now().GetSeconds(), receiverNodeId); 
    }
    else
    {
       NS_FATAL_ERROR("Search Tuple not found");   
    }
  }
}

#endif
