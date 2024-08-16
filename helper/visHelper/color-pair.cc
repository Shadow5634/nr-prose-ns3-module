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

#include "ns3/color-pair.h"

namespace ns3
{

ColorPair::ColorPair()
{
    m_idxColor = 0;
    m_colors = {netsimulyzer::BLUE_VALUE,
                netsimulyzer::DARK_BLUE_VALUE,

                netsimulyzer::ORANGE_VALUE,
                netsimulyzer::DARK_ORANGE_VALUE,

                netsimulyzer::YELLOW_VALUE,
                netsimulyzer::DARK_YELLOW_VALUE,

                netsimulyzer::RED_VALUE,
                netsimulyzer::DARK_RED_VALUE,

                netsimulyzer::GREEN_VALUE,
                netsimulyzer::DARK_GREEN_VALUE};
}

netsimulyzer::Color3Value
ColorPair::GetNextColor()
{
    netsimulyzer::Color3Value returnValue = m_colors[m_idxColor];
    m_idxColor++;
    if (m_idxColor > m_colors.size() - 1)
    {
        m_idxColor = 0;
    }
    return returnValue;
}

void
ColorPair::SetColorVector(std::vector<netsimulyzer::Color3Value> colors)
{
    NS_ABORT_MSG_IF(colors.empty(), "Provided colors vector is empty");

    m_colors.clear();

    for (uint32_t i = 0; i < colors.size(); i++)
    {
        m_colors.push_back(colors[i]);
    }

    m_idxColor = 0;
}

std::vector<netsimulyzer::Color3Value>
ColorPair::GetColorVector()
{
    return m_colors;
}

} // namespace ns3

#endif
