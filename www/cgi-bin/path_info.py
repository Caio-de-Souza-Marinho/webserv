#!/usr/bin/env python3
import os

# Demo CGI: shows how the server splits SCRIPT_NAME from PATH_INFO.
# Try: /cgi-bin/path_info.py/extra/path?foo=bar
print("Content-Type: text/html")
print()
print("<html><body>")
print("<h1>PATH_INFO demo</h1>")
print("<p>SCRIPT_NAME = %s</p>" % os.environ.get("SCRIPT_NAME", ""))
print("<p>PATH_INFO = %s</p>" % os.environ.get("PATH_INFO", ""))
print("<p>SCRIPT_FILENAME = %s</p>" % os.environ.get("SCRIPT_FILENAME", ""))
print("<p>QUERY_STRING = %s</p>" % os.environ.get("QUERY_STRING", ""))
print("</body></html>")
