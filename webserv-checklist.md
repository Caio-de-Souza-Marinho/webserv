# Webserv — Checklist Completo

> Progresso atual: **~67% mandatório** · **~57% mandatório + bônus**
> Última atualização: refatoração do `readFile`, fix do DELETE, fix dos links/entities do autoindex

---

## 📐 Arquitetura e Setup

- [x] Estrutura de diretórios (`src/`, `include/`, `config/`, `www/`)
- [x] Makefile com regras `all`, `clean`, `fclean`, `re`
- [x] Compilação com `-Wall -Wextra -Werror -std=c++98`
- [x] Separação modular de classes
- [x] Sistema de Logger com níveis (DEBUG/INFO/WARNING/ERROR)
- [x] Header de Colors
- [x] MimeTypes com cobertura ampla
- [ ] Remover arquivos de teste do build de produção (`Tester.cpp`, `Router-Tester.cpp`, `Mock-Config.cpp`)
- [ ] Adicionar alvo `make test` separado
- [ ] `.gitignore` ignorar uploads de teste (`www/uploads/*.dat`)

---

## 📄 ConfigParser

- [x] Abrir e ler arquivo de configuração
- [x] Parsear bloco `server { }`
- [x] Parsear bloco `location { }`
- [x] Diretiva `listen` (host:port ou só port)
- [x] Diretiva `root` (server e location)
- [x] Diretiva `index` (múltiplos arquivos)
- [x] Diretiva `client_max_body_size` (sufixo `M`)
- [x] Diretiva `error_page` (múltiplas)
- [x] Diretiva `allow_methods`
- [x] Diretiva `autoindex` (on/off)
- [x] Diretiva `upload_path`
- [x] Diretiva `return` (redirect com código)
- [x] Diretiva `cgi` (extensão + interpretador)
- [x] Validação de porta (1-65535)
- [x] Validação de código HTTP (100-599)
- [x] Tratamento de comentários (`#`)
- [x] Herança de `root` e `index` do server para location
- [ ] Suporte a `server_name` (necessário para virtual hosts)
- [ ] Caminho default de config quando argv[1] não passado

---

## 🌐 Sockets e Loop epoll

- [x] `socket()` + `SO_REUSEADDR` + `bind()` + `listen()`
- [x] `epoll_create()` e registro de fds
- [x] Sockets servidor em modo non-blocking
- [x] `accept()` em loop com `EAGAIN`
- [x] Sockets cliente em modo non-blocking
- [x] Mapeamento `fdToServer`
- [x] Loop principal `epoll_wait` com `MAX_EVENTS=64`
- [x] Tratamento de `EINTR`
- [x] Tratamento de `EPOLLERR | EPOLLHUP`
- [x] `modifyEpoll` para alternar EPOLLIN/EPOLLOUT
- [x] `setNonBlocking` via `fcntl(F_SETFL, O_NONBLOCK)`
- [ ] `signal(SIGPIPE, SIG_IGN)` — **CRÍTICO**: write em socket fechado mata o processo
- [ ] Handler de `SIGINT`/`SIGTERM` para shutdown limpo
- [ ] Single epoll para TODAS operações de I/O (reforçar quando CGI entrar)
- [ ] Auditar que NENHUMA leitura/escrita acontece sem passar pelo epoll (quando CGI chegar)

---

## 📥 RequestParser

- [x] Parsear request line (method + URI + version)
- [x] Validar método (uppercase only)
- [x] Validar versão HTTP (1.0 ou 1.1)
- [x] Parsear headers com chave normalizada para lowercase
- [x] Detectar `Content-Length`
- [x] Detectar `Transfer-Encoding: chunked`
- [x] Detectar `Connection: keep-alive` / default por versão
- [x] Parsear body regular (Content-Length)
- [x] Parsear body chunked (EOF marker 0)
- [x] Percent-decoding de path
- [x] Separação de query string
- [x] Estado COMPLETE atingível
- [x] `reset()` para reuso em keep-alive
- [ ] Adicionar estado `PARSE_ERROR` ao enum
- [ ] Adicionar campo `errorCode` em `Request` (400/413/414/501)
- [ ] Validar URI muito longa → 414
- [ ] Validar método desconhecido → 501
- [ ] Validar `Content-Length` não-numérico → 400
- [ ] Tratar headers duplicados
- [ ] **Enforçar `client_max_body_size` → 413** (obrigatório, não feito)
- [ ] Limite de tamanho de headers (proteção DoS)
- [ ] Tratar `Expect: 100-continue`

---

## 🧭 Router

- [x] `matchRoute()` com longest-prefix match
- [x] `routeMatchesPath()` lida com `/`, exact match, prefix com `/`
- [x] `isMethodAllowed()`
- [x] `resolvePath()` (root + path relativo à rota)
- [ ] `matchCGI()` — atualmente retorna NULL hardcoded
- [ ] Tratamento de PATH_INFO (parte do path depois do script CGI)

---

## 🛠️ ResponseBuilder

### Estrutura geral
- [x] `buildResponse()` despacha por método
- [x] `buildErrorResponse()` para erros externos
- [x] Resposta de redirect (`handleRedirect`)
- [x] Verificação de método permitido → 405
- [x] Route NULL → 404
- [x] Método desconhecido → 501

### GET
- [x] Servir arquivo estático
- [x] `fileExists()`, `isDirectory()`
- [x] Servir index file de diretório (lista de fallbacks)
- [x] Autoindex (geração de listagem HTML)
- [x] Autoindex com links corretos via `joinPath(urlPath, ...)`
- [x] Autoindex com HTML entities (sem bytes UTF-8 inválidos)
- [x] Content-Type via MimeTypes
- [x] 404 se arquivo não existe
- [x] 403 se diretório sem autoindex e sem index
- [x] **403 se arquivo existe mas não é legível (`chmod 000`)**
- [ ] ~~Suporte a `Range:`~~ (adiado — opcional, não exigido pelo subject)

### POST (upload)
- [x] Validar `upload_path` configurado
- [x] Validar que `upload_path` existe e é diretório
- [x] Extrair filename de `Content-Disposition`
- [x] Gerar filename default com timestamp
- [x] Escrever arquivo
- [x] Retornar 201 com `Location`
- [ ] **Parsear multipart/form-data real** (boundary, partes) — upload via navegador grava lixo sem isso
- [ ] Suportar uploads múltiplos no mesmo POST
- [x] Sanitizar filename (rejeitar `../`, paths absolutos)

### DELETE
- [x] Resolver path via Router
- [x] **404/403/500 com `return` correto** (bug corrigido)
- [x] Validar permissões de escrita no diretório pai

### Helpers de I/O
- [x] `readFile(path, out)` com parâmetro de saída + retorno bool
- [x] Distingue "não abriu" de "abriu vazio"
- [x] Race condition `isFileReadable`+`readFile` eliminada (abre uma vez só)

### Erros
- [x] `handleError()` busca em `errorPages` da config
- [x] Fallback para HTML default
- [x] `buildDefaultErrorBody()` gera HTML mínimo
- [x] Páginas de erro padrão para 400/403/404/405/413/500/501

### Headers gerais
- [x] `Content-Length` calculado automaticamente
- [x] `Content-Type` default `text/html` se body não vazio
- [x] Status message correto para cada código
- [ ] Header `Server: webserv/1.0`
- [ ] Header `Date:` em formato RFC 1123
- [ ] Header `Connection: close` ou `keep-alive` explícito
- [ ] **BUG**: "Request Timout" → corrigir para "Request Timeout" no `getStatusMessage`

---

## 🔌 Client e gerenciamento de conexão

- [x] Struct Client com todos os campos necessários
- [x] `readBuffer` / `writeBuffer` / `writeOffset`
- [x] Estado (`READING`, `WRITING`, `CGI_RUNNING`)
- [x] `lastActivity` para timeout
- [x] `keepAlive` honrado no writeClient
- [x] Reset de buffers + parser ao final de request keep-alive
- [x] `acceptClient` adiciona ao epoll com EPOLLIN
- [x] `readClient` lê em loop até EAGAIN
- [x] `writeClient` envia em loop tratando EAGAIN
- [ ] **CRÍTICO: `closeClient` usa iterator após `erase()` (UB, risco de crash = nota 0)**
- [ ] Limite de conexões simultâneas
- [ ] Detectar buffer de leitura grande demais

---

## ⏱️ Timeouts

- [x] `checkTimeouts()` varre clients
- [x] Hardcoded 30s para conexões idle
- [ ] Timeout configurável via config
- [ ] Resposta 408 ao expirar (atualmente só fecha)
- [ ] Timeout específico para CGI (504)
- [ ] Verificar que requisição nunca trava indefinidamente sob carga

---

## 🐍 CGI (obrigatório — ~0%)

- [ ] `CGIHandler::execute()` — fork + pipes + dup2 + execve
- [ ] `buildEnv()` com variáveis CGI/1.1:
  - [ ] `REQUEST_METHOD`
  - [ ] `QUERY_STRING`
  - [ ] `CONTENT_TYPE`
  - [ ] `CONTENT_LENGTH`
  - [ ] `SCRIPT_FILENAME`
  - [ ] `SCRIPT_NAME`
  - [ ] `PATH_INFO`
  - [ ] `SERVER_NAME`
  - [ ] `SERVER_PORT`
  - [ ] `SERVER_PROTOCOL`
  - [ ] `GATEWAY_INTERFACE`
  - [ ] `REDIRECT_STATUS` (necessário para php-cgi)
  - [ ] Headers HTTP → `HTTP_*`
- [ ] `buildEnvp()` converte vector pra `char**`
- [ ] `buildArgv()` constrói argv
- [ ] `freeEnvp()` / `freeArgv()` (sem leak no parent)
- [ ] `chdir()` para o diretório do script antes do execve
- [ ] Setar pipes como non-blocking
- [ ] Adicionar `cgiOutputFd` ao epoll (EPOLLIN)
- [ ] Adicionar `cgiInputFd` ao epoll (EPOLLOUT)
- [ ] `Router::matchCGI()` real (iterar `cgiHandlers`, comparar extensão)
- [ ] Integração no `handleRequest`: chamar CGI quando matchCGI ≠ NULL
- [ ] `WebServer::handleCGI()` — ler stdout em chunks
- [ ] Escrever request body em stdin do CGI
- [ ] Detectar EOF e fechar fd
- [ ] `waitpid()` não-bloqueante para coletar exit status
- [ ] Parsear output do CGI (headers + linha em branco + body)
- [ ] Status code via header `Status:` ou default 200
- [ ] CGI exit != 0 → 502 Bad Gateway
- [ ] Timeout de CGI → SIGKILL + 504
- [ ] Suporte a chunked input (CGI espera EOF)
- [ ] Sem `content_length` no output → EOF marca fim
- [ ] Suportar pelo menos um interpretador (Python ou PHP)

---

## 🔢 Multi-porta e Multi-server

- [x] Config aceita múltiplos `server { }` blocks
- [x] `default.conf` tem listen 8080 e 8081
- [x] Sockets criados para cada server
- [x] `fdToServer` mapeia fd → config correta
- [ ] **Testar de fato** que ambas portas servem conteúdo diferente simultaneamente
- [ ] Verificar comportamento se dois `server` listarem mesma porta com hosts diferentes

---

## 🍪 Bônus — Cookies e Session

- [ ] Parser de header `Cookie:` no Request
- [ ] Helper pra setar `Set-Cookie:` no Response
- [ ] Estrutura de sessão in-memory (`map<sessionId, sessionData>`)
- [ ] Geração de sessionId único
- [ ] Expiração de sessão
- [ ] Endpoint `/session-test` funcional (já no config)
- [ ] Página HTML demonstrando contador por sessão / login / logout
- [ ] Limpeza de sessões expiradas

---

## 🐍 Bônus — Múltiplos CGI

- [ ] Suporte simultâneo a `.py` e `.php` (config já prevê)
- [ ] Testar com php-cgi instalado
- [ ] Página de demo executando ambos

---

## 🌐 Conteúdo de demonstração (`www/`)

- [x] `www/index.html`
- [x] `www/about.html`
- [x] `www/error_pages/` completo (400/403/404/405/413/500/501)
- [x] `www/uploads/` (diretório existe)
- [ ] `www/uploads/` com formulário de upload (`<form enctype="multipart/form-data">`)
- [ ] `www/uploads/` com botão/JS de DELETE para demo
- [ ] `www/readonly/` — referenciado no config mas não existe
- [ ] `www/cgi-bin/` — referenciado no config mas não existe
- [ ] `www/cgi-bin/hello.py` — lê QUERY_STRING
- [ ] `www/cgi-bin/form.py` — lê POST body
- [ ] `www/cgi-bin/info.php` (se for fazer múltiplos CGI)
- [ ] `www/listener/` — config aponta mas dir não existe
- [ ] `www/session-test/` com `session.html` (config aponta)
- [ ] Página de demo de redirect funcionando

---

## 🧪 Testes e validação

- [ ] Testes manuais com `curl` para GET/POST/DELETE
- [ ] Testes com navegador (Chrome/Firefox)
- [ ] Teste de upload via formulário HTML real
- [ ] Teste de CGI Python via navegador
- [ ] Teste de stress com `siege -b -t 30s` (availability ≥ 99.5%)
- [ ] Teste de telnet para inspeção manual de headers
- [ ] Comparação com NGINX (headers e códigos de status)
- [ ] Script de teste em Python ou Go (subject pede explicitamente)
- [ ] Teste de body grande (validar 413)
- [ ] Teste de método não permitido (validar 405)
- [ ] Teste de redirect (validar 301/302 + Location)
- [ ] Teste de chunked transfer encoding
- [ ] Teste de keep-alive (múltiplos requests na mesma conexão)
- [ ] Teste de timeout
- [ ] Teste de cliente desconectando no meio do POST
- [x] Teste de 403 com `chmod 000` (arquivo sem permissão de leitura)

---

## 📜 Conformidade com regras do subject

- [x] Padrão C++98
- [x] Compila com `-Wall -Wextra -Werror`
- [x] Sem bibliotecas externas (sem Boost)
- [x] Uso de C++ idiomático
- [x] Funções externas usadas estão na lista permitida
- [x] Makefile sem religação desnecessária
- [ ] Verificar que não usa `errno` após read/write (auditar quando CGI entrar)
- [ ] Verificar que `fork` é usado APENAS para CGI
- [ ] Verificar que não trava sob nenhuma circunstância (siege, conexões abortadas, body gigante)
- [ ] No macOS: `fcntl` apenas com `F_SETFL`, `O_NONBLOCK`, `FD_CLOEXEC` (se testar em Mac)

---

## 📖 README.md (Capítulo V do subject)

- [x] Diagrama de classes (Mermaid)
- [x] Diagrama de lifecycle (Mermaid)
- [ ] Primeira linha italicizada: `*Este projeto foi criado como parte do currículo da 42 por <login1>[, <login2>...].*`
- [ ] Seção "Descrição" (objetivo + visão geral)
- [ ] Seção "Instruções" (compilação, execução)
- [ ] Seção "Recursos" (referências + como a IA foi usada e em quais partes)
- [ ] Seção opcional de exemplos de uso
- [ ] Seção opcional de escolhas técnicas

---

## 🎓 Preparação para defesa

- [ ] Cada pessoa do grupo entende todo o código
- [ ] Saber explicar fluxo completo de uma request
- [ ] Saber explicar funcionamento do epoll
- [ ] Saber explicar pipes e fork no CGI
- [ ] Saber explicar parser de config
- [ ] Saber justificar escolhas (por que epoll, longest-prefix, etc.)
- [ ] Estar preparado para modificação ao vivo
- [ ] Ter exemplos prontos de teste para cada feature

---

## 📊 Resumo por categoria

| Categoria | Progresso |
|---|---|
| Arquitetura e Setup | ~85% |
| ConfigParser | ~90% |
| Sockets e Loop epoll | ~75% |
| RequestParser | ~70% |
| Router | ~80% |
| ResponseBuilder GET | ~95% |
| ResponseBuilder POST | ~60% (sem multipart) |
| ResponseBuilder DELETE | ~80% |
| Client e gerenciamento | ~70% (bug closeClient) |
| Timeouts | ~50% |
| CGI | ~0% |
| Multi-porta | ~70% (não testado) |
| Bônus Cookies/Session | 0% |
| Bônus Múltiplos CGI | 0% |
| Conteúdo de demo | ~40% |
| Testes | ~12% |
| Conformidade subject | ~85% |
| README | ~30% |

**Total mandatório: ~67%**
**Total mandatório + bônus: ~57%**

---

## 🎯 Próximas prioridades (ordem sugerida)

1. **Bugs críticos** (rápidos, tiram do risco de nota 0): `closeClient` iterator, `SIGPIPE`, typo "Request Timout"
2. **`client_max_body_size` → 413** + `PARSE_ERROR` no enum (obrigatório)
3. **CGI completo** (maior bloco restante, obrigatório)
4. **Multipart parsing** para upload real
5. **Páginas de demo + scripts CGI**
6. **README nos padrões do subject**
7. **Testes de stress (siege)**
8. **Bônus: cookies/session**
9. **Limpeza: remover Testers do build**
