/* gprintf.c */
/* originally from: http://www.efgh.com/software/gprintf.htm */

/* modified by Scott McPeak, April 2003, to use va_list instead
 * of 'const int*' for the pointer-to-argument type (for portability) */

/* Code for general_printf() */
/* Change extension to .c before compiling */

#include "gprintf.h"     /* this module */

#define BITS_PER_BYTE           8

struct parameters
{
  int number_of_output_chars;
  short minimum_field_width;
  char options;
    #define MINUS_SIGN    1
    #define RIGHT_JUSTIFY 2
    #define ZERO_PAD      4
    #define CAPITAL_HEX   8
  short edited_string_length;
  short leading_zeros;
  int (*output_function)(void *, int);
  void *output_pointer;
};

static void output_and_count(struct parameters *p, int c)
{
  if (p->number_of_output_chars >= 0)
  {
    int n = (*p->output_function)(p->output_pointer, c);
    if (n>=0) p->number_of_output_chars++;
    else p->number_of_output_chars = n;
  }
}

static void output_field(struct parameters *p, char *s)
{
  short justification_length =
    p->minimum_field_width - p->leading_zeros - p->edited_string_length;
  if (p->options & MINUS_SIGN)
  {
    if (p->options & ZERO_PAD)
      output_and_count(p, '-');
    justification_length--;
  }
  if (p->options & RIGHT_JUSTIFY)
    while (--justification_length >= 0)
      output_and_count(p, p->options & ZERO_PAD ? '0' : ' ');
  if (p->options & MINUS_SIGN && !(p->options & ZERO_PAD))
    output_and_count(p, '-');
  while (--p->leading_zeros >= 0)
    output_and_count(p, '0');
  while (--p->edited_string_length >= 0)
    output_and_count(p, *s++);
  while (--justification_length >= 0)
    output_and_count(p, ' ');
}


int general_vprintf(Gprintf_output_function output_function, 
                    void *output_pointer,
                    const char *control_string, 
                    va_list argument_pointer)
{
  struct parameters p;
  char control_char;
  p.number_of_output_chars = 0;
  p.output_function = output_function;
  p.output_pointer = output_pointer;
  control_char = *control_string++;
  while (control_char != '\0')
  {
    if (control_char == '%')
    {
      short precision = -1;
      short long_argument = 0;
      short base = 0;
      control_char = *control_string++;
      p.minimum_field_width = 0;
      p.leading_zeros = 0;
      p.options = RIGHT_JUSTIFY;
      if (control_char == '-')
      {
        p.options = 0;
        control_char = *control_string++;
      }
      if (control_char == '0')
      {
        p.options |= ZERO_PAD;
        control_char = *control_string++;
      }
      if (control_char == '*')
      {
        p.minimum_field_width = va_arg(argument_pointer, int);
        control_char = *control_string++;
      }
      else
      {
        while ('0' <= control_char && control_char <= '9')
        {
          p.minimum_field_width =
            p.minimum_field_width * 10 + control_char - '0';
          control_char = *control_string++;
        }
      }
      if (control_char == '.')
      {
        control_char = *control_string++;
        if (control_char == '*')
        {
          precision = va_arg(argument_pointer, int);
          control_char = *control_string++;
        }
        else
        {
          precision = 0;
          while ('0' <= control_char && control_char <= '9')
          {
            precision = precision * 10 + control_char - '0';
            control_char = *control_string++;
          }
        }
      }
      if (control_char == 'l')
      {
        long_argument = 1;
        control_char = *control_string++;
      }
      if (control_char == 'd')
        base = 10;
      else if (control_char == 'x')
        base = 16;
      else if (control_char == 'X')
      {
        base = 16;
        p.options |= CAPITAL_HEX;
      }
      else if (control_char == 'u')
        base = 10;
      else if (control_char == 'o')
        base = 8;
      else if (control_char == 'b')
        base = 2;
      else if (control_char == 'c')
      {
        base = -1;
        p.options &= ~ZERO_PAD;
      }
      else if (control_char == 's')
      {
        base = -2;
        p.options &= ~ZERO_PAD;
      }
      if (base == 0)  /* invalid conversion type */
      {
        if (control_char != '\0')
        {
          output_and_count(&p, control_char);
          control_char = *control_string++;
        }
      }
      else
      {
        if (base == -1)  /* conversion type c */
        {
          char c = va_arg(argument_pointer, char);
          p.edited_string_length = 1;
          output_field(&p, &c);
        }
        else if (base == -2)  /* conversion type s */
        {
          char *string;
          p.edited_string_length = 0;
          string = va_arg(argument_pointer, char*);
          while (string[p.edited_string_length] != 0)
            p.edited_string_length++;
          if (precision >= 0 && p.edited_string_length > precision)
            p.edited_string_length = precision;
          output_field(&p, string);
        }
        else  /* conversion type d, b, o or x */
        {
          unsigned long x;
          char buffer[BITS_PER_BYTE * sizeof(unsigned long) + 1];
          p.edited_string_length = 0;
          if (long_argument) 
          {
            x = va_arg(argument_pointer, unsigned long);
          }
          else if (control_char == 'd')
            x = va_arg(argument_pointer, long);
          else
            x = va_arg(argument_pointer, unsigned);
          if (control_char == 'd' && (long) x < 0)
          {
            p.options |= MINUS_SIGN;
            x = - (long) x;
          }
          do 
          {
            int c;
            c = x % base + '0';
            if (c > '9')
            {
              if (p.options & CAPITAL_HEX)
                c += 'A'-'9'-1;
              else
                c += 'a'-'9'-1;
            }
            buffer[sizeof(buffer) - 1 - p.edited_string_length++] = c;
          }
          while ((x/=base) != 0);
          if (precision >= 0 && precision > p.edited_string_length)
            p.leading_zeros = precision - p.edited_string_length;
          output_field(&p, buffer + sizeof(buffer) - p.edited_string_length);
        }
        control_char = *control_string++;
      }
    }
    else
    {
      output_and_count(&p, control_char);
      control_char = *control_string++;
    }
  }
  return p.number_of_output_chars;
}


/* ------------------ test code --------------------- */
#ifdef TEST_GPRINTF

#include <stdio.h>     /* fputc, printf, vsprintf */
#include <string.h>    /* strcmp, strlen */
#include <stdlib.h>    /* exit */


int string_output(void *extra, int ch)
{                          
  /* the 'extra' argument is a pointer to a pointer to the
   * next character to write */
  char **s = (char**)extra;

  **s = ch;     /* write */
  (*s)++;       /* advance */

  return 0;
}

int general_vsprintf(char *dest, char const *format, va_list args)
{
  char *s = dest;
  int ret;

  ret = general_vprintf(string_output, &s, format, args);
  *s = 0;

  return ret;
}


char output1[1024];    // for libc
char output2[1024];    // for this module


void expect_vector_len(int expect_len, char const *expect_output,
                       char const *format, va_list args)
{
  int len;
  static int vectors = 0;

  /* keep track of how many vectors we've tried, to make it
   * a little easier to correlate failures with the inputs
   * in this file */
  vectors++;

  /* run the generalized vsprintf */
  len = general_vsprintf(output2, format, args);

  /* compare */
  if (len!=expect_len ||
      0!=strcmp(expect_output, output2)) {
    printf("outputs differ for vector %d!\n", vectors);
    printf("  format: %s\n", format);
    printf("  expect: %s (%d)\n", expect_output, expect_len);
    printf("      me: %s (%d)\n", output2, len);
    exit(2);
  }
}


void expect_vector(char const *expect_output,
                   char const *format, va_list args)
{
  expect_vector_len(strlen(expect_output), expect_output, format, args);
}


void vector(char const *format, ...)
{
  va_list args;
  int len;

  /* run the real vsprintf */
  va_start(args, format);
  len = vsprintf(output1, format, args);
  va_end(args);

  /* test against the generalized vsprintf */
  va_start(args, format);
  expect_vector_len(len, output1, format, args);
  va_end(args);
}


int main()
{
  printf("testing gprintf...\n");

  vector("simple");
  vector("a %s more", "little");
  vector("some %4d more %s complicated %c stuff",
         33, "yikes", 'f');
  //vector("number %f floats", 3.4);

  printf("gprintf works\n");
  return 0;
}

#endif /* TEST_GPRINTF */
