<?php

include "include/session.php";

$script = "stats.php";

if ( !$session->logged_in )
{
  header( "Location:" . $cssdk2_base_url );
  die();
}

PrintHead( "Stats" );

$query =
    "SELECT os_type, cpu_type, COUNT(*) as count, SUM(memory) as memory "
  . "FROM hosts WHERE online = 1 and last_pong IS NOT NULL "
  . "GROUP BY os_type, cpu_type ORDER BY os_type, cpu_type;";
$result = $database->query($query);

$ram = 0;
$hosts = 0;
echo "<h3>Online Hosts</h3>";
echo "<table border=1 cellpadding=3 cellspacing=0>\n";
echo "<tr><th>OS<th>CPU<th>Hosts<th>Memory (MiB)<th>Avg</tr>\n";
while ( $row = pg_fetch_assoc( $result ) )
{
  echo "<tr><td>" . $os_types[$row['os_type']] . "</td>";
  echo "<td>" . $cpu_types[$row['cpu_type']] . "</td>";
  echo '<td align="right">' . number_format( $row['count'] ) . "</td>";
  echo '<td align="right">' . number_format( $row['memory'] ) . "</td>";
  echo '<td align="right">' . number_format( $row['memory'] / $row['count'] ) . "</td>";
  echo "</tr>";

  $hosts += $row['count'];
  $ram += $row['memory'];
}
echo '<tr bgcolor="#00FF00"><th>Totals:<td>&nbsp;<th align="right">' . number_format( $hosts )
   . '<th align="right">' . number_format( $ram ) . "<th>&nbsp</tr>\n";
echo "</table>\n";


$query =
    "SELECT os_type, cpu_type, COUNT(*) as count, SUM(memory) as memory "
  . "FROM hosts WHERE last_pong IS NOT NULL "
  . "GROUP BY os_type, cpu_type ORDER BY os_type, cpu_type;";
$result = $database->query($query);

$ram = 0;
$hosts = 0;
echo "<h3>All Ponged Hosts</h3>";
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
echo '<tr bgcolor="#FFFF00"><th>Totals:<td>&nbsp;<th align="right">' . number_format( $hosts )
   . '<th align="right">' . number_format( $ram ) . "</tr>\n";
echo "</table>\n";

echo "<h3>Stats</h3>";
echo "<table border=1 cellpadding=3 cellspacing=0>\n";

$query = "SELECT COUNT(*) as n FROM hosts WHERE last_checkin IS NOT NULL;";
$result = $database->query($query);
$row = pg_fetch_assoc( $result );
echo '<tr><td>Checked in</td><td>' . $row['n'] . '</td></tr>';

$query = "SELECT COUNT(*) as n FROM hosts;";
$result = $database->query($query);
$row = pg_fetch_assoc( $result );
echo '<tr><td>Clicked "add host"</td><td>' . $row['n'] . '</td></tr>';

$query = "SELECT COUNT(*) as n FROM users;";
$result = $database->query($query);
$row = pg_fetch_assoc( $result );
echo "<tr><td>Signups</td><td>" . $row['n'] . '</td></tr>';


echo "</table>\n";


PrintTail();
?>
