//#include <Windows.h> //Całkiem niepotrzebne
#include <iostream>
#include <cassert>
#include "INCLUDE/wb_ptr.hpp"
#include "MISCCLASSES/TabelaTabDeli.h"
#include "SYMSHELL/symshell.h"
#include "SYMSHELL/sshutils.hpp"
#include "SPS/spsNazwaPliku.h"

const char* TAB_MARKER="$TAB";
const unsigned SubNameSize=2048;
extern "C" int WB_error_enter_before_clean=1;


using namespace std; //úatwe u┐ycie nazw z przestrzeni "std"

const char* MYFILTER="All\0*.*\0LOG\0*.log\0DAT\0*.dat\0TXT\0*.txt\0\0";// Funkcja wczytywania nazwy pliku
char InputFile[2048];

int SCALE_WIDTH=30;  //Jaki rozmiar skali? Może być 0 wtedy jej nie ma
bool PRINT_SUBNAMES=true;//Czy napis pod spodem ma być?
int  Width=800+(SCALE_WIDTH>0?SCALE_WIDTH+10:0); //Rozmiary ekranu
int  Height=600+(PRINT_SUBNAMES?16:0);//--//---
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
void ReverseScale();

int main(int argc, const char* argv[])
{
	cout<<"This program makes color maps from data file produced by Honor model in batch mode"<<endl;
	cout<<"Each table is localized by "<<TAB_MARKER<<" marker in upper left corner of table"<<endl<<"(first cell o the heads line)"<<endl;
	cout<<"For old Honor*.log files you have to add this markers."<<endl<<endl;


	if(argc>1 && argv[1]!=NULL && argv[1][0]!='?' && argv[1][0]!='*' )
		strncpy(InputFile, argv[1],2048);
	else
		Podaj_nazwe_pliku(InputFile,2048,"Chose tab delimited input file with one or more table",MYFILTER);
	
	cout<<"Name of input file is \""<<InputFile<<"\""<<endl;

	TabelaTabDelimited Dane;
	if(!Dane.WczytajZPliku(InputFile,'\t'/*,16,16*/)) 
	{
		Komunikat(InputFile,"Nie udało się wczytać");
		cerr<<"Nie udało się wczytać \""<<InputFile<<"\""<<endl;
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
			wbrtm::wb_pchar SubTabFileName(2048+1024+10);
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
	int StepS=Height/256;
	int StepW=Width/(X2-X1);
	int StepH=Height/(Y2-Y1);

	for(int x=X1+1;x<=X2;x++)
		for(int y=Y1+1;y<=Y2;y++)
		{
			double Val=0;
			KonwertujDo(Da(y,x).c_str(),Val);

			int ColorInd;
			if (Val >= 0)
				ColorInd = Val * 256; //DEBUG - TYLKO JAK WARTOŚCI OD 0 do 1 !!!
			else
				ColorInd = 256 + (-Val) * 256;


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
		PRINT_SUBNAMES = !PRINT_SUBNAMES; return 0;
	case 'g': 
		SkalaSzarosci(); cout <<"g-Gray"<<endl;return 0;
	case 'm': 
		SkalaMapowaNormalna();cout <<"m-Mapp"<<endl; return 0;
	case 'a': 
		SkalaSymShA();cout <<"a-SymShA"<<endl; return 0;
	case 'b': 
		SkalaSymShB(); cout <<"b-SymShB"<<endl;return 0;
	case 'B': 
		SkalaSymShB2();cout <<"B-SymShB2"<<endl; return 0;
	case 'c': 
		SkalaSymShC(); cout <<"c-SymShC"<<endl;return 0;
	case 'd': 
		SkalaSymShD(); cout <<"d-SymShD"<<endl;return 0;
	case 't': 
		SkalaJetT(); cout <<"t-JetT"<<endl;return 0;
	case 'j': 
		SkalaJet1(); cout <<"j-Jet 1"<<endl;return 0;
	case '1': 
		SkalaJetS(1.5); cout <<"1-Jet 1.5"<<endl; return 0;
	case '2': 
		SkalaJet2(); cout <<"2-Jet 2"<<endl;return 0;
	case '3': 
		SkalaJetS(2.3); cout <<"3-Jet 2.3"<<endl; return 0;
	case '4': 
		SkalaJetS(2.5); cout <<"4-Jet 2.5"<<endl;return 0;
    case '5': 
		SkalaJetS(2.66); cout <<"5-Jet 2.66"<<endl;return 0; 
	case '6': 
		SkalaJet3(); cout <<"6-Jet 3"<<endl;return 0;
	case 'r': 
		SkalaRedBlue(); cout <<"r-Red2Blue"<<endl;return 0;
	case '0': 
		SkalaZero();  cout <<"0- Zero!"<<endl; return 0;
	case 'w': set_rgb(0,0,0,0);set_rgb(255,255,255,255);
				cout <<"w - 0:Black, 255:white"<<endl;
		return 0;
	case '-': ReverseScale(); cout <<"Scale was reversed!"<< endl; return 0;
	case '\t': Y_MIRRORED = !Y_MIRRORED; return 0;
	case '\r':
	default:
		cout <<"\nCommands are:\n"
			<< "g-Gray" << endl
			<< "r-Red2Blue" << endl
			<< "m-Mapp" << endl
			<< "a-SymShA" << endl
			<< "b-SymShB" << endl
			<< "B-SymShB2" << endl
			<< "c-SymShC" << endl
			<< "d-SymShD" << endl
			<< "t-JetT" << endl
			<< "j-Jet 1" << endl
			<< "1-Jet 1.5" << endl
			<< "2-Jet 2" << endl
			<< "3-Jet 2.3" << endl
			<< "4-Jet 2.5" << endl
			<< "5-Jet 2.66" << endl
			<< "6-Jet 3" << endl
			<< "0- Zero!" << endl
			<< "w - 0:Black, 255:white" << endl
			<< "s - subtitle" << endl
			<< "TAB - mirrored" << endl
			;
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


void SkalaSzarosci()
{
	int k;
	for(k=0;k<=255;k++)
	{
		long wal=k;
		//fprintf(stderr,"%u %ul\n",k,wal);
		set_rgb(k,wal,wal,wal); //Color part
	}
}

void SkalaRedBlue()
{
	int k;
	for(k=0;k<=255;k++)
	{
		long wal=k;
		//fprintf(stderr,"%u %ul\n",k,wal);
		set_rgb(k,wal,0,255-wal); //Color part
	}
}

void SkalaZero()
{
	int k;
	for(k=0;k<=255;k++)
	{
		int R=0,G=0,B=0;
		if(k<64)
			R=k*3;
		else
			R=255;
		if(64<=k && k<=128)
			B=(k-20)*2;
		else if(k>128)
			B=250;
		if(k>128)
			G=(k-128)*2;
		
		set_rgb(k,R,G,B); //Color part
	}
}

void SkalaSymShA()
{
  int k;
  for(k=0;k<=255;k++)
  {
		  long wal1,wal2,wal3;
		  double kat=(M_PI*2)*k/255.;

		  //  LONG USED SCALE
		  wal1=(long)(255*sin(kat*1.22));
		  if(wal1<0) wal1=0;

		  wal2=(long)(255*(-cos(kat*0.46)));
		  if(wal2<0) wal2=0;

		  wal3=(long)(255*(-cos(kat*0.9)));
		  if(wal3<0) wal3=0;

		  set_rgb(k,wal1,wal2,wal3);
  }
}

void SkalaSymShB()
{
  int k;
  for(k=0;k<=255;k++)
  {
	   long wal1,wal2,wal3;
		double kat=(M_PI*2)*k/255.;

		  wal1=(long)(255*sin(kat*1.6));

		   if(wal1<0) wal1=0;
		   wal2=(long)(255*(-sin(kat*0.85)));
		   if(wal2<0) wal2=0;
		   wal3=(long)(255*(-cos(kat*1.08)));
		   if(wal3<0) wal3=0;
		 set_rgb(k,wal1,wal2,wal3);
  }
}

void SkalaSymShB2()
{
	int k;
	for(k=0;k<=255;k++)
	{
		long wal1,wal2,wal3;
		double kat=(M_PI*2)*k/255.;

		wal1=(long)(255*sin(kat*1.25));
		if(wal1<0) wal1=0;

		wal2=(long)(255*(-sin(kat*0.85)));
		if(wal2<0) wal2=0;

		wal3=(long)(255*(-cos(kat*1.1)));
		if(wal3<0) wal3=0;

		set_rgb(k,wal1,wal2,wal3);
	}
}

void SkalaSymShC()
{
  int k;
  for(k=0;k<=255;k++)
  {
	long wal1,wal2,wal3;
	double kat=(M_PI*2)*k/255.;
	
	wal1=(long)(255*sin(kat*1.2)*0.9);
	if(wal1<0) wal1=0;

		  wal2=(long)(255*(-cos(kat*0.38+1.25)));
		  if(wal2<0) wal2=0;

		  wal3=(long)(255*(-cos(kat*0.9)*0.9));
		  if(wal3<0) wal3=0;

		  set_rgb(k,wal1,wal2,wal3);
  }
}

//double ROff=0.9,GOff=1.25,BOff=0.9;
//double RMul=1.2 ,GMul=0.38 ,BMul=0.9;

double ROff=1.25-M_PI,GOff=0.9+M_PI,BOff=0.9;
double RMul=0.38,GMul=1.2,BMul=0.9;

void SkalaSymShD()
{
	int k;
	for(k=0;k<=255;k++)
	{
		long walR,walG,walB;
		double kat=(M_PI*2)*k/255.;

		walR=(long)(255*sin(kat*RMul)*ROff);
		if(walR<0) walR=0;

		walG=(long)(255*(-cos(kat*GMul+GOff)));
		if(walG<0) walG=0;

		walB=(long)(255*(-cos(kat*BMul)*BOff));
		if(walB<0) walB=0;

		set_rgb(k,walR,walG,walB);
	}
}

/*
   Return a RGB colour value given a scalar v in the range [vmin,vmax]
   In this case each colour component ranges from 0 (no contribution) to
   1 (fully saturated), modifications for other ranges is trivial.
   The colour is clipped at the end of the scales if v is outside
   the range [vmin,vmax]


   from http://paulbourke.net/texture_colour/colourspace/
*/

typedef struct {
	  double r,g,b;
   } D_COLOUR;

D_COLOUR GetColour(double v,double vmin,double vmax)
{
   D_COLOUR c = {1.0,1.0,1.0}; // white
   double dv;

   if (v < vmin)
	  v = vmin;
   if (v > vmax)
      v = vmax;
   dv = vmax - vmin;

   if (v < (vmin + 0.25 * dv)) {
      c.r = 0;
      c.g = 4 * (v - vmin) / dv;  //Zmiana G, R=0, B=1
   } else if (v < (vmin + 0.5 * dv)) {
      c.r = 0;
      c.b = 1 + 4 * (vmin + 0.25 * dv - v) / dv; //Zmiana B, R=0, G=1
   } else if (v < (vmin + 0.75 * dv)) {
      c.r = 4 * (v - vmin - 0.5 * dv) / dv;//Zmiana R, B=0, G=1
      c.b = 0;
   } else {
	  c.g = 1 + 4 * (vmin + 0.75 * dv - v) / dv;//Zmiana G, B=0, R=1
      c.b = 0;
   }

   return(c);
}

void SkalaMapowaNormalna()
{
  int k;
  for(k=0;k<=255;k++)
  {
	D_COLOUR pom = GetColour( (double)k,(double)0,(double)255);
	set_rgb(k,pom.r*255,pom.g*255,pom.b*255);
  }
}

double interpolate( double val, double y0, double x0, double y1, double x1 ) {
    return (val-x0)*(y1-y0)/(x1-x0) + y0;
}

//const double A=-0.75,B=-0.25,C=0.0,D=0.25,E=0.75,F=1.0,M=0.5;// od -1 do 1 - dla HONORu bez sensu
const double   A=0.00, B=0.11, C=0.17, D=0.33, E=0.66, F=1.0, M=0.40;//Próba 2
//const double   A=0.00, B=0.15, C=0.20, D=0.33, E=0.60, F=1.0, M=0.35;//Próba 2B
//const double   A=0.00, B=0.15, C=0.3, D=0.4, E=0.5, F=1.0, M=0.35;//Próba 3, chmmmm... lepsze wrogiem dobrego?


double base( double val ) {
    if ( val <= A) return 0;
    else if ( val <= B ) return interpolate( val, C, A, F, B );
    else if ( val <= D ) return 1.0;
    else if ( val <= E ) return interpolate( val, F, D, C, E );
    else return 0.0;
}

double red( double gray ) {
    return base( gray - M );
}
double green( double gray ) {
    return base( gray );
}
double blue( double gray ) {
    return base( gray + M );
}

void SkalaJetT()
{
  int k;
  for(k=0;k<=255;k++)
  {
	set_rgb(k,red(tanh(k/255.))*255,green(tanh(k/255.))*255,blue(tanh(k/255.))*255);
  }
  set_rgb(0,0,0,30);
  //set_rgb(255,255,255,255);
}

void SkalaJet1()
{
  int k;
  for(k=0;k<=255;k++)
  {
	set_rgb(k,red(k/255.)*255,green(k/255.)*255,blue(k/255.)*255);
  }
  set_rgb(0,0,0,50);
  //set_rgb(255,255,255,255);
}

void SkalaJetS(double S)
{
  int k;
  for(k=0;k<=255;k++)
  {
	set_rgb(k,red(pow(k/255.,1/S))*255,green(pow(k/255.,1/S))*255,blue(pow(k/255.,1/S))*255);
  }
  set_rgb(0,0,0,30);
  //set_rgb(255,255,255,255);
}

void SkalaJet2()
{
  int k;
  for(k=0;k<=255;k++)
  {
	set_rgb(k,red(pow(k/255.,1/2.))*255,green(pow(k/255.,1/2.))*255,blue(pow(k/255.,1/2.))*255);
  }
  set_rgb(0,0,0,30);
  //set_rgb(255,255,255,255);
}

void SkalaJet2ipol()
{
  int k;
  for(k=0;k<=255;k++)
  {
	set_rgb(k,red(pow(k/255.,1/2.5))*255,green(pow(k/255.,1/2.5))*255,blue(pow(k/255.,1/2.5))*255);
  }
  set_rgb(0,0,0,30);
  //set_rgb(255,255,255,255);
}

void SkalaJet3()
{
  int k;
  for(k=0;k<=255;k++)
  {
	set_rgb(k,red(pow(k/255.,1/3.))*255,green(pow(k/255.,1/3.))*255,blue(pow(k/255.,1/3.))*255);
  }
  set_rgb(0,0,0,30);
  //set_rgb(255,255,255,255);
}

void ReverseScale()
{
	int k;
	for (k = 0; k <= 127; k++)
	{
		ssh_rgb pom1 = get_rgb_from(k);        /* Jakie są ustawienia RGB konkretnego kolorku w palecie */
		ssh_rgb pom2 = get_rgb_from(255-k);
		set_rgb(k, pom2.r, pom2.g, pom2.b);
		set_rgb(255-k, pom1.r, pom1.g, pom1.b);
	}
}

/*
void SzukanieMinMax(TabelaTabDelimited& Da,int& X1,int& Y1,int& X2,int& Y2)
	//Ustawianie MIN i MAX
{
	Min=DBL_MAX;
	Max=-DBL_MAX;
	for(int x=X1+1;x<=X2;x++)
		for(int y=Y1+1;y<=Y2;y++)
		{
			double Val=0;
			KonwertujDo(Da(y,x).c_str(),Val);
			if(Val<Min) 
				Min=Val;
			if(Val>Max)
				Max=Val; 
		}
}
*/