/* Bug 1190: EOF conditions are supposed to be sticky.
   Copyright (C) 2018 Free Software Foundation.
   Copying and distribution of this file, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved. This file is offered as-is,
   without any warranty.  */

/* ISO C1999 specification of fgetc:

       #include <stdio.h>
       int fgetc (FILE *stream);

   Description

     If the end-of-file indicator for the input stream pointed to by
     stream is not set and a next character is present, the fgetc
     function obtains that character as an unsigned char converted to
     an int and advances the associated file position indicator for
     the stream (if defined).

   Returns

     If the end-of-file indicator for the stream is set, or if the
     stream is at end-of-file, the end-of-file indicator for the
     stream is set and the fgetc function returns EOF. Otherwise, the
     fgetc function returns the next character from the input stream
     pointed to by stream. If a read error occurs, the error indicator
     for the stream is set and the fgetc function returns EOF.

   The requirement to return EOF "if the end-of-file indicator for the
   stream is set" was new in C99; the language in the 1989 edition of
   the standard was ambiguous.  Historically, BSD-derived Unix always
   had the C99 behavior, whereas in System V fgetc would attempt to
   call read() again before returning EOF again.  Prior to version 2.28,
   glibc followed the System V behavior even though this does not
   comply with C99.

   See
   <https://sourceware.org/bugzilla/show_bug.cgi?id=1190>,
   <https://sourceware.org/bugzilla/show_bug.cgi?id=19476>,
   and the thread at
   <https://sourceware.org/ml/libc-alpha/2012-09/msg00343.html>
   for more detail.  */

#include <support/tty.h>
#include <support/check.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define XWRITE(fd, s, msg) do {                         \
    if (write (fd, s, sizeof s - 1) != sizeof s - 1)    \
      {                                                 \
        perror ("write " msg);                          \
        return 1;                                       \
      }                                                 \
  } while (0)

int
do_test (void)
{
  /* The easiest way to set up the conditions under which you can
     notice whether the end-of-file indicator is sticky, is with a
     pseudo-tty.  This is also the case which applications are most
     likely to care about.  And it avoids any question of whether and
     how it is legitimate to access the same physical file with two
     independent FILE objects.  */
  int outer_fd, inner_fd;
  FILE *fp;

  support_openpty (&outer_fd, &inner_fd, 0, 0, 0);
  fp = fdopen (inner_fd, "r+");
  if (!fp)
    {
      perror ("fdopen");
      return 1;
    }

  XWRITE (outer_fd, "abc\n\004", "first line + EOF");
  TEST_COMPARE (fgetc (fp), 'a');
  TEST_COMPARE (fgetc (fp), 'b');
  TEST_COMPARE (fgetc (fp), 'c');
  TEST_COMPARE (fgetc (fp), '\n');
  TEST_COMPARE (fgetc (fp), EOF);

  TEST_VERIFY_EXIT (feof (fp));
  TEST_VERIFY_EXIT (!ferror (fp));

  XWRITE (outer_fd, "d\n", "second line");

  /* At this point, there is a new full line of input waiting in the
     kernelside input buffer, but we should still observe EOF from
     stdio, because the end-of-file indicator has not been cleared.  */
  TEST_COMPARE (fgetc (fp), EOF);

  /* Clearing EOF should reveal the next line of input.  */
  clearerr (fp);
  TEST_COMPARE (fgetc (fp), 'd');
  TEST_COMPARE (fgetc (fp), '\n');

  fclose (fp);
  close (outer_fd);
  return 0;
}

#include <support/test-driver.c>
