#include <stdio.h>
#include <ctype.h>

const unsigned long hash(const char *str) {
    unsigned long hash = 5381;  
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

int main() {
  char options[] = "-lg:ann\n-lg:phprior\n-decoder\n-fe:inputrate\n-fe:numfilters\n-fe:numceps\n-fe:lowfreq\n-fe:hifreq\n-fe:framestep\n-fe:framelen\n-fe:preemph\n-fe:lifter\n-vt:hmm\n-vt:lookahead\n-vt:gramfact\n-vt:insweight\n";
  char option[100];
  char name[100];
  char *p, *q, *r;
  
  printf("%s\n", options);

  r = name;
  *r++ = 'C';
  *r++ = 'O';
  *r++ = 'N';
  *r++ = 'F';
  *r++ = '_';
  
  p = options;
  q = option;
  while(*p!='\0') {
    if(*p=='\n') {
      *q = '\0';
      *r = '\0';
      p++;
      //printf("oprion: %s, name: %s, hash: %ld\n", option, name, hash(option));
      printf("#define %s %ld /* %s */\n", name, hash(option), option);
      q = option;
      r = name+5;
    }
    *q++ = *p;
    if(*p != '-') {
      if(*p == ':') *r = '_';
      else *r = toupper(*p);
      r++;
    }
    p++;
  }
  
  return 0;
}
