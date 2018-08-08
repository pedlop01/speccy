#include <stdio.h>
#include <string.h>
#include <iostream.h>

void chopDirectory(char *file)
{
     char *ptr;
     
     ptr = strtok(file, "/\\");
     while( ptr != NULL )
     {
            cout << ptr << endl;
            ptr = strtok(NULL, "/\\");            
     }
     
}
