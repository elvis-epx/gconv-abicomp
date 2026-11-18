/* glibc gconv module for the Brazilian ABICOMP and BRASCII character sets.

   ABICOMP is the 8-bit character set defined in the 1980s by the
   Associação Brasileira da Indústria de Computadores e Periféricos for
   Brazilian equipment (printers, terminals, Elebra/Itautec/Scopus...).

     0x00..0x7F  ASCII
     0x80..0x9F  undefined
     0xA0..0xDF  Latin letters with diacritics and symbols (see table)
     0xE0..0xFF  undefined

   BRASCII (ABNT NBR 9614:1986, revised as NBR 9611:1991; Star printers'
   code page 3847) is ISO-8859-1 as in its early drafts, with Œ and œ
   where the final standard put × and ÷:

     0x00..0x7F  ASCII
     0x80..0x9F  undefined
     0xA0..0xFF  as ISO-8859-1, except 0xD7 = U+0152 Œ, 0xF7 = U+0153 œ

   This module converts each of them <-> INTERNAL (glibc's UCS-4, host
   byte order), so glibc can chain it with any other character set.

   It is built out-of-tree against the public <gconv.h> only.  Because
   glibc mangles function pointers of dynamically loaded steps, calling
   the next step of the chain requires demangling, which is
   architecture-specific (see ptr_demangle below).

   Copyright (C) 2026 Elvis Pfutzenreuter.
   Licensed under the GNU LGPL v2.1 or later, see LICENSE.  */

#include <gconv.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>

/* Both charsets are ASCII below 0x80, undefined in 0x80..0x9F and differ
   only in 0xA0..0xFF, so each one is described by that range alone.  */
struct charset
{
  const char *name;		/* as in gconv-modules, with "//" */
  uint16_t to_ucs[96];		/* 0xA0..0xFF -> Unicode, 0 = undefined */
  unsigned char from_latin1[96];  /* U+00A0..U+00FF -> byte, 0 = unmappable;
				   built when the module is loaded */
};

static struct charset charsets[] =
{
  {
    "ABICOMP//",
    {
      /* 0xA0 */ 0x00A0, 0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C7, 0x00C8,
      /* 0xA8 */ 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF, 0x00D1,
      /* 0xB0 */ 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x0152, 0x00D9, 0x00DA,
      /* 0xB8 */ 0x00DB, 0x00DC, 0x0178, 0x00A8, 0x00A3, 0x00A6, 0x00A7, 0x00B0,
      /* 0xC0 */ 0x00A1, 0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E7, 0x00E8,
      /* 0xC8 */ 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF, 0x00F1,
      /* 0xD0 */ 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x0153, 0x00F9, 0x00FA,
      /* 0xD8 */ 0x00FB, 0x00FC, 0x00FF, 0x00DF, 0x00AA, 0x00BA, 0x00BF, 0x00B1,
      /* 0xE0..0xFF undefined */
    },
    { 0 }
  },
  {
    "BRASCII//",
    {
      /* 0xA0 */ 0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
      /* 0xA8 */ 0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
      /* 0xB0 */ 0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
      /* 0xB8 */ 0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
      /* 0xC0 */ 0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
      /* 0xC8 */ 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
      /* 0xD0 */ 0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x0152,
      /* 0xD8 */ 0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
      /* 0xE0 */ 0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
      /* 0xE8 */ 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
      /* 0xF0 */ 0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x0153,
      /* 0xF8 */ 0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF,
    },
    { 0 }
  },
};

#define NCHARSETS (sizeof charsets / sizeof charsets[0])

/* Fill in the reverse tables once, when the module is loaded, so that
   gconv_init never writes shared data.  */
static void __attribute__ ((constructor))
build_reverse_tables (void)
{
  for (size_t c = 0; c < NCHARSETS; ++c)
    for (int i = 0; i < 96; ++i)
      {
	uint16_t u = charsets[c].to_ucs[i];
	if (u >= 0xA0 && u <= 0xFF)
	  charsets[c].from_latin1[u - 0xA0] = 0xA0 + i;
      }
}

static uint32_t
byte_to_ucs (const struct charset *cs, unsigned char c)
{
  return c < 0x80 ? c : c >= 0xA0 ? cs->to_ucs[c - 0xA0] : 0;
}

/* Returns the byte for CH, or -1 if CS cannot represent it.  */
static int
ucs_to_byte (const struct charset *cs, uint32_t ch)
{
  if (ch < 0x80)
    return ch;
  if (ch >= 0xA0 && ch <= 0xFF)
    return cs->from_latin1[ch - 0xA0] ? cs->from_latin1[ch - 0xA0] : -1;
  /* Outside Latin-1 there are only a few characters (Œ œ Ÿ).  */
  if (ch > 0xFF && ch <= 0xFFFF)
    for (int i = 0; i < 96; ++i)
      if (cs->to_ucs[i] == ch)
	return 0xA0 + i;
  return -1;
}

/* ------------------------------------------------------------------ */
/* Pointer demangling (mirrors glibc's PTR_DEMANGLE).                  */

static inline __gconv_fct
ptr_demangle (__gconv_fct fct)
{
  uintptr_t p = (uintptr_t) fct;
#if defined __x86_64__
  /* PTR_MANGLE: xor %fs:0x30 ; rol $17  */
  __asm__ ("ror $17, %0\n\txor %%fs:0x30, %0" : "+r" (p));
#elif defined __i386__
  /* PTR_MANGLE: xor %gs:0x18 ; rol $9  */
  __asm__ ("rorl $9, %0\n\txorl %%gs:0x18, %0" : "+r" (p));
#elif defined __aarch64__
  extern uintptr_t __pointer_chk_guard;
  p ^= __pointer_chk_guard;
#else
# error "Pointer demangling not implemented for this architecture"
#endif
  return (__gconv_fct) p;
}

/* ------------------------------------------------------------------ */

enum direction { FROM_CHARSET, TO_CHARSET };

/* What a step converts: which charset, and in which direction.  */
struct step_info
{
  const struct charset *cs;
  enum direction dir;
};

static const struct step_info step_infos[][2] =
{
  { { &charsets[0], FROM_CHARSET }, { &charsets[0], TO_CHARSET } },
  { { &charsets[1], FROM_CHARSET }, { &charsets[1], TO_CHARSET } },
};

_Static_assert (sizeof step_infos / sizeof step_infos[0] == NCHARSETS,
		"step_infos must have one row per charset");

int
gconv_init (struct __gconv_step *step)
{
  const struct step_info *info = NULL;

  for (size_t c = 0; c < NCHARSETS && info == NULL; ++c)
    {
      if (strcasecmp (step->__from_name, charsets[c].name) == 0)
	info = &step_infos[c][FROM_CHARSET];
      else if (strcasecmp (step->__to_name, charsets[c].name) == 0)
	info = &step_infos[c][TO_CHARSET];
    }
  if (info == NULL)
    return __GCONV_NOCONV;

  step->__data = (void *) info;
  step->__stateful = 0;
  if (info->dir == FROM_CHARSET)
    {
      step->__min_needed_from = step->__max_needed_from = 1;
      step->__min_needed_to = step->__max_needed_to = 4;
    }
  else
    {
      step->__min_needed_from = step->__max_needed_from = 4;
      step->__min_needed_to = step->__max_needed_to = 1;
    }
  return __GCONV_OK;
}

void
gconv_end (struct __gconv_step *step)
{
  (void) step;
}

/* Core conversion loops.  Return EMPTY_INPUT, FULL_OUTPUT,
   ILLEGAL_INPUT or INCOMPLETE_INPUT.  */

typedef int (*loop_fct) (const struct charset *cs,
			 const unsigned char **inptrp, const unsigned char *inend,
			 unsigned char **outptrp, unsigned char *outend,
			 size_t *irreversible, int flags);

static int
from_charset_loop (const struct charset *cs,
		   const unsigned char **inptrp, const unsigned char *inend,
		   unsigned char **outptrp, unsigned char *outend,
		   size_t *irreversible, int flags)
{
  const unsigned char *in = *inptrp;
  unsigned char *out = *outptrp;
  int status = __GCONV_EMPTY_INPUT;

  while (in < inend)
    {
      unsigned char c = *in;
      uint32_t ch = byte_to_ucs (cs, c);

      if (ch == 0 && c != 0)
	{
	  if (!(flags & __GCONV_IGNORE_ERRORS))
	    {
	      status = __GCONV_ILLEGAL_INPUT;
	      break;
	    }
	  /* Skip it, but remember the error: like glibc's own loops,
	     report ILLEGAL_INPUT once the input is consumed so that
	     "iconv -c" exits with the same status as "iconv".  */
	  status = __GCONV_ILLEGAL_INPUT;
	  ++*irreversible;
	  ++in;
	  continue;
	}
      if (outend - out < 4)
	{
	  status = __GCONV_FULL_OUTPUT;
	  break;
	}
      memcpy (out, &ch, 4);
      out += 4;
      ++in;
    }

  *inptrp = in;
  *outptrp = out;
  return status;
}

static int
to_charset_loop (const struct charset *cs,
		 const unsigned char **inptrp, const unsigned char *inend,
		 unsigned char **outptrp, unsigned char *outend,
		 size_t *irreversible, int flags)
{
  const unsigned char *in = *inptrp;
  unsigned char *out = *outptrp;
  int status = __GCONV_EMPTY_INPUT;

  while (in < inend)
    {
      uint32_t ch;
      int b;

      if (inend - in < 4)
	{
	  status = __GCONV_INCOMPLETE_INPUT;
	  break;
	}
      if (out >= outend)
	{
	  status = __GCONV_FULL_OUTPUT;
	  break;
	}
      memcpy (&ch, in, 4);
      b = ucs_to_byte (cs, ch);
      if (b < 0)
	{
	  if (flags & __GCONV_TRANSLIT)
	    b = '?';
	  else if (flags & __GCONV_IGNORE_ERRORS)
	    {
	      status = __GCONV_ILLEGAL_INPUT;	/* see from_charset_loop */
	      ++*irreversible;
	      in += 4;
	      continue;
	    }
	  else
	    {
	      status = __GCONV_ILLEGAL_INPUT;
	      break;
	    }
	  ++*irreversible;
	}
      *out++ = b;
      in += 4;
    }

  *inptrp = in;
  *outptrp = out;
  return status;
}

/* Main entry point; follows the protocol of glibc's iconv/skeleton.c.  */
int
gconv (struct __gconv_step *step, struct __gconv_step_data *data,
       const unsigned char **inptrp, const unsigned char *inend,
       unsigned char **outbufstart, size_t *irreversible, int do_flush,
       int consume_incomplete)
{
  struct __gconv_step *next_step = step + 1;
  struct __gconv_step_data *next_data = data + 1;
  int is_last = data->__flags & __GCONV_IS_LAST;
  const struct step_info *info = step->__data;
  __gconv_fct fct = NULL;
  loop_fct loop;
  int status;

  if (!is_last)
    {
      fct = next_step->__fct;
      if (next_step->__shlib_handle != NULL)
	fct = ptr_demangle (fct);
    }

  if (do_flush)
    {
      /* Stateless: nothing to emit or reset, just pass the flush on.  */
      memset (data->__statep, 0, sizeof (*data->__statep));
      if (is_last)
	return __GCONV_OK;
      return fct (next_step, next_data, NULL, NULL, NULL, irreversible,
		  do_flush, consume_incomplete);
    }

  loop = info->dir == FROM_CHARSET ? from_charset_loop : to_charset_loop;

  unsigned char *outbuf = outbufstart != NULL ? *outbufstart : data->__outbuf;
  unsigned char *outend = data->__outbufend;

  for (;;)
    {
      const unsigned char *instart = *inptrp;
      unsigned char *outstart = outbuf;
      size_t lirreversible = 0;

      status = loop (info->cs, inptrp, inend, &outbuf, outend,
		     &lirreversible, data->__flags);

      /* Called from mbrtowc & co. with a caller-supplied buffer.  */
      if (outbufstart != NULL)
	{
	  *outbufstart = outbuf;
	  *irreversible += lirreversible;
	  return status;
	}

      ++data->__invocation_counter;

      if (is_last)
	{
	  data->__outbuf = outbuf;
	  *irreversible += lirreversible;
	  break;
	}

      if (outbuf > outstart)
	{
	  const unsigned char *outerr = data->__outbuf;
	  int result;

	  result = fct (next_step, next_data, &outerr, outbuf, NULL,
			irreversible, 0, consume_incomplete);

	  if (result != __GCONV_EMPTY_INPUT)
	    {
	      if (outerr != outbuf)
		{
		  /* The next step did not consume all our output.  Redo
		     the conversion from the start, stopping exactly where
		     the next step stopped, so *inptrp is accurate.  */
		  unsigned char *redo_out = outstart;
		  size_t redo_irrev = 0;

		  *inptrp = instart;
		  loop (info->cs, inptrp, inend, &redo_out,
			(unsigned char *) outerr, &redo_irrev, data->__flags);
		  lirreversible = redo_irrev;
		}
	      /* Propagate the next step's status (e.g. its output is
		 full, or it hit an illegal character).  */
	      status = result;
	    }
	  else if (status == __GCONV_FULL_OUTPUT)
	    /* The next step drained our whole buffer; go again.  */
	    status = __GCONV_OK;
	}

      *irreversible += lirreversible;

      if (status != __GCONV_OK)
	break;
      outbuf = data->__outbuf;
    }

  return status;
}
