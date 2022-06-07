#include "csapp.h"
#include <stdio.h>
#include "libtomcrypt/tomcrypt_hash.h"

char *hashtostr(unsigned char tmp[20], char *hashString)
{
  int i;
  sprintf(hashString, "%2x", tmp[0]);
  for (i = 1; i < 20; i++)
    sprintf(hashString, "%s %2x", hashString, tmp[i]);
  return hashString;
}

int main(void)
{
  char *buf, *p;
  char msg[MAXLINE], dificultyString[MAXLINE], content[MAXLINE];
  int dificulty = 0;
  unsigned long k = 0;
  unsigned char tmp[20];
  hash_state md;
  char hashString[60]; // af 1a 23 ... etc usarei 2chars e um espaço para cada byte

  /* Extract the two arguments */
  if ((buf = getenv("QUERY_STRING")) != NULL)
  {
    p = strchr(buf, '&');
    *p = '\0';

    strcpy(msg, buf);
    strcpy(dificultyString, p + 1);
    dificulty = atoi(dificultyString);

    // dificulty -> Inteiro que representa a dificuldade
    // msg -> Stirng com a mensagem passada como primeiro argumento
    char *msgStr = (char *)malloc(strlen(msg) + 10 + 1);

    while (1)
    {
// TENHO a certeza que alocei bem memoria para sprintf
#pragma GCC diagnostic ignored "-Wformat-overflow"
      sprintf(msgStr, "%s%lu", msg, k);

      sha1_init(&md);
      sha1_process(&md, (unsigned char *)msgStr, (unsigned long)strlen(msgStr));
      sha1_done(&md, tmp);

            int encontrou = 0;
      for (int i = 0; i < dificulty; i++)
      {
        if (0 == tmp[i])
          encontrou++;
      }

      if (encontrou == dificulty)
      {
        // printf("Done! k = %lu\n", k);
        // for (int i = 0; i < 20; i++)
        // printf("%x ", tmp[i]);
        // printf("\n");
        break;
      }
      // if (k % 5000000 == 0)
      //  printf("searching %lu\n", k);

      k++;
    }
  }

  hashtostr(tmp, hashString);
  /* Make the response body */
  sprintf(content, "Welcome to proofofwork.com: ");
  sprintf(content, "%sTHE Internet HASH portal.\r\n<p>", content);
  sprintf(content, "%sHASH:%s // Nonce = %d\r\n<p>", content, hashString, k);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  /* Generate the HTTP response */
  /* Paul Crocker - changed so that headers always produced on the parent process */
  // printf("Content-length: %d\r\n", (int)strlen(content));
  // printf("Content-type: text/html\r\n\r\n");

  printf("%s", content);
  fflush(stdout);
  exit(0);
}
/* $end adder */
