
#ifndef ENTRADASORTIDA_H
#define ENTRADASORTIDA_H

#include <allegro.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>


#define LECTURA 0
#define ESCRIPTURA 1

/*
 * No description
 */
class entradaSortida
{
    private:
        int fd;
        bool opened;
        int nBytes;
	public:
		// class constructor
		entradaSortida();
		// class destructor
		~entradaSortida();
		void destroy();
		
		bool obrirFitxer(char* path, int mode);
		bool tancarFitxer();
		bool llegirEnter(int* val);
		bool escriureEnter(int enter);
		bool escriureParaula(char* cadena);
	    bool escriureEspai();
		bool escriureSaltDeLinia();
		bool llegirParaula(char* cadena);
		bool llegirNParaula(char* cadena,int ncars);
		bool llegirLinea(char* cadena);
		bool llegirTot(char* cadena);
		bool llegirReal(float* fl);
		int  longitudFitxer();
};

#endif // ENTRADASORTIDA_H

