<?php
/*
prics.php
A part of prics, a system for controlling CounterStrike-servers from the web
Written by Kung den Knege (Eli Kaufman) for Joel Nygårds
Contact the author att kungdenknege@gmail.com
*/

// THE CLIENT WHO IS ALLOWED TO RUN THIS SCRIPT
define("CLIENT_IP", "127.0.0.1");

define("DEFAULT_PORT", 1025);

// Env checks
if ($_SERVER['REMOTE_ADDR'] != CLIENT_IP)
	die("Hey mupphead, you're not allowed to run this script!");	
if (!(isset($_POST['host']) && isset($_POST['rcon']) && isset($_POST['pass']) && isset($_POST['time']) && is_numeric($_POST['time'])))
	die("Hoy comrade, it looks like you're forgetting a few of the arguments!");
	
$port = (isset($_POST['port']) && is_numeric($_POST['port'])) ? (int)$_POST['port'] : DEFAULT_PORT;

// Connect
if (!($sock = @fsockopen($_POST['host'], $port, $noneed, $forthis, 10)))
	die("Couldn't connect to host!");

$str = $_POST['rcon'] . " " . $_POST['pass'] . " " . $_POST['time']."\n";
if (@fwrite($sock, $str) < strlen($str))
	die("Couldn't send data to host!");
	
pclose($sock);

?>