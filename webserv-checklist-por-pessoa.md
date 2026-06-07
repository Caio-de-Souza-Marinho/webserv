# Webserv — Checklist Dividido por Pessoa

> Progresso: **~85% mandatorio** · **~72% mandatorio + bonus**
> Divisao baseada na propriedade de arquivos do roadmap-sprint

---

# 👤 PESSOA 1 — Loop / epoll / integracao
*(WebServer.cpp, WebServer-Init.cpp, main.cpp)*

## 🌐 Sockets e Loop epoll
- [x] `socket()` + `SO_REUSEADDR` + `bind()` + `listen()`
- [x] `epoll_create()` e registro de fds
- [x] Sockets servidor non-blocking
- [x] `accept()` em loop com `EAGAIN`
- [x] Sockets cliente non-blocking
- [x] Mapeamento `fdToServer`
- [x] Loop principal `epoll_wait`
- [x] Tratamento de `EINTR`
- [x] Tratamento de `EPOLLERR | EPOLLHUP`
- [x] `modifyEpoll` para alternar EPOLLIN/EPOLLOUT
- [x] `setNonBlocking` via `fcntl`
- [x] `signal(SIGPIPE, SIG_IGN)`
- [ ] Handler de `SIGINT`/`SIGTERM` para shutdown limpo
- [ ] Otimizar scan O(N) de fds CGI no loop (mapa reverso) — opcional

## 📥 RequestParser (integracao no loop)
- [ ] **`readClient` tratar `PARSE_ERROR` -> buildErrorResponse(413)** <- FURO DO MERGE, o 413 nao sai sem isso

## 🔌 Client e gerenciamento de conexao
- [x] Struct Client completo (incl. campos CGI)
- [x] readBuffer/writeBuffer/writeOffset
- [x] Estado (READING/WRITING/CGI_RUNNING)
- [x] lastActivity
- [x] keepAlive honrado
- [x] Reset em keep-alive
- [x] acceptClient
- [x] readClient (loop ate EAGAIN)
- [x] writeClient (loop com EAGAIN)
- [x] `closeClient` corrigido (salva iterator, erase por ultimo, reap de CGI)
- [ ] Limite de conexoes simultaneas
- [ ] Detectar buffer de leitura grande demais

## ⏱️ Timeouts
- [x] `checkTimeouts()` varre clients
- [x] 30s para conexoes idle
- [x] Timeout de CGI (10s) -> SIGKILL + 504
- [ ] Timeout configuravel via config
- [ ] Resposta 408 ao expirar (idle so fecha, nao manda 408)

## 📄 ConfigParser (integracao)
- [ ] Caminho default de config quando argv[1] nao passado

## 🔢 Multi-porta
- [x] Config aceita multiplos `server`
- [x] Sockets por server
- [x] `fdToServer` mapeia
- [ ] **Testar** ambas portas servindo simultaneamente

---

# 👤 PESSOA 2 — Handlers / parser de body
*(ResponseBuilder*, Response.cpp, RequestParser.cpp, Request.hpp, RequestParser.hpp)*

## 📥 RequestParser
- [x] Parsear request line (method + URI + version)
- [x] Validar metodo (uppercase only)
- [x] Validar versao HTTP (1.0 ou 1.1)
- [x] Parsear headers (chave lowercase)
- [x] Detectar `Content-Length`
- [x] Detectar `Transfer-Encoding: chunked`
- [x] Detectar `Connection: keep-alive`
- [x] Parsear body regular
- [x] Parsear body chunked
- [x] Percent-decoding de path
- [x] Separacao de query string
- [x] Estado COMPLETE
- [x] `reset()` para keep-alive
- [x] `PARSE_ERROR` no enum
- [x] `errorCode` em `Request`
- [x] `maxBodySize` chega no `parse()`
- [x] body regular: `expectedBodySize > maxBodySize` -> 413 + PARSE_ERROR
- [x] chunked: acumular e checar a cada chunk -> 413
- [ ] URI muito longa -> 414
- [ ] metodo desconhecido -> 501
- [ ] `Content-Length` nao-numerico -> 400
- [ ] headers duplicados
- [ ] limite de tamanho de headers (DoS)
- [ ] `Expect: 100-continue`

## 🛠️ ResponseBuilder — Estrutura geral
- [x] `buildResponse()` despacha por metodo
- [x] `buildErrorResponse()`
- [x] `handleRedirect`
- [x] Metodo nao permitido -> 405
- [x] Route NULL -> 404
- [x] Metodo desconhecido -> 501

## 🛠️ ResponseBuilder — GET
- [x] Servir arquivo estatico
- [x] `fileExists()`, `isDirectory()`
- [x] Index file de diretorio
- [x] Autoindex
- [x] Autoindex com links corretos (`joinPath`)
- [x] Autoindex com HTML entities
- [x] Content-Type via MimeTypes
- [x] 404 / 403 corretos
- [x] 403 se arquivo `chmod 000`
- [ ] ~~Range~~ (adiado, opcional)

## 🛠️ ResponseBuilder — POST (upload)
- [x] Validar `upload_path`
- [x] Validar dir existe
- [x] Extrair filename de `Content-Disposition`
- [x] Filename default com timestamp
- [x] Escrever arquivo
- [x] 201 com `Location`
- [x] Sanitizar filename (`isSafeFilename`, rejeita `/` e `..`)
- [ ] Parsear multipart/form-data real (upload via navegador grava lixo sem isso)
- [ ] Uploads multiplos no mesmo POST

## 🛠️ ResponseBuilder — DELETE
- [x] Resolver path
- [x] 404/403/500 com `return` correto
- [x] 403 por permissao do dir pai (`isParentWritable`)

## 🛠️ ResponseBuilder — Helpers de I/O
- [x] `readFile(path, out)` com retorno bool
- [x] Distingue "nao abriu" de "vazio"
- [x] Race condition eliminada

## 🛠️ ResponseBuilder — Erros
- [x] `handleError()` busca em `errorPages`
- [x] Fallback HTML default
- [x] Paginas de erro 400/403/404/405/413/500/501

## 🛠️ Response — Headers
- [x] `Content-Length` automatico
- [x] `Content-Type` default
- [x] Status messages (incl. 502, 504 do CGI)
- [ ] Header `Server: webserv/1.0`
- [ ] Header `Date:` RFC 1123
- [ ] Header `Connection:` explicito
- [ ] **BUG: "Request Timout" -> "Request Timeout"**

---

# 👤 PESSOA 3 — CGI
*(CGIHandler.cpp, WebServer-CGI.cpp, Router.cpp)*

## 🧭 Router
- [x] `matchRoute()` com longest-prefix
- [x] `routeMatchesPath()`
- [x] `isMethodAllowed()`
- [x] `resolvePath()`
- [x] `matchCGI()` real (itera cgiHandlers, compara extensao)
- [ ] Tratamento de PATH_INFO

## 🐍 CGI
- [x] `CGIHandler::execute()` — fork + pipes + dup2 + execve
- [x] `buildEnv()` — REQUEST_METHOD
- [x] `buildEnv()` — QUERY_STRING
- [x] `buildEnv()` — CONTENT_TYPE
- [x] `buildEnv()` — CONTENT_LENGTH
- [x] `buildEnv()` — SCRIPT_FILENAME
- [x] `buildEnv()` — SCRIPT_NAME
- [x] `buildEnv()` — PATH_INFO
- [x] `buildEnv()` — SERVER_NAME
- [x] `buildEnv()` — SERVER_PORT
- [x] `buildEnv()` — SERVER_PROTOCOL
- [x] `buildEnv()` — GATEWAY_INTERFACE
- [x] `buildEnv()` — REDIRECT_STATUS
- [x] `buildEnv()` — Headers HTTP -> HTTP_*
- [x] `buildEnvp()` / `buildArgv()`
- [x] `freeEnvp()` / `freeArgv()`
- [x] `chdir()` para o diretorio do script
- [x] Resolver script para path absoluto
- [x] Pipes non-blocking
- [x] `cgiOutputFd` no epoll (EPOLLIN)
- [x] `cgiInputFd` no epoll (EPOLLOUT)
- [x] Integracao no `handleRequest`
- [x] `handleCGI()` — escreve body / le stdout
- [x] Detectar EOF e fechar fd
- [x] `waitpid()` para exit status
- [x] `parseCgiOutput()` (headers + body, header Status:)
- [x] CGI exit != 0 -> 502
- [x] Timeout -> SIGKILL + 504
- [x] Sem content-length -> EOF marca fim
- [ ] Testar de fato com script `.py` via navegador
- [ ] PATH_INFO mais preciso (separar script da parte extra do path)

## 🐍 Bonus — Multiplos CGI
- [x] Config aceita `.py` e `.php` (matchCGI funciona com varios)
- [ ] Testar com php-cgi instalado
- [ ] Pagina de demo executando ambos

---

# 🔲 SEM ATRIBUICAO — decidir no grupo

## 📐 Arquitetura e Setup
- [ ] Remover arquivos de teste do build (`Tester.cpp`, `Router-Tester.cpp`, `Mock-Config.cpp`)
- [ ] Adicionar alvo `make test` separado
- [ ] `.gitignore` ignorar uploads de teste

## 📄 ConfigParser
- [ ] Suporte a `server_name` (para virtual hosts)

## 🍪 Bonus — Cookies e Session
- [ ] Parser de `Cookie:` no Request
- [ ] `Set-Cookie:` no Response
- [ ] Sessao in-memory (`map<sessionId, sessionData>`)
- [ ] Geracao de sessionId
- [ ] Expiracao de sessao
- [ ] Endpoint `/session-test` funcional
- [ ] Pagina HTML demonstrando contador por sessao / login / logout
- [ ] Limpeza de sessoes expiradas

## 🌐 Conteudo de demonstracao (`www/`)
- [x] `www/index.html`
- [x] `www/about.html`
- [x] `www/error_pages/` completo
- [x] `www/uploads/` (existe)
- [ ] Formulario de upload (`<form enctype="multipart/form-data">`)
- [ ] Botao/JS de DELETE
- [ ] `www/readonly/` (referenciado no config, nao existe)
- [ ] `www/cgi-bin/` (referenciado no config, nao existe)
- [ ] `www/cgi-bin/hello.py` (le QUERY_STRING)
- [ ] `www/cgi-bin/form.py` (le POST body)
- [ ] `www/cgi-bin/info.php` (se fizer multiplos CGI)
- [ ] `www/listener/` (config aponta, nao existe)
- [ ] `www/session-test/session.html` (config aponta)
- [ ] Demo de redirect funcionando

## 🧪 Testes e validacao
- [ ] curl GET/POST/DELETE
- [ ] Navegador (Chrome/Firefox)
- [ ] Upload via formulario real
- [ ] CGI Python via navegador
- [ ] Stress com `siege -b -t 30s` (>= 99.5%)
- [ ] telnet (inspecao de headers)
- [ ] Comparacao com NGINX
- [ ] Script de teste em Python/Go
- [ ] Body grande (validar 413)
- [ ] Metodo nao permitido (405)
- [ ] Redirect (301/302 + Location)
- [ ] Chunked transfer encoding
- [ ] Keep-alive (multiplos requests na mesma conexao)
- [ ] Timeout
- [ ] Cliente desconectando no meio do POST
- [x] 403 com `chmod 000`

## 📜 Conformidade com o subject
- [x] Padrao C++98
- [x] `-Wall -Wextra -Werror`
- [x] Sem bibliotecas externas
- [x] C++ idiomatico
- [x] Funcoes externas na lista permitida
- [x] Makefile sem religacao desnecessaria
- [x] `fork` usado APENAS para CGI
- [ ] Auditar uso de `errno` (parser/CGI usam EAGAIN — confirmar conformidade)
- [ ] Verificar que nao trava sob nenhuma circunstancia (siege, aborts, body gigante)
- [ ] macOS: `fcntl` so com flags permitidas (se testar em Mac)

## 📖 README.md
- [x] Diagrama de classes
- [x] Diagrama de lifecycle
- [ ] Primeira linha italicizada (formato 42)
- [ ] Secao "Descricao" (objetivo + visao geral)
- [ ] Secao "Instrucoes" (compilacao, execucao)
- [ ] Secao "Recursos" (referencias + como a IA foi usada)

## 🎓 Preparacao para defesa
- [ ] Cada pessoa entende todo o codigo
- [ ] Explicar fluxo completo de uma request
- [ ] Explicar funcionamento do epoll
- [ ] Explicar pipes e fork no CGI
- [ ] Explicar parser de config
- [ ] Justificar escolhas (epoll, longest-prefix, etc.)
- [ ] Preparado para modificacao ao vivo
- [ ] Exemplos de teste prontos para cada feature
