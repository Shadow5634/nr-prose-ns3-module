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

#include "netsim-prose-direct-link-tracer.h"

#include "ns3/nr-sl-pc5-signalling-header.h"
#include "ns3/nr-sl-ue-prose.h"

#include <ns3/boolean.h>
#include <ns3/logical-link-helper.h>
#include <ns3/lte-ue-rrc.h>
#include <ns3/nr-ue-net-device.h>

namespace ns3
{

using namespace std;

int
NetSimulyzerProseDirectLogicalLinkTracer::EnableVisualizations(
    Ptr<netsimulyzer::Orchestrator> orchestrator,
    Ptr<NetDevice> initiatingNetDev,
    Ptr<NetDevice> targetNetDev)
{
    if (m_isSetUp)
    {
        return 0;
    }

    bool isParamNull =
        ((orchestrator == nullptr) || (initiatingNetDev == nullptr) || (targetNetDev == nullptr));
    NS_ABORT_MSG_IF(isParamNull, "One of the passed in parameters was null");

    m_initiatingNetDev = initiatingNetDev;
    m_targetNetDev = targetNetDev;
    m_orchestrator = orchestrator;

    LinkTraces();
    m_isSetUp = true;
    return 1;
}

void
NetSimulyzerProseDirectLogicalLinkTracer::LinkTraces()
{
    auto initProse = m_initiatingNetDev->GetObject<NrSlUeProse>();
    initProse->TraceConnectWithoutContext(
        "PC5SignallingPacketTrace",
        MakeCallback(&NetSimulyzerProseDirectLogicalLinkTracer::CreateLink, this));

    auto targetProse = m_initiatingNetDev->GetObject<NrSlUeProse>();
    targetProse->TraceConnectWithoutContext(
        "PC5SignallingPacketTrace",
        MakeCallback(&NetSimulyzerProseDirectLogicalLinkTracer::CreateLink, this));
}

uint32_t
NetSimulyzerProseDirectLogicalLinkTracer::GetSrcL2Id(Ptr<NetDevice> netDev)
{
    Ptr<NrSlUeProse> nodeProse = netDev->GetObject<NrSlUeProse>();
    auto ueRrc = nodeProse->GetObject<NrUeNetDevice>()->GetRrc();
    uint32_t srcL2Id = ueRrc->GetSourceL2Id();

    return srcL2Id;
}

void
NetSimulyzerProseDirectLogicalLinkTracer::CreateLink(uint32_t srcL2Id,
                                                     uint32_t dstL2Id,
                                                     bool isTx,
                                                     Ptr<Packet> p)
{
    if (!isTx)
    {
        NrSlPc5SignallingMessageType pc5smt;
        p->PeekHeader(pc5smt);

        // Inititating node received accept message - show link (creating if needed)
        if ((dstL2Id == GetSrcL2Id(m_initiatingNetDev)) &&
            (pc5smt.GetMessageName().find("DIRECT LINK ESTABLISHMENT ACCEPT") != std::string::npos))
        {
            netsimulyzer::LogicalLinkHelper linkHelper(m_orchestrator);

            if (m_link == nullptr)
            {
                m_link = linkHelper.Link(m_initiatingNetDev->GetNode(), m_targetNetDev->GetNode());
            }
            else
            {
                m_link->SetAttribute("Active", BooleanValue(true));
            }
        }
        else if (pc5smt.GetMessageName().find("DIRECT LINK RELEASE ACCEPT") != std::string::npos)
        {
            m_link->SetAttribute("Active", BooleanValue(false));
        }
    }
}

} /* namespace ns3 */

#endif /* HAS_NETSIMULYZRER */
