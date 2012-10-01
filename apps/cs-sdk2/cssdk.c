/*
  This software is available as part of this Package under and subject to the
  terms of the Mithral License A or Mithral License B where "Creator" and
  "Package" are defined as:

  Creator(s): Mithral Communications & Design, Inc.
  Package: Cosm CS-SDK2 - Client Server Software Development Kit

  A copy of the license(s) is enclosed with this Package and by using this
  Package you agree to the license terms. The Package is Copyright (C)
  2009-2012 by Creator. All rights reserved. Further information about the
  Package can be found at the Creator's web site:
  http://www.mithral.com/
*/

#include "cssdk.h"

/* 2048 bit keys */
#define SIG_BYTES ( 34 + 256 )
static u8 dylib_signing_key[] = 
{
  0x00
};

ascii * Now( void )
{
  static ascii str[128];
  cosmtime mytime;
  cosm_TIME_UNITS myunits;

  CosmSystemClock( &mytime );
  CosmTimeUnitsGregorian( &myunits, mytime );
  
  CosmPrintStr( str, 128, "%04j-%02u-%02u %02u:%02u:%02u ",
    myunits.year, myunits.month + 1, myunits.day + 1,
    myunits.hour, myunits.min, myunits.sec );
  
  return str;
}

void PacketEncode( packet * pkt )
{
  CosmU32Save( &pkt->signature, &pkt->signature );
  CosmU16Save( &pkt->ver_major, &pkt->ver_major );
  CosmU16Save( &pkt->ver_minor_min, &pkt->ver_minor_min );
  CosmU16Save( &pkt->ver_minor_max, &pkt->ver_minor_max );
  /* u8 os */
  /* u8 cpu */
  CosmU32Save( &pkt->mem, &pkt->mem );
  CosmU32Save( &pkt->id, &pkt->id );
  CosmU32Save( &pkt->address, &pkt->address );
  CosmU32Save( &pkt->seq, &pkt->seq );
  CosmS128Save( &pkt->timestamp, &pkt->timestamp );
  /* u8 type */
}

void PacketDecode( packet * pkt )
{
  CosmU32Load( &pkt->signature, &pkt->signature );
  CosmU16Load( &pkt->ver_major, &pkt->ver_major );
  CosmU16Load( &pkt->ver_minor_min, &pkt->ver_minor_min );
  CosmU16Load( &pkt->ver_minor_max, &pkt->ver_minor_max );
  /* u8 os */
  /* u8 cpu */
  CosmU32Load( &pkt->mem, &pkt->mem );
  CosmU32Load( &pkt->id, &pkt->id );
  CosmU32Load( &pkt->address, &pkt->address );
  CosmU32Load( &pkt->seq, &pkt->seq );
  CosmS128Load( &pkt->timestamp, &pkt->timestamp );
  /* u8 type */
}

s32 UpdateFile( const ascii * path, const ascii * filename,
  u32 * updated_version_minor, const u32 current_version_major,
  const u32 current_version_minor, cosm_LOG * log )
{
  static const ascii * os_types[] = COSM_OS_TYPES;
  static const ascii * cpu_types[] = COSM_CPU_TYPES;
  cosm_FILENAME url;
  cosm_HTTP http;
  cosm_FILENAME fullpath, fullpath_ver;
  utf8 version_str[16];
  u32 status, bytes32, version;
  u64 bytes64;
  cosm_FILE file;
  u8 * buffer = NULL;
  cosm_FILE_INFO info;
  cosm_HASH data_hash, sig_hash;
  cosm_TRANSFORM transform;
  cosm_PKI_KEY pub;
  cosm_PKI_SIG sig;
  cosmtime sig_time;
  u8 type, share;
  
  /*
    Fetch the version from filename.ver
    Compare to existing version.
    Download the file if updated.
    Verify signature and write out file.
  */

  /* base/os/cpu/filename.ver */
  CosmPrintStr( url, sizeof( url ), "%.32s%.02X/%.8s/%.8s/%.64s.ver",
    CSSDK_SRV_BASE, current_version_major,
    os_types[( OS_TYPE > COSM_OS_TYPE_MAX ) ? 0 : OS_TYPE],
    cpu_types[( CPU_TYPE > COSM_CPU_TYPE_MAX ) ? 0 : CPU_TYPE],
    filename );

  /* get the version, base 16 */
  CosmMemSet( &http, sizeof( http ), 0 );
  if ( ( _COSM_HTTPOPEN( &http, CSSDK_SRV_HOST ) != COSM_PASS )
    || ( CosmHTTPGet( &http, &status, url, 5000 ) != COSM_PASS )
    || ( status != 200 )
    || ( CosmHTTPRecv( version_str, &bytes32, &http, 8, 5000 ) != COSM_PASS )
    || ( bytes32 == 0 )
    || ( CosmU32Str( &version, NULL, version_str, 16 ) != COSM_PASS ) )
  {
    CosmHTTPClose( &http );
    CosmLog( log, 0, COSM_LOG_ECHO,
      "%.32sUnable to open URL.\n", Now() );
    return COSM_FAIL;
  }

  /* clear HTTP state since we didn't loop */
  CosmHTTPRecv( version_str, &bytes32, &http, 8, 10 );
  
  /* names for our files */
  CosmPrintStr( fullpath, sizeof( fullpath ), "%.*s%.*s",
    sizeof( fullpath ), path, sizeof( fullpath ), filename );
  CosmPrintStr( fullpath_ver, sizeof( fullpath_ver ), "%.*s%.*s.%04X",
    sizeof( fullpath ), path, sizeof( fullpath ), filename, version );

  /* if the current_verison is out of date, download to fullpath_ver */
  if ( version > current_version_minor )
  {
    /* base/os/cpu/filename.XXXX */
    CosmPrintStr( url, sizeof( url ), "%.32s%.02X%.8s/%.8s/%.64s.%04X",
      CSSDK_SRV_BASE, current_version_major,
      os_types[( OS_TYPE > COSM_OS_TYPE_MAX ) ? 0 : OS_TYPE],
      cpu_types[( CPU_TYPE > COSM_CPU_TYPE_MAX ) ? 0 : CPU_TYPE],
      filename, version );

    /* do HTTP GET, and open output file, create buffer */
    CosmMemSet( &file, sizeof( file ), 0 );
    if ( ( CosmHTTPGet( &http, &status, url, 5000 ) != COSM_PASS )
      || ( status != 200 )
      || ( CosmFileOpen( &file, fullpath_ver,
      COSM_FILE_MODE_WRITE | COSM_FILE_MODE_CREATE | COSM_FILE_MODE_TRUNCATE,
      COSM_FILE_LOCK_WRITE ) != COSM_PASS )
      || ( ( ( buffer = CosmMemAlloc( CSSDK_BUFFER ) ) == NULL ) ) )
    {
      CosmHTTPClose( &http );
      CosmFileClose( &file );
      CosmLog( log, 0, COSM_LOG_ECHO,
        "%.32sUnable to open file, GET, or allocate memory.\n", Now() );
      return COSM_FAIL;
    }

    do
    {
      if ( ( CosmHTTPRecv( buffer, &bytes32, &http,
        CSSDK_BUFFER, CSSDK_BUFFER ) != COSM_PASS )
        || ( CosmFileWrite( &file, &bytes64, buffer, bytes32 ) != COSM_PASS )
        || ( bytes64 != bytes32 ) )
      {
        CosmHTTPClose( &http );
        CosmLog( log, 0, COSM_LOG_ECHO,
          "%.32sUnable to read URL or write file.\n", Now() );
        return COSM_FAIL;
      }
    } while ( bytes32 > 0 );

    CosmFileClose( &file );
    CosmMemFree( buffer );
  }

  CosmHTTPClose( &http );

  /* load up dylib key */
  CosmMemSet( &pub, sizeof( pub ), 0 );
  if ( CosmPKIKeyLoad( &pub, &bytes64, dylib_signing_key,
    sizeof( dylib_signing_key ), NULL ) != COSM_PASS )
  {
    CosmLog( log, 0, COSM_LOG_ECHO,
      "%.32sKey error.\n", Now() );
    return COSM_FAIL;
  }

  /* read in the file */
  /* !!! more error checking */
  CosmMemSet( &info, sizeof( info ), 0 );
  CosmFileInfo( &info, fullpath_ver );
  buffer = CosmMemAlloc( info.length );
  CosmMemSet( &file, sizeof( file ), 0 );
  CosmFileOpen( &file, fullpath_ver, COSM_FILE_MODE_READ,
    COSM_FILE_LOCK_READ );
  CosmFileRead( buffer, &bytes64, &file, info.length );
  CosmFileClose( &file );
  
  /* hash all but the signature, and check it */
  CosmMemSet( &data_hash, sizeof( data_hash ), 0 );
  CosmMemSet( &transform, sizeof( transform ), 0 );
  if ( ( CosmTransformInit( &transform, COSM_HASH_SHA256, NULL, &data_hash )
    != COSM_PASS )
    || ( CosmTransform( &transform, buffer, info.length - SIG_BYTES )
    != COSM_PASS )
    || ( CosmTransformEnd( &transform ) != COSM_PASS ) )
  {
    CosmMemSet( &transform, sizeof( transform ), 0 );
    CosmMemSet( &data_hash, sizeof( data_hash ), 0 );
    CosmMemFree( buffer );
    CosmLog( log, 0, COSM_LOG_ECHO,
      "%.32sHash check error.\n", Now() );
    return COSM_FAIL;
  }
  CosmMemSet( &transform, sizeof( transform ), 0 );

  /* read sig, compare with signed one */
  CosmMemSet( &sig, sizeof( sig ), 0 );
  if ( CosmPKISigLoad( &sig, &bytes64, &buffer[info.length - SIG_BYTES],
    SIG_BYTES ) != COSM_PASS )
  {
    CosmLog( log, 0, COSM_LOG_ECHO,
      "%.32sInvalid/Missing Signature.\n", Now() );
    CosmMemFree( buffer );
    return COSM_FAIL;
  }

  if ( CosmPKIDecode( &sig_hash, &sig_time, &type, &share, &sig, &pub )
    != COSM_PASS )
  {
    CosmLog( log, 0, COSM_LOG_ECHO,
      "%.32sSignature error.\n", Now() );
    CosmMemFree( buffer );
    return COSM_FAIL;
  }

  if ( !CosmHashEq( &data_hash, &sig_hash )
    || ( type != COSM_PKI_SIG_SIGN ) || ( share != COSM_PKI_SHARED_YES ) )
  {
    CosmLog( log, 0, COSM_LOG_ECHO,
      "%.32sSignature is not valid for file.\n", Now() );
    CosmMemFree( buffer );
    return COSM_FAIL;
  }
  
  /* write out the file */
  if ( ( CosmFileOpen( &file, fullpath,
    COSM_FILE_MODE_WRITE | COSM_FILE_MODE_CREATE | COSM_FILE_MODE_TRUNCATE,
    COSM_FILE_LOCK_WRITE ) != COSM_PASS )
    || ( CosmFileWrite( &file, &bytes64, buffer, info.length - SIG_BYTES )
    != COSM_PASS )
    || ( bytes64 != info.length - SIG_BYTES )
    || ( CosmFileClose( &file ) != COSM_PASS ) )
  {
    CosmMemFree( buffer );
    return COSM_FAIL;  
  }
  CosmMemFree( buffer );

  *updated_version_minor = version;
  return COSM_PASS;
}
