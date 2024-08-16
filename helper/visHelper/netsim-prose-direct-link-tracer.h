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

#ifndef NETSIM_PROSE_DIRECT_LINK_TRACER_H
#define NETSIM_PROSE_DIRECT_LINK_TRACER_H

#ifdef HAS_NETSIMULYZER

#include <ns3/logical-link.h>
#include <ns3/net-device.h>
#include <ns3/orchestrator.h>
#include <ns3/packet.h>
#include <ns3/ptr.h>

namespace ns3
{

/*
 * Structure that creates static logical links for direct links between nodes through
 * EstablishRealDirectLink/EstablishL3U2NRelayConnection i.e. uses the Pc5SignallingPacketTrace of a
 * NrSlUeProse Logical link is established when target node receives the accept and disabled on
 * released messages IMP: Relay should be the target node for remote, relay cases
 */
class NetSimulyzerProseDirectLogicalLinkTracer
{
  public:
    NetSimulyzerProseDirectLogicalLinkTracer() = default;

    /*
     * \return 1 - setup successful
     * 0 - recall setup after a succesfull
     * -1 - setup unsuccessful (orchestrator = nullptr | netDevPtr = nullptr)
     */
    int EnableVisualizations(Ptr<netsimulyzer::Orchestrator> orchestrator,
                             Ptr<NetDevice> initiatingNetDev,
                             Ptr<NetDevice> targetNetDev);

  private:
    /*
     * Links to the PC5Signalling trace for the nodes
     * to track status of direct link establishment
     */
    void LinkTraces();

    /*
     * Gets the Src2Id for the given netDevice
     * Uses the LteUeRrc for the same
     * TODO: Update when lte and nr models are separated fully
     */
    uint32_t GetSrcL2Id(Ptr<NetDevice> netDev);

    /*
     * Trace sink for PC5Signalling trace to toggle logical links
     */
    void CreateLink(uint32_t srcL2Id, uint32_t dstL2Id, bool isTx, Ptr<Packet> p);

    /*
     * Boolean flag denoting succesful setup
     */
    bool m_isSetUp = false;
    /*
     * Orchestrator used to create logical links
     */
    Ptr<netsimulyzer::Orchestrator> m_orchestrator;
    /*
     * Ptr to NetDevice of initiating UE of direct link
     */
    Ptr<NetDevice> m_initiatingNetDev;
    /*
     * Ptr to NetDevice of target UE of direct link
     */
    Ptr<NetDevice> m_targetNetDev;
    /*
     * Ptr to the logical link between the nodes
     * Used to toggle Active param
     */
    Ptr<netsimulyzer::LogicalLink> m_link;
};

} /* namespace ns3 */

#endif /* HAS_NETSIMULYZER */

#endif /* NETSIM_PROSE_DIRECT_LINK_TRACER_H */
