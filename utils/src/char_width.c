
#include "config.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define CHAR_POINTS 2

const int char_width[]={
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     11,14,14,22,28,28,24,10,12,12,16,20,14,20,12,18,
     26,16,26,20,26,20,24,20,22,26,12,12,20,20,20,20,
     36,29,29,28,26,29,21,30,29,9,21,27,22,32,28,32,
     25,32,26,25,23,27,27,37,27,25,27,12,14,12,20,25,
     10,24,22,22,20,20,12,24,20,8,10,19,9,32,20,20,
     20,24,14,18,12,19,20,28,19,20,21,12,10,12,22,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,29,29,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,32,0,0,0,0,0,0,0,0,0,
     0,0,0,0,24,24,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,20,0,0,0,0,0,0,0,0,0
};

/***************************************************************/
/* GetStringDisplayLength:		                       */
/* inputs: string to be sized 				       */
/*         string's font size to use                           */
/* returns: length of string in gEDA points                    */
/***************************************************************/
int GetStringDisplayLength(char *str,int font_size)
{ int width=0;
  int i, len;
  len = strlen(str);
  for (i=0;i<len;i++)
      width += char_width[(int)str[i]];
  width = (font_size*width)/CHAR_POINTS;
  return width;
}
