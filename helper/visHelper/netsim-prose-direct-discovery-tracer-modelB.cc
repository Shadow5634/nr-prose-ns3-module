#ifdef HAS_NETSIMULYZER

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/nr-sl-ue-prose.h"
#include "ns3/nr-sl-discovery-header.h"

#include "netsim-prose-direct-discovery-tracer-modelB.h"
#include "netsim-prose-tracer-helper.h"

using namespace ns3;
using namespace std;

NetSimulyzerProseDirectDiscoveryTracerModelB::NetSimulyzerProseDirectDiscoveryTracerModelB()
{
  m_isSetUp = false;
  m_orchestrator = nullptr;
}

int
NetSimulyzerProseDirectDiscoveryTracerModelB::SetUp(Ptr<netsimulyzer::Orchestrator> orchestrator, NodeContainer nodes, NetDeviceContainer netDevs)
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

  MakeGraphs(nodes, netDevs);
  LinkTraces(netDevs);
  m_isSetUp = true;

  return 1;
}

void
NetSimulyzerProseDirectDiscoveryTracerModelB::LinkTraces(NetDeviceContainer netDevs)
{
  for (uint32_t i = 0; i < netDevs.GetN(); i++)
  {
    auto prose = netDevs.Get(i)->GetObject<NrSlUeProse>();
    prose->TraceConnectWithoutContext("DiscoveryTrace", MakeCallback(&NetSimulyzerProseDirectDiscoveryTracerModelB::DiscoveryTraceCallback, this));
  }
}

void
NetSimulyzerProseDirectDiscoveryTracerModelB::MakeGraphs(NodeContainer nodes, NetDeviceContainer netDevs)
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
NetSimulyzerProseDirectDiscoveryTracerModelB::MakeDiscoveredUesCountHistogram(std::set<uint32_t> allNodeIds, std::set<uint32_t> allL2Ids)
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
    //xySeries->SetAttribute("Color", GetNextColor());
    xySeries->SetAttribute("Visible", BooleanValue(false));
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
NetSimulyzerProseDirectDiscoveryTracerModelB::MakeDiscoveriesReceivedGraphs(std::set<uint32_t> allL2Ids)
{
  PointerValue xAxis;
  PointerValue yAxis;

  for (uint32_t receiverL2Id : allL2Ids)
  {
    auto receiverNode = this->GetNode(receiverL2Id);
    auto seriesCollect = CreateObject<netsimulyzer::SeriesCollection>(m_orchestrator);
    seriesCollect->SetAttribute("Name", StringValue("Discoveries vs Time (All senders) - For Receiver Node: " + std::to_string(receiverNode->GetId())));
    seriesCollect->GetAttribute("XAxis", xAxis);
    seriesCollect->GetAttribute("YAxis", yAxis);
    xAxis.Get<netsimulyzer::ValueAxis>()->SetAttribute("Name", StringValue("Time(s)"));
    yAxis.Get<netsimulyzer::ValueAxis>()->SetAttribute("Name", StringValue("Node Id"));
    seriesCollect->SetAttribute("HideAddedSeries", BooleanValue(false));

    m_discoveriesReceivedCollectionPerNode.insert
    (
      std::pair<uint32_t, Ptr<netsimulyzer::SeriesCollection>>
      (receiverL2Id, seriesCollect)
    );

    for (uint32_t senderL2Id : allL2Ids)
    {
      std::pair<uint32_t, uint32_t> searchTuple = {receiverL2Id, senderL2Id};
      auto senderNode = this->GetNode(senderL2Id);
      
      if (senderL2Id != receiverL2Id)
      {
        auto responsesRxSeries = CreateObject<netsimulyzer::XYSeries>(m_orchestrator);
        responsesRxSeries->SetAttribute("Name", StringValue("Responses Rx from Node: " + std::to_string(senderNode->GetId())));
        responsesRxSeries->SetAttribute("Connection", StringValue("None"));
        responsesRxSeries->SetAttribute("Visible", BooleanValue(false));
        seriesCollect->Add(responsesRxSeries);

        m_discoveriesReceivedGraphPerL2IdPair.insert
        (
          std::pair<std::pair<uint32_t, uint32_t>, Ptr<netsimulyzer::XYSeries>>
          (searchTuple, responsesRxSeries)
        );
      }
      else
      {
        auto solicitationsTxSeries = CreateObject<netsimulyzer::XYSeries>(m_orchestrator);
        solicitationsTxSeries->SetAttribute("Name", StringValue("Solicitations Tx by Node: " + std::to_string(senderNode->GetId())));
        solicitationsTxSeries->SetAttribute("Connection", StringValue("None"));
        solicitationsTxSeries->SetAttribute("Color", netsimulyzer::RED_VALUE);
        solicitationsTxSeries->SetAttribute("Visible", BooleanValue(false));
        seriesCollect->Add(solicitationsTxSeries);     

        m_discoveriesReceivedGraphPerL2IdPair.insert
        (
          std::pair<std::pair<uint32_t, uint32_t>, Ptr<netsimulyzer::XYSeries>>
          (searchTuple, solicitationsTxSeries)
        );
      }
    }
  }
}

Ptr<Node> 
NetSimulyzerProseDirectDiscoveryTracerModelB::GetNode(uint32_t l2Id)
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

void 
NetSimulyzerProseDirectDiscoveryTracerModelB::DiscoveryTraceCallback(uint32_t senderL2Id,
                                                                      uint32_t receiverL2Id,
                                                                      bool isTx,
                                                                      NrSlDiscoveryHeader discMsg)
{
  if(!m_isSetUp)
  {return;}

  if ((isTx) && (discMsg.GetDiscoveryMsgType() == NrSlDiscoveryHeader::DiscoveryMsgType::DISC_RESTRICTED_QUERY))
  {
    // for <sender, sender> add tx point if of type solicitation
    std::pair<uint32_t, uint32_t> searchTuple = {senderL2Id, senderL2Id};
    auto solicitTxIt = m_discoveriesReceivedGraphPerL2IdPair.find(searchTuple);
    if (solicitTxIt != m_discoveriesReceivedGraphPerL2IdPair.end())
    {
      solicitTxIt->second->Append(Simulator::Now().GetSeconds(), GetNode(senderL2Id)->GetId());
    }
    else
    {
      NS_FATAL_ERROR("Sender, sender not a valid point"); 
    }
  }
  else if ((isTx == false) && (discMsg.GetDiscoveryMsgType() == NrSlDiscoveryHeader::DiscoveryMsgType::DISC_RESTRICTED_RESPONSE))
  {
    // Add to discovery series
    // for receiver. sender add rx point if of type response

    std::pair<uint32_t, uint32_t> searchTuple = {receiverL2Id, senderL2Id};
    auto responseRxIt = m_discoveriesReceivedGraphPerL2IdPair.find(searchTuple);
    if (responseRxIt != m_discoveriesReceivedGraphPerL2IdPair.end())
    {
      responseRxIt->second->Append(Simulator::Now().GetSeconds(), GetNode(senderL2Id)->GetId());
    }
    else
    {
      NS_FATAL_ERROR("Sender, sender not a valid point"); 
    }   

    // Add to histogram
    uint32_t receiverNodeId = GetNode(receiverL2Id)->GetId(); 
    auto discL2IdsIt = m_discoveredL2Ids.find(receiverL2Id);

    if (discL2IdsIt != m_discoveredL2Ids.end())
    {
      if (discL2IdsIt->second.contains(senderL2Id) == false)
      {
        discL2IdsIt->second.insert(senderL2Id);

        auto discoveredHistLine = m_discoveredUesCountGraphPerNode[receiverNodeId];
        discoveredHistLine->Append(receiverNodeId,discL2IdsIt->second.size());
      }
    }
    else
    {
      NS_FATAL_ERROR("L2Id not found");
    }
  }
  else
  {}
}

#endif
