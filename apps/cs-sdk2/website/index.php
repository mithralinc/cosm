<?php

include("include/session.php");

PrintHead( "Host Management" );

if($session->logged_in)
{
  $user_id = sprintf( "%08X", $session->userinfo['user_id'] );
  echo "<h3>Host Management</h3>";
  echo "<p>Welcome <b>$session->username</b></p>";

  echo "<h3>Your Hosts</h3>";
  $result = $database->query( "SELECT * FROM hosts WHERE user_id = '". $session->userinfo['user_id'] . "';" );
  echo "Count: " . pg_numrows( $result ) . " of 1<br>\n";

  echo "<table border=1 cellpadding=3 cellspacing=0>\n";
  echo "<tr><th>Host ID<th>OS<th>CPU<th>Last IP<th>Last Pong<th>Online</tr>\n";
  while ( $row = pg_fetch_assoc( $result ) )
  {
    echo "<tr>";
    $hex = strtoupper( dechex( $row['host_id'] ) );
    if ( strlen( $hex ) == 16 )
    {
      $hex = substr( $hex, 8, 8 );
    }
    echo '<td><a href="lookup.php?host_id=' . $hex . '">' .
      $hex . '</a></td>';
    if ( $row['online'] == -1 )
    {
      echo '<td colspan=5>You can now setup your client with this ID</td>';
    }
    else
    {
      echo "<td>" . $os_types[$row['os_type']] . "</td>";
      echo "<td>" . $cpu_types[$row['cpu_type']] . "</td>";
      echo "<td>" . $row['addr_inet'] . "</td>";
      echo "<td>" . PrintTime( $row['last_pong'] ) . "</td>";
      echo "<td>" . $row['online'] . "</td>";
    }
    echo "</tr>";
  }
  echo "</table>\n";
  echo "<a href=\"hostadd.php\">Add new Host ID</a>";


  echo "<br><br>[<a href=\"useredit.php\">Edit Account</a>] &nbsp;&nbsp;";
  echo "[<a href=\"process.php\">Logout</a>]";
}
else
{
  global $cssdk2_title_prefix;
  echo "<h3>" . $cssdk2_title_prefix . "Login</h3>";
  if($form->num_errors > 0){
    echo "<font size=\"2\" color=\"#ff0000\">" . $form->num_errors .
      " error(s) found</font>";
}
?>
<form action="process.php" method="POST">
<table align="left" border="0" cellspacing="0" cellpadding="3">
<tr><td>Username:</td><td><input type="text" name="user" maxlength="30" value="<?php echo $form->value("user"); ?>"></td><td><?php echo $form->error("user"); ?></td></tr>
<tr><td>Password:</td><td><input type="password" name="pass" maxlength="30" value="<?php echo $form->value("pass"); ?>"></td><td><?php echo $form->error("pass"); ?></td></tr>
<tr><td colspan="2" align="left"><input type="checkbox" name="remember" <?php if($form->value("remember") != ""){ echo "checked"; } ?>>
<font size="2">Remember me next time &nbsp;&nbsp;&nbsp;&nbsp;
<input type="hidden" name="sublogin" value="1">
<input type="submit" value="Login"></td></tr>
<tr><td colspan="2" align="left"><br>Not registered? <a href="register.php">Sign-Up!</a></td></tr>
</table>
</form>

<?php
}

PrintTail();

?>
