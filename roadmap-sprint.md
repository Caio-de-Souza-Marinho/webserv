# Roadmap — Sprint de Integração · webserv

> Objetivo do sprint: integrar ConfigParser, RequestParser e ResponseBuilder no WebServer,
> implementar POST/DELETE/autoindex e montar o pipeline de CGI — sem conflito de arquivos entre as pessoas.

---

## Contratos de interface (acordar antes de começar)

Antes de qualquer pessoa abrir o editor, as três precisam alinhar:

1. **Estado de erro do parser** — P2 vai adicionar detecção de body-too-large em `RequestParser`. P1 precisa saber como ler esse erro. Decisão sugerida: adicionar `PARSE_ERROR` como novo valor do enum `State` e um campo `int errorCode` em `Request` (ex: 413, 400). P2 define, P1 lê.

2. **Assinatura de `handleCGI`** — já declarada em `WebServer.hpp`. Não muda. P1 chama, P3 implementa.

3. **Struct `Client`** — os campos CGI (`cgiPid`, `cgiInputFd`, `cgiOutputFd`, `cgiBuffer`, `cgiDone`) já existem. P3 escreve, P1 lê. Ninguém altera `Client.hpp` sem avisar o grupo.

---

## Pessoa 1 — Loop do servidor (epoll + integração)

### Objetivo
Implementar o `WebServer` do zero: socket, epoll, loop de leitura/escrita e integração com todas as outras classes.

### Arquivos sob responsabilidade

| Arquivo | Ação |
|---|---|
| `src/WebServer.cpp` | **Criar** — métodos do loop principal |
| `src/WebServer-Init.cpp` | **Criar** — construtor, bind/listen, epoll_create |
| `src/main.cpp` | **Atualizar** — instanciar `WebServer` com config real |

### O que implementar

**`WebServer-Init.cpp`**
- Construtor: rodar `ConfigParser`, criar sockets para cada `ServerConfig`, `bind()`, `listen()`, `epoll_create()`
- Mapear cada fd de servidor em `fdToServer`
- Instanciar `Router`, `ResponseBuilder` e `CGIHandler`

**`WebServer.cpp`**
- `run()` — loop principal com `epoll_wait`, despachar eventos para `acceptClient`, `readClient`, `writeClient` ou `handleCGI`
- `acceptClient(int serverFd)` — `accept()`, setar socket non-blocking, adicionar ao epoll com `EPOLLIN`, criar `Client`
- `readClient(int fd)` — `recv()` no `readBuffer`, chamar `parser->parse()`, checar estado:
  - `PARSE_ERROR` → `buildErrorResponse(errorCode)` → `WRITING`
  - `COMPLETE` → `handleRequest(client)`
- `handleRequest(Client &client)` — chamar `Router::matchRoute`, checar método permitido, verificar se é CGI (`matchCGI`), chamar `buildResponse` ou `execute` do CGI
- `writeClient(int fd)` — enviar `writeBuffer` a partir de `writeOffset`, ao terminar checar `keepAlive` (reset ou close)
- `closeClient(int fd)` — remover do epoll, fechar fd, apagar de `clients`
- `modifyEpoll` / `removeFromEpoll` — wrappers do `epoll_ctl`
- `checkTimeouts()` — varrer `clients`, comparar `lastActivity` com `time(NULL)`, matar CGIs travados (504), enviar 408 para leituras paradas

**`main.cpp`**
- Remover o bloco de testes atual
- Receber `argv[1]` como caminho do config
- Instanciar e chamar `WebServer(argv[1]).run()` dentro de try/catch

### Não toca em
`ResponseBuilder.cpp`, `CGIHandler.cpp`, `RequestParser.cpp`, `Router.cpp`

---

## Pessoa 2 — POST, DELETE, autoindex e body size

### Objetivo
Implementar os handlers HTTP que faltam em `ResponseBuilder`, enforçar o limite de body no parser e corrigir o bug de typo.

### Arquivos sob responsabilidade

| Arquivo | Ação |
|---|---|
| `src/ResponseBuilder.cpp` | **Atualizar** — implementar stubs |
| `src/Response.cpp` | **Corrigir** — typo no header |
| `src/RequestParser.cpp` | **Atualizar** — enforçar `maxBodySize` |
| `include/Request.hpp` | **Atualizar** — adicionar `errorCode` (alinhado com P1) |
| `include/RequestParser.hpp` | **Atualizar** — adicionar `PARSE_ERROR` ao enum |

### O que implementar

**`Response.cpp`**
- [x] Corrigir `"Content-Lenght"` → `"Content-Length"` no método `build()`

**`ResponseBuilder.cpp` — `handlePOST`**
- [ ] Se `route.uploadPath` estiver vazio → 403
- [ ] Extrair nome do arquivo do header `Content-Disposition` (se existir) ou gerar nome com timestamp
- [ ] Escrever `request.body` em `route.uploadPath + "/" + filename`
- [ ] Retornar 201 Created com `Location` apontando para o arquivo

**`ResponseBuilder.cpp` — `handleDELETE`**
- [ ] Resolver o path com `Router::resolvePath`
- [ ] Se arquivo não existe → 404
- [ ] Se `unlink()` falhar → 500
- [ ] Sucesso → 204 No Content (sem body)

**`ResponseBuilder.cpp` — `generateAutoindex`**
- [ ] Usar `opendir` / `readdir` para listar o diretório
- [ ] Gerar HTML simples com links para cada entrada
- [ ] Já chamado em `handleGET` quando `route.autoindex == true` e o path é um diretório

**`RequestParser.cpp` — body size**
- [ ] Adicionar parâmetro `size_t maxBodySize` ao método `parse()` (ou ao construtor)
- [ ] Em `parseBody()`, antes de acumular: se `expectedBodySize > maxBodySize` → setar `request.errorCode = 413`, retornar `PARSE_ERROR`
- [ ] Mesmo controle para chunked: acumular tamanho e checar a cada chunk

### Não toca em
`WebServer.cpp`, `CGIHandler.cpp`, `Router.cpp`

---

## Pessoa 3 — Pipeline de CGI

### Objetivo
Implementar o `CGIHandler` completo e o `handleCGI` do WebServer como arquivo separado, mais o `matchCGI` no Router.

### Arquivos sob responsabilidade

| Arquivo | Ação |
|---|---|
| `src/CGIHandler.cpp` | **Criar** — implementação completa |
| `src/WebServer-CGI.cpp` | **Criar** — método `handleCGI` da classe `WebServer` |
| `src/Router.cpp` | **Atualizar** — implementar `matchCGI()` |

### O que implementar

**`CGIHandler.cpp` — `execute()`**
- Criar dois pipes: `pipeIn[2]` (stdin do CGI) e `pipeOut[2]` (stdout do CGI)
- `fork()`:
  - **filho**: `dup2` nos pipes, chamar `buildEnvp()` e `buildArgv()`, `execve()` o interpretador com o script
  - **pai**: fechar extremidades desnecessárias dos pipes, salvar `pipeIn[1]` em `client.cgiInputFd` e `pipeOut[0]` em `client.cgiOutputFd`, salvar PID em `client.cgiPid`, setar ambos os fds como non-blocking, adicionar `cgiOutputFd` ao epoll com `EPOLLIN` e `cgiInputFd` com `EPOLLOUT`, mudar `client.state = CGI_RUNNING`

**`CGIHandler.cpp` — `buildEnv()`**
- Montar as variáveis obrigatórias do CGI/1.1:
  - `REQUEST_METHOD`, `QUERY_STRING`, `CONTENT_TYPE`, `CONTENT_LENGTH`
  - `SCRIPT_FILENAME`, `SCRIPT_NAME`, `PATH_INFO`
  - `SERVER_NAME`, `SERVER_PORT`, `SERVER_PROTOCOL`
  - Headers HTTP do request → `HTTP_*` (ex: `HTTP_HOST`)

**`WebServer-CGI.cpp` — `handleCGI(Client &client)`**
- Evento `EPOLLIN` em `cgiOutputFd`: ler em chunks e acumular em `client.cgiBuffer`; ao receber EOF, fechar o fd, chamar `waitpid()`:
  - exit 0 → parsear `cgiBuffer` (separar headers do body pela linha em branco), montar `Response`, setar `client.state = WRITING`
  - exit != 0 → `buildErrorResponse(502)`
- Evento `EPOLLOUT` em `cgiInputFd`: escrever `request.body` no pipe; ao terminar, fechar `cgiInputFd`
- Timeout (verificado em `checkTimeouts`): `kill(client.cgiPid, SIGKILL)`, `waitpid()`, `buildErrorResponse(504)`

**`Router.cpp` — `matchCGI()`**
- Iterar sobre `route.cgiHandlers` (mapa de extensão → interpretador)
- Verificar se o path do request termina com alguma das extensões
- Retornar ponteiro para a string do interpretador, ou `NULL`

### Não toca em
`ResponseBuilder.cpp`, `RequestParser.cpp`, `WebServer.cpp` (só `WebServer-CGI.cpp`)

---

## Makefile — atualizar ao final

Adicionar os dois arquivos novos às `SRCS`:

```makefile
${SRC_DIR}/WebServer.cpp \
${SRC_DIR}/WebServer-Init.cpp \
${SRC_DIR}/WebServer-CGI.cpp \
${SRC_DIR}/CGIHandler.cpp \
```

Remover os arquivos de teste (`Tester.cpp`, `Router-Tester.cpp`, `Mock-Config.cpp`) da build de produção — ou mover para um alvo `make test` separado.

---

## Ordem de integração sugerida

1. P1 entrega `WebServer-Init.cpp` funcionando (servidor sobe, aceita conexão) — sem precisar de P2 ou P3
2. P2 entrega `handleGET` completo com autoindex — P1 já pode testar com `curl`
3. P2 entrega POST e DELETE
4. P3 entrega `CGIHandler.cpp` + `matchCGI` — P1 integra chamando `execute()` em `handleRequest`
5. P3 entrega `WebServer-CGI.cpp` — loop CGI completo, testar com script `.py`
