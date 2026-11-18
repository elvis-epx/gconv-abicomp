/* Exercise the installed ABICOMP module through the iconv(3) C API.  */
#include <errno.h>
#include <iconv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(cond, ...) do { \
    if (cond) printf ("PASS: "); else { printf ("FAIL: "); ++failures; } \
    printf (__VA_ARGS__); putchar ('\n'); } while (0)

/* Convert in one shot with a large buffer.  Returns iconv()'s result,
   leaves errno, output length and bytes consumed in *outlen / *used.  */
static size_t
conv (const char *to, const char *from, const char *in, size_t inlen,
      char *out, size_t outsize, size_t *outlen, size_t *used)
{
  iconv_t cd = iconv_open (to, from);
  if (cd == (iconv_t) -1)
    { perror ("iconv_open"); exit (1); }
  char *ip = (char *) in, *op = out;
  size_t il = inlen, ol = outsize;
  size_t r = iconv (cd, &ip, &il, &op, &ol);
  int e = errno;
  if (r != (size_t) -1)
    iconv (cd, NULL, NULL, &op, &ol);	/* flush */
  *outlen = op - out;
  *used = ip - in;
  iconv_close (cd);
  errno = e;
  return r;
}

int
main (void)
{
  char out[4096];
  size_t olen, used, r;

  /* 1. Basic decode / encode.  */
  const char abc[] = "A\xc6\xc4o \xb5uvre \xdf\xdc";
  r = conv ("UTF-8", "ABICOMP", abc, strlen (abc), out, sizeof out, &olen, &used);
  CHECK (r == 0 && olen == strlen ("Ação Œuvre ±ª")
	 && memcmp (out, "Ação Œuvre ±ª", olen) == 0,
	 "decode to UTF-8: \"%.*s\"", (int) olen, out);

  const char *u8 = "Ação Œuvre ±ª";
  r = conv ("ABICOMP", "UTF-8", u8, strlen (u8), out, sizeof out, &olen, &used);
  CHECK (r == 0 && olen == strlen (abc) && memcmp (out, abc, olen) == 0,
	 "encode from UTF-8 (%zu bytes)", olen);

  /* 2. Direct to/from wchar_t (INTERNAL-like) and UCS-4.  */
  r = conv ("WCHAR_T", "ABICOMP", "\xc6", 1, out, sizeof out, &olen, &used);
  wchar_t wc; memcpy (&wc, out, sizeof wc);
  CHECK (r == 0 && olen == sizeof (wchar_t) && wc == 0xE7, "decode to WCHAR_T: U+%04X", (unsigned) wc);
  r = conv ("ABICOMP", "UCS-4BE", "\0\0\x01\x53", 4, out, sizeof out, &olen, &used);
  CHECK (r == 0 && olen == 1 && (unsigned char) out[0] == 0xD5, "encode U+0153 from UCS-4BE: 0x%02X", (unsigned char) out[0]);

  /* 3. Illegal input byte: EILSEQ, input pointer left at the bad byte.  */
  r = conv ("UTF-8", "ABICOMP", "ab\x85" "cd", 5, out, sizeof out, &olen, &used);
  CHECK (r == (size_t) -1 && errno == EILSEQ && used == 2 && olen == 2,
	 "invalid byte 0x85 -> EILSEQ at offset %zu", used);

  /* 4. Unmappable char on output: EILSEQ at its UTF-8 offset.  */
  r = conv ("ABICOMP", "UTF-8", "a\xe2\x82\xac" "b", 5, out, sizeof out, &olen, &used);
  CHECK (r == (size_t) -1 && errno == EILSEQ && used == 1 && olen == 1,
	 "unmappable EURO SIGN -> EILSEQ at offset %zu", used);

  /* 5. //IGNORE: bad chars skipped, all valid output produced and all input
	consumed, but iconv() still returns -1/EILSEQ (glibc semantics).  */
  r = conv ("ABICOMP//IGNORE", "UTF-8", "a\xe2\x82\xac" "b", 5, out, sizeof out, &olen, &used);
  CHECK (r == (size_t) -1 && errno == EILSEQ && olen == 2 && memcmp (out, "ab", 2) == 0 && used == 5,
	 "//IGNORE on output: \"%.*s\" (r=%zd)", (int) olen, out, (ssize_t) r);
  r = conv ("UTF-8//IGNORE", "ABICOMP", "a\x90\xe0" "b", 4, out, sizeof out, &olen, &used);
  CHECK (r == (size_t) -1 && errno == EILSEQ && olen == 2 && memcmp (out, "ab", 2) == 0 && used == 4,
	 "//IGNORE on input: \"%.*s\" (r=%zd)", (int) olen, out, (ssize_t) r);

  /* 6. //TRANSLIT: irreversible count returned.  */
  r = conv ("ABICOMP//TRANSLIT", "UTF-8", "a\xe2\x82\xac" "b", 5, out, sizeof out, &olen, &used);
  CHECK (r == 1 && olen == 3 && memcmp (out, "a?b", 3) == 0,
	 "//TRANSLIT: \"%.*s\", %zd irreversible", (int) olen, out, (ssize_t) r);

  /* 7. E2BIG with tiny output buffers, resuming until done, both directions
	and through a module-based charset (ISO-8859-1, exercises chaining).  */
  static const char *const targets[] = { "UTF-8", "ISO-8859-1", "UTF-16LE" };
  char big[200 * 191];
  size_t bl = 0;
  for (int k = 0; k < 200; ++k)
    for (int c = 1; c < 256; ++c)
      if (c < 0x80 || (c >= 0xA0 && c < 0xE0))
	big[bl++] = c;

  for (int t = 0; t < 3; ++t)
    {
      const char *tgt = targets[t];
      iconv_t to = iconv_open (tgt, "ABICOMP"), back = iconv_open ("ABICOMP", tgt);
      /* ISO-8859-1 lacks Œ œ Ÿ: the next step (ISO8859-1.so) rejects them
	 with EILSEQ and our module must report the exact input position;
	 the loop below then skips the offending byte.  */
      static char mid[200000], fin[200000];
      char *ip = big, *op = mid; size_t il = bl, e2big = 0, skipped = 0;
      while (il > 0)
	{
	  size_t ol = 3;		/* deliberately tiny */
	  char *before = op;
	  if (iconv (to, &ip, &il, &op, &ol) == (size_t) -1 && errno != E2BIG)
	    { if (errno == EILSEQ && t == 1 && strchr ("\xb5\xd5\xba", *ip)) { ++ip; --il; ++skipped; continue; }
	      perror ("iconv"); break; }
	  if (errno == E2BIG) ++e2big;
	  if (op == before && il > 0 && errno == E2BIG && ol == 3) break; /* no progress */
	}
      size_t midlen = op - mid;
      ip = mid; il = midlen; op = fin;
      while (il > 0)
	{
	  size_t ol = 1;
	  if (iconv (back, &ip, &il, &op, &ol) == (size_t) -1 && errno != E2BIG)
	    { perror ("iconv back"); break; }
	}
      size_t finlen = op - fin;
      int ok;
      if (t == 1)
	{
	  /* Œ œ Ÿ were skipped; everything else must round-trip.  */
	  static char want[200000];
	  size_t wl = 0;
	  for (size_t i = 0; i < bl; ++i)
	    if ((unsigned char) big[i] != 0xB5 && (unsigned char) big[i] != 0xD5
		&& (unsigned char) big[i] != 0xBA)
	      want[wl++] = big[i];
	  ok = finlen == wl && memcmp (fin, want, wl) == 0;
	}
      else
	ok = finlen == bl && memcmp (fin, big, bl) == 0;
      if (t == 1)
	ok = ok && skipped == 600;
      CHECK (ok, "tiny-buffer round trip via %s: %zu bytes, %zu E2BIG resumptions, %zu EILSEQ skips",
	     tgt, bl, e2big, skipped);
      iconv_close (to); iconv_close (back);
    }

  /* 8. BRASCII, sharing the module with ABICOMP in one process.  */
  const char bra[] = "A\xe7\xe3o \xd7uvre \xf7";
  r = conv ("UTF-8", "BRASCII", bra, strlen (bra), out, sizeof out, &olen, &used);
  CHECK (r == 0 && olen == strlen ("Ação Œuvre œ")
	 && memcmp (out, "Ação Œuvre œ", olen) == 0,
	 "BRASCII decode to UTF-8: \"%.*s\"", (int) olen, out);
  r = conv ("BRASCII", "ABICOMP", abc, strlen (abc), out, sizeof out, &olen, &used);
  CHECK (r == 0 && olen == strlen (abc)
	 && memcmp (out, "A\xe7\xe3o \xd7uvre \xb1\xaa", olen) == 0,
	 "ABICOMP -> BRASCII (%zu bytes)", olen);
  r = conv ("BRASCII", "UTF-8", "a\xc3\x97" "b", 4, out, sizeof out, &olen, &used);
  CHECK (r == (size_t) -1 && errno == EILSEQ && used == 1 && olen == 1,
	 "BRASCII: unmappable MULTIPLICATION SIGN -> EILSEQ at offset %zu", used);

  /* 9. Many open/close cycles (module refcounting, no leaks/crashes).  */
  int okc = 1;
  for (int i = 0; i < 10000 && okc; ++i)
    {
      iconv_t cd = iconv_open (i & 1 ? "ABICOMP" : "ISO-8859-1", i & 1 ? "ISO-8859-1" : "ABICOMP");
      okc = cd != (iconv_t) -1;
      if (okc) iconv_close (cd);
    }
  CHECK (okc, "10000 iconv_open/iconv_close cycles");

  printf ("%s\n", failures ? "SOME TESTS FAILED" : "ALL TESTS PASSED");
  return failures != 0;
}
