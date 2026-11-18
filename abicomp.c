/* glibc gconv module for the ABICOMP character set.

   ABICOMP is the 8-bit character set defined in the 1980s by the
   Associação Brasileira da Indústria de Computadores e Periféricos for
   Brazilian equipment (printers, terminals, Elebra/Itautec/Scopus...).

     0x00..0x7F  ASCII
     0x80..0x9F  undefined
     0xA0..0xDF  Latin letters with diacritics and symbols (see table)
     0xE0..0xFF  undefined

   This module converts ABICOMP <-> INTERNAL (glibc's UCS-4, host byte
   order), so glibc can chain it with any other character set.

   It is built out-of-tree against the public <gconv.h> only.  Because
   glibc mangles function pointers of dynamically loaded steps, calling
   the next step of the chain requires demangling, which is
   architecture-specific (see ptr_demangle below).

   Copyright (C) 2026 Elvis Pfutzenreuter.  MIT License, see LICENSE.  */

#include <gconv.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>

#define CHARSET_NAME "ABICOMP//"

/* ABICOMP 0xA0..0xFF -> Unicode.  0 = undefined.  */
static const uint16_t to_ucs[96] =
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
};

/* Unicode U+00A0..U+00FF -> ABICOMP, 0 = unmappable.  Built at init.  */
static unsigned char from_latin1[96];

static int
ucs_to_abicomp (uint32_t ch)
{
  if (ch < 0x80)
    return ch;
  if (ch >= 0xA0 && ch <= 0xFF)
    return from_latin1[ch - 0xA0] ? from_latin1[ch - 0xA0] : -1;
  switch (ch)
    {
    case 0x0152: return 0xB5;	/* Œ */
    case 0x0153: return 0xD5;	/* œ */
    case 0x0178: return 0xBA;	/* Ÿ */
    }
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

enum direction { FROM_ABICOMP = 1, TO_ABICOMP = 2 };

int
gconv_init (struct __gconv_step *step)
{
  enum direction dir;

  if (strcasecmp (step->__from_name, CHARSET_NAME) == 0)
    dir = FROM_ABICOMP;
  else if (strcasecmp (step->__to_name, CHARSET_NAME) == 0)
    dir = TO_ABICOMP;
  else
    return __GCONV_NOCONV;

  if (from_latin1[0] == 0)
    for (int i = 0; i < 96; ++i)
      if (to_ucs[i] >= 0xA0 && to_ucs[i] <= 0xFF)
	from_latin1[to_ucs[i] - 0xA0] = 0xA0 + i;

  step->__data = (void *) (uintptr_t) dir;
  step->__stateful = 0;
  if (dir == FROM_ABICOMP)
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

static int
from_abicomp_loop (const unsigned char **inptrp, const unsigned char *inend,
		   unsigned char **outptrp, unsigned char *outend,
		   size_t *irreversible, int flags)
{
  const unsigned char *in = *inptrp;
  unsigned char *out = *outptrp;
  int status = __GCONV_EMPTY_INPUT;

  while (in < inend)
    {
      unsigned char c = *in;
      uint32_t ch = c < 0x80 ? c : c >= 0xA0 ? to_ucs[c - 0xA0] : 0;

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
to_abicomp_loop (const unsigned char **inptrp, const unsigned char *inend,
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
      b = ucs_to_abicomp (ch);
      if (b < 0)
	{
	  if (flags & __GCONV_TRANSLIT)
	    b = '?';
	  else if (flags & __GCONV_IGNORE_ERRORS)
	    {
	      status = __GCONV_ILLEGAL_INPUT;	/* see from_abicomp_loop */
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
  __gconv_fct fct = NULL;
  int (*loop) (const unsigned char **, const unsigned char *,
	       unsigned char **, unsigned char *, size_t *, int);
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

  loop = (uintptr_t) step->__data == FROM_ABICOMP
	 ? from_abicomp_loop : to_abicomp_loop;

  unsigned char *outbuf = outbufstart != NULL ? *outbufstart : data->__outbuf;
  unsigned char *outend = data->__outbufend;

  for (;;)
    {
      const unsigned char *instart = *inptrp;
      unsigned char *outstart = outbuf;
      size_t lirreversible = 0;

      status = loop (inptrp, inend, &outbuf, outend, &lirreversible,
		     data->__flags);

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
		  loop (inptrp, inend, &redo_out, (unsigned char *) outerr,
			&redo_irrev, data->__flags);
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
