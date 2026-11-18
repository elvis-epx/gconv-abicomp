#!/bin/bash
# Tests for the ABICOMP gconv module.  Run from the build directory.
export GCONV_PATH="$(cd "$(dirname "$0")" && pwd)"
fail=0
ok()   { echo "PASS: $1"; }
bad()  { echo "FAIL: $1"; fail=1; }
check(){ if [ "$2" == "$3" ]; then ok "$1"; else bad "$1 (got '$2', want '$3')"; fi; }

T=$(mktemp -d); trap 'rm -rf "$T"' EXIT

# Every defined byte: 0x01-0x7F and 0xA0-0xDF
for i in $(seq 1 127) $(seq 160 223); do printf "\\x$(printf %02x $i)"; done > "$T/all.abc"

iconv -f ABICOMP -t UTF-8 "$T/all.abc" > "$T/all.u8" && ok "decode all bytes" || bad "decode all bytes"
check "accented range" "$(tail -c +128 "$T/all.u8")" \
  $'\u00a0'"ÀÁÂÃÄÇÈÉÊËÌÍÎÏÑÒÓÔÕÖŒÙÚÛÜŸ¨£¦§°¡àáâãäçèéêëìíîïñòóôõöœùúûüÿßªº¿±"
iconv -f UTF-8 -t ABICOMP "$T/all.u8" | cmp -s - "$T/all.abc" && ok "round trip UTF-8" || bad "round trip UTF-8"

# Chained with module-based charsets (next/previous step is a .so: mangled ptrs)
printf 'Ação\u00a0Ñ' | iconv -f UTF-8 -t ISO-8859-1 | iconv -f ISO-8859-1 -t ABICOMP > "$T/l1.abc" \
  && ok "ISO-8859-1 -> ABICOMP" || bad "ISO-8859-1 -> ABICOMP"
check "ABICOMP -> ISO-8859-1 -> UTF-8" "$(iconv -f ABICOMP -t ISO-8859-1 "$T/l1.abc" | iconv -f ISO-8859-1 -t UTF-8)" $'Ação Ñ'
check "ABICOMP -> CP1252 (Œ)" "$(printf '\xb5\xd5\xba' | iconv -f ABICOMP -t CP1252 | od -An -tx1 | tr -d ' ')" "8c9c9f"
check "ABICOMP -> UTF-16BE" "$(printf 'A\xc6' | iconv -f ABICOMP -t UTF-16BE | od -An -tx1 | tr -d ' ')" "004100e7"
check "alias ABICOMP-BR" "$(printf '\xc6' | iconv -f ABICOMP-BR -t UTF-8)" "ç"

# Errors
printf 'ab\x85cd' | iconv -f ABICOMP -t UTF-8 >/dev/null 2>"$T/err"; rc=$?
[ $rc -ne 0 ] && grep -q 'position 2' "$T/err" && ok "invalid byte 0x85 rejected at pos 2" || bad "invalid byte: rc=$rc $(cat "$T/err")"
check "//IGNORE on input" "$(printf 'ab\x85\xe0cd' | iconv -f ABICOMP -t UTF-8//IGNORE 2>/dev/null)" "abcd"
printf 'ação€x' | iconv -f UTF-8 -t ABICOMP >/dev/null 2>"$T/err"; rc=$?
[ $rc -ne 0 ] && grep -q 'position 6' "$T/err" && ok "unmappable € rejected at byte 6" || bad "unmappable: rc=$rc $(cat "$T/err")"
check "//IGNORE on output" "$(printf 'a€b' | iconv -f UTF-8 -t ABICOMP//IGNORE 2>/dev/null)" "ab"
check "//TRANSLIT on output" "$(printf 'a€b' | iconv -f UTF-8 -t ABICOMP//TRANSLIT)" "a?b"
# //IGNORE must still exit 1, like glibc's own charsets (control: UTF-8)
printf 'a\xffb' | iconv -f UTF-8 -t UTF-16LE//IGNORE >/dev/null 2>&1; want=$?
printf 'a\x85b' | iconv -f ABICOMP -t UTF-8//IGNORE >/dev/null 2>&1; check "//IGNORE input exit status matches glibc" "$?" "$want"
printf 'a€b' | iconv -f UTF-8 -t ABICOMP//IGNORE >/dev/null 2>&1; check "//IGNORE output exit status matches glibc" "$?" "$want"
# Error raised by the *next* step (exercises input re-sync)
printf 'abc\xc6def' | iconv -f ABICOMP -t ASCII >/dev/null 2>"$T/err"; rc=$?
[ $rc -ne 0 ] && grep -q 'position 3' "$T/err" && ok "next-step error at pos 3" || bad "next-step error: rc=$rc $(cat "$T/err")"

# Large input (many buffer refills in both directions)
for i in $(seq 2000); do cat "$T/all.abc"; done > "$T/big.abc"
iconv -f ABICOMP -t UTF-16LE "$T/big.abc" | iconv -f UTF-16LE -t ABICOMP | cmp -s - "$T/big.abc" \
  && ok "large round trip ($(stat -c %s "$T/big.abc") bytes)" || bad "large round trip"

# ---------------------------------------------------------------- BRASCII
# Every defined byte: 0x01-0x7F and 0xA0-0xFF
for i in $(seq 1 127) $(seq 160 255); do printf "\\x$(printf %02x $i)"; done > "$T/all.bra"

iconv -f BRASCII -t UTF-16BE "$T/all.bra" > "$T/all.u16" && ok "BRASCII: decode all bytes" || bad "BRASCII: decode all bytes"
# Expected: ISO-8859-1, except 0xD7 -> U+0152 and 0xF7 -> U+0153
want=$(for i in $(seq 1 127) $(seq 160 255); do
         case $i in 215) printf 0152;; 247) printf 0153;; *) printf %04x $i;; esac
       done)
check "BRASCII: table = ISO-8859-1 with Œ/œ at 0xD7/0xF7" "$(od -An -tx1 -v "$T/all.u16" | tr -d ' \n')" "$want"
iconv -f BRASCII -t UTF-8 "$T/all.bra" | iconv -f UTF-8 -t BRASCII | cmp -s - "$T/all.bra" \
  && ok "BRASCII: round trip UTF-8" || bad "BRASCII: round trip UTF-8"
check "BRASCII: decode Œ œ" "$(printf '\xd7uvre \xf7' | iconv -f BRASCII -t UTF-8)" "Œuvre œ"
check "BRASCII: encode Œ œ" "$(printf 'Œœ' | iconv -f UTF-8 -t BRASCII | od -An -tx1 | tr -d ' ')" "d7f7"
check "BRASCII: alias CP3847" "$(printf 'A\xe7\xe3o' | iconv -f CP3847 -t UTF-8)" "Ação"

# Through module-based charsets: identical to Latin-1 outside 0xD7/0xF7
check "BRASCII -> ISO-8859-1" "$(printf 'A\xe7\xe3o \xa9' | iconv -f BRASCII -t ISO-8859-1 | od -An -tx1 | tr -d ' ')" "41e7e36f20a9"
check "BRASCII -> CP1252 (Œ œ)" "$(printf '\xd7\xf7' | iconv -f BRASCII -t CP1252 | od -An -tx1 | tr -d ' ')" "8c9c"
check "ABICOMP -> BRASCII" "$(printf '\xa6\xb5\xd5\xc4' | iconv -f ABICOMP -t BRASCII | od -An -tx1 | tr -d ' ')" "c7d7f7e3"

# Errors: undefined bytes, and × ÷ (which BRASCII lacks)
printf 'ab\x85cd' | iconv -f BRASCII -t UTF-8 >/dev/null 2>"$T/err"; rc=$?
[ $rc -ne 0 ] && grep -q 'position 2' "$T/err" && ok "BRASCII: invalid byte 0x85 rejected at pos 2" || bad "BRASCII invalid byte: rc=$rc $(cat "$T/err")"
printf 'a×b' | iconv -f UTF-8 -t BRASCII >/dev/null 2>"$T/err"; rc=$?
[ $rc -ne 0 ] && grep -q 'position 1' "$T/err" && ok "BRASCII: unmappable × rejected at byte 1" || bad "BRASCII unmappable: rc=$rc $(cat "$T/err")"
check "BRASCII: //TRANSLIT on output" "$(printf '2×3÷€' | iconv -f UTF-8 -t BRASCII//TRANSLIT)" "2?3??"
check "BRASCII: //IGNORE on input" "$(printf 'a\x80\x9fb' | iconv -f BRASCII -t UTF-8//IGNORE 2>/dev/null)" "ab"

# Large input
for i in $(seq 2000); do cat "$T/all.bra"; done > "$T/big.bra"
iconv -f BRASCII -t UTF-16LE "$T/big.bra" | iconv -f UTF-16LE -t BRASCII | cmp -s - "$T/big.bra" \
  && ok "BRASCII: large round trip ($(stat -c %s "$T/big.bra") bytes)" || bad "BRASCII: large round trip"

exit $fail
