
#include "entradasortida.h" // class's header file

// class constructor
entradaSortida::entradaSortida()
{
	opened = false;
}

// class destructor
entradaSortida::~entradaSortida()
{
}

void entradaSortida::destroy()
{
}

///////////////////////////////////////////// FUNCIONS DE LLEGIR/ESCRIURE ENTRADA ///////////////////////////////////////////////////

bool entradaSortida::obrirFitxer(char* path, int mode)
{
   if (opened)
      tancarFitxer();

   if (mode == LECTURA)
      fd = open(path, O_RDONLY);
   else if (mode == ESCRIPTURA)
   {
      fd = open(path, O_RDWR | O_TRUNC | O_CREAT, 0xFFF); // 0xFFF son els permisos, ho fara be en linux?
   }
   if (fd>=0)
      opened = true;

   return(fd>=0);
}

bool entradaSortida::tancarFitxer()
{
   if (close(fd)==-1)
      return(false);
   
   opened = false;
   return(true);
}

bool entradaSortida::llegirEnter(int* val)
{
char c='a';
char cadena[80];
int i=0;

nBytes = read(fd,&c,1);
if (nBytes<=0) return(false);
while(c<=32)
{
	nBytes = read(fd,&c,1);
	if (nBytes<=0) return(false);
}

while(c>32)
{
	cadena[i++]=c;
	nBytes = read(fd,&c,1);
    if (nBytes<=0) return(false);
}
cadena[i]='\0';
*val=atoi(cadena);
return(true);
}

bool entradaSortida::escriureEnter(int enter)
{
char cadena[8];

sprintf(cadena,"%d",enter);
nBytes = write(fd,cadena,strlen(cadena));
return (nBytes != -1);
}


bool entradaSortida::escriureParaula(char* cadena)
{
nBytes = write(fd,cadena,strlen(cadena));
return (nBytes != -1);
}

bool entradaSortida::escriureSaltDeLinia()
{
char str[3];
sprintf(str,"\n");
nBytes = escriureParaula(str);
return (nBytes!=-1);
}

bool entradaSortida::escriureEspai()
{
char str[3];
sprintf(str," ");
nBytes = escriureParaula(str);
return (nBytes!=-1);
}

bool entradaSortida::llegirParaula(char* cadena)
{
char c=0;
int i=0;

while(c<=32)
{
	nBytes = read(fd,&c,1);
	if (nBytes<=0) return(false);
}


while(c>32)
{
	cadena[i++]=c;
	nBytes = read(fd,&c,1);
	if (nBytes<=0) return(false);
}

cadena[i]='\0';
return(true);
}

bool entradaSortida::llegirNParaula(char* cadena,int ncars)
{
char c=0;
int i=0;

while(c<=32)
{
	nBytes = read(fd,&c,1);
    if (nBytes<=0) return(false);
}

while(c>=32 && i<ncars)
{
	cadena[i++]=c;
	nBytes = read(fd,&c,1);
    if (nBytes<=0) return(false);
}

cadena[i]='\0';
return(true);
}

bool entradaSortida::llegirLinea(char* cadena)
{
int  i = 0;
char c = 0;

   while( c != '\n' )
   {
      nBytes = read(fd, &c, 1);
      if( nBytes <= 0 )
         return false;
      cadena[i] = c;
      i++;
   }
   cadena[i - 1] = '\0';

return true;
}

bool entradaSortida::llegirTot(char* cadena)
{
int  i = 0;
char c = 0;

   while( (nBytes = read(fd, &c, 1)) )
   {
      cadena[i] = c;
      if( cadena[i] == '\\' )
      {
         read(fd, &c, 1);
         if( c == 'n' )
            cadena[i] = '\n';
         else
            cadena[i] = '\t';
      }
      i++;
   }
   cadena[i] = '\0';
   
   if( nBytes < 0 )
      return false;

return true;
}


bool entradaSortida::llegirReal(float* fl)
{
char c='a';
char cadena[80];
int i=0;

nBytes = read(fd,&c,1);
if (nBytes<=0) return(false);
while(c<=32)
{
	nBytes = read(fd,&c,1);
	if (nBytes<=0) return(false);
}

while(c>32)
{
	cadena[i++]=c;
	nBytes = read(fd,&c,1);
	if (nBytes<=0) return(false);
}
cadena[i]='\0';
*fl = atof(cadena);
return(true);
}

int entradaSortida::longitudFitxer()
{
int res;

    res = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    
return res;
}
