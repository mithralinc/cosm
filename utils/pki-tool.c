/*
  This software is available as part of this Package under and subject to the
  terms of the Mithral License A or Mithral License B where "Creator" and
  "Package" are defined as:

  Creator(s): Mithral Communications & Design, Inc.
  Package: Cosm Libraries - Utility Layer

  A copy of the license(s) is enclosed with this Package and by using this
  Package you agree to the license terms. The Package is Copyright (C)
  1995-2010 by Creator. All rights reserved. Further information about the
  Package and pricing information can be found at the Creator's web site:
  http://www.mithral.com/
*/

#include "cosm.h"

void Usage( void );
s32 CommandKeygen( s32 argc, utf8 * argv[] );
s32 CommandSign( s32 argc, utf8 * argv[] );
s32 CommandVerify( s32 argc, utf8 * argv[] );

typedef struct
{
  const utf8 * command;
  const s32 min_params;
  const s32 (*function)( s32, utf8 ** );
  /* const utf8 * help_string; */
} COMMAND;

const COMMAND commands[] =
{
  { "keygen", 2, CommandKeygen },
  { "sign", 2, CommandSign },
  { "verify", 2, CommandVerify },
  { NULL, 0, NULL }
};

void Usage( void )
{
  CosmPrint(
    "pki-tool handles key generation, sign, and verify operations.\n\n"
    "Usage:\n\n"
    "pki-tool keygen <base-filename> <bits> <hex-ID> <alias> <days-expire>\n"
    "  Generate a bits-bit key with id and the up-to 15 character alias.\n"
    "  put the pub and private keys into filename.pub and filename.pri.\n"
    "pki-tool sign <sigfile> <file> <private-keyfile>\n"
    "  Sign the file with the private key and put the sig into sigfile.sig\n"
    "pki-tool verify <sigfile> <file> <public-keyfile>\n"
    "  Verify that the sigfile is a signature on the file with key.\n\n" );
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
      if ( ( CosmStrCmp( argv[2], i->command, 16 ) == 0 )
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