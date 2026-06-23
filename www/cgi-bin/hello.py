#!/usr/bin/env python3
import os

# Demo CGI: reads the query string (e.g. /cgi-bin/hello.py?name=World)
query = os.environ.get("QUERY_STRING", "")

name = "World"
for pair in query.split("&"):
    if pair.startswith("name="):
        name = pair[len("name="):]

print("Content-Type: text/html")
print()
print("<html><body>")
print("<h1>Hello, %s!</h1>" % name)
print("<p>QUERY_STRING = %s</p>" % query)
print("</body></html>")
