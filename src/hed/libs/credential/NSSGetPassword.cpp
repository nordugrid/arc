#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <nss.h>
#include <p12.h>
#include <pk11pub.h>
#include <nspr.h>

#include <stdio.h>

#include "NSSGetPassword.h"

// The following code is introduced from mozilla/security/nss/cmd/lib/secutil.h and 
// its relevant implementation, because the code about getting password is not
// exposed to external.

#ifdef XP_UNIX
#include <termios.h>
#endif

#if defined(XP_UNIX) || defined(XP_BEOS)
#include <unistd.h>  /* for isatty() */
#endif

#if( defined(_WINDOWS) && !defined(_WIN32_WCE))
#include <conio.h>
#include <io.h>
#define QUIET_FGETS quiet_fgets
static char * quiet_fgets (char *buf, int length, FILE *input);
#else
#define QUIET_FGETS fgets
#endif

namespace AuthN {

  static void echoOff(int fd) {
#if defined(XP_UNIX)
    if (isatty(fd)) {
        struct termios tio;
        tcgetattr(fd, &tio);
        tio.c_lflag &= ~ECHO;
        tcsetattr(fd, TCSAFLUSH, &tio);
    }
#endif
  }

  static void echoOn(int fd) {
#if defined(XP_UNIX)
    if (isatty(fd)) {
        struct termios tio;
        tcgetattr(fd, &tio);
        tio.c_lflag |= ECHO;
        tcsetattr(fd, TCSAFLUSH, &tio);
    }
#endif
  }

  static char* getPassword(FILE *input, FILE *output, char *prompt,
                               PRBool (*ok)(char *)) {
#if defined(_WINDOWS)
    int isTTY = (input == stdin);
#define echoOn(x)
#define echoOff(x)
#else
    int infd  = fileno(input);
    int isTTY = isatty(infd);
#endif
    char phrase[200] = {'\0'};      /* ensure EOF doesn't return junk */

    for (;;) {
      /* Prompt for password */
      if (isTTY) {
        fprintf(output, "%s", prompt);
        fflush (output);
        echoOff(infd);
      }

      QUIET_FGETS ( phrase, sizeof(phrase), input);

      if (isTTY) {
        fprintf(output, "\n");
        echoOn(infd);
      }

      /* stomp on newline */
      phrase[PORT_Strlen(phrase)-1] = 0;

      /* Validate password */
      if (!(*ok)(phrase)) {
        /* Not weird enough */
        if (!isTTY) return 0;
        fprintf(output, "Password must be at least 8 characters long with one or more\n");
        fprintf(output, "non-alphabetic characters\n");
        continue;
      }
      return (char*) PORT_Strdup(phrase);
    }
    return NULL;
  }

  static PRBool CheckPassword(char *cp) {
    int len;
    char *end;

    len = PORT_Strlen(cp);
    if (len < 8) {
      return PR_FALSE;
    }
    end = cp + len;
    while (cp < end) {
      unsigned char ch = *cp++;
      if (!((ch >= 'A') && (ch <= 'Z')) &&
          !((ch >= 'a') && (ch <= 'z'))) {
        /* pass phrase has at least one non alphabetic in it */
        return PR_TRUE;
      }
    }
    return PR_FALSE;
  }

  static PRBool BlindCheckPassword(char *cp) {
    if (cp != NULL) {
      return PR_TRUE;
    }
    return PR_FALSE;
  }

  /* Get a password from the input terminal, without echoing */
#if defined(_WINDOWS)
  static char * quiet_fgets (char *buf, int length, FILE *input) {
    int c;
    char *end = buf;

    /* fflush (input); */
    memset (buf, 0, length);

    if (!isatty(fileno(input))) {
      return fgets(buf,length,input);
    }

    while (1) {
#if defined (_WIN32_WCE)
      c = getchar();      /* gets a character from stdin */
#else
      c = getch();        /* getch gets a character from the console */
#endif
      if (c == '\b') {
      if (end > buf)
        end--;
      }

      else if (--length > 0)
        *end++ = c;

      if (!c || c == '\n' || c == '\r')
        break;
    }

    return buf;
  }
#endif



  static char consoleName[] =  {
#ifdef XP_UNIX
    "/dev/tty"
#else
#ifdef XP_OS2
    "\\DEV\\CON"
#else
    "CON:"
#endif
#endif
  };

  static char* getPasswordString(void *arg, char *prompt) {
#ifndef _WINDOWS
    char *p = NULL;
    FILE *input, *output;

    /* open terminal */
    input = fopen(consoleName, "r");
    if (input == NULL) {
      fprintf(stderr, "Error opening input terminal for read\n");
      return NULL;
    }

    output = fopen(consoleName, "w");
    if (output == NULL) {
      fprintf(stderr, "Error opening output terminal for write\n");
      return NULL;
    }

    p = getPassword (input, output, prompt, BlindCheckPassword);

    fclose(input);
    fclose(output);

    return p;

#else
    /* Win32 version of above. opening the console may fail
       on windows95, and certainly isn't necessary.. */

    char *p = NULL;

    p = getPassword (stdin, stdout, prompt, BlindCheckPassword);
    return p;

#endif
  }

  char* nss_get_password_from_console(PK11SlotInfo* slot, PRBool retry, void *arg) {
    char prompt[255];
    char* pw = NULL;

    if(arg != NULL) pw = (char *)PORT_Strdup((char *)arg);
    if(pw != NULL) return pw;

    sprintf(prompt, "Enter Password or Pin for \"%s\":",
                         PK11_GetTokenName(slot));
    return getPasswordString(NULL, prompt);

    PR_fprintf(PR_STDERR, "Password check failed:  No password found.\n");
    return NULL;

  }
}
