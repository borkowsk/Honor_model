//#include <Windows.h> //Całkiem niepotrzebne
#include <iostream>
#include <cassert>
#include <direct.h>
#include "INCLUDE/wb_ptr.hpp"
#include "MISCCLASSES/TabelaTabDeli.h"
#include "SYMSHELL/symshell.h"
#include "SYMSHELL/sshutils.hpp"

const char* TAB_MARKER="$TAB";
const unsigned SubNameSize=2048;
extern "C" int WB_error_enter_before_clean=1;


using namespace std; //úatwe u┐ycie nazw z przestrzeni "std"


bool Komunikat(const char* Bufor,const char* Caption);//Funkcja wyświetlana ważnego komunikatu
bool Podaj_nazwe_pliku(char* Bufor,unsigned size,const char* Monit,const char* FILTER
					   ="All\0*.*\0LOG\0*.log\0DAT\0*.dat\0TXT\0*.txt\0\0");// Funkcja wczytywania nazwy pliku - w SPS ale cały nagłowek niepotrzebny bo tu nie ma żadnego modelu
char InputFile[10000];

int SCALE_WIDTH=40;  //Jaki rozmiar skali? Może być 0 wtedy jej nie ma
bool PRINT_SUBNAMES=true;//Czy napis pod spodem ma być?
int  Width=800+(SCALE_WIDTH>0?SCALE_WIDTH+10:0); //Rozmiary ekranu
int  Height=595+(PRINT_SUBNAMES?16:0);//--//---
bool Y_MIRRORED=true;

/*
double Min=0; //Minimum i maksimum do skalowania danych
double Max=1; //Mogłoby być też szukane - ustawione trzecim parmaterem
bool SearchMinMax=false; //Czy szukać MIN i MAX każdej tablicy osobno
void SzukanieMinMax(TabelaTabDelimited& Da,int& X1,int& Y1,int& X2,int& Y2); //Ustawianie MIN i MAX
*/

bool Znajdz_tabele(TabelaTabDelimited& Da,int& X1,int& Y1,int& X2,int& Y2,char* SubTabName); //Znajdowanie kolejnej tabeli
bool Pokazuj_tabele(TabelaTabDelimited& Da,int X1,int Y1,int X2,int Y2,const char* SubTabName);//Wyświetlanie, zmiany opcji, ENTER zatwierdza ESC przeskakuje
int  Czytaj_komendy();// Zwraca 0 jesli kontynuować, 1 jak zatwierdzone, -1 jak pominięte
int KonwertujDo(const char* LiczbaTxt,double& naco);//Przekształca tekst na liczbę uwzględniając i . i , i %

//Rożne skale kolorów
void SkalaSzarosci();
void SkalaZero();
void SkalaMapowaNormalna();
void SkalaSymShA();
void SkalaSymShB();
void SkalaSymShB2();
void SkalaSymShC();
void SkalaSymShD();
void SkalaRedBlue();
void SkalaJet1();
void SkalaJetS(double S);
void SkalaJet2();
void SkalaJet3();
void SkalaJetT();

int main(int argc, const char* argv[])
{
	cout<<"This program makes color maps from data file produced by Honor model in batch mode"<<endl;
	cout<<"Each table is localized by "<<TAB_MARKER<<" marker in upper left corner of table"<<endl<<"(first cell o the heads line)"<<endl;
	cout<<"For old Honor*.log files you have to add this markers."<<endl<<endl;


	if(argc>1 && argv[1]!=NULL && argv[1][0]!='?' && argv[1][0]!='*' )
		strncpy(InputFile, argv[1],10000);
	else
		Podaj_nazwe_pliku(InputFile,10000,"Chose tab delimited input file with one or more table");
	
	cout<<"Name of input file is \""<<InputFile<<"\""<<endl;

	TabelaTabDelimited Dane;
	if(!Dane.WczytajZPliku(InputFile,'\t'/*,16,16*/)) 
	{
		char bufor_katalogu[2048];
		Komunikat(InputFile,"Nie udało się wczytać");
		_getcwd(bufor_katalogu,2048);
		cerr<<"Nie udało się wczytać \""<<InputFile<<"\" in "<<bufor_katalogu<<endl;
		perror("Taki error:");
		return -1;
	}
	cout<<"Size in R x K :"<<Dane.IleWierszy()<<"x"<<Dane.IleKolumn()<<"  Name:"<<Dane.JakaNazwa()<<"  "<<endl;
	if(Dane.IleWierszy()<2 || Dane.IleKolumn()<2)
	{
		cerr<<"Invalid file"<<endl;
		return -2;
	}

	fix_size(false);
	set_background(511);//511, ostatni z palety szarości - na pewno biały
	shell_setup(InputFile,argc,argv);// Przekazanie parametrow wywolania do symshell'a
	if(!init_plot(Width,Height,0,0)) // inicjacja grafiki/semigrafiki */
	{
		Komunikat(InputFile,"Nie udało się otworzyć grafiki");
		cerr<<"Nie udało się otworzyć grafiki \""<<InputFile<<"\""<<endl;
		return -3;
	}

	//Procedura szukania po kolei wszystkich tabel i robienia z nich plików BMP (lub XPM w Linuxie)
	int X1=0,X2=0,Y1=0,Y2=0;
	char SubTabName[SubNameSize]="NONAME SUB TABLE";//Miejsce na nazwę tabeli
	while(Znajdz_tabele(Dane,X1,Y1,X2,Y2,SubTabName))
	{
		if(!Pokazuj_tabele(Dane,X1,Y1,X2,Y2,SubTabName)) //Trwa, aż do wyjścia 
		{
			cerr<<"Zapisywanie tabeli \""<<SubTabName<<"\" zostało pominięte"<<endl;
		}
		else
		{
			wbrtm::wb_pchar SubTabFileName(10000+1024+10);
			SubTabFileName.prn("%s_%s_%dx%d_%s_%s",InputFile,SubTabName,screen_width(),screen_height(),(SCALE_WIDTH>0?"withScale":""),(Y_MIRRORED?"Ymirrored":""));//Mógł pozmieniać parametry
			dump_screen(SubTabFileName.get());
		}
		X1=X2;//PRzesuniecie
	}

	close_plot();
  return 0;
}

bool Znajdz_tabele(TabelaTabDelimited& Da,int& X1,int& Y1,int& X2,int& Y2,char* SubTabName)
//Znajdowanie kolejnej tabeli
{
	for(;Y1<Da.IleWierszy();Y1++)
	{
		for(;X1<Da.IleKolumn();X1++)
		{
			string Val=Da(Y1,X1);
			if(Val==TAB_MARKER)
			{
				//Szukaj w dół wzdłuż nagłówków wierszy aż do pustego, ewentualnie liczby
				for(Y2=Y1;Y2<Da.IleWierszy();Y2++)
				{   //O jeden w dół względem pozycji bo to ma być ostatni wiersz danych a nie za danymi
					if( Da(Y2+1,X1).c_str()==NULL || Da(Y2+1,X1)=="" || isdigit(Da(Y2+1,X1)[0]) )
						break;
				}
				//Szukaj w bok wzdłuż nagłówków kolumn aż do pustego, ewentualnie liczby
				for(X2=X1;X2<Da.IleKolumn();X2++)
				{   //O jeden w dół względem pozycji bo to ma być ostatni wiersz danych a nie za danymi
					if( Da(Y1,X2+1).c_str()==NULL || Da(Y1,X2+1)=="" || isdigit(Da(Y1,X2+1)[0]) )
						break;
				}
				//Szukaj nazwy w drugiej kolumnie pod spodem
				if( Da(Y2+1,X1+1).c_str()!=NULL && Da(Y2+1,X1+1)!="" && (!isdigit(Da(Y2+1,X1+1)[0])) )
				{
					strncpy(SubTabName, Da(Y2+1,X1+1).c_str(),SubNameSize);
					if(Da(Y2+1,X1+2).size()>0)
						strncat(SubTabName, Da(Y2+1,X1+2).c_str(),SubNameSize-Da(Y2+1,X1+1).size());
				}
				return true;
			}
		}
		X1=0;//Reset kolumny bo wiersz się skończył, a nic nie było
	}
	return false;
}

bool Pokazuj_tabele(TabelaTabDelimited& Da,int X1,int Y1,int X2,int Y2,const char* SubTabName)
//Wyświetlanie, zmiany opcji, ENTER zatwierdza ESC przeskakuje
{
	int ret=0;
	clear_screen();
	do{
	int Width=screen_width()-SCALE_WIDTH;
	int Height=(PRINT_SUBNAMES?screen_height()-char_height('X'):screen_height());
	int StepS=min(2,Height/256);
	int StepW=Width/(X2-X1);
	int StepH=Height/(Y2-Y1);

	for(int x=X1+1;x<=X2;x++)
		for(int y=Y1+1;y<=Y2;y++)
		{
			double Val=0;
			KonwertujDo(Da(y,x).c_str(),Val);
			int ColorInd=( Val>=0 ? int(Val*256)%256 : int(-Val*256)%256+256 ); 
			if(Y_MIRRORED)
				fill_rect((x-X1-1)*StepW,(Y2-y)*StepH,(x-X1)*StepW,(Y2-y+1)*StepH,  ColorInd   );
			else
				fill_rect((x-X1-1)*StepW,(y-Y1-1)*StepH,(x-X1)*StepW,(y-Y1)*StepH,  ColorInd   );
		}
	//fill_circle(Width/2,Height/2,Height/2,128);
	if(PRINT_SUBNAMES) 
		printc(1,screen_height()-char_height('X'),500,300,"%s",SubTabName);//Szarości
	/*
	for(int i=0;i<256;i++)
		fill_rect(screen_width()-SCALE_WIDTH,Height-2-(i+1)*StepS,screen_width(),Height-2-i*StepS,  i   );
	rect(screen_width()-SCALE_WIDTH-2,Height-4-256*StepS,screen_width()-2,Height-2,420,2);
	*/
	for(int i=0;i<256;i++) //Górna
		fill_rect(screen_width()-SCALE_WIDTH,1+(i+1)*StepS,screen_width(),1+i*StepS,  255-i   );

	}while( !(ret=Czytaj_komendy()) );//Powtarza aż nie 0

	
	if(ret==1) return true;
	else
	if(ret==-1) return false;
	else
	{
		Komunikat("Niespodziewana wartość z funkcji 'Czytaj_komendy()'","ABORTING!");
		exit(ret);
	}
}

int  Czytaj_komendy()// Zwraca 0 jesli kontynuować, 1 jak zatwierdzone, -1 jak pominięte
{
	flush_plot();
	int InpChar=get_char();
	switch(InpChar)
	{
	case -1://EOF
	case 'Q': 
		exit(0); break;
	case ' ':
	case '\n':
			return  1;
	case  27: 
			return -1;
    //Kontynuacja czyli wyrysowanie
/*			
	case '?': //Szukanie min i max nie bardzo działa, bo skale na razie w ogóle tego nie uwzględniają!
		SearchMinMax=!SearchMinMax; cout<<"Fixed scale: "<<(SearchMinMax?"No":"Yes")<<endl;return 0;
*/
	case 's':
	case 'g': 
		SkalaSzarosci(); cout<<"g-Gray"<<endl;return 0;
	case 'm': 
		SkalaMapowaNormalna();cout<<"m-Mapp"<<endl; return 0;
	case 'a': 
		SkalaSymShA();cout<<"a-SymShA"<<endl; return 0;
	case 'b': 
		SkalaSymShB(); cout<<"b-SymShB"<<endl;return 0;
	case 'B': 
		SkalaSymShB2();cout<<"B-SymShB2"<<endl; return 0;
	case 'c': 
		SkalaSymShC(); cout<<"c-SymShC"<<endl;return 0;
	case 'd': 
		SkalaSymShD(); cout<<"d-SymShD"<<endl;return 0;
	case 't': 
		SkalaJetT(); cout<<"t-JetT"<<endl;return 0;
	case 'j': 
		SkalaJet1(); cout<<"j-Jet 1"<<endl;return 0;
	case '1': 
		SkalaJetS(1.5); cout<<"1-Jet 1.5"<<endl; return 0;
	case '2': 
		SkalaJet2(); cout<<"2-Jet 2"<<endl;return 0;
	case '3': 
		SkalaJetS(2.3); cout<<"3-Jet 2.3"<<endl; return 0;
	case '4': 
		SkalaJetS(2.5); cout<<"4-Jet 2.5"<<endl;return 0;
    case '5': 
		SkalaJetS(2.66); cout<<"5-Jet 2.66"<<endl;return 0; 
	case '6': 
		SkalaJet3(); cout<<"6-Jet 3"<<endl;return 0;
	case 'r': 
		SkalaRedBlue(); cout<<"r-Red2Blue"<<endl;return 0;
	case '0': 
		SkalaZero();  cout<<"0- Zero!"<<endl; return 0;
	case 'w': set_rgb(0,0,0,0);set_rgb(255,255,255,255);
				cout<<"w - 0:Black, 255:white"<<endl;
		return 0;
	case '\r':
	default:
		return 0;
	};
}

int KonwertujDo(const char* LiczbaTxt,double& naco)
{
	bool procent=false;
	char  temp[512];//Duża tablica na stosie
	char* pom=temp;
	strncpy(temp,LiczbaTxt,511);

	if(strlen(temp)==0) //PUSTY!!!
		   { naco=0;  return 0; }

	if( strchr(temp,'.')==NULL //Gdy nie ma kropki
	  && (pom=strrchr(temp,','))!=NULL ) //To ostatni przecinek zmienia na kropkę
			*pom='.';

	if( (pom=strrchr(temp,'%'))!=NULL )
	{
			procent=true;
			*pom='\0';
	}

	char* endptr=NULL;
	naco=strtod(temp,&endptr);
	if(endptr!=NULL && *endptr!='\0')
		return endptr-temp;

	if(procent)
		naco/=100.0;

	return -1;  //Wbrew pozorom OK
}


