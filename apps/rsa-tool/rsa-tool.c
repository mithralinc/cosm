/*
  Copyright 1995-2012 Mithral Communications & Design Inc.

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

void Notice( void );
void Usage( void );
s32 CommandKeygen( s32 argc, utf8 * argv[] );
s32 CommandSign( s32 argc, utf8 * argv[] );
s32 CommandVerify( s32 argc, utf8 * argv[] );

typedef struct
{
  const utf8 * command;
  const s32 min_params;
  s32 (*function)( s32, utf8 ** );
  /* const utf8 * help_string; */
} COMMAND;

const COMMAND commands[] =
{
  { "keygen", 6, CommandKeygen },
  { "sign", 4, CommandSign },
  { "verify", 4, CommandVerify },
  { NULL, 0, NULL }
};

void Notice( void )
{
  CosmPrint(
    "Cosm(R) RSA Tool.\n"
    "Copyright Mithral Communications & Design, Inc. 1995-2012.\n" );
}

void Usage( void )
{
  CosmPrint(
    "\nrsa-tool usage: rsa-tool <command> [arguments]\n"
    "keygen <base-filename> <bits> <hex-ID> <alias> <days-expire>\n"
    "  Generate a bits-bit key with id and the up-to 15 character alias.\n"
    "  put the pub and private keys into filename.pub and filename.pri.\n"
    "sign <sigfile> <file> <private-keyfile>\n"
    "  Sign the file with the private key and put the sig into sigfile.sig\n"
    "verify <sigfile> <file> <public-keyfile>\n"
    "  Verify that the sigfile is a signature on the file with key.\n\n" );
}

void my_progress( s32 type, s32 count, void *param )
{
  switch ( type )
  {
    case 0:
      CosmPrint( "." );
      break;
    case 1:
      CosmPrint( "+" );
      break;
    case 2:
      CosmPrint( "\n" );
      break;
    case 3:
      CosmPrint( "[testing keypair..." );
      break;
    case 4:
      CosmPrint( "]\n" );
      break;
    default:
      break;
  }
}

s32 GetHashPhrase( cosm_HASH * hash )
{
  cosm_TRANSFORM transform;
  ascii input[1024];
  u32 len;

  CosmMemSet( hash, sizeof( cosm_HASH ), 0 );
  
  CosmPrint( "Enter your secret passphase (24+ chars):\n" );
  len = 0;
  while ( len < 24 )
  {
    CosmMemSet( input, sizeof( input ), 0 );
    len = CosmInput( input, sizeof( input ), COSM_IO_NOECHO );
    if ( ( len == -1 ) )
    {
      CosmMemSet( &input, sizeof( input ), 0 );
      return COSM_FAIL;
    }
    if ( len < 24 )
    {
      CosmPrint( "passPHRASE not passWORD, try again\n" );
    }
  }

  /* hash passphrase */
  CosmMemSet( &transform, sizeof( transform ), 0 );
  if ( ( CosmTransformInit( &transform, COSM_HASH_SHA256, NULL, hash )
    != COSM_PASS )
    || ( CosmTransform( &transform, input, CosmStrBytes( input ) )
    != COSM_PASS )
    || ( CosmTransformEnd( &transform ) != COSM_PASS ) )
  {
    CosmMemSet( &transform, sizeof( transform ), 0 );
    CosmMemSet( &input, sizeof( input ), 0 );
    CosmMemSet( hash, sizeof( cosm_HASH ), 0 );
    return COSM_FAIL;
  }

  CosmMemSet( &transform, sizeof( transform ), 0 );
  CosmMemSet( &input, sizeof( input ), 0 );
  len = 0;
  
  return COSM_PASS;
}

s32 HashFile( cosm_HASH * hash, const ascii * filename )
{
  cosm_FILE file;
  cosm_TRANSFORM transform;
  u8 buffer[4096];
  u64 len;

  CosmMemSet( hash, sizeof( cosm_HASH ), 0 );

  CosmMemSet( &transform, sizeof( transform ), 0 );
  if ( CosmTransformInit( &transform, COSM_HASH_SHA256, NULL, hash )
    != COSM_PASS )
  {
    return COSM_FAIL;
  }

  CosmMemSet( &file, sizeof( file ), 0 );
  if ( CosmFileOpen( &file, filename, COSM_FILE_MODE_READ,
    COSM_FILE_LOCK_READ ) != COSM_PASS )
  {
    return COSM_FAIL;
  }

  while ( ( CosmFileRead( buffer, &len, &file, sizeof( buffer ) )
    == COSM_PASS )
    && ( len > 0 ) )
  {
    if ( CosmTransform( &transform, buffer, len ) != COSM_PASS )
    {
      CosmMemSet( &transform, sizeof( transform ), 0 );
      CosmMemSet( buffer, sizeof( buffer ), 0 );
      CosmFileClose( &file );
      return COSM_FAIL;
    }
  }
  CosmMemSet( buffer, sizeof( buffer ), 0 );
  CosmFileClose( &file );
  
  if ( CosmTransformEnd( &transform ) != COSM_PASS )
  {
    CosmMemSet( &transform, sizeof( transform ), 0 );
    CosmMemSet( hash, sizeof( cosm_HASH ), 0 );
    return COSM_FAIL;
  }

  return COSM_PASS;
}

s32 CommandKeygen( s32 argc, utf8 * argv[] )
{
  /* keygen <base-filename> <bits> <id> <alias> <days-expire> */
  cosm_RSA_KEY pub, pri;
  cosm_HASH pass_hash;
  cosm_PRNG rnd;
  cosm_FILE file;
  cosm_FILENAME filename;
  ascii input[1024];
  ascii alias[16];
  cosmtime create, expire, now;
  u8 * rnd_bits;
  u8 * save_buf;
  u64 id, len, write_len;
  u32 bits, i;
  s32 days;
  s32 error;

  /* bits >= 512 and a power of 2 */
  if ( ( CosmU32Str( &bits, NULL, argv[3], 10 ) != COSM_PASS )
    || ( bits < 512 )
    || ( ( bits & ( bits - 1 ) ) != 0 ) ) /* power of 2 test */
  {
    CosmPrint( "bits must be a power of 2, and 512 or higher.\n" );
    return COSM_FAIL;
  }

  if ( CosmU64Str( &id, NULL, argv[4], 16 ) != COSM_PASS )
  {
    CosmPrint( "Cannot parse id.\n" );
    return COSM_FAIL;
  }

  if ( CosmStrCopy( alias, argv[5], 16 ) != COSM_PASS )
  {
    CosmPrint( "Alias too long, limit 15 characters.\n" );
    return COSM_FAIL;
  }

  /* set create and expire */
  CosmSystemClock( &create );
  /* 1 day */
  _COSM_SET128( expire, 0000000000015180, 0000000000000000 );
  if ( CosmS32Str( &days, NULL, argv[6], 10 ) != COSM_PASS )
  {
    CosmPrint( "Cannot parse days until expiry.\n" );
    return COSM_FAIL;
  }
  expire = CosmS128Mul( expire, CosmS128S32( days ) );
  expire = CosmS128Add( create, expire );

  /* get user data and carefully construct rnd_bits */
  if ( ( rnd_bits = CosmMemAlloc( ( u64 ) bits / 8 ) ) == NULL )
  {
    CosmPrint( "Memory problems.\n" );
    return COSM_FAIL;
  }

  /*
     seed, then for each char typed, feed it and the time to the
     PRNG and get 8 bits out.
   */
  CosmSystemClock( &now );
  CosmMemSet( &rnd, sizeof( rnd ), 0 );
  CosmPRNG( &rnd, rnd_bits, bits / 8, &now, sizeof( now ) );

  CosmPrint( "Type RANDOM things - letters, numbers, dont forget to use\n" );
  CosmPrint( "the shift key sometimes, etc - until we say stop:\n" );
  for ( i = 0; i < bits / 8; i++ )
  {
    if ( ( len = CosmInputRaw( input, 1, COSM_IO_NOECHO ) ) != 1 )
    {
      CosmPrint( "Input Error.\n" );
      return COSM_FAIL;
    }
    CosmPrint( "." );
    CosmSystemClock( &now );
    CosmPRNG( &rnd, NULL, 0, &now, sizeof( now ) );
    CosmPRNG( &rnd, &rnd_bits[i], 1, input, 1 );
  }
  CosmPrint( "\nSTOP\n" );

  CosmPrint( "Hit enter key to start key generation\n" );
  CosmInput( input, sizeof( input ), COSM_IO_NOECHO );

  /* generate the keys */
  CosmMemSet( &pub, sizeof( pub ), 0 );
  CosmMemSet( &pri, sizeof( pri ), 0 );
  if ( ( error = CosmRSAKeyGen( &pub, &pri, bits, rnd_bits, id, alias,
    create, expire, my_progress, NULL ) ) != COSM_PASS )
  {
    CosmPrint( "Key generation failed.\n" );
    return COSM_FAIL;
  }

  CosmPrint( "\nHit enter key\n" );
  CosmInput( input, 1024, COSM_IO_NOECHO );

  /* alloc save buffer */
  if ( ( save_buf = CosmMemAlloc( bits * 2 ) ) == NULL )
  {
    CosmPrint( "Memory problems.\n" );
    return COSM_FAIL;
  }

  /* save the public key */
  if ( CosmRSAKeySave( save_buf, &len, &pub, bits * 2, NULL, NULL )
    != COSM_PASS )
  {
    CosmPrint( "Saving error.\n" );
    return COSM_FAIL;
  }

  CosmPrint( "Hexdump of public key? [y/N] " );
  CosmInput( input, 4, COSM_IO_ECHO );
  if ( ( input[0] == 'y' ) || ( input[0] == 'Y' ) )
  {
    for ( i = 0; i < ( u32 ) len; i++ )
    {
      if ( ( i % 10 ) == 0 )
      {
        CosmPrint( "\n  0x%02X,", save_buf[i] );
      }
      else
      {
        CosmPrint( " 0x%02X,", save_buf[i] );
      }
    }
  }
  CosmPrint( "\n" );

  if ( CosmStrCopy( filename, argv[2], sizeof( filename ) - 4 ) != COSM_PASS )
  {
    CosmPrint( "filename too long.\n" );
    return COSM_FAIL;
  }
  CosmStrAppend( filename, ".pub", sizeof( filename ) );

  CosmMemSet( &file, sizeof( file ), 0 );
  if ( CosmFileOpen( &file, filename, COSM_FILE_MODE_CREATE
    | COSM_FILE_MODE_WRITE | COSM_FILE_MODE_TRUNCATE, COSM_FILE_LOCK_WRITE )
    != COSM_PASS )
  {
    CosmPrint( "Error opening key file.\n" );
    return COSM_FAIL;
  }
  if ( ( CosmFileWrite( &file, &write_len, save_buf, len ) != COSM_PASS )
    || ( ( write_len != len ) ) )
  {
    CosmPrint( "File error.\n" );
    return COSM_FAIL;
  }
  CosmFileClose( &file );

  /* get the passphase */
  if ( GetHashPhrase( &pass_hash ) != COSM_PASS )
  {
    CosmPrint( "Passphrase enry error.\n" );
    return COSM_FAIL;  
  }

  /* save the private key file */
  CosmSystemClock( &now );
  CosmPRNG( &rnd, rnd_bits, 16, &now, sizeof( now ) );

  if ( CosmRSAKeySave( save_buf, &len, &pri, bits * 2,
    rnd_bits, &pass_hash ) != COSM_PASS )
  {
    CosmPrint( "Saving error.\n" );
    CosmMemSet( &pass_hash, sizeof( pass_hash ), 0 );
    return COSM_FAIL;
  }
  CosmMemSet( &pass_hash, sizeof( pass_hash ), 0 );

  if ( CosmStrCopy( filename, argv[2], sizeof( filename ) - 4 ) != COSM_PASS )
  {
    CosmPrint( "filename too long.\n" );
    return COSM_FAIL;
  }
  CosmStrAppend( filename, ".pri", sizeof( filename ) );

  CosmMemSet( &file, sizeof( file ), 0 );
  if ( CosmFileOpen( &file, filename, COSM_FILE_MODE_CREATE
    | COSM_FILE_MODE_WRITE | COSM_FILE_MODE_TRUNCATE, COSM_FILE_LOCK_WRITE )
    != COSM_PASS )
  {
    CosmPrint( "Error opening key file.\n" );
    return COSM_FAIL;
  }
  if ( ( CosmFileWrite( &file, &write_len, save_buf, len ) != COSM_PASS )
    || ( write_len != len ) )
  {
    CosmPrint( "File error.\n" );
    return COSM_FAIL;
  }
  CosmFileClose( &file );

  return COSM_PASS;
}

s32 CommandSign( s32 argc, utf8 * argv[] )
{
  /* sign <sigfile> <file> <private-keyfile> */
  cosm_RSA_KEY pri;
  cosm_RSA_SIG sig;
  cosm_HASH pass_hash, data_hash;
  cosm_FILE file;
  cosm_FILE_INFO info;
  cosm_FILENAME filename;
  cosmtime now;
  u64 len, write_len;
  u8 * buf;

  /* hash the file */
  if ( HashFile( &data_hash, argv[3] ) != COSM_PASS )
  {
    CosmPrint( "Error hashing file.\n" );
    return COSM_FAIL;
  }

  /* get the passphase */
  if ( GetHashPhrase( &pass_hash ) != COSM_PASS )
  {
    CosmPrint( "Passphrase enry error.\n" );
    return COSM_FAIL;  
  }

  /* load key file */
  if ( CosmFileInfo( &info, argv[4] ) != COSM_PASS )
  {
    CosmPrint( "Invalid keyfile.\n" );
    return COSM_FAIL;
  }
  if ( ( buf = CosmMemAlloc( info.length ) ) == NULL )
  {
    CosmPrint( "Memory problems.\n" );
    return COSM_FAIL;
  }

  CosmMemSet( &file, sizeof( file ), 0 );
  if ( CosmFileOpen( &file, argv[4], COSM_FILE_MODE_READ,
    COSM_FILE_LOCK_READ ) != COSM_PASS )
  {
    CosmPrint( "Error opening keyfile.\n" );
    return COSM_FAIL;
  }
  if ( ( CosmFileRead( buf, &len, &file, info.length ) != COSM_PASS )
    || ( ( len != info.length ) ) )
  {
    CosmPrint( "Error reading keyfile.\n" );
    return COSM_FAIL;
  }
  CosmFileClose( &file );

  CosmMemSet( &pri, sizeof( pri ), 0 );
  if ( CosmRSAKeyLoad( &pri, &len, buf, info.length, &pass_hash )
    != COSM_PASS )
  {
    CosmPrint( "Invalid Keyfile or passphrase.\n" );
    return COSM_FAIL;
  }

  CosmMemSet( &pass_hash, sizeof( pass_hash ), 0 );

  /* now sign */
  CosmSystemClock( &now );
  CosmMemSet( &sig, sizeof( sig ), 0 );
  if ( CosmRSAEncode( &sig, &data_hash, now, COSM_RSA_SIG_SIGN,
    COSM_RSA_SHARED_YES, &pri ) != COSM_PASS )
  {
    CosmPrint( "Can't sign.\n" );
    return COSM_FAIL;
  }
  CosmRSAKeyFree( &pri );

  /* buf is always large enough to save the sig into */
  if ( CosmRSASigSave( buf, &len, &sig, info.length ) != COSM_PASS )
  {
    CosmPrint( "Cant save signature.\n" );
    return COSM_FAIL;
  }
  CosmRSASigFree( &sig );

  /* append .sig */
  if ( CosmStrCopy( filename, argv[2], sizeof( filename ) - 4 ) != COSM_PASS )
  {
    CosmPrint( "filename too long.\n" );
    return COSM_FAIL;
  }
  CosmStrAppend( filename, ".sig", sizeof( filename ) );

  CosmMemSet( &file, sizeof( file ), 0 );
  if ( CosmFileOpen( &file, filename, COSM_FILE_MODE_CREATE
    | COSM_FILE_MODE_WRITE | COSM_FILE_MODE_TRUNCATE, COSM_FILE_LOCK_WRITE )
    != COSM_PASS )
  {
    CosmPrint( "Error opening sig file.\n" );
    return COSM_FAIL;
  }
  if ( ( CosmFileWrite( &file, &write_len, buf, len ) != COSM_PASS )
    || ( ( write_len != len ) ) )
  {
    CosmPrint( "Sig writing error.\n" );
    return COSM_FAIL;
  }
  CosmFileClose( &file );

  CosmMemFree( buf );

  return COSM_PASS;
}

s32 CommandVerify( s32 argc, utf8 * argv[] )
{
  /* verify <file> <sigfile> <public-keyfile> */
  cosm_RSA_KEY pub;
  cosm_RSA_SIG sig;
  cosm_HASH data_hash, sig_hash;
  cosm_FILE file;
  cosm_FILE_INFO info;
  cosm_TIME_UNITS myunits;
  cosmtime sig_time, tmp_time;
  u8 * buf;
  u64 len;
  u8 type, share;
  static const ascii * months[12] = COSM_TIME_MONTHS;
  static const ascii * days[7] = COSM_TIME_DAYS;
  
  /* hash the file */
  if ( HashFile( &data_hash, argv[3] ) != COSM_PASS )
  {
    CosmPrint( "Error hashing file.\n" );
    return COSM_FAIL;
  }

  /* load key */
  if ( CosmFileInfo( &info, argv[4] ) != COSM_PASS )
  {
    CosmPrint( "Invalid keyfile.\n" );
    return COSM_FAIL;
  }

  if ( ( buf = CosmMemAlloc( info.length ) ) == NULL )
  {
    CosmPrint( "Memory problems.\n" );
    return COSM_FAIL;
  }

  CosmMemSet( &file, sizeof( file ), 0 );
  if ( CosmFileOpen( &file, argv[4], COSM_FILE_MODE_READ,
      COSM_FILE_LOCK_READ ) != COSM_PASS )
  {
    CosmPrint( "Error opening keyfile.\n" );
    CosmMemFree( buf );
    return COSM_FAIL;
  }
  if ( ( CosmFileRead( buf, &len, &file, info.length ) != COSM_PASS )
    || ( ( len != info.length ) ) )
  {
    CosmPrint( "Error reading keyfile.\n" );
    CosmMemFree( buf );
    CosmFileClose( &file );
    return COSM_FAIL;
  }
  CosmFileClose( &file );

  CosmMemSet( &pub, sizeof( pub ), 0 );
  if ( CosmRSAKeyLoad( &pub, &len, buf, info.length, NULL ) != COSM_PASS )
  {
    CosmPrint( "Invalid Keyfile.\n" );
    CosmMemFree( buf );
    return COSM_FAIL;
  }
  CosmMemFree( buf );

  /* load sig */
  if ( CosmFileInfo( &info, argv[2] ) != COSM_PASS )
  {
    CosmPrint( "Invalid sigfile.\n" );
    return COSM_FAIL;
  }

  if ( ( buf = CosmMemAlloc( info.length ) ) == NULL )
  {
    CosmPrint( "Memory problems.\n" );
    return COSM_FAIL;
  }

  CosmMemSet( &file, sizeof( file ), 0 );
  if ( CosmFileOpen( &file, argv[2], COSM_FILE_MODE_READ,
      COSM_FILE_LOCK_READ ) != COSM_PASS )
  {
    CosmPrint( "Error opening sigfile.\n" );
    CosmMemFree( buf );
    return COSM_FAIL;
  }

  if ( ( CosmFileRead( buf, &len, &file, info.length ) != COSM_PASS )
    || ( ( len != info.length ) ) )
  {
    CosmPrint( "Error reading sigfile.\n" );
    CosmMemFree( buf );
    CosmFileClose( &file );
    return COSM_FAIL;
  }
  CosmFileClose( &file );

  CosmMemSet( &sig, sizeof( sig ), 0 );
  if ( CosmRSASigLoad( &sig, &len, buf, info.length ) != COSM_PASS )
  {
    CosmPrint( "Invalid signature file.\n" );
    CosmMemFree( buf );
    return COSM_FAIL;
  }
  CosmMemFree( buf );

  if ( CosmRSADecode( &sig_hash, &sig_time, &type, &share, &sig, &pub )
    != COSM_PASS )
  {
    CosmPrint( "Can't decode signature.\n" );
    return COSM_FAIL;
  }

  /* print out match and types */
  if ( !CosmHashEq( &data_hash, &sig_hash )
    || ( type != COSM_RSA_SIG_SIGN ) || ( share != COSM_RSA_SHARED_YES ) )
  {
    CosmPrint( "Signature is _NOT_ valid for file.\n" );
    return COSM_FAIL;
  }

  CosmPrint( "Signature is VALID\n" );
  CosmPrint( "Signed by ID = %Y, %u bits, ", pub.id, pub.bits );
  tmp_time.hi = pub.create;
  tmp_time.lo = ( u64 ) 0;
  CosmTimeUnitsGregorian( &myunits, tmp_time );
  CosmPrint( "Created: %.9s %.9s %u, %j %02u:%02u:%02u UTC\n",
    days[myunits.wday], months[myunits.month], myunits.day + 1,
    myunits.year, myunits.hour, myunits.min, myunits.sec );

  CosmTimeUnitsGregorian( &myunits, sig_time );
  CosmPrint( "Signed: %.9s %.9s %u, %j %02u:%02u:%02u UTC\n",
    days[myunits.wday], months[myunits.month], myunits.day + 1,
    myunits.year, myunits.hour, myunits.min, myunits.sec );

  CosmRSAKeyFree( &pub );
  CosmRSASigFree( &sig );

  return COSM_PASS;
}

int main( int argc, char *argv[] )
{
  const COMMAND * i;

  Notice();

  /* scan the commands to see if which one was used */
  if ( argc > 1 )
  {
    for ( i = &commands[0] ; i->command != NULL ; i++ )
    {
      if ( ( CosmStrCmp( argv[1], i->command, 16 ) == 0 )
        && ( argc > i->min_params ) )
      {
        return (*i->function)( (s32) argc, (utf8 **) argv );
      }
    }
  }

  /* otherwise print usage */
  Usage();

  return 0;
}
