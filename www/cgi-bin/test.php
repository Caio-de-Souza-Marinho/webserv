#!/usr/bin/env php
<?php

echo "Content-Type: text/plain\r\n\r\n";

echo "REQUEST_METHOD=" . ($_SERVER['REQUEST_METHOD'] ?? '') . "\n";
echo "QUERY_STRING=" . ($_SERVER['QUERY_STRING'] ?? '') . "\n";
?>
