#!/usr/bin/env python3
import os
import sys

# Demo CGI: reads CONTENT_LENGTH bytes of the POST body from stdin.
length = 0
try:
    length = int(os.environ.get("CONTENT_LENGTH", "0"))
except ValueError:
    length = 0

body = sys.stdin.read(length) if length > 0 else ""

print("Content-Type: text/html")
print()
print("<html><body>")
print("<h1>Form received</h1>")
print("<p>REQUEST_METHOD = %s</p>" % os.environ.get("REQUEST_METHOD", ""))
print("<p>CONTENT_LENGTH = %s</p>" % os.environ.get("CONTENT_LENGTH", ""))
print("<p>Body = %s</p>" % body)
print("</body></html>")
