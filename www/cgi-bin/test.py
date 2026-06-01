#!/usr/bin/env python3
import os
import sys

# Read request body from stdin (for POST)
body_in = sys.stdin.read()

# CGI scripts must print headers, a blank line, then the body.
print("Content-Type: text/html")
print()
print("<html><body>")
print("<h1>Ola do CGI Python!</h1>")
print("<p>REQUEST_METHOD = %s</p>" % os.environ.get("REQUEST_METHOD", ""))
print("<p>QUERY_STRING = %s</p>" % os.environ.get("QUERY_STRING", ""))
print("<p>CONTENT_LENGTH = %s</p>" % os.environ.get("CONTENT_LENGTH", ""))
print("<p>Corpo recebido: %s</p>" % body_in)
print("</body></html>")
