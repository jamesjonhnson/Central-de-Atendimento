# API Fila de Atendimento em C

Este projeto é uma API HTTP simples implementada em C, utilizando as bibliotecas **libmicrohttpd**, **jansson** e **pthread** para manipulação de conexões HTTP, JSON e controle de concorrência com threads.

A API mantém uma **fila circular** de pessoas a serem chamadas e armazena também os **3 últimos chamados**.

---

## ✳️ Funcionalidades

- ✅ Adicionar pessoa à fila (`POST /chamadas`)
- ✅ Chamar próxima pessoa da fila (`DELETE /chamadas/proxima`)
- ✅ Exibir toda a fila (`GET /chamadas/mostrar_fila`)
- ✅ Listar os 3 últimos chamados (`GET /chamadas/ultimos`)

---

## 📦 Bibliotecas Necessárias

Instale as seguintes bibliotecas no seu sistema para compilar o projeto:

- [`libmicrohttpd`]
- [`jansson`]
- `pthread` (nativa em Linux/macOS, no MinGW no Windows)

---

## ⚙️ Compilação no Windows (MSYS2 + MinGW64)

Abra o terminal **MSYS2** e certifique-se de estar no ambiente `mingw64`.

Instale as bibliotecas (caso ainda não tenha):

```bash
pacman -S mingw-w64-x86_64-libmicrohttpd mingw-w64-x86_64-jansson mingw-w64-x86_64-toolchain

Compilar Com
gcc -o api_fila.exe projeto.c -I/mingw64/include -L/mingw64/lib -lmicrohttpd -ljansson -lpthread