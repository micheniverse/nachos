/* this? */

#include "syscall.h"

int
main()
{
    SpaceId newProc;
    OpenFileId input = ConsoleInput;
    OpenFileId output = ConsoleOutput;

    char prompt[3], ch, buffer[300], arg1[60], arg2[60], arg3[60], arg4[60], arg5[60], arg6[60];
    int i,error, curarg;
    char *args[6]; 
    char *argsNoFilename[5];
    int j,curIndex, lastArg;

    prompt[0] = '-';
    prompt[1] = '-';
    prompt[2] = '>';

    args[0] = arg1;
    args[1] = arg2;
    args[2] = arg3;
    args[3] = arg4;
    args[4] = arg5;
    args[5] = arg6;

    /*While loop starts the console and lasts forever*/
    while( 1 )
    {
      /*Write a greeting message*/
      Write(prompt, 3, output);

      /*the length of the command input*/

      i = 0;
      curarg = 0;
      lastArg = 0;
      /*THIS IS THE KEY BOARD LISTENER FOR A COMMAND TO BE INPUTED WITH ENTER*/
      do {
          Read(&buffer[i], 1, input);
          if (buffer[i] == ' ' || buffer[i] == '\n') {
          	curIndex = 0;
          	for(j = lastArg; j < i; j++) {
          		args[curarg][curIndex] = buffer[j];
          		curIndex++;
          	}
          	args[curarg][curIndex] = '\0'; 
            
          	lastArg = i+1;
          	curarg++;
          }


      } while(buffer[i++] != '\n' );

      args[curarg] = (char *)0;

      /*add null terminating string*/
      buffer[--i] = '\0';

      for(j=1; j<6; j++) {
      	argsNoFilename[j-1] = args[j];

      }

      if( i > 0 ) {
        newProc = Fork();
        for (i = 0; i < 300; i++) {
          buffer[i] = ' ';
        }
        if (newProc == 0) {
          error = Exec(args[0], argsNoFilename);
          if(error == -1){
            prints("There is no such scripted or function or class or anything called -> ",ConsoleOutput);
            prints(args[0],ConsoleOutput);
            prints("!\n",ConsoleOutput);
            Exit(-1);
          }
          prints("post exec", ConsoleOutput);
        }
        else {
          Join(newProc);
        }
      }
    }
}

prints(s,file)
char *s;
OpenFileId file;

{
  int count = 0;
  char *p;

  p = s;
  while (*p++ != '\0') count++;
  Write(s, count, file);  

}

