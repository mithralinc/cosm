<?php

include "include/session.php";

$script = "lookup.php";

if ( !$session->logged_in )
{
  header( "Location:" . $cssdk2_base_url );
  die();
}

if ( is_numeric( '0x' . $_REQUEST['host_id'] ) )
{
  $host_id = $_REQUEST['host_id'];
}
else
{
  $host_id = "";
}

PrintHead( "Host Lookup" );

if ( strlen( $host_id ) == 8 )
{
  $query = "SELECT * FROM hosts WHERE host_id = x'" . $host_id
    . "'::int AND user_id = " . $session->userinfo['user_id'] . ";";
  $result = $database->query($query);
  if ( pg_numrows( $result ) != 1 )
  {
    echo "<p>Host not found</p>";
    PrintTail();
    die;
  }
  $row = pg_fetch_assoc( $result );

  echo "<h3>Host ID: " . $host_id . "</h3>\n";

  echo "<p><b>Status: ";
  if ( $row['online'] == -1 )
  {
    echo '<font color="#CC0000">Never checked in.</font>';
  }
  else if ( $row['online'] == 0 )
  {
    echo '<font color="#CC0000">Offline</font>';
  }
  else if ( $row['online'] == 1 && $row['last_pong'] > 0 )
  {
    echo '<font color="#00CC00">Online</font>';
  }
  else
  {
    echo '<font color="#CC0000">Checked in, but never ponged - check your port fowarding and firewalls.</font>';
  }
  echo '</b></p>';

  echo "<table border=1 cellpadding=3 cellspacing=0>\n";
  echo "<tr><td>" . "OS" . "</td><td>" . $os_types[$row['os_type']] . "</td></tr>";
  echo "<tr><td>" . "CPU" . "</td><td>" . $cpu_types[$row['cpu_type']] . "</td></tr>";
  echo "<tr><td>" . "RAM (MiB)" . "</td><td>" . $row['memory'] . "</td></tr>";
  echo "<tr><td>" . "IP" . "</td><td>" . $row['addr_inet'] . "</td></tr>";
  echo "<tr><td>" . "Last Check-in" . "</td><td>" . PrintTime( $row['last_checkin'] ) . "</td></tr>";
  echo "<tr><td>" . "Last Pong" . "</td><td>" . PrintTime( $row['last_pong'] ) . "</td></tr>";
  echo "<tr><td>" . "Online?" . "</td><td>" . $row['online'] . "</td></tr>";
  echo "</table>\n";

  $query = "SELECT * FROM pongs WHERE host_id = x'" . $host_id
    . "'::int ORDER BY pong_time DESC LIMIT 12;";
  $result = $database->query($query);

  echo "<h3>Last ~1 hour of Pongs</h3>\n";
  echo "<table border=1 cellpadding=3 cellspacing=0>\n";
  echo "<tr><th>Time<th>Address<th>Latency (ms)</tr>\n";
  while ( $row = pg_fetch_assoc( $result ) )
  {
    echo "<tr>";
    echo "<td>" . PrintTime( $row['pong_time'] ) . "</td>";
    echo "<td>" . PrintAddr( $row['addr_int'] ) . "</td>";
    echo "<td>" . $row['latency_ms'] . "</td>";
    echo "</tr>";
  }
  echo "</table>\n";

  $url = $cssdk2_base_url . $script . '?host_id=' . $host_id;
  echo '<p>Bookmark: <a href="' . $url . ' ">' . $url . '</a></p>';
  echo '<p>Do not reveal your Host ID to others.</p>';


$query =
    "SELECT os_type, cpu_type, COUNT(*) as count, SUM(memory) as memory "
  . "FROM hosts WHERE online = 1 and last_pong IS NOT NULL "
  . "GROUP BY os_type, cpu_type ORDER BY os_type, cpu_type;";
$result = $database->query($query);

$hosts = 0;
$ram = 0;
echo "<h3>Total Hosts Online and Ponged</h3>";
echo "<table border=1 cellpadding=3 cellspacing=0>\n";
echo "<tr><th>OS<th>CPU<th>Hosts<th>Memory (MiB)</tr>\n";
while ( $row = pg_fetch_assoc( $result ) )
{
  echo "<tr><td>" . $os_types[$row['os_type']] . "</td>";
  echo "<td>" . $cpu_types[$row['cpu_type']] . "</td>";
  echo '<td align="right">' . number_format( $row['count'] ) . "</td>";
  echo '<td align="right">' . number_format( $row['memory'] ) . "</td>";
  echo "</tr>";

  $hosts += $row['count'];
  $ram += $row['memory'];
}
echo '<tr bgcolor="#00FF00"><th>Totals:<th>&nbsp;'
   . '<th align="right">' . number_format( $hosts )
   . '<th align="right">' . number_format( $ram ) . "</tr>\n";
echo "</table>\n";

}
else
{
  echo "Bad Host ID";
}

PrintTail();
?>
