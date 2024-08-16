#ifdef HAS_NETSIMULYZER

#include "netsim-prose-relay-discovery-tracer-modelB.h"
#include "ns3/nr-sl-ue-prose.h"
#include "color-pair.h"


#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include <ns3/nr-module.h>
#include <ns3/netsimulyzer-module.h>

using namespace ns3;
using namespace std;

NetSimulyzerProseRelayDiscoveryTracerModelB::NetSimulyzerProseRelayDiscoveryTracerModelB()
{
  m_isSetUp = false;
  m_orchestrator = nullptr;
}

int
NetSimulyzerProseRelayDiscoveryTracerModelB::SetUp(Ptr<netsimulyzer::Orchestrator> orchestrator, 
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
  CreateGraphs(remoteNodes, remoteNetDevs, relayNodes, relayNetDevs);
  CreateRelaySideGraphs(remoteNodes, remoteNetDevs, relayNodes, relayNetDevs);

  LinkTraces(remoteNetDevs, relayNetDevs);
  m_isSetUp = true;
  return 1;
}

uint32_t
NetSimulyzerProseRelayDiscoveryTracerModelB::GetSrcL2Id(Ptr<NetDevice> netDev)
{
    Ptr<NrSlUeProse> nodeProse = netDev->GetObject<NrSlUeProse>();
    auto ueRrc = nodeProse->GetObject<NrUeNetDevice>()->GetRrc();
    uint32_t srcL2Id = ueRrc->GetSourceL2Id();

    return srcL2Id;
}

void
NetSimulyzerProseRelayDiscoveryTracerModelB::LinkTraces(NetDeviceContainer remoteNetDevs, NetDeviceContainer relayNetDevs)
{
  for (uint32_t i = 0; i < remoteNetDevs.GetN(); i++)
  {
    auto prose = remoteNetDevs.Get(i)->GetObject<NrSlUeProse>();
    prose->TraceConnectWithoutContext("DiscoveryTrace", MakeCallback(&NetSimulyzerProseRelayDiscoveryTracerModelB::DiscoveryTraceCallback, this));
  }

  for (uint32_t i = 0; i < relayNetDevs.GetN(); i++)
  {
    auto prose = relayNetDevs.Get(i)->GetObject<NrSlUeProse>();
    prose->TraceConnectWithoutContext("DiscoveryTrace", MakeCallback(&NetSimulyzerProseRelayDiscoveryTracerModelB::DiscoveryTraceCallback, this));
  }
}

void
NetSimulyzerProseRelayDiscoveryTracerModelB::AddNodes(NodeContainer nodes, NetDeviceContainer netDevs, std::vector<uint32_t> dstL2Ids)
{

  for (uint32_t i = 0; i < nodes.GetN(); i++)
  {
    Ptr<Node> node = nodes.Get(i);
    uint32_t srcL2Id = GetSrcL2Id(netDevs.Get(i));

    m_L2IdToNodePtr.insert(std::pair<uint32_t, Ptr<Node>>
      (srcL2Id, node));

    if (dstL2Ids.size() != 0)
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

      m_allRelayL2Ids.insert(srcL2Id);
    }
    else
    {
      m_allRemoteL2Ids.insert(srcL2Id);
    }
  }

}

void
NetSimulyzerProseRelayDiscoveryTracerModelB::CreateGraphs(NodeContainer remoteNodes, NetDeviceContainer remoteNetDevs, 
                                                          NodeContainer relayNodes, NetDeviceContainer relayNetDevs)
{
  PointerValue xAxis;
  PointerValue yAxis;
  ColorPair colorGetter;
  
  for (uint32_t i = 0; i < remoteNodes.GetN(); i++)
  {
    auto remoteNode = remoteNodes.Get(i);

    auto seriesCollect = CreateObject<netsimulyzer::SeriesCollection>(m_orchestrator);
    seriesCollect->SetAttribute("Name", StringValue("Discovery messages vs Time (All relays) - For Remote Node: " + std::to_string(remoteNode->GetId())));
    seriesCollect->GetAttribute("XAxis", xAxis);
    seriesCollect->GetAttribute("YAxis", yAxis);
    xAxis.Get<netsimulyzer::ValueAxis>()->SetAttribute("Name", StringValue("Time(s)"));
    yAxis.Get<netsimulyzer::ValueAxis>()->SetAttribute("Name", StringValue("Relay Node Id"));
    seriesCollect->SetAttribute("HideAddedSeries", BooleanValue(false));

    for (uint32_t j = 0; j < relayNodes.GetN(); j++)
    {
      auto relayNode = relayNodes.Get(j);

      auto solicitationXYSeries = CreateObject<netsimulyzer::XYSeries>(m_orchestrator);
      solicitationXYSeries->SetAttribute("Name", StringValue("Solicitation Tx - Relay Node " + std::to_string(relayNode->GetId())));
      solicitationXYSeries->SetAttribute("Connection", StringValue("None"));
      solicitationXYSeries->SetAttribute("Color", colorGetter.GetNextColor());
      solicitationXYSeries->SetAttribute("Visible", BooleanValue(false));
      solicitationXYSeries->GetXAxis()->SetAttribute("Name", StringValue("Time(s)"));
      solicitationXYSeries->GetYAxis()->SetAttribute("Name", StringValue("Discovery Messages"));
      seriesCollect->Add(solicitationXYSeries);

      auto responseXYSeries = CreateObject<netsimulyzer::XYSeries>(m_orchestrator);
      responseXYSeries->SetAttribute("Name", StringValue("Response Rx - Relay Node " + std::to_string(relayNode->GetId())));
      responseXYSeries->SetAttribute("Connection", StringValue("None"));
      responseXYSeries->SetAttribute("Color", colorGetter.GetNextColor());
      responseXYSeries->SetAttribute("Visible", BooleanValue(false));
      responseXYSeries->GetXAxis()->SetAttribute("Name", StringValue("Time(s)"));
      responseXYSeries->GetYAxis()->SetAttribute("Name", StringValue("Discovery Messages"));
      seriesCollect->Add(responseXYSeries);

      std::pair<uint32_t, uint32_t> searchTuple = {GetSrcL2Id(remoteNetDevs.Get(i)), GetSrcL2Id(relayNetDevs.Get(j))};
      TxRxMessageSeries graphTuple = {.txSeries = solicitationXYSeries, .rxSeries = responseXYSeries};

      m_discoveryMessageSeries.insert
      (
        std::pair<std::pair<uint32_t, uint32_t>, TxRxMessageSeries>
        (
          searchTuple,
          graphTuple
        )
      );

    }
  }
}

void
NetSimulyzerProseRelayDiscoveryTracerModelB::CreateRelaySideGraphs(NodeContainer remoteNodes, NetDeviceContainer remoteNetDevs, 
                                                          NodeContainer relayNodes, NetDeviceContainer relayNetDevs)
{
  PointerValue xAxis;
  PointerValue yAxis;
  ColorPair colorGetter;

  for (uint32_t i = 0; i < relayNodes.GetN(); i++)
  {
    auto relayNode = relayNodes.Get(i);

    auto seriesCollect = CreateObject<netsimulyzer::SeriesCollection>(m_orchestrator);
    seriesCollect->SetAttribute("Name", StringValue("Solicitation requests vs Time (All remotes) - For Relay Node: " + std::to_string(relayNode->GetId())));
    seriesCollect->GetAttribute("XAxis", xAxis);
    seriesCollect->GetAttribute("YAxis", yAxis);
    xAxis.Get<netsimulyzer::ValueAxis>()->SetAttribute("Name", StringValue("Time(s)"));
    yAxis.Get<netsimulyzer::ValueAxis>()->SetAttribute("Name", StringValue("Remote Node Id"));
    seriesCollect->SetAttribute("HideAddedSeries", BooleanValue(false));

    for (uint32_t j = 0; j < remoteNodes.GetN(); j++)
    {
      auto remoteNode = remoteNodes.Get(j);

      auto solicitationsRxSeries = CreateObject<netsimulyzer::XYSeries>(m_orchestrator);
      solicitationsRxSeries->SetAttribute("Name", StringValue("Solicitations Rx for Remote Node " + std::to_string(remoteNode->GetId())));
      solicitationsRxSeries->SetAttribute("Connection", StringValue("None"));
      solicitationsRxSeries->SetAttribute("Color", colorGetter.GetNextColor());
      solicitationsRxSeries->SetAttribute("Visible", BooleanValue(false));
      solicitationsRxSeries->GetXAxis()->SetAttribute("Name", StringValue("Time(s)"));
      solicitationsRxSeries->GetYAxis()->SetAttribute("Name", StringValue("Solicitation Requests"));
      seriesCollect->Add(solicitationsRxSeries);

      auto solicitationsTxSeries = CreateObject<netsimulyzer::XYSeries>(m_orchestrator);
      solicitationsTxSeries->SetAttribute("Name", StringValue("Solicitations Tx for Remote Node " + std::to_string(remoteNode->GetId())));
      solicitationsTxSeries->SetAttribute("Connection", StringValue("None"));
      solicitationsTxSeries->SetAttribute("Color", colorGetter.GetNextColor());
      solicitationsTxSeries->SetAttribute("Visible", BooleanValue(false));
      solicitationsTxSeries->GetXAxis()->SetAttribute("Name", StringValue("Time(s)"));
      solicitationsTxSeries->GetYAxis()->SetAttribute("Name", StringValue("Solicitation Requests"));
      seriesCollect->Add(solicitationsTxSeries);

      std::pair<uint32_t, uint32_t> searchTuple = {GetSrcL2Id(relayNetDevs.Get(i)), GetSrcL2Id(remoteNetDevs.Get(j))};
      TxRxMessageSeries graphTuple = {.txSeries = solicitationsTxSeries, .rxSeries = solicitationsRxSeries};
      m_relaySolicitationRequestsSeries.insert
      (
        std::pair<std::pair<uint32_t, uint32_t>, TxRxMessageSeries>
        (searchTuple, graphTuple)
      );
    }

  }
}

void
NetSimulyzerProseRelayDiscoveryTracerModelB::DiscoveryTraceCallback(uint32_t senderL2Id,
                                                                    uint32_t receiverL2Id,
                                                                    bool isTx,
                                                                    NrSlDiscoveryHeader discMsg)
{
  if (!m_isSetUp)
  {
    return;
  }

  if(isTx)
  {
    if (m_allRemoteL2Ids.contains(senderL2Id))
    {
      // virtual receiver are used only by remotes
      auto it = m_virtualToActualL2Id.find(receiverL2Id);
      if (it != m_virtualToActualL2Id.end())
      {
        uint32_t actualRecvL2Id = m_virtualToActualL2Id[receiverL2Id];
        std::pair<uint32_t, uint32_t> searchTuple = {senderL2Id, actualRecvL2Id};
        std::pair<uint32_t, uint32_t> relaySearchTuple = {actualRecvL2Id, senderL2Id};       

        m_discoveryMessageSeries[searchTuple].txSeries->Append(Simulator::Now().GetSeconds(), m_L2IdToNodePtr[actualRecvL2Id]->GetId()+0.1);
        m_relaySolicitationRequestsSeries[relaySearchTuple].txSeries->Append(Simulator::Now().GetSeconds(), m_L2IdToNodePtr[senderL2Id]->GetId()+0.1);
      }
      else
      {
        std::cout << "\n\nERROR ERROR ERROR ERROR!!!!\n" << std::endl;
        NS_FATAL_ERROR("Receiver id " + std::to_string(receiverL2Id) + " is not one of the provided virtual relay ids");
      }
    }
    else if (m_allRelayL2Ids.contains(senderL2Id))
    {
      
    }
    else
    {
      NS_FATAL_ERROR("Sender L2Id: " + std::to_string(senderL2Id) + " not recognized");
    }
  }
  else
  {
    std::pair<uint32_t, uint32_t> searchTuple = {receiverL2Id, senderL2Id};

    if (m_allRemoteL2Ids.contains(receiverL2Id))
    {
      auto it = m_discoveryMessageSeries.find(searchTuple);
      if (it != m_discoveryMessageSeries.end())
      {
        it->second.rxSeries->Append(Simulator::Now().GetSeconds(), m_L2IdToNodePtr[senderL2Id]->GetId()-0.1);
      }
      else
      {
        std::cout << "\n\nERROR ERROR ERROR ERROR!!!!\n" << std::endl;
        NS_FATAL_ERROR("(" + std::to_string(receiverL2Id) + ", " + std::to_string(senderL2Id) + ") is not a valid (remote, relay) l2id pair.");
      }
    }
    else if(m_allRelayL2Ids.contains(receiverL2Id))
    {
      auto it = m_relaySolicitationRequestsSeries.find(searchTuple);
      if (it != m_relaySolicitationRequestsSeries.end())
      {
        it->second.rxSeries->Append(Simulator::Now().GetSeconds(), m_L2IdToNodePtr[senderL2Id]->GetId()-0.1);
      }
      else
      {
        std::cout << "\n\nERROR ERROR ERROR ERROR!!!!\n" << std::endl;
        NS_FATAL_ERROR("(" + std::to_string(receiverL2Id) + ", " + std::to_string(senderL2Id) + ") is not a valid (relay, remote) l2id pair.");
      }

    }
    else
    {
      NS_FATAL_ERROR("Receiver L2Id: " + std::to_string(receiverL2Id) + " not recognized");
    }
  }
}

#endif /* HAS_NETSIMULYZER */
