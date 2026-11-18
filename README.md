# gconv-abicomp

[Português](#português) · [English](#english)

---

## Português

Módulo **gconv** para a glibc que adiciona suporte ao conjunto de caracteres
**ABICOMP**, o código de 8 bits definido nos anos 1980 pela Associação
Brasileira da Indústria de Computadores e Periféricos e usado em impressoras,
terminais e microcomputadores brasileiros (Elebra, Itautec, Scopus, etc.).

Depois de instalado, o ABICOMP fica disponível para `iconv(1)`, `iconv(3)` e
qualquer programa que use a glibc, convertendo de e para qualquer outra
codificação que a glibc conheça (UTF-8, ISO-8859-1, CP1252, UTF-16…).

### Codificação

| Faixa        | Conteúdo                                         |
|--------------|--------------------------------------------------|
| `0x00–0x7F`  | ASCII                                            |
| `0x80–0x9F`  | não definida                                     |
| `0xA0–0xDF`  | letras acentuadas e símbolos ([tabela](#tabela--table)) |
| `0xE0–0xFF`  | não definida                                     |

Nomes aceitos: `ABICOMP`, `ABICOMP-BR`, `CP-ABICOMP`.

### Compilação e instalação

Requisitos: gcc, make e glibc 2.34 ou mais recente (por causa do diretório
`gconv-modules.d`). Arquiteturas suportadas: x86-64, i386 e AArch64.

```sh
make
make test           # opcional
sudo make install   # instala em /usr/lib/<multiarch>/gconv e roda iconvconfig
```

Para testar sem instalar, aponte `GCONV_PATH` para o diretório de compilação:

```sh
GCONV_PATH=$PWD iconv -f ABICOMP -t UTF-8 arquivo.txt
```

Para remover: `sudo make uninstall`.

### Uso

```sh
iconv -f ABICOMP -t UTF-8 entrada.txt > saida.txt
iconv -f UTF-8 -t ABICOMP entrada.txt > saida.abc
```

Caracteres sem equivalente em ABICOMP (por exemplo `€`) geram erro. Use
`//TRANSLIT` para trocá-los por `?` ou `//IGNORE` para descartá-los:

```sh
printf 'a€b' | iconv -f UTF-8 -t ABICOMP//TRANSLIT   # a?b
```

### Observação técnica

O módulo é compilado fora da árvore da glibc, usando só o `<gconv.h>` público.
Como a glibc embaralha (*mangles*) os ponteiros de função dos módulos
carregados dinamicamente, chamar o próximo passo da cadeia exige
desembaralhá-los, e isso depende da arquitetura. Por isso só x86-64, i386 e
AArch64 são suportadas; outras arquiteturas falham na compilação.

### Licença

GNU LGPL versão 2.1 ou posterior. Veja o arquivo [LICENSE](LICENSE).

---

## English

A glibc **gconv** module that adds the **ABICOMP** character set: the 8-bit
code defined in the 1980s by the Brazilian computer industry association
(Associação Brasileira da Indústria de Computadores e Periféricos) and used by
Brazilian printers, terminals and microcomputers (Elebra, Itautec, Scopus,
etc.).

Once installed, ABICOMP is available to `iconv(1)`, `iconv(3)` and any program
that uses glibc, converting to and from every other encoding glibc knows
(UTF-8, ISO-8859-1, CP1252, UTF-16…).

### Encoding

| Range        | Contents                                         |
|--------------|--------------------------------------------------|
| `0x00–0x7F`  | ASCII                                            |
| `0x80–0x9F`  | undefined                                        |
| `0xA0–0xDF`  | accented letters and symbols ([table](#tabela--table)) |
| `0xE0–0xFF`  | undefined                                        |

Accepted names: `ABICOMP`, `ABICOMP-BR`, `CP-ABICOMP`.

### Building and installing

Requirements: gcc, make and glibc 2.34 or newer (for the `gconv-modules.d`
directory). Supported architectures: x86-64, i386 and AArch64.

```sh
make
make test           # optional
sudo make install   # installs into /usr/lib/<multiarch>/gconv and runs iconvconfig
```

To try it without installing, point `GCONV_PATH` at the build directory:

```sh
GCONV_PATH=$PWD iconv -f ABICOMP -t UTF-8 file.txt
```

To remove it: `sudo make uninstall`.

### Usage

```sh
iconv -f ABICOMP -t UTF-8 input.txt > output.txt
iconv -f UTF-8 -t ABICOMP input.txt > output.abc
```

Characters with no ABICOMP equivalent (such as `€`) are an error. Use
`//TRANSLIT` to replace them with `?`, or `//IGNORE` to drop them:

```sh
printf 'a€b' | iconv -f UTF-8 -t ABICOMP//TRANSLIT   # a?b
```

### Technical note

The module is built outside the glibc source tree, against the public
`<gconv.h>` only. Because glibc mangles the function pointers of dynamically
loaded modules, calling the next step of the conversion chain requires
demangling them, which is architecture-specific. That is why only x86-64, i386
and AArch64 are supported; other architectures fail to compile.

### License

GNU LGPL version 2.1 or later. See the [LICENSE](LICENSE) file.

---

## Tabela / Table

ABICOMP `0xA0–0xDF` (linha = dígito alto, coluna = dígito baixo /
row = high digit, column = low digit). `NBSP` = U+00A0.

|        | _0   | _1 | _2 | _3 | _4 | _5 | _6 | _7 | _8 | _9 | _A | _B | _C | _D | _E | _F |
|--------|------|----|----|----|----|----|----|----|----|----|----|----|----|----|----|----|
| **A_** | NBSP | À  | Á  | Â  | Ã  | Ä  | Ç  | È  | É  | Ê  | Ë  | Ì  | Í  | Î  | Ï  | Ñ  |
| **B_** | Ò    | Ó  | Ô  | Õ  | Ö  | Œ  | Ù  | Ú  | Û  | Ü  | Ÿ  | ¨  | £  | ¦  | §  | °  |
| **C_** | ¡    | à  | á  | â  | ã  | ä  | ç  | è  | é  | ê  | ë  | ì  | í  | î  | ï  | ñ  |
| **D_** | ò    | ó  | ô  | õ  | ö  | œ  | ù  | ú  | û  | ü  | ÿ  | ß  | ª  | º  | ¿  | ±  |
