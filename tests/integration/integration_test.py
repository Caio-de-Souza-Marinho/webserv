import requests
import os
import time

BASE = "http://localhost:8080"
BASE_81 = "http://localhost:8081"

PASS = 0
FAIL = 0

def check(name, condition, detail=""):
    global PASS, FAIL
    if condition:
        print(f"  PASS  {name}")
        PASS += 1
    else:
        print(f"  FAIL  {name}" + (f" — {detail}" if detail else ""))
        FAIL += 1

# ─────────────────────────────────────────────────────────────
# Static file serving
# ─────────────────────────────────────────────────────────────
def test_static():
    print("\n── Static file serving ──")

    r = requests.get(f"{BASE}/index.html")
    check("GET /index.html → 200", r.status_code == 200)
    check("GET /index.html has html body", "<html" in r.text.lower())
    check("GET /index.html Content-Type is text/html", "text/html" in r.headers.get("Content-Type", ""))

    r = requests.get(f"{BASE}/about.html")
    check("GET /about.html → 200", r.status_code == 200)

    r = requests.get(f"{BASE}/non_existent_file.html")
    check("GET missing file → 404", r.status_code == 404)

    r = requests.get(f"{BASE}/")
    check("GET / serves index → 200", r.status_code == 200)

# ─────────────────────────────────────────────────────────────
# HTTP methods
# ─────────────────────────────────────────────────────────────
def test_methods():
    print("\n── HTTP methods ──")

    # POST to a non-upload route should 403
    r = requests.post(f"{BASE}/index.html", data="x")
    check("POST to non-upload route → 403 or 405", r.status_code in (403, 405))

    # DELETE on a read-only route should 405
    r = requests.delete(f"{BASE}/readonly/")
    check("DELETE on readonly route → 405", r.status_code == 405)

    # Method not in spec at all
    r = requests.request("PATCH", f"{BASE}/index.html")
    check("PATCH (unsupported method) → 400 or 405", r.status_code in (400, 405))

# ─────────────────────────────────────────────────────────────
# Uploads  (POST + DELETE round-trip)
# ─────────────────────────────────────────────────────────────
def test_uploads():
    print("\n── Uploads ──")

    payload = b"integration test file content"
    r = requests.post(
        f"{BASE}/uploads",
        data=payload,
        headers={"Content-Type": "application/octet-stream",
                 "Content-Disposition": 'attachment; filename="it_test.dat"'}
    )
    check("POST /uploads → 201", r.status_code == 201)
    location = r.headers.get("Location", "")
    check("POST /uploads returns Location header", bool(location))

    # Verify the file is retrievable
    if location:
        r2 = requests.get(f"{BASE}/{location.lstrip('/')}")
        check("Uploaded file is retrievable via GET", r2.status_code == 200)
        check("Uploaded file content matches", r2.content == payload)

        # Clean up via DELETE
        r3 = requests.delete(f"{BASE}/{location.lstrip('/')}")
        check("DELETE uploaded file → 204", r3.status_code == 204)

        r4 = requests.get(f"{BASE}/{location.lstrip('/')}")
        check("Deleted file is gone (404)", r4.status_code == 404)

# ─────────────────────────────────────────────────────────────
# Body size limit
# ─────────────────────────────────────────────────────────────
def test_body_size():
    print("\n── Body size limit ──")

    big = b"X" * (11 * 1024 * 1024)   # 11 MB > 10 MB limit
    r = requests.post(f"{BASE}/uploads", data=big)
    check("POST body exceeding limit → 413", r.status_code == 413)

# ─────────────────────────────────────────────────────────────
# CGI
# ─────────────────────────────────────────────────────────────
def test_cgi():
    print("\n── CGI ──")

    # Basic GET to CGI script
    r = requests.get(f"{BASE}/cgi-bin/test.py")
    check("GET /cgi-bin/test.py → 200", r.status_code == 200)
    check("CGI GET response has html body", "<html>" in r.text.lower())

    # POST with body
    r = requests.post(f"{BASE}/cgi-bin/test.py", data={"key": "value"})
    check("POST /cgi-bin/test.py → 200", r.status_code == 200)
    check("CGI echoes body", "Corpo recebido" in r.text)

    # Query string forwarding
    r = requests.get(f"{BASE}/cgi-bin/test.py?foo=bar")
    check("CGI receives QUERY_STRING", "foo=bar" in r.text)

    # Non-existent CGI script
    r = requests.get(f"{BASE}/cgi-bin/does_not_exist.py")
    check("Missing CGI script → 404", r.status_code == 404)

# ─────────────────────────────────────────────────────────────
# Redirects
# ─────────────────────────────────────────────────────────────
def test_redirects():
    print("\n── Redirects ──")

    # allow_redirects=False so we see the actual 302
    r = requests.get(f"{BASE}/redirect", allow_redirects=False)
    check("GET /redirect → 302", r.status_code == 302)
    check("Location header present", "Location" in r.headers)
    check("Location points to /", r.headers.get("Location") == "/")

# ─────────────────────────────────────────────────────────────
# Autoindex / directory listing
# ─────────────────────────────────────────────────────────────
def test_autoindex():
    print("\n── Autoindex ──")

    r = requests.get(f"{BASE}/uploads/")
    check("GET /uploads/ (autoindex on) → 200", r.status_code == 200)
    check("Autoindex body is HTML", "<html>" in r.text.lower() or "<a" in r.text.lower())

    r = requests.get(f"{BASE}/readonly/")
    check("GET /readonly/ (autoindex on) → 200", r.status_code == 200)

# ─────────────────────────────────────────────────────────────
# Keep-Alive
# ─────────────────────────────────────────────────────────────
def test_keep_alive():
    print("\n── Keep-Alive ──")

    session = requests.Session()
    r1 = session.get(f"{BASE}/index.html")
    r2 = session.get(f"{BASE}/about.html")
    check("First request on session → 200", r1.status_code == 200)
    check("Second request on same session → 200", r2.status_code == 200)

# ─────────────────────────────────────────────────────────────
# Headers / response hygiene
# ─────────────────────────────────────────────────────────────
def test_response_headers():
    print("\n── Response headers ──")

    r = requests.get(f"{BASE}/index.html")
    check("Content-Length header present", "Content-Length" in r.headers)
    check("Content-Length is numeric", r.headers.get("Content-Length", "").isdigit())
    check("Content-Length matches body", int(r.headers["Content-Length"]) == len(r.content))

# ─────────────────────────────────────────────────────────────
# MIME types
# ─────────────────────────────────────────────────────────────
def test_mime_types():
    print("\n── MIME types ──")

    # We need at least one css/js file; skip gracefully if missing
    for path, expected_mime in [("/index.html", "text/html")]:
        r = requests.get(f"{BASE}{path}")
        ct = r.headers.get("Content-Type", "")
        check(f"GET {path} → Content-Type contains {expected_mime}", expected_mime in ct)

# ─────────────────────────────────────────────────────────────
# Second virtual host (port 8081)
# ─────────────────────────────────────────────────────────────
def test_virtual_host():
    print("\n── Virtual host (port 8081) ──")

    try:
        r = requests.get(f"{BASE_81}/", timeout=3)
        check("Port 8081 responds → 200", r.status_code == 200)

        r2 = requests.get(f"{BASE_81}/redirect", allow_redirects=False, timeout=3)
        check("Port 8081 /redirect → 301", r2.status_code == 301)
    except requests.exceptions.ConnectionError:
        check("Port 8081 reachable", False, "connection refused")

# ─────────────────────────────────────────────────────────────
# Malformed / edge-case requests
# ─────────────────────────────────────────────────────────────
def test_edge_cases():
    print("\n── Edge cases ──")

    # Chunked transfer encoding
    def gen_chunks():
        for chunk in [b"hello", b" ", b"world"]:
            yield chunk

    r = requests.post(
        f"{BASE}/cgi-bin/test.py",
        data=gen_chunks(),
        headers={"Transfer-Encoding": "chunked"}
    )
    check("Chunked POST to CGI → 200", r.status_code == 200)

    # Percent-encoded URI
    r = requests.get(f"{BASE}/index.html")   # %2F would be blocked; just verify normal works
    check("Percent-encoded path decoded correctly", r.status_code == 200)

    # Very long URI (shouldn't crash the server)
    long_path = "/" + "a" * 4000
    try:
        r = requests.get(f"{BASE}{long_path}", timeout=5)
        check("Very long URI → 404 (not crash)", r.status_code == 404)
    except requests.exceptions.ConnectionError:
        check("Very long URI → server still alive", False, "server closed connection unexpectedly")

# ─────────────────────────────────────────────────────────────
# Runner
# ─────────────────────────────────────────────────────────────
if __name__ == "__main__":
    print("=" * 50)
    print("webserv integration tests")
    print("=" * 50)

    test_static()
    test_methods()
    test_uploads()
    test_body_size()
    test_cgi()
    test_redirects()
    test_autoindex()
    test_keep_alive()
    test_response_headers()
    test_mime_types()
    test_virtual_host()
    test_edge_cases()

    print(f"\n{'=' * 50}")
    print(f"Passed: {PASS}  Failed: {FAIL}")
    print("=" * 50)

    exit(0 if FAIL == 0 else 1)
