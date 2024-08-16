#ifdef HAS_NETSIMULYZER

#include "ns3/nr-sl-ue-prose.h"
#include "ns3/nr-sl-discovery-header.h"

#include "netsim-prose-direct-discovery-tracer.h"

#include <ns3/lte-ue-rrc.h>
#include <ns3/nr-ue-net-device.h>
#include <ns3/core-module.h>
#include <ns3/network-module.h>

namespace ns3
{

using namespace std;

NetSimulyzerProseDirectDiscoveryTracer::NetSimulyzerProseDirectDiscoveryTracer()
{
  m_isSetUp = false;
  m_orchestrator = nullptr;
}

int
NetSimulyzerProseDirectDiscoveryTracer::SetUp(std::string model, Ptr<netsimulyzer::Orchestrator> orchestrator, NetDeviceContainer netDevs)
{
  if (m_isSetUp)
  {
    return 0;
  }
  
  if (((model != "ModelA") && (model != "ModelB")) || (orchestrator == nullptr) || (netDevs.GetN() == 0))
  {
    m_isSetUp = false;
    return -1;
  }

  m_orchestrator = orchestrator; 
  m_model = model;

  this->MakeGraphs(netDevs);
  this->LinkTraces(netDevs);
  m_isSetUp = true;

  return 1;
}

void
NetSimulyzerProseDirectDiscoveryTracer::LinkTraces(NetDeviceContainer netDevs)
{
  for (uint32_t i = 0; i < netDevs.GetN(); i++)
  {
    auto prose = netDevs.Get(i)->GetObject<NrSlUeProse>();
    prose->TraceConnectWithoutContext("DiscoveryTrace", MakeCallback(&NetSimulyzerProseDirectDiscoveryTracer::DiscoveryTraceCallback, this));
  }
}

void
NetSimulyzerProseDirectDiscoveryTracer::MakeGraphs(NetDeviceContainer netDevs)
{
  std::set<uint32_t> allL2Ids;
  std::set<uint32_t> allNodeIds;

  for (uint32_t i = 0; i < netDevs.GetN(); i++)
  {
    auto node = netDevs.Get(i)->GetNode();
    uint32_t srcL2Id = GetSrcL2Id(netDevs.Get(i));
    
    allL2Ids.insert(srcL2Id);
    allNodeIds.insert(node->GetId());
    
    m_L2IdToNodePtr.insert
    (
      std::pair<uint32_t, Ptr<Node>>
      (srcL2Id, node)
    );

    std::cout << "Node " << std::to_string(node->GetId()) << " has L2Id: " << std::to_string(srcL2Id) << std::endl; 
  }

  MakeDiscoveredUesCountHistogram(allNodeIds, allL2Ids);
  MakeDiscoveriesReceivedGraphs(allL2Ids);
}

void
NetSimulyzerProseDirectDiscoveryTracer::MakeDiscoveredUesCountHistogram(std::set<uint32_t> allNodeIds, std::set<uint32_t> allL2Ids)
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
NetSimulyzerProseDirectDiscoveryTracer::MakeDiscoveriesReceivedGraphs(std::set<uint32_t> allL2Ids)
{
  PointerValue xAxis;
  PointerValue yAxis;

  // ModelA --> sender of announcements 
  // ModelB --> sender of solicitations
  for (uint32_t senderL2Id : allL2Ids)
  {
    auto senderNodeId = this->GetNode(senderL2Id)->GetId();
    auto seriesCollect = CreateObject<netsimulyzer::SeriesCollection>(m_orchestrator);
    seriesCollect->SetAttribute("Name", StringValue("Discoveries vs Time - Node: " + std::to_string(senderNodeId)));
    seriesCollect->GetAttribute("XAxis", xAxis);
    seriesCollect->GetAttribute("YAxis", yAxis);
    xAxis.Get<netsimulyzer::ValueAxis>()->SetAttribute("Name", StringValue("Time(s)"));
    yAxis.Get<netsimulyzer::ValueAxis>()->SetAttribute("Name", StringValue("Node Id"));
    seriesCollect->SetAttribute("HideAddedSeries", BooleanValue(false));

    m_discoveryMsgCollectionPerNode.insert
    (
      std::pair<uint32_t, Ptr<netsimulyzer::SeriesCollection>>
      (senderL2Id, seriesCollect)
    );

    for (uint32_t receiverL2Id : allL2Ids)
    {
      std::pair<uint32_t, uint32_t> searchTuple = {senderL2Id, receiverL2Id};
      auto receiverNodeId = this->GetNode(receiverL2Id)->GetId();
      
      std::string graphName;
      // ModelA --> receiver is the receiver of the announcements
      // ModelB --> receiver is the sender of the reponse/receiver of the solicitation
      if (receiverL2Id != senderL2Id)
      {
        if (m_model == "ModelA")
        {
          graphName = "Announcements Rx by Node: " + std::to_string(receiverNodeId);       
        }
        else
        {
          graphName = "Responses Rx from Node: " + std::to_string(receiverNodeId); 
        }

        // ModelA --> rxSeries = announcements
        // ModelB --> rxSeries = responses
        auto rxSeries = CreateObject<netsimulyzer::XYSeries>(m_orchestrator);
        rxSeries->SetAttribute("Name", StringValue(graphName));
        rxSeries->SetAttribute("Connection", StringValue("None"));
        rxSeries->SetAttribute("Visible", BooleanValue(false));
        seriesCollect->Add(rxSeries);

        m_discoveryMsgGraphPerL2IdPair.insert
        (
          std::pair<std::pair<uint32_t, uint32_t>, Ptr<netsimulyzer::XYSeries>>
          (searchTuple, rxSeries)
        );
      }
      else
      {
        if (m_model == "ModelA")
        {
          graphName = "Announcements Tx by Node: " + std::to_string(senderNodeId);       
        }
        else
        {
          graphName = "Solicitations Tx by Node: " + std::to_string(senderNodeId); 
        }

        // ModelA --> txSeries = announcements
        // ModelB --> txSeries = solicitations
        auto txSeries = CreateObject<netsimulyzer::XYSeries>(m_orchestrator);
        txSeries->SetAttribute("Name", StringValue(graphName));
        txSeries->SetAttribute("Connection", StringValue("None"));
        txSeries->SetAttribute("Color", netsimulyzer::RED_VALUE);
        txSeries->SetAttribute("Visible", BooleanValue(false));
        seriesCollect->Add(txSeries);     

        m_discoveryMsgGraphPerL2IdPair.insert
        (
          std::pair<std::pair<uint32_t, uint32_t>, Ptr<netsimulyzer::XYSeries>>
          (searchTuple, txSeries)
        );
      }
    }
  }
}

Ptr<Node> 
NetSimulyzerProseDirectDiscoveryTracer::GetNode(uint32_t l2Id)
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

uint32_t
NetSimulyzerProseDirectDiscoveryTracer::GetSrcL2Id(Ptr<NetDevice> netDev)
{
    Ptr<NrSlUeProse> nodeProse = netDev->GetObject<NrSlUeProse>();
    auto ueRrc = nodeProse->GetObject<NrUeNetDevice>()->GetRrc();
    uint32_t srcL2Id = ueRrc->GetSourceL2Id();

    return srcL2Id;
}

void 
NetSimulyzerProseDirectDiscoveryTracer::DiscoveryTraceCallback(uint32_t senderL2Id,
                                                                      uint32_t receiverL2Id,
                                                                      bool isTx,
                                                                      NrSlDiscoveryHeader discMsg)
{
  if(!m_isSetUp)
  {return;}

  // ModelA - add to sender, sender graph as the announcement sent
  // MobelB - ONLY IF SOLICITATION, sender, sender graph as the solicitation sent
  if (isTx)
  {
    if ((m_model == "ModelB") && (discMsg.GetDiscoveryMsgType() != NrSlDiscoveryHeader::DiscoveryMsgType::DISC_RESTRICTED_QUERY))
    {
      return;  
    }

    std::pair<uint32_t, uint32_t> searchTuple = {senderL2Id, senderL2Id};
    auto txGraphIt = m_discoveryMsgGraphPerL2IdPair.find(searchTuple);
    if (txGraphIt != m_discoveryMsgGraphPerL2IdPair.end())
    {
      txGraphIt->second->Append(Simulator::Now().GetSeconds(), this->GetNode(senderL2Id)->GetId());
    }
    else
    {
      NS_FATAL_ERROR("Sender, sender not a valid point"); 
    }   
  }
  // ModelA - add to sender, receiver graph as the announcement received 
  // MobelB - ONLY IF RESPONSE, sender, receiver graph as the response received
  else
  {
    if ((m_model == "ModelB") && (discMsg.GetDiscoveryMsgType() != NrSlDiscoveryHeader::DiscoveryMsgType::DISC_RESTRICTED_RESPONSE))
    {
      return;  
    } 

    // Model A - the sender is the sender of the announcements
    // Model B - the sender is the sender of the response (but we want sender of the solicitation)
    std::pair<uint32_t, uint32_t> searchTuple = {0, 0};
    if (m_model == "ModelA")
    {
      searchTuple = {senderL2Id, receiverL2Id};
    }
    else
    {
      searchTuple = {receiverL2Id, senderL2Id};   
    }

    auto rxGraphIt = m_discoveryMsgGraphPerL2IdPair.find(searchTuple);
    if (rxGraphIt != m_discoveryMsgGraphPerL2IdPair.end())
    {
      if (m_model == "ModelA")
      {
        rxGraphIt->second->Append(Simulator::Now().GetSeconds(), this->GetNode(receiverL2Id)->GetId());
      }
      else
      {
        rxGraphIt->second->Append(Simulator::Now().GetSeconds(), this->GetNode(senderL2Id)->GetId());   
      }
    }
    else
    {
      NS_FATAL_ERROR("Sender, sender not a valid point"); 
    }

    // Add to histogram
    uint32_t receiverNodeId = this->GetNode(receiverL2Id)->GetId(); 
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

}

} /* namespace ns3 */

#endif
