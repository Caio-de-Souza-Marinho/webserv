import  requests

def test_get_index():
    r = requests.get("http://localhost:8080/index.html")
    assert r.status_code == 200
    print("GET /index.html: OK")

def test_404():
    r = requests.get("http://localhost:8080/non_existent")
    assert r.status_code == 404
    print("404 Error Page: OK")

def test_cgi_post():
    data = {'test': 'data'}
    r = requests.post("http://localhost:8080/cgi-bin/test.py", data=data)
    assert r.status_code == 200
    assert "Corpo recebido" in r.text
    print("CGI POST: OK")

if __name__ == "__main__":
    test_get_index()
    test_404()
    test_cgi_post()
