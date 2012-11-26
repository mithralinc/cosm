<?php

include "include/session.php";

$script = "hostadd.php";

if ( !$session->logged_in )
{
  header( "Location:" . $cssdk2_base_url );
  die();
}

PrintHead( "Add Host" );

$new_host = $database->addNewHost( $session->userinfo['user_id'] );
if ( $new_host == 0 )
{
  echo "<p>Unable to create more Machine ID's for this user.</p>";
}
else
{
  $hex = strtoupper( dechex( $new_host ) );
  if ( strlen( $hex ) == 16 )
  {
    $hex = substr( $hex, 8, 8 );
  }
  echo "<p>New Machine ID " . $hex . " added.</p>";
  echo "<p>This Machine ID is now ready for you to setup your sah-monitor.exe. "
     . "Be sure to use the proper Machine ID.</p>";
}


echo '<p>[<a href="' . $cssdk2_base_url . 'index.php">Back</a>]</p>';

PrintTail();
?>
