// Class automatically generated by Dev-C++ New Class wizard

#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <conio.h>
#include <map.h>
#include <string>

using namespace std;

/*
 * No description
 */
typedef class symbolTable* symbolTablePtr;
class symbolTable
{
    private:
       map<string, unsigned long*> table;
       
	public:
        symbolTable();   		// class constructor		
		~symbolTable();         // class destructor

      inline unsigned long* GetPointerByName(string name) { assert(table.count(name) == 1); return table[name]; }
      void addVarToSymbolTable(const char* var_name, unsigned long* address)
      {     
          string varName(var_name);
          assert(table.count(varName) == 0);
          table[varName] = address;
      }
      
      inline void ClearSymbolTable() { table.clear(); }
      
      void EraseElement(string key);
      
      void DumpSymbolTable()
      {
           printf(" Symbol Table Dump \n");
           printf(" ================================================ \n");;
           for( map<string, unsigned long*>::iterator it = table.begin() ;
                it != table.end() ; it++ )
           {
                printf(" Var name = %s  | Address = %u \n",  it->first.c_str(), it->second);
           }
           printf(" ================================================ \n");          
      }
};

#endif // SYMBOLTABLE_H
