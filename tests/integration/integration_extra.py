"""
integration_extra.py — additional integration tests for webserv
Run AFTER the server is already started on localhost:8080 / 8081.
"""

import requests
import time
import threading
import socket
import os

BASE = "http://localhost:8080"

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
# Error-page hygiene
# ─────────────────────────────────────────────────────────────
def test_error_pages():
    print("\n── Error pages ──")

    r = requests.get(f"{BASE}/this_does_not_exist_at_all.txt")
    check("404 body non-empty",           len(r.content) > 0)
    check("404 Content-Type is text/html",
          "text/html" in r.headers.get("Content-Type", ""))
    check("404 body mentions 404",        b"404" in r.content)

    r = requests.delete(f"{BASE}/readonly/")
    check("405 body non-empty",           len(r.content) > 0)
    check("405 body mentions 405",        b"405" in r.content)


# ─────────────────────────────────────────────────────────────
# Response header hygiene
# ─────────────────────────────────────────────────────────────
def test_response_header_hygiene():
    print("\n── Response header hygiene ──")

    r = requests.get(f"{BASE}/index.html")
    check("Server header present",
          "Server" in r.headers)
    check("Date header present",
          "Date" in r.headers)
    check("Content-Length numeric and correct",
          r.headers.get("Content-Length", "").isdigit()
          and int(r.headers["Content-Length"]) == len(r.content))

    r404 = requests.get(f"{BASE}/no_such_page_xyz.html")
    check("404 response has Content-Length",
          "Content-Length" in r404.headers)
    check("404 Content-Length matches body",
          int(r404.headers.get("Content-Length", -1)) == len(r404.content))


# ─────────────────────────────────────────────────────────────
# MIME types for common extensions
# ─────────────────────────────────────────────────────────────
def test_mime_types_extended():
    """Check MIME types served for static assets that exist in www/."""
    print("\n── MIME types (extended) ──")

    # We only test paths that actually exist on the server
    cases = [
        ("/index.html",  "text/html"),
        ("/about.html",  "text/html"),
    ]
    for path, mime in cases:
        r = requests.get(f"{BASE}{path}")
        ct = r.headers.get("Content-Type", "")
        check(f"GET {path} → Content-Type contains {mime}", mime in ct)


# ─────────────────────────────────────────────────────────────
# Upload → overwrite → delete lifecycle
# ─────────────────────────────────────────────────────────────
def test_upload_lifecycle():
    print("\n── Upload lifecycle (overwrite + delete) ──")

    payload1 = b"version one"
    payload2 = b"version two - overwritten"
    fname = "lifecycle_test.dat"

    # Upload first version
    r = requests.post(
        f"{BASE}/uploads",
        data=payload1,
        headers={
            "Content-Type": "application/octet-stream",
            "Content-Disposition": f'attachment; filename="{fname}"',
        },
    )
    check("First upload → 201", r.status_code == 201)
    loc = r.headers.get("Location", "")
    check("Location header set", bool(loc))

    if loc:
        # Retrieve and verify first version
        r2 = requests.get(f"{BASE}/{loc.lstrip('/')}")
        check("First version retrievable", r2.status_code == 200)
        check("First version content correct", r2.content == payload1)

        # Upload second version to the same name
        r3 = requests.post(
            f"{BASE}/uploads",
            data=payload2,
            headers={
                "Content-Type": "application/octet-stream",
                "Content-Disposition": f'attachment; filename="{fname}"',
            },
        )
        check("Second upload (overwrite) → 201", r3.status_code == 201)

        r4 = requests.get(f"{BASE}/{loc.lstrip('/')}")
        check("Overwritten file is retrievable", r4.status_code == 200)
        check("Overwritten content is new version", r4.content == payload2)

        # Clean up
        rd = requests.delete(f"{BASE}/{loc.lstrip('/')}")
        check("Delete after overwrite → 204", rd.status_code == 204)


# ─────────────────────────────────────────────────────────────
# Concurrent requests — basic thread safety
# ─────────────────────────────────────────────────────────────
def test_concurrent_requests():
    print("\n── Concurrent requests ──")

    results = []
    errors  = []

    def fetch():
        try:
            r = requests.get(f"{BASE}/index.html", timeout=10)
            results.append(r.status_code)
        except Exception as e:
            errors.append(str(e))

    threads = [threading.Thread(target=fetch) for _ in range(20)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()

    check("All 20 concurrent GETs returned 200",
          len(results) == 20 and all(s == 200 for s in results),
          f"results={results} errors={errors}")


# ─────────────────────────────────────────────────────────────
# Keep-Alive reuse across multiple request types
# ─────────────────────────────────────────────────────────────
def test_keep_alive_mixed():
    print("\n── Keep-Alive mixed methods ──")

    s = requests.Session()
    r1 = s.get(f"{BASE}/index.html")
    r2 = s.get(f"{BASE}/about.html")
    r3 = s.get(f"{BASE}/no_such_file_xyz.html")
    r4 = s.get(f"{BASE}/index.html")

    check("Session req 1 → 200", r1.status_code == 200)
    check("Session req 2 → 200", r2.status_code == 200)
    check("Session req 3 → 404 (error mid-session)", r3.status_code == 404)
    check("Session req 4 → 200 (recovered)", r4.status_code == 200)


# ─────────────────────────────────────────────────────────────
# Multipart upload — multiple files in one request
# ─────────────────────────────────────────────────────────────
def test_multipart_upload():
    print("\n── Multipart upload ──")

    files = {
        "file1": ("alpha.txt", b"content of alpha", "text/plain"),
        "file2": ("beta.txt",  b"content of beta",  "text/plain"),
    }
    r = requests.post(f"{BASE}/uploads", files=files)
    check("Multipart POST → 201", r.status_code == 201)

    # Clean up both uploaded files (best-effort)
    for name in ("alpha.txt", "beta.txt"):
        requests.delete(f"{BASE}/uploads/{name}")


# ─────────────────────────────────────────────────────────────
# Redirect chain sanity
# ─────────────────────────────────────────────────────────────
def test_redirect_followed():
    print("\n── Redirect followed ──")

    # With allow_redirects=True (default) the client should land on /
    r = requests.get(f"{BASE}/redirect", allow_redirects=True)
    check("Followed redirect lands on 200", r.status_code == 200)


# ─────────────────────────────────────────────────────────────
# Very small request / minimal valid request
# ─────────────────────────────────────────────────────────────
def test_minimal_request():
    print("\n── Minimal / edge requests ──")

    # Root path with explicit HTTP/1.0 style (no keep-alive negotiation)
    r = requests.get(f"{BASE}/", headers={"Connection": "close"})
    check("GET / with Connection: close → 200", r.status_code == 200)

    # Path with trailing slash that matches a route
    r2 = requests.get(f"{BASE}/uploads/")
    check("GET /uploads/ → 200", r2.status_code == 200)


# ─────────────────────────────────────────────────────────────
# Zero-byte body POST to upload route
# ─────────────────────────────────────────────────────────────
def test_empty_post_body():
    print("\n── Zero-byte POST body ──")

    r = requests.post(
        f"{BASE}/uploads",
        data=b"",
        headers={
            "Content-Type": "application/octet-stream",
            "Content-Disposition": 'attachment; filename="empty.dat"',
        },
    )
    # Server may return 201 (empty file) or 400; must not 500/crash
    check("Empty POST → not 500",
          r.status_code != 500,
          f"got {r.status_code}")


# ─────────────────────────────────────────────────────────────
# Runner
# ─────────────────────────────────────────────────────────────
if __name__ == "__main__":
    print("=" * 50)
    print("webserv extra integration tests")
    print("=" * 50)

    test_error_pages()
    test_response_header_hygiene()
    test_mime_types_extended()
    test_upload_lifecycle()
    test_concurrent_requests()
    test_keep_alive_mixed()
    test_multipart_upload()
    test_redirect_followed()
    test_minimal_request()
    test_empty_post_body()

    print(f"\n{'=' * 50}")
    print(f"Passed: {PASS}  Failed: {FAIL}")
    print("=" * 50)

    exit(0 if FAIL == 0 else 1)
