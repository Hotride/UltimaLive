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

#include "Atlas.h"
#include "..\UltimaLive.h"
#include "..\Network\NetworkManager.h"
#include "..\FileSystem\BaseFileManager.h"

#pragma region Self Registration
SelfRegisteringClass <Atlas> Atlas::m_registration;

/**
 * @brief Class Registration Configuration function that creates the Atlas global and registers it
 *
 */
void Atlas::Configure()
{
  Logger::g_pLogger->LogPrint("Atlas configure\n");
  UltimaLive::g_pUltimaLive->Register("Atlas", new Atlas());
}

/**
 * @brief Initializes Atlas class. Gets pointers to the client global pointer and subscribes to packets.
 *
 * @return true on success
 */
bool Atlas::Initialize()
{
  bool success = true;

  Logger::g_pLogger->LogPrint("Initializing Atlas\n");
  Atlas* pInstance = static_cast<Atlas*>(UltimaLive::g_pUltimaLive->Lookup("Atlas"));

  if (pInstance != NULL)
  {
    BaseFileManager* pFileManager = static_cast<BaseFileManager*>(UltimaLive::g_pUltimaLive->Lookup("FileManager"));
    if (pFileManager == NULL)
    {
      Logger::g_pLogger->LogPrintError("Atlas: Failed to acquire File Manager Instance\n");
      success = false;
    }

    Client* pClient = static_cast<Client*>(UltimaLive::g_pUltimaLive->Lookup("Client"));
    if (pClient == NULL)
    {
      Logger::g_pLogger->LogPrintError("Atlas: Failed to acquire Client class Instance\n");
      success = false;
    }

    NetworkManager* pNetworkManager = static_cast<NetworkManager*>(UltimaLive::g_pUltimaLive->Lookup("NetworkManager"));
    if (pNetworkManager == NULL)
    {
      Logger::g_pLogger->LogPrintError("Atlas: Failed to acquire Network Manager Instance\n");
      success = false;
    }
    else
    {
      pNetworkManager->subscribeToOnBeforeMapChange(std::bind(&Atlas::onBeforeMapChange, pInstance, std::placeholders::_1));
      pNetworkManager->subscribeToOnMapChange(std::bind(&Atlas::onMapChange, pInstance, std::placeholders::_1));
      pNetworkManager->subscribeToRefreshClient(std::bind(&Atlas::onRefreshClientView, pInstance));
      pNetworkManager->subscribeToBlocksViewRange(std::bind(&Atlas::onBlocksViewRange, pInstance, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
      pNetworkManager->subscribeToBlockQueryRequest(std::bind(&Atlas::onHashQuery, pInstance, std::placeholders::_1, std::placeholders::_2));
      pNetworkManager->subscribeToBlockQuery32Request(std::bind(&Atlas::onHashQuery32, pInstance, std::placeholders::_1, std::placeholders::_2));
      pNetworkManager->subscribeToStaticsUpdate(std::bind(&Atlas::onUpdateStatics, pInstance, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
      pNetworkManager->subscribeToMapDefinitionUpdate(std::bind(&Atlas::onUpdateMapDefinitions, pInstance, std::placeholders::_1));
      pNetworkManager->subscribeToLandUpdate(std::bind(&Atlas::onUpdateLand, pInstance, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
      pNetworkManager->subscribeToUltimaLiveLoginComplete(std::bind(&Atlas::onShardIdentifierUpdate, pInstance, std::placeholders::_1));
      pNetworkManager->subscribeToLogout(std::bind(&Atlas::onLogout, pInstance));
    }

    pInstance->m_pFileManager = pFileManager;
    pInstance->m_pNetworkManager = pNetworkManager;
    pInstance->m_pClient = pClient;
  }
  else
  {
      Logger::g_pLogger->LogPrintError("Atlas: Failed to acquire Atlas Instance\n");
    success = false;
  }

  return success;
}
#pragma endregion

/**
 * @brief Atlast Constructor
 */
Atlas::Atlas()
  : m_pNetworkManager(NULL),
  m_pFileManager(NULL),
  m_pClient(NULL),
  m_mapDefinitions(),
  m_currentMap(0),
  m_shardIdentifier(),
  m_firstMapLoad(true)
{
  //do nothing
}

/**
 * @brief setter for Shard Identifier
 *
 * @param shardIdentifier 
 */
void Atlas::onShardIdentifierUpdate(std::string shardIdentifier)
{
  m_shardIdentifier = shardIdentifier;
}

/**
 * @brief Calls file manager logout handler
 */
void Atlas::onLogout()
{
  m_pFileManager->onLogout();
}

/**
 * @brief Loads a map based on map number
 *
 * @param map Map Number
 */
void Atlas::LoadMap(uint8_t map)
{
  Logger::g_pLogger->LogPrint("ON BEFORE LOAD MAP: %i\n", map);

  if (m_firstMapLoad)
  {
    m_firstMapLoad = false;
    m_pFileManager->InitializeShardMaps(m_shardIdentifier, m_mapDefinitions);
  }

  if (m_mapDefinitions.find((uint32_t)map) != m_mapDefinitions.end())
  {
    MapTileDefinition definition;

    definition.mapWidthInTiles = m_mapDefinitions[map].mapWidthInTiles;
    definition.mapHeightInTiles = m_mapDefinitions[map].mapHeightInTiles;
    definition.mapWrapWidthInTiles = m_mapDefinitions[map].mapWrapWidthInTiles;
    definition.mapWrapHeightInTiles = m_mapDefinitions[map].mapWrapHeightInTiles;

    m_pClient->SetMapDimensions(definition);
    m_pFileManager->LoadMap(map);
    m_currentMap = map;
  }
#ifdef DEBUG
  else 
  {
    Logger::g_pLogger->LogPrint("MAP DEFINITION NOT FOUND: %i\n", m_mapDefinitions.size());

    for (std::map<uint32_t, MapDefinition>::iterator itr = m_mapDefinitions.begin(); itr != m_mapDefinitions.end(); itr++)
    {
      Logger::g_pLogger->LogPrint("Map Definition: %i/%i\n", itr->first, itr->second.mapNumber);
    }
  }
#endif
}

/**
 * @brief Updates a static block by calling the filemanager and then refreshes the client
 *
 * @param mapNumber Map Number
 * @param blockNumber Statics Block Number
 * @param pData Pointer to the new memory block containing the statics data
 * @param length Length of the new memory block
 */
void Atlas::onUpdateStatics(uint8_t mapNumber, uint32_t blockNumber, uint8_t* pData, uint32_t length)
{
  Logger::g_pLogger->LogPrint("Block: %i Map Number: %i Length: %i\n", blockNumber, mapNumber, length);
  m_pFileManager->writeStaticsBlock(mapNumber, blockNumber, pData, length);
  m_pClient->refreshClientStatics(blockNumber);
}

/**
* @brief @ Updates a land block on a given map
*
* @param mapNumber Map Number
* @param blockNumber Land Block Number
* @param pLandData Pointer to the new land data which is of fixed size
*/
void Atlas::onUpdateLand(uint8_t mapNumber, uint32_t blockNumber, uint8_t* pLandData)
{
  m_pFileManager->updateLandBlock(mapNumber, blockNumber, pLandData);
  m_pClient->refreshClientLand();
}

/**
 * @brief Responds to a server blocks view range
 *
 * @param minBlockX min block x
 * @param maxBlockX max block x
 * @param minBlockY min block y
 * @param maxBlockY max block y
 */
void Atlas::onBlocksViewRange(int32_t minBlockX, int32_t maxBlockX, int32_t minBlockY, int32_t maxBlockY)
{
    m_MinBlockX = minBlockX;
    m_MaxBlockX = maxBlockX;
    m_MinBlockY = minBlockY;
    m_MaxBlockY = maxBlockY;

    m_BlocksWidth = m_MaxBlockX - m_MinBlockX + 1;
    m_BlocksHeight = m_MaxBlockY - m_MinBlockY + 1;

    std::vector<uint8_t> response(23, 0);
    uint8_t* pResponse = &response[0];

    pResponse[0] = 0x3F;                                               //byte 000              -  cmd
    *reinterpret_cast<uint16_t*>(pResponse + 1) = 71;                  //byte 001 through 002  -  packet size
    *reinterpret_cast<uint32_t*>(pResponse + 3) = htonl(0);     //byte 003 through 006  -  central block number for the query (block that player is standing in)
    *reinterpret_cast<uint32_t*>(pResponse + 7) = htonl(8);            //byte 007 through 010  -  number of statics in the packet (8 for a query response)
    *reinterpret_cast<uint16_t*>(pResponse + 11) = htons(0);           //byte 011 through 012  -  UltimaLive sequence number
    pResponse[13] = 0x04;                                              //byte 013              -  UltimaLive command (0x04 is a blocks view range)
    pResponse[14] = 0;                                         //byte 014              -  UltimaLive mapnumber
    *reinterpret_cast<uint16_t*>(pResponse + 15) = htons((uint16_t)minBlockX);          //byte 015 through 016 - min block x
    *reinterpret_cast<uint16_t*>(pResponse + 17) = htons((uint16_t)maxBlockX);          //byte 017 through 018 - max block x
    *reinterpret_cast<uint16_t*>(pResponse + 19) = htons((uint16_t)minBlockY);          //byte 019 through 020 - min block y
    *reinterpret_cast<uint16_t*>(pResponse + 21) = htons((uint16_t)maxBlockY);          //byte 021 through 022 - max block y

    m_pNetworkManager->sendPacketToServer(pResponse);
}

/**
 * @brief Responds to a server hash query for a particular map block (land and statics)
 *
 * @param blockNumber Map Block Number
 * @param mapNumber Map Number
 */
void Atlas::onHashQuery(uint32_t blockNumber, uint8_t mapNumber)
{
  Logger::g_pLogger->LogPrint("Atlas: Got Hash Query\n");
  std::vector<uint16_t> crcs = GetGroupOfBlockCrcs(mapNumber, blockNumber);
  std::vector<uint8_t> response(15 + crcs.size() * sizeof(uint16_t), 0);
  uint8_t *pResponse = &response[0];

  pResponse[0] = 0x3F;                                               //byte 000              -  cmd
  *reinterpret_cast<uint16_t*>(pResponse + 1) = 71;                  //byte 001 through 002  -  packet size
  *reinterpret_cast<uint32_t*>(pResponse + 3) = htonl(blockNumber);  //byte 003 through 006  -  central block number for the query (block that player is standing in)
  *reinterpret_cast<uint32_t*>(pResponse + 7) = htonl(8);            //byte 007 through 010  -  number of statics in the packet (8 for a query response)
  *reinterpret_cast<uint16_t*>(pResponse + 11) = htons(0);           //byte 011 through 012  -  UltimaLive sequence number
  pResponse[13] = 0xFF;                                              //byte 013              -  UltimaLive command (0xFF is a block Query Response)
  pResponse[14] = mapNumber;                                         //byte 014              -  UltimaLive mapnumber
  //byte 015 through 64   -  25 block CRCs
  for (size_t i = 0; i < crcs.size(); i++)
  {
    *reinterpret_cast<uint16_t*>(pResponse + 15 + (i * sizeof(uint16_t))) = htons(crcs[i]);
  }

  //pResponse[65] = 0xFF;                                           //byte 065              -  padding
  //pResponse[66] = 0xFF;                                           //byte 066              -  padding
  //pResponse[67] = 0xFF;                                           //byte 067              -  padding
  //pResponse[68] = 0xFF;                                           //byte 068              -  padding
  //pResponse[69] = 0xFF;                                           //byte 069              -  padding
  //pResponse[70] = 0xFF;                                           //byte 070              -  padding

  m_pNetworkManager->sendPacketToServer(pResponse);
}

/**
 * @brief Responds to a server hash query32 for a particular map block (land and statics)
 *
 * @param blockNumber Map Block Number
 * @param mapNumber Map Number
 */
void Atlas::onHashQuery32(uint32_t blockNumber, uint8_t mapNumber)
{
    Logger::g_pLogger->LogPrint("Atlas: Got Hash Query\n");
    std::vector<uint32_t> crcs = GetGroupOfBlockCrcs32(mapNumber, blockNumber);
    std::vector<uint8_t> response(15 + crcs.size() * sizeof(uint32_t), 0);
    uint8_t* pResponse = &response[0];

    pResponse[0] = 0x3F;                                               //byte 000              -  cmd
    *reinterpret_cast<uint16_t*>(pResponse + 1) = 71;                  //byte 001 through 002  -  packet size
    *reinterpret_cast<uint32_t*>(pResponse + 3) = htonl(blockNumber);  //byte 003 through 006  -  central block number for the query (block that player is standing in)
    *reinterpret_cast<uint32_t*>(pResponse + 7) = htonl(8);            //byte 007 through 010  -  number of statics in the packet (8 for a query response)
    *reinterpret_cast<uint16_t*>(pResponse + 11) = htons(0);           //byte 011 through 012  -  UltimaLive sequence number
    pResponse[13] = 0xFF;                                              //byte 013              -  UltimaLive command (0xFF is a block Query Response)
    pResponse[14] = mapNumber;                                         //byte 014              -  UltimaLive mapnumber
    //byte 015 through 64   -  25 block CRCs
    for (size_t i = 0; i < crcs.size(); i++)
    {
        *reinterpret_cast<uint32_t*>(pResponse + 15 + (i * sizeof(uint32_t))) = htonl(crcs[i]);
    }

    //pResponse[65] = 0xFF;                                           //byte 065              -  padding
    //pResponse[66] = 0xFF;                                           //byte 066              -  padding
    //pResponse[67] = 0xFF;                                           //byte 067              -  padding
    //pResponse[68] = 0xFF;                                           //byte 068              -  padding
    //pResponse[69] = 0xFF;                                           //byte 069              -  padding
    //pResponse[70] = 0xFF;                                           //byte 070              -  padding

    m_pNetworkManager->sendPacketToServer(pResponse);
}

/**
* @brief Updates map definitions as received from the server
*
* @param definitions New map definitions
*/
void Atlas::onUpdateMapDefinitions(std::vector<MapDefinition> definitions)
{
  m_mapDefinitions.clear();

  for (std::vector<MapDefinition>::iterator itr = definitions.begin(); itr != definitions.end(); itr++)
  {
    m_mapDefinitions[itr->mapNumber] = *itr;
    Logger::g_pLogger->LogPrint("Registering Map #%i, dim=%ix%i, wrap=%ix%i\n", itr->mapNumber, itr->mapWidthInTiles, itr->mapHeightInTiles, itr->mapWrapWidthInTiles, itr->mapWrapHeightInTiles);
  }
}

/**
* @brief Refreshes the client's view to match the current state of the map
*/
void Atlas::onRefreshClientView()
{
  Logger::g_pLogger->LogPrint("Atlas: Refreshing Client View\n");
  uint8_t aPacketData[11];
  aPacketData[0] = (byte)0x21; //char move reject command
  aPacketData[1] = (byte)0xFF;  //seq num
  *reinterpret_cast<uint16_t*>(&aPacketData[2]) = 0;
  *reinterpret_cast<uint16_t*>(&aPacketData[4]) = 0;
  aPacketData[6] = 0;
  aPacketData[7] = 0;
  aPacketData[8] = (byte)0x22;
  aPacketData[9] = (byte)0x00;
  aPacketData[10] = (byte)0x00;
  m_pNetworkManager->sendPacketToClient(aPacketData);
  uint8_t aPacketData2[3];
  aPacketData2[0] = 0x22;
  aPacketData2[1] = 0xFF; //sequence number
  aPacketData2[2] = 0x00;
  m_pNetworkManager->sendPacketToClient(aPacketData2);
}

/**
 * @brief Changes the client's map to map1 so that the client will load a map0 and refresh the view
 *        Map0 and Map1 point to the same place internally. This is a way of making sure the client actually performs the load.
 */
void Atlas::onBeforeMapChange(uint8_t&)
{
  //send a packet to tell the client to change to map 1
  uint8_t packet[6];
  packet[0] = 0xBF;
  packet[1] = 0x06;
  packet[2] = 0x00;
  packet[3] = 0x00;
  packet[4] = 0x08;
  packet[5] = 0x01;
  m_pNetworkManager->sendPacketToClient(packet);
}

/**
 * @brief Loads the specified map
 *
 * @param rMap Map number to load
 */
void Atlas::onMapChange(uint8_t& rMap)
{
  //modify map change packet to point to map 0
  LoadMap(rMap);
  rMap = 0x00;
}

/**
 * @brief Getter for the current map number
 *
 * @return Current map number
 */
uint8_t Atlas::getCurrentMap()
{
  return m_currentMap;
}

/**
 * @brief Calculates a CRC using the combined land and statics data
 *
 * @param pBlockData Pointer to the memory containing land data
 * @param pStaticsData Pointer to the memory containing statics data
 * @param staticsLength Number of bytes in the statics data
 *
 * @ return 16 bit CRC
 */
uint16_t Atlas::fletcher16(uint8_t* pBlockData, uint8_t* pStaticsData, uint32_t staticsLength)
{
  uint16_t sum1 = 0;
  uint16_t sum2 = 0;

  if (pBlockData != NULL)
  {
    for (int index = 0; index < 192; ++index)
    {
      sum1 = (uint16_t)((sum1 + pBlockData[index]) % 255);
      sum2 = (uint16_t)((sum2 + sum1) % 255);
    }
  }

  if (pStaticsData != NULL)
  {
    for (uint32_t index = 0; index < staticsLength; ++index)
    {
      sum1 = (uint16_t)((sum1 + pStaticsData[index]) % 255);
      sum2 = (uint16_t)((sum2 + sum1) % 255);
    }
  }

  return (uint16_t)((sum2 << 8) | sum1);
}

/**
 * @brief Calculates a CRC32 using the combined land and statics data
 *
 * @param pBlockData Pointer to the memory containing land data
 * @param pStaticsData Pointer to the memory containing statics data
 * @param staticsLength Number of bytes in the statics data
 *
 * @ return 32 bit CRC
 */
uint32_t Atlas::calculateCrc32(uint8_t* pBlockData, uint8_t* pStaticsData, uint32_t staticsLength)
{
    const uint32_t crc32Table[256] =
    {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
        0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
        0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
        0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
        0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
        0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
        0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
        0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
        0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
        0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
        0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
        0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
        0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
        0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
        0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
        0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
        0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
        0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
        0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
        0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
        0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
        0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
        0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
        0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
        0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
        0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
        0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
        0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
        0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
        0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
        0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
        0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
        0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
        0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
        0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
        0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
        0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
        0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
        0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
        0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
        0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
        0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
        0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
        0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
        0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
        0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
        0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
        0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
        0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
        0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
        0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
        0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
        0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
        0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
        0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
        0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
        0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
        0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
        0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
        0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
        0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
        0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
        0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
        0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
    };

    uint32_t currentCrc = 0xFFFFFFFF;

    if (pBlockData != NULL)
    {
        for (int index = 0; index < 192; ++index)
        {
            currentCrc = (currentCrc >> 8) ^ crc32Table[(currentCrc & 0xFF) ^ pBlockData[index]];
        }
    }

    if (pStaticsData != NULL)
    {
        for (uint32_t index = 0; index < staticsLength; ++index)
        {
            currentCrc = (currentCrc >> 8) ^ crc32Table[(currentCrc & 0xFF) ^ pStaticsData[index]];
        }
    }

    return (uint32_t)(currentCrc & 0xFFFFFFFF);
}

int32_t Atlas::BLOCK_POSITION_OFFSETS[5] = { -2, -1, 0, 1, 2 };

/**
 * @brief Gets CRCs for the 25 blocks surrounding the current players position
 *
 * @param mapNumber Map Number
 * @param blockNumber Number of the block containing the current player position
 * 
 * @return Array of 25 block numbers
 */
std::vector<uint16_t> Atlas::GetGroupOfBlockCrcs(uint32_t mapNumber, uint32_t blockNumber)
{
    std::vector<uint16_t> pCrcs(m_BlocksWidth * m_BlocksHeight, 0);
  if (m_mapDefinitions.find(mapNumber) != m_mapDefinitions.end())
  {
    MapDefinition def = m_mapDefinitions[mapNumber];
    int32_t blockX = ((int32_t)blockNumber /  (def.mapHeightInTiles >> 3));
    int32_t blockY = ((int32_t)blockNumber % (def.mapHeightInTiles >> 3));

    int32_t wrapWidthInBlocks = (def.mapWrapWidthInTiles >> 3);
    int32_t wrapHeightInBlocks = (def.mapWrapHeightInTiles >> 3);
    int32_t mapWidthInBlocks = (def.mapWidthInTiles >> 3);
    int32_t mapHeightInBlocks = (def.mapHeightInTiles >> 3);

    if (blockX < 0 || blockX >= mapWidthInBlocks|| blockY < 0 || blockY >= mapHeightInBlocks)
    {
    }
    else
    {
        int32_t boundX = m_MinBlockX - 1;
        int32_t absOffsetX = std::abs(m_MinBlockX);
        int32_t absOffsetY = std::abs(m_MinBlockY);

      for (int x = m_MinBlockX; x <= m_MaxBlockX; x++)
      {
        int xBlockItr = -1;
        if (blockX < wrapWidthInBlocks)
        {
          xBlockItr = (blockX + x) % wrapWidthInBlocks;
          if (xBlockItr < 0 && xBlockItr > boundX)
          {
            xBlockItr += wrapWidthInBlocks;
          }
        }
        else
        {
          xBlockItr = (blockX + x) % mapWidthInBlocks;
          if (xBlockItr < 0 && xBlockItr > boundX)
          {
            xBlockItr += mapWidthInBlocks;
          }
        }
        for (int y = m_MinBlockY; y <= m_MaxBlockY; y++)
        {
          int yBlockItr = 0;
          if (blockY < wrapHeightInBlocks)
          {
            yBlockItr = (blockY + y) % wrapHeightInBlocks;
            if (yBlockItr < 0)
            {
              yBlockItr += wrapHeightInBlocks;
            }
          }
          else
          {
            yBlockItr = (blockY + y) % mapHeightInBlocks;
            if (yBlockItr < 0)
            {
              yBlockItr += mapHeightInBlocks;
            }
          }

          int32_t currentBlock = (xBlockItr * mapHeightInBlocks) + yBlockItr;

          if (currentBlock >= 0 && currentBlock <= (mapHeightInBlocks * mapWidthInBlocks))
          {
            pCrcs[((x + absOffsetX) * m_BlocksHeight) + (y + absOffsetY)] = getBlockCrc(mapNumber, currentBlock);
          }
        }
      }
    }
  }

  return pCrcs;
}

/**
 * @brief Gets CRCs32 for the 25 blocks surrounding the current players position
 *
 * @param mapNumber Map Number
 * @param blockNumber Number of the block containing the current player position
 *
 * @return Array of 25 block numbers
 */
std::vector<uint32_t> Atlas::GetGroupOfBlockCrcs32(uint32_t mapNumber, uint32_t blockNumber)
{
    std::vector<uint32_t> pCrcs(m_BlocksWidth * m_BlocksHeight, 0);
    if (m_mapDefinitions.find(mapNumber) != m_mapDefinitions.end())
    {
        MapDefinition def = m_mapDefinitions[mapNumber];
        int32_t blockX = ((int32_t)blockNumber / (def.mapHeightInTiles >> 3));
        int32_t blockY = ((int32_t)blockNumber % (def.mapHeightInTiles >> 3));

        int32_t wrapWidthInBlocks = (def.mapWrapWidthInTiles >> 3);
        int32_t wrapHeightInBlocks = (def.mapWrapHeightInTiles >> 3);
        int32_t mapWidthInBlocks = (def.mapWidthInTiles >> 3);
        int32_t mapHeightInBlocks = (def.mapHeightInTiles >> 3);

        if (blockX < 0 || blockX >= mapWidthInBlocks || blockY < 0 || blockY >= mapHeightInBlocks)
        {
        }
        else
        {
            int32_t boundX = m_MinBlockX - 1;
            int32_t absOffsetX = std::abs(m_MinBlockX);
            int32_t absOffsetY = std::abs(m_MinBlockY);

            for (int x = m_MinBlockX; x <= m_MaxBlockX; x++)
            {
                int xBlockItr = -1;
                if (blockX < wrapWidthInBlocks)
                {
                    xBlockItr = (blockX + x) % wrapWidthInBlocks;
                    if (xBlockItr < 0 && xBlockItr > boundX)
                    {
                        xBlockItr += wrapWidthInBlocks;
                    }
                }
                else
                {
                    xBlockItr = (blockX + x) % mapWidthInBlocks;
                    if (xBlockItr < 0 && xBlockItr > boundX)
                    {
                        xBlockItr += mapWidthInBlocks;
                    }
                }
                for (int y = m_MinBlockY; y <= m_MaxBlockY; y++)
                {
                    int yBlockItr = 0;
                    if (blockY < wrapHeightInBlocks)
                    {
                        yBlockItr = (blockY + y) % wrapHeightInBlocks;
                        if (yBlockItr < 0)
                        {
                            yBlockItr += wrapHeightInBlocks;
                        }
                    }
                    else
                    {
                        yBlockItr = (blockY + y) % mapHeightInBlocks;
                        if (yBlockItr < 0)
                        {
                            yBlockItr += mapHeightInBlocks;
                        }
                    }

                    int32_t currentBlock = (xBlockItr * mapHeightInBlocks) + yBlockItr;

                    if (currentBlock >= 0 && currentBlock <= (mapHeightInBlocks * mapWidthInBlocks))
                    {
                        pCrcs[((x + absOffsetX) * m_BlocksHeight) + (y + absOffsetY)] = getBlockCrc32(mapNumber, currentBlock);
                    }
                }
            }
        }
    }

    return pCrcs;
}

/**
 * @brief Looks up the memory blocks for a specified block number and map number, returns the CRC of the block
 *
 * @param mapNumber Map Number
 * @param blockNumber Block Number
 */
uint16_t Atlas::getBlockCrc(uint32_t mapNumber, uint32_t blockNumber)
{
  uint16_t crc = 0; 
  if (m_mapDefinitions.find(mapNumber) != m_mapDefinitions.end())
  {
    uint8_t* pBlockData = m_pFileManager->readLandBlock(static_cast<uint8_t>(mapNumber), blockNumber);
    uint32_t staticsLength = 0;

    uint8_t* pStaticsData = m_pFileManager->readStaticsBlock(mapNumber, blockNumber, staticsLength);

    if (pBlockData != NULL)
    {
      crc = fletcher16(pBlockData, pStaticsData, staticsLength);
      delete pBlockData;
    }

    if (pStaticsData != NULL)
    {
      delete pStaticsData;
    }
  }

  return crc;
}

/**
 * @brief Looks up the memory blocks for a specified block number and map number, returns the CRC32 of the block
 *
 * @param mapNumber Map Number
 * @param blockNumber Block Number
 */
uint32_t Atlas::getBlockCrc32(uint32_t mapNumber, uint32_t blockNumber)
{
    uint32_t crc = 0;
    if (m_mapDefinitions.find(mapNumber) != m_mapDefinitions.end())
    {
        uint8_t* pBlockData = m_pFileManager->readLandBlock(static_cast<uint8_t>(mapNumber), blockNumber);
        uint32_t staticsLength = 0;

        uint8_t* pStaticsData = m_pFileManager->readStaticsBlock(mapNumber, blockNumber, staticsLength);

        if (pBlockData != NULL)
        {
            crc = calculateCrc32(pBlockData, pStaticsData, staticsLength);
            delete pBlockData;
        }

        if (pStaticsData != NULL)
        {
            delete pStaticsData;
        }
    }

    return crc;
}
