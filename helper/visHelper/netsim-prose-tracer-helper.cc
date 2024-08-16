#ifdef HAS_NETSIMULYZER

#include "netsim-prose-tracer-helper.h"

#include <ns3/lte-ue-rrc.h>
#include <ns3/nr-ue-net-device.h>
#include "ns3/nr-sl-ue-prose.h"

using namespace ns3;
using namespace std;

uint32_t 
GetSrcL2Id(Ptr<NetDevice> netDev)
{
  Ptr<NrSlUeProse> nodeProse = netDev->GetObject<NrSlUeProse>();
  auto ueRrc = nodeProse->GetObject<NrUeNetDevice>()->GetRrc();
  uint32_t srcL2Id = ueRrc->GetSourceL2Id();

  return srcL2Id;
}

#endif
