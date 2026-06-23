<?php
// Demo CGI for the multiple-CGI bonus (requires php-cgi installed).
header("Content-Type: text/html");
echo "<html><body>";
echo "<h1>Hello from PHP CGI!</h1>";
echo "<p>REQUEST_METHOD = " . htmlspecialchars($_SERVER["REQUEST_METHOD"]) . "</p>";
echo "<p>QUERY_STRING = " . htmlspecialchars($_SERVER["QUERY_STRING"]) . "</p>";
echo "<p>PATH_INFO = " . htmlspecialchars(isset($_SERVER["PATH_INFO"]) ? $_SERVER["PATH_INFO"] : "") . "</p>";
echo "</body></html>";
?>
