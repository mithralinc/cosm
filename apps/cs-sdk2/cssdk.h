/*
  Copyright 1995-2019 Mithral Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "cosm/cosm.h"

#define CSSDK_VER_MAJOR     1  /* main compatability version */
#define CSSDK_VER_MINOR     1  /* engine version */
#define CSSDK_VER_MINOR_MIN 1  /* minimum version for the dylib */
#define CSSDK_VER_MINOR_MAX 1  /* maximum available version of dylib */

#define EXPIRE_YEAR   2013
#define EXPIRE_MONTH  12
#define EXPIRE_DAY    31

#define CSSDK_SIGNATURE  0x4353444b  /* CSDK */

/* Server's IP. It's common for the server to have a 192.168.* or 10.* IP
address but known from the outside world as a fixed or dynamic IP address.
On Amazon EC2 this will be the Elastic IP for the instance and the SELF
address will be the 10.* address of the machine you get from ifconfig. */
#define CSSDK_SERVER      "127.0.0.1" /* seen from outside */
#define CSSDK_SERVER_SELF "127.0.0.1" /* as the server knows itself */

#define CSSDK_PORT_SERVER   4432
#define CSSDK_PORT_CLIENT   4433

#define CSSDK_STATS     "http://127.0.0.1/cssdk/"

/* Location to download engine.dylib when enabled */
#define CSSDK_SRV_HOST  "http://127.0.0.1/"
#define CSSDK_SRV_BASE  "/cssdk/"

typedef enum cssdk_engine_result
{
  CSSDK_ENGINE_SHUTDOWN = -13,
  CSSDK_ENGINE_EXPIRED_CLIENT = -37,
  CSSDK_ENGINE_EXPIRED_ENGINE = -52,
  CSSDK_ENGINE_NETWORK = -81
} cssdk_engine_result;

#define CSSDK_DB_REST      5            /* 1-5 */
#define CSSDK_PING_RATE    ( 300 - 5 )  /* 5-300 */
#define CSSDK_CHECKIN_RATE ( 300 + 60 )

#define CSSDK_BUFFER    ( 64 * 1024 )

typedef enum packet_type
{
  TYPE_VERSION  = 'V',

  TYPE_RESTART  = 'R',
  TYPE_CHECKIN  = 'C',
  TYPE_SHUTDOWN = 'S',

  TYPE_PING     = 'I',
  TYPE_PONG     = 'O'
} packet_type;

PACKED_STRUCT_BEGIN
typedef struct packet
{
  u32 signature;
  u16 ver_major;
  u16 ver_minor_min;
  u16 ver_minor_max;
  u8  os;
  u8  cpu; /* 8 */
  u32 mem;
  u32 id;  /* 16 */

  u32 address;
  u32 seq;
  cosmtime timestamp;
  u8 type; /* 45 */
} packet;
PACKED_STRUCT_END

ascii * Now( void );
void PacketEncode( packet * pkt );
void PacketDecode( packet * pkt );
s32 UpdateFile( const ascii * path, const ascii * filename,
  u32 * updated_version_minor, const u32 current_version_major,
  const u32 current_version_minor, cosm_LOG * log );
