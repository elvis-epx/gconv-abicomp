# gconv-abicomp

[Português](#português) · [English](#english)

---

## Português

Módulo **gconv** para a glibc que adiciona suporte a dois conjuntos de
caracteres brasileiros de 8 bits usados em impressoras, terminais e
microcomputadores dos anos 1980 (Elebra, Itautec, Scopus, etc.):

- **ABICOMP**, definido pela Associação Brasileira da Indústria de
  Computadores e Periféricos;
- **BRASCII**, a norma ABNT NBR 9614:1986 (revista como NBR 9611:1991),
  também chamada de página de código 3847 nas impressoras Star.

Depois de instalados, ambos ficam disponíveis para `iconv(1)`, `iconv(3)` e
qualquer programa que use a glibc, convertendo de e para qualquer outra
codificação que a glibc conheça (UTF-8, ISO-8859-1, CP1252, UTF-16…).

### Codificação

ABICOMP:

| Faixa        | Conteúdo                                         |
|--------------|--------------------------------------------------|
| `0x00–0x7F`  | ASCII                                            |
| `0x80–0x9F`  | não definida                                     |
| `0xA0–0xDF`  | letras acentuadas e símbolos ([tabela](#tabela--table)) |
| `0xE0–0xFF`  | não definida                                     |

BRASCII:

| Faixa        | Conteúdo                                         |
|--------------|--------------------------------------------------|
| `0x00–0x7F`  | ASCII                                            |
| `0x80–0x9F`  | não definida                                     |
| `0xA0–0xFF`  | igual à ISO-8859-1, exceto `0xD7` = `Œ` (U+0152) e `0xF7` = `œ` (U+0153), no lugar de `×` e `÷` |

Nomes aceitos: `ABICOMP`, `ABICOMP-BR`, `CP-ABICOMP`; `BRASCII`, `CP3847`.

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
iconv -f BRASCII -t UTF-8 entrada.txt > saida.txt
```

Caracteres sem equivalente no conjunto de destino (por exemplo `€`, ou `×`
em BRASCII) geram erro. Use
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

A glibc **gconv** module that adds two Brazilian 8-bit character sets used by
1980s printers, terminals and microcomputers (Elebra, Itautec, Scopus, etc.):

- **ABICOMP**, defined by the Brazilian computer industry association
  (Associação Brasileira da Indústria de Computadores e Periféricos);
- **BRASCII**, the ABNT NBR 9614:1986 standard (revised as NBR 9611:1991),
  also known as code page 3847 on Star printers.

Once installed, both are available to `iconv(1)`, `iconv(3)` and any program
that uses glibc, converting to and from every other encoding glibc knows
(UTF-8, ISO-8859-1, CP1252, UTF-16…).

### Encoding

ABICOMP:

| Range        | Contents                                         |
|--------------|--------------------------------------------------|
| `0x00–0x7F`  | ASCII                                            |
| `0x80–0x9F`  | undefined                                        |
| `0xA0–0xDF`  | accented letters and symbols ([table](#tabela--table)) |
| `0xE0–0xFF`  | undefined                                        |

BRASCII:

| Range        | Contents                                         |
|--------------|--------------------------------------------------|
| `0x00–0x7F`  | ASCII                                            |
| `0x80–0x9F`  | undefined                                        |
| `0xA0–0xFF`  | same as ISO-8859-1, except `0xD7` = `Œ` (U+0152) and `0xF7` = `œ` (U+0153) instead of `×` and `÷` |

Accepted names: `ABICOMP`, `ABICOMP-BR`, `CP-ABICOMP`; `BRASCII`, `CP3847`.

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
iconv -f BRASCII -t UTF-8 input.txt > output.txt
```

Characters with no equivalent in the target set (such as `€`, or `×` in
BRASCII) are an error. Use
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
