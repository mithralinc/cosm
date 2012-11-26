<?php

include "include/session.php";

$base = "http://localhost/cssdk/";
$script = "hostadd.php";

if ( !$session->logged_in )
{
  header( "Location:" . $base );
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
  printf( "<p>New Machine ID %08X added.</p>", $new_host );
  echo "<p>This Machine ID is now ready for you to setup your sah-monitor.exe. "
     . "Be sure to use the proper Machine ID.</p>";
}


echo '<p>[<a href="' . $base . 'index.php">Back</a>]</p>';

PrintTail();
?>