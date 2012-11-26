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
  echo "<p>Unable to create more Host ID's for this user.</p>";
}
else
{
  $hex = strtoupper( dechex( $new_host ) );
  if ( strlen( $hex ) == 16 )
  {
    $hex = substr( $hex, 8, 8 );
  }
  echo "<p>New Host ID " . $hex . " added.</p>";
  echo "<p>This Host ID is now ready for use with a client. "
     . "Be sure to use the proper Host ID.</p>";
}


echo '<p>[<a href="' . $cssdk2_base_url . 'index.php">Back</a>]</p>';

PrintTail();
?>
