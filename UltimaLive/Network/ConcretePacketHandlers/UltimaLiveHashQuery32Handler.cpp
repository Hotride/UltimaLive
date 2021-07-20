/* @file
 *
 * Copyright(c) 2016 UltimaLive
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "UltimaLiveHashQuery32Handler.h"
#include "..\NetworkManager.h"

 /**
 * @brief UltimaLiveHashQuery32Handler constructor
 */
UltimaLiveHashQuery32Handler::UltimaLiveHashQuery32Handler(NetworkManager* pManager)
  : BasePacketHandler(pManager)
{
  //do nothing
}

/**
 * @brief Fires onBlockQueryRequest event
 *
 * @param pPacketData Pointer to packet data
 *
 * @return False (Does not pass this packet on to the client)
 */
bool UltimaLiveHashQuery32Handler::handlePacket(uint8_t* pPacketData)
{
  uint32_t blockNum = ntohl(*reinterpret_cast<uint32_t*>(&pPacketData[3]));
  uint8_t mapNum = pPacketData[14];
  m_pManager->onBlockQueryRequest(blockNum, mapNum);

  return false;
}
