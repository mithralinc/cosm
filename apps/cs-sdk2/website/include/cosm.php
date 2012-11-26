<?php

# SETUP: You need to edit the following 6 variables for your setup.
$cssdk2_base_url = "http://demo.mithral.com/cosm/cs-sdk2/";
$cssdk2_title_prefix = "Cosm CS-SDK2 Demo - ";
$cssdk2_db_host = "localhost";
$cssdk2_db_name = "cssdk2";
$cssdk2_db_user = "cssdk2_user";
$cssdk2_db_pass = "cssdk2_pass";

$cpu_types = array( "UNKNOWN","x86", "PPC", "MIPS", "MIPS64", "Alpha",
  "undef", "68K", "Sparc", "Sparc64", "undef", "IA64",  "ARM", "Cell",
  "undef", "PPC64", "x64" );

$os_types = array( "UNKNOWN", "Win32", "Win64", "OSX", "Linux", "undef",
  "NetBSD", "FreeBSD", "OpenBSD", "undef", "undef", "Irix", "Irix64",
  "SunOS", "Solaris", "SonyPS3", "QNX", "Tru64" );

function cosmtime()
{
  return ( time() - 0x386D4380 );
}

function rand31()
{
  // 31 bits of random, 1...2^31-1
  return ( mt_rand( 0, 127 ) << 24 | mt_rand( 0, 255 ) << 16
    | mt_rand( 0, 255 ) << 8 | mt_rand( 1, 255 ) );
}

function PrintHead( $title )
{
  global $cssdk2_title_prefix;
echo '<!DOCTYPE html>
<html>
<head>
  <title>' . $cssdk2_title_prefix . $title . '</title>
  <meta charset="utf-8">
</head>
<body>
';
}

function PrintTail()
{
  echo '</body></html>';
}

function PrintAddr( $addr )
{
  $a = ( $addr >> 24 ) & 0xFF;
  $b = ( $addr >> 16 ) & 0xFF;
  $c = ( $addr >> 8 ) & 0xFF;
  $d = $addr & 0xFF;
  return "$a.$b.$c.$d";
}

function PrintTime( $time )
{
  if ( $time > 0 )
  {
    $unix = $time + 0x386D4380;
    $delta = time() - $unix;
    date_default_timezone_set('UTC');
    return date( 'Y-m-d H:i:s', $unix ) . " UTC (-" . $delta . "sec)";
  }
  else
  {
    return "NULL";
  }
}

# takes in the 9-line format, returns ( cosmid, key[512] ) strings
function parse_key( $field )
{
  
  $lines = preg_split( '/\r\n/', $field );
  
  # line one... "Cosmid:id"
  if ( sscanf( $lines[0], "CosmID:%s", $id ) != 1 )
    return NULL;
  
  # line 2-9, "8-chars * 8 "
  $key = "";
  for ( $i = 1 ; $i < 10 ; $i++ )
  {
    $key = $key . preg_replace( '/ /', '', $lines[$i] );
  }
  if ( strlen( $key ) != 512 )
    return NULL;
  
  return array( $id, $key );
}

?>
