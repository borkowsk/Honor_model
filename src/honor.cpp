// Prototypowy program "ewolucji" kultury honoru w kultur� policyjn�
//                                                             wersja 21-08-2012
////////////////////////////////////////////////////////////////////////////////
//#include <process.h> //nie ma w GCC?

#define _USE_MATH_DEFINES //bo M_PI

#include <cmath>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <process.h>

#define HIDE_WB_PTR_IO 0
#include "INCLUDE/wb_ptr.hpp"
#include "INCLUDE/optParam.hpp"

#include "SYMSHELL/symshell.h"
#include "SYMSHELL/sshutils.hpp"

#include "HonorAgent.hpp"

using namespace std;
using namespace wbrtm;

const char* MODELNAME="Culture of honor";
const char* VERSIONNUM="0.43d (08-11-2014)"
#ifdef TESTING_RULE_LITERALS
" RulesTestDiv"
#else
""
#endif
#ifdef HONOR_WITHOUT_REPUTATION
"HonorWithoutReputation"
#endif
"";

bool  batch_mode=false;       //Czy tryb pracy przeszukiwania przestrzeni parametr�w?
enum  BATCH_MODES {NO_BAT=0,BAT_SELECTION=1,BAT_HONORvsCPOLL=2,BAT_HONORvsAGRR=3} batch_sele=BAT_SELECTION;	//Czy tryb przeszukiwania szuka po proporcjach czy po sile selekcji?
const char*  batch_names[]={"NO_BAT","Bt_SEL","Bt_HvC","Bt_HvA"}; //for batch_names[batch_sele]
const BATCH_MODES batch_vals[]={NO_BAT,BAT_SELECTION,BAT_HONORvsCPOLL,BAT_HONORvsAGRR};//W zasadzie nadmiarowe. Do OptEnumParam

unsigned population_growth=1;//SPOSOBY ROZMNA�ANIA
							 // 0 - wg. inicjalnych proporcji
							 // 1 - lokalne rozmazanie losowe s�siad
							 // 2 - lokalne rozmazanie proporcjonalne do sily  //NIE ZAIMPLEMENTOWANE!
							 // 3 - globalne, losowy ziutek z ca�o�ci
// Przykladowa lista parametr�w:
//POLICEEF=0 BULLYPR=.25 HONORPR=.11 CALLPRP=.22 MAXSTEP=10000 VISSTEP=10 GROWMODE=1 REPETITIONS=10
//
//ZMIANY:
//0.43    - wersja z TEST_DIVIDER'em stałych w regułach agenta
//0.39a   - wprowadzenie z powrotem spontanicznej agresywno�ci honorowych
//0.38abc - wprowadzenie mapy miary zr�nicowania dodatkowo obok mapy proporcji i mapy �redniej si�y
//        ale wariacja liczona pomi�dzy krokiem a jego bezpo�rednim poprzednikiem jest bardzo ma�a.
//		  ei�c wprowadzono parametr  PREVSTEP ustawiony na 80 (cz�ciej ni� co 100 krok�w i tak nie liczy w trybie przestrzeni parametr�w)
//        POPRAWIONO LICZENIE KROK�W W D�UGICH EKSPERYMENTACH PRZESTRZENI PARAMETR�W!
//        Wprowadzono parametr ONLY3STRAT do testowania zubo�onej wersji modelu (bez racjonalnych)
//		  i wyprowadzenie PREVSTEP  i ONLY3STRAT jako parametr�w wywo�ania.
//0.37a - statystyki po akcji po grupach. Inne defaulty startowe
//		  rozbudowanie statystyk akcji wypisywanych do logu
//		  testowanie mo�liwo�ci modelu ze �miertelno�ci�
//0.36b - drobne zmiany w tekstach konsolowych (29-01-2014)
//0.36a - drobne zmiany na wydrukach konsoli i zwi�zane z kompilacj� pod MSVC++
//      - wyb�r prostego wy�wietlania w g��wnym oknie  (25-09-2013)
//DO RAPORTU 2013
//0.35c - u�ycie clasy OptEnumParam dla niekt�rych parametr�w wywo�ania
//0.35b - i stworzenie automatycznego generatora nazw plik�w log i bitmap.
//0.35  - ostateczna implementacja "MAFII" dwupoziomowej (pokrewie�stwo do dwu poziom�w przodk�w liczy si� jako rodzina)
//		  Wyprowadzenie tego jako parametru. Zmiana parametr�w nazw LOG�w z char* na wb_pchar
//        (trzeba by�o rozbudowa� klas� OptParam.
//0.31  - podzia� na osobne �r�d�a w celu implementacji rodzinnego honoru. "b" - implementacja "mafii"
//0.30a - r�ne zmiany w sposobie wyprowadzania danych tekstowych i trzeci rodzaj eksploracji przestrzeni
//0.28aa- pocz�tek tworzenia trzeciego rodzaju przekroju przestrzeni parametr�w, oraz poprawki
//0.27  - wykres przestrzeni selekcja x polic.effic.
//0.2631 - Drobne poprawki???
//0.263 - Wi�kszo�� parametr�w obs�ugiwana przez szablon klasy z "OptParam"
//0.262 - WA�NE - WIMPowie patrz� teraz nie na swoj� realn� si��, ale na w�asn� reputacje,
//		  wi�c de facto poza dzieci�stwem prawdopodobnie nigdy si� nie broni�
//		  Techniczne - wprowadzenie u�ycia klasy parametr�w do obs�ugi parametr�w modelu
//DO RAPORTU 2012
//0.26 -  Wykres przestrzeni - w proporcji honorowych i policyjnych do siebie dziel�cych 1/2 lub 2/3 ca�o�ci.
//		  Inny spos�b kolorowania "kultur" na obrazie �wiata symulacji (czerwony-agresja, zielony-honor, niebieski-callPolice)
//		  Pr�ba zmiany rozk�adu �omotu dla GIVEUP - �eby mogli zgin��, ale bardziej niszczy bullys!?!?
//		  Do ewolucji potrzebny szumowe �mierci - parametr NOISE_KILL
//		  ewolucja:
//		  - lokalne rozmazanie losowe s�siad
//		  TODO  - losowy ziutek z ca�o�ci
//		  TODO  - lokalne rozmazanie proporcjonalne do sily
//		  Statystyki: ile razy agent jest atakowany i ile razy wygra� ,
//						klastering index dla kultur
//        Czesto�ci atak�w na poszczeg�lne grupy w czasie - bez selekcji, zysk indywidualny
//        Du�a mapa z gradientem policji
//
//		  ewentualnie r�na aktywnosc honorowych. Nie zawsze musz� stawa�, ale z prawdopodobienstwem
//		 NA POZNIEJ: ilo�� policji zale�y od ilo�ci atak�w? Inny model - podatkowy.
//
//0.251 - poprawienie b��du w liczeniu "always give up" na metryczce pliku log
//
//0.25 - wprowadzenie statystyk z si�y r�nych grup (najwy�szy decyl tylko)
//		 i ich drukowania do pliku i na konsol�
//		 Uelastycznienie wydruku metryczki, �eby nadawa�a si� i do pionu i do poziomu ...
//		 WPROWADZENIE TRYBU PRZESZUKIWANIA PRZESTRZENI PARAMETR�W
//		 W tym ZAPISU BITMAP Z WYNIKAMI
//0.24 - wprowadzenie parametru RATIONALITY, bo u�ycie w pe�ni realistycznej oceny si�y psuje selekcje
//		 Zmiany kosmetyczne w nag��wku pliku wyj�ciowego
//0.23 - znaj� swoj� realn� si�� i j� por�wnuj� z reputacj� drugiego gdy maj� atakowa� lub si� broni�
//		 Ale to nie dzia�a�o dobrze...
//0.20 - pierwsza wersja w pe�ni dzia�aj�ca



//INNE GLOBALNE WLASCIWOSCI SWIATA
bool     MAFIAHONOR=false; //Czy reputacja przenosi si� na cz�onk�w rodziny
FLOAT    USED_SELECTION=0.050;//0.10; //Jak bardzo przegrani umieraj� (0 - brak selekcji w og�le)
FLOAT    MORTALITY=0.0009;  //=0.01 //Jak �atwo mo�na zgin�� z przyczyn niespo�ecznych - horoba, wypadek itp. JAK 0 TO S� "ELFY"
FLOAT    EXTERNAL_REPLACE=0.0001; //Jakie jest prawdopodobienstwo przypadkowej zamiany na zupe�nie innego
FLOAT    HONOR_AGRESSION=0.01250;//0.015;//Bazowy poziom agresji dla HONOROWYCH
FLOAT    AGRES_AGRESSION=0.01250;//POZIOM PRZYPADKOWEJ AGRESJI AGRESYWNYCH (bez calkulacji kto silniejszy!)

#ifdef TESTING_RULE_LITERALS
FLOAT	 TEST_DIVIDER=1.0; //Służy do modyfikacji stałych liczbowych używanych w regułach reakcji agenta
                                // wersja ze zmienną istotnie SPOWALNIA model
#endif  //Ale może być też jako stała "rozliczana" podczas kompilacji

FLOAT    RECOVERY_POWER=0.005;//Jak� cz�� si�y odzyskuje w kroku

//DEFINICJE ROZK�AD�W WARTO�CI DLA INDYWIDUALNYCH CECH AGENT�W
FLOAT    BULLI_POPUL=-0.25;//0.2;//0.100;//Albo zero-jedynkowo. Jak 1 to decyduje rozk�ad sterowany BULLISM_LIMIT
						   //("-" jest sygna�em zafiksowania w trybie batch
FLOAT	 HONOR_POPUL=0.25;//0.10;//.17;0.33;//Jaka cz�� agent�w populacji jest �ci�le honorowa
FLOAT    CALLER_POPU=0.25;//0.10;//.17;       //Jaka cz�� wzywa policje zamiast si� poddawa�
FLOAT    POLICE_EFFIC=0.05;//0.05;//0.33;0.5//0.650;//0.950; //Z jakim prawdopodobie�stwem wezwana policja obroni agenta
bool     ONLY3STRAT=false; //Czy nie u�ywamy strategii "racjonalnej"?

//Eksploracja przestrzeni parametr�w
bool  Compensation_mode=true;//Czy przestrze� gdzie Honorowi i CallPolice, albo Agresywni i Honorowi uzupe�niaj� si� do PROP_MAX
							 //Ma sens tylko dla zafiksowanych pozosta�ych
FLOAT POLICE_EFFIC_STEP=0.05;
FLOAT POLICE_EFFIC_MAX=1;
FLOAT POLICE_EFFIC_MIN=0;

FLOAT SELECTION_STEP=0.05;
FLOAT SELECTION_MAX=1;
FLOAT SELECTION_MIN=0;

FLOAT PROPORTION_STEP=0.05;
FLOAT PROPORTION_MAX=1.0/3.0;
FLOAT PROPORTION_MIN=0.0;

//Sterowanie statystykami i powt�rzeniami
unsigned REPETITION_LIMIT=1; //10//Ile ma zrobi� powt�rze� tego samego eksperymentu;
unsigned RepetNum=1; //Kt�ra to kolejna repetycja?  - NIE ZMIENIA� R�CZNIE!
unsigned STOP_AFTER=60000;//Po jakim czasie staje automatycznie
unsigned STAT_AFTER=0; //Po jakim czasie zacz�� zlicza� koncow� statystyk�
unsigned PREVSTEP=80;//Ile krok�w przed g��wn� statystyk� wylicza� poprzedni stan dla wariacji
unsigned EveryStep=10;//Cz�stotliwo�� wizualizacji i zapisu do logu
					 //Ujemne oznacza tryb automatycznej inkrementacji
unsigned DumpStep=10000;//Cz�sto�� zrzut�w stan�w agent�w

//Parametry techniczne steruj�ce wizualizacj� i wydrukami
unsigned VSIZ=5; //Maksymalny rozmiar boku agenta w wizualizacji kompozytowej
unsigned SSIZ=1; //Bok agenta w wizualizacji uzupe�niaj�cej (ma�ej)
bool  ConsoleLog=true;//Czy u�ywa logowania zdarze� ma konsoli. Wa�ne dla startu, potem si� da prze��cza�
bool  VisShorLinks=false; //Wizualizacja bliskich link�w
bool  VisFarLinks=false;  //Wizualizacja dalekich
bool  VisAgents=true;     //Wizualizacja w�a�ciwo�ci agent�w
bool  VisReputation=false; //Czy robi� obw�dk� z reputacji agent�w
bool  VisDecision=false;   //Czy wizualizowa� ostatni� decyzj�?
bool  dump_screens=false;
bool  BatchPlotPower=false;   //Czy w trakcie wy�wietla� MnPower czy jednak MnProportions
bool  Batch_true_color=false;//Czy skale kolor�w true-color czy 256 kolor�w t�czy

//Nazwy plik�w
wb_pchar LogName("HonorXXX"); //   string?
wb_pchar DumpNam("HonorXXX");
string  Comment;

//DLA LICZBY LOSOWEJ
long int RandSeed=-1; //Ewentualny zasiew liczby losowej

OptionalParameterBase* Parameters[]={ //sizeof(Parameters)/sizeof(Parameters[])
new ParameterLabel("PARAMETERS FOR SINGLE SIMULATION"),
new OptionalParameter<long>(RandSeed,1,0x01FFFFFF,"RANDSEED","Use, if you want the same simulation many times"),//Zasiewanie liczby losowej
new OptionalParameter<unsigned>(population_growth,0,3,"GROWMODE","How population growth?\n\t  0-as initial distribution, 1-local distribution, 3-global distribution\n\t "),
new OptionalParameter<bool>(HonorAgent::CzyTorus,false,true,"TORUS","Is the world topology toroidal or not"), //Czy geometria torusa czy wyspy z brzegami
new OptionalParameter<bool>(MAFIAHONOR ,false,true,"FAMILIES","Is a family relationship taken into account in reputation changes?"),//Czy u�ywamy mechanizmu rodzinnego zmian reputacji
new OptionalParameter<FLOAT>(MORTALITY,0,0.05,"MORTALITY","Ratio of agents randomly killed each step by other factors"), //Jakie jest prawdopodobienstwo przypadkowej �mierci
new OptionalParameter<FLOAT>(EXTERNAL_REPLACE,0,0.05,"EXTREPLACE","Ratio of agents randomly replaced by random newcomers every step"), //Wymiana imigracyjno/emigracyjna MA�A MA BY�!
new OptionalParameter<FLOAT>(USED_SELECTION,0,0.75,"SELECTION","Minimal level of strenght required to survive"),//0.10; //Jak bardzo przegrani umieraj� (0 - brak selekcji w og�le)
new OptionalParameter<FLOAT>(HONOR_AGRESSION,0,0.15,"HONAGRES","Probability of random agresion of HONOR agents"),//0.015;//Bazowy poziom agresji zale�ny od honoru
new OptionalParameter<FLOAT>(AGRES_AGRESSION,0,0.15,"AGRAGRES","Probability of random agresion of AGRESSIVE agents"),//0.015;//Bazowy poziom agresji zale�ny od honoru
#ifdef TESTING_RULE_LITERALS
new OptionalParameter<FLOAT>(TEST_DIVIDER,1.0/3.0,1.0,"TEST_DIVIDER","For testing real meaning of litera values used in rules"),
#endif
//FLOAT    RECOVERY_POWER=0.005;//Jak� cz�� si�y odzyskuje w kroku
new OptionalParameter<FLOAT>(RECOVERY_POWER,0.00001,0.5,"RECPOWER","How fast agent recovery for after fight demage"),
new OptionalParameter<FLOAT>(POLICE_EFFIC,0,1,"POLICEEF","Probability of efficient police intervention"),//=0.50;//0.650;//0.950; //Z jakim prawdopodobie�stwem wezwana policja obroni agenta
new OptionalParameter<FLOAT>(BULLI_POPUL,0,1,"BULLYPR","Initial probability to born as bully agent"),//=-0.25;//0.2;//0.100;//Albo zero-jedynkowo. Jak 1 to decyduje rozk�ad sterowany BULLISM_LIMIT ("-" jest sygna�em zafiksowania w trybie batch
new OptionalParameter<FLOAT>(HONOR_POPUL,0,1,"HONORPR","Initial probability to born as honor agent"),//=0.18;//0.3333;//Jaka cz�� agent�w populacji jest �ci�le honorowa
//BULLYPR=0.333   HONORPR=0.333 CALLPRP=0.333
new OptionalParameter<FLOAT>(CALLER_POPU,0,1,"CALLPRP","Initial probability to born as police caller"),//=0.25;//Jaka cz�� wzywa policje zamiast si� poddawa�
new OptionalParameter<bool>(ONLY3STRAT,false,true,"CALLPOLISREST","Is police callers the last strategies?"
																	"\n\t\tIf not the rational strategies take the rest to 100%"),
new ParameterLabel("PARAMETERS FOR MULTIPLE SIMULATIONS (EXPLORATION/BATCH MODE)"),
new OptionalParameter<bool>(batch_mode,false,true,"BATCH","To switch into parametr space batch mode"),
//Nie ma jeszcze szablonu dla enumeracji wi�c chamski rzut na "unsigned int"
//new OptionalParameter<unsigned>(*((unsigned*)&batch_sele),1,3,"BSELE","To switch batches bettwen SELECTION=1,HONORvsCPOLL=2,HONORvsAGRR=3"),
//bool  batch_mode=true;       //Czy tryb pracy przeszukiwania przestrzeni parametr�w?
//enum BAT_MODE {NO_BAT=0,BAT_SELECTION=1,BAT_HONORvsCPOLL=2,BAT_HONORvsAGRR=3} batch_sele=BAT_SELECTION;		  //Czy tryb przeszukiwania szuka po proporcjach czy po sile selekcji?
//char*  batch_names[]={"NO_BAT","Bt_SEL","Bt_HvC","Bt_HvA"}; //batch_names[batch_sele]
new OptEnumParametr<BATCH_MODES>(batch_sele,BAT_SELECTION,BAT_HONORvsAGRR,
								 "BSELE","To switch batches job.",//Valid are Bt_SEL(or 1),Bt_HvC(or 2),Bt_HvA(or 3)
								 4-1,batch_names+1/*,batch_vals+1*/), //Nie u�ywa pierwszego
new OptionalParameter<FLOAT>(POLICE_EFFIC_STEP,0,1.0,"PEFFSTEP","Pol. efficiency exploration step"),//=0.1;
new OptionalParameter<FLOAT>(POLICE_EFFIC_MAX,0,1.0,"PEFFMAX","Pol. efficiency exploration maximum"),//=1;
new OptionalParameter<FLOAT>(POLICE_EFFIC_MIN,0,1.0,"PEFFMIN","Pol. efficiency exploration minimum"),//=0;
new OptionalParameter<FLOAT>(PROPORTION_STEP,0,1.0,"PROPSTEP","Proportion exploration step"),//=0.03;
new OptionalParameter<FLOAT>(PROPORTION_MAX,0,1.0,"PROPMAX","Proportion exploration maximum"),//=1.0/3.0;
new OptionalParameter<FLOAT>(PROPORTION_MIN,0,1.0,"PROPMIN","Proportion exploration minimum"),//=0.0;
new OptionalParameter<FLOAT>(SELECTION_STEP,0,1.0,"SELESTEP","Selection exploration step"),//=0.1;
new OptionalParameter<FLOAT>(SELECTION_MAX,0,1.0,"SELEMAX","Selection exploration maximum"),//=1;
new OptionalParameter<FLOAT>(SELECTION_MIN,0,1.0,"SELEMIN","Selection exploration minimum"),//=0;
new ParameterLabel("STATISTIC & SO... HOW MANY, HOW OFFTEN, WHERE"),
new OptionalParameter<unsigned>(REPETITION_LIMIT,1,1000,"REPETITIONS","How many repetitions for each settings we expect?"),
new OptionalParameter<unsigned>(STAT_AFTER,1,1000000,"STATSTART","When start to calculate statistics in batch mode?"),
new OptionalParameter<unsigned>(STOP_AFTER,1,1000000,"MAXSTEP","To stop each simulation after this number of steps"),
new OptionalParameter<unsigned>(EveryStep,1,10000,   "VISSTEP","For set how offten visualisation and statistics occur?"),
new OptionalParameter<unsigned>(DumpStep,1,1000000,  "DMPSTEP","For set how offten agents attributes and graphix view is dumped?"),
new OptionalParameter<unsigned>(PREVSTEP,1,100,		 "PREVSTEP","When previous state for calculating of variatio is calculated?"),
//new OptionalParameter<const char*>(LogName,"honor","HONOR","LOGNAME","Name for main log file"),
//new OptionalParameter<const char*>(DumpNam,"honor_dump","DUMP","DUMPNAME","Name for detailed log file"),
new OptionalParameter<wb_pchar>(LogName,wb_pchar("honor"),wb_pchar("HONOR"),"LOGNAME","Name for main log file"),
new OptionalParameter<wb_pchar>(DumpNam,wb_pchar("honor_dump"),wb_pchar("DUMP"),"DUMPNAME","Name for detailed log file"),
new OptionalParameter<string>(Comment,"bla bala bla","TEST#1","COMMENT","Comment for the experiment, will be put into log file"),
new ParameterLabel("VISUALISATION OPTIONS"),
new OptionalParameter<bool>(ConsoleLog,  false,true, "CONSOLOG","To log events on console window"),
new OptionalParameter<bool>(dump_screens,false,true, "DUMPSCR","Dump graphix screen after every visualisation frame"),
new OptionalParameter<bool>(VisShorLinks,false,true, "VISSHORT","To visualise short links"),
new OptionalParameter<bool>(VisFarLinks, false,true, "VISFARLN","To visualise far connections"),
new OptionalParameter<bool>(VisAgents,   false,true, "VISAGENT","To visualise composed view of agents"),
new OptionalParameter<bool>(BatchPlotPower,false,true,"BATPOWER","To plot mean power during batch simulations"),
new OptionalParameter<bool>(Batch_true_color,false,true,"BATTRCOL","To plot in true colors during batch sim."),
new OptionalParameter<unsigned>(VSIZ,3,20,"AGENTSIZ1","Side of an agent on the composed visualisation"), //Maksymalny rozmiar boku agenta w wizualizacji kompozytowej
new OptionalParameter<unsigned>(SSIZ,1,5,"AGENTSIZ2","Side of an agent on the small visualisations"), //Bok agenta w wizualizacji uzupe�niaj�cej (ma�ej)
new ParameterLabel("END OF LIST")
};                                              // Comment PREVSTEP ONLY3STRAT

wb_pchar MakeFileName(const char* Core)
{
	wb_pchar SPom(1024);
	SPom.prn("%s%sS%gF%cRA%gNK%gG%uT%u___MC%uST%uev%uRx%u_%u",
		Core,(!batch_mode?"Mod":batch_names[batch_sele]),USED_SELECTION,(MAFIAHONOR?'y':'n'),HONOR_AGRESSION
		,
		EXTERNAL_REPLACE,population_growth,HonorAgent::CzyTorus,
		//...tu proporcje? na razie nie...
		STOP_AFTER,STAT_AFTER,EveryStep,REPETITION_LIMIT,(getpid()));
	return SPom;
}

void Parameters_dump(ostream& o,const char* SEP="\t",const char* ENDL="\n",bool FL=true)
{
	o<<MODELNAME<<"\tv.:"<<SEP<<VERSIONNUM;
	if(RandSeed>0)
		o<<SEP<<"RANDSEED:"<<SEP<<RandSeed;
	o<<ENDL<<Comment.c_str()<<ENDL;// Comment
#ifndef unix
	for(int i=0;i<_argc;i++)
		o<<_argv[i]<<SEP;
	o<<ENDL;
#endif
	o<<"uint"<<SEP<<"SIDE"<<SEP<<SIDE<<ENDL;//SIDE*SIDE to rozmiar �wiata symulacji
	o<<"bool"<<SEP<<"TORUS"<<SEP<<(HonorAgent::CzyTorus?"true":"false")<<ENDL;

	o<<"uint "<<SEP<<"MOORE_RAD"<<SEP<<MOORE_RAD<<ENDL;
	o<<"FLOAT"<<SEP<<"OUT_FAR_LINKS_PER_AGENT"<<SEP<<OUTFAR_LINKS_PER_AGENT<<ENDL;//Ile jest dodatkowych link�w jako u�amek liczby agent�w
	o<<"FLOAT"<<SEP<<"RECOVERY_POWER"<<SEP<<RECOVERY_POWER<<ENDL;//Jak� cz�� si�y odzyskuje w kroku
	o<<"uint "<<SEP<<"POP_GROWTH_MODE"<<SEP<<population_growth<<ENDL;//Rodzaj wzrostu populacji (z prop. bazowych,globalnych lub lokalnych)
	o<<"FLOAT"<<SEP<<"HONOR_AGRESSION"<<SEP<<HONOR_AGRESSION<<ENDL;//0.950;
	o<<"FLOAT"<<SEP<<"AGRES_AGRESSION"<<SEP<<AGRES_AGRESSION<<ENDL;//0.950;
	o<<"FLOAT"<<SEP<<"EXTERNAL_REPLACE"<<SEP<<EXTERNAL_REPLACE<<ENDL;
	o<<"FLOAT"<<SEP<<"MORTALITY"<<SEP<<MORTALITY<<ENDL;

	if(batch_mode)
	{
		o<<"FLOAT"<<SEP<<"POLICE_EFFIC_STEP"<<SEP<<POLICE_EFFIC_STEP
				  <<SEP<<"POLICE_EFFIC_MIN "<<SEP<<POLICE_EFFIC_MIN
				  <<SEP<<"POLICE_EFFIC_MAX "<<SEP<<POLICE_EFFIC_MAX<<ENDL;
		o<<"FLOAT"<<SEP<<"PROPORTION_STEP"<<SEP<<PROPORTION_STEP
				  <<SEP<<"PROPORTION_MIN "<<SEP<<PROPORTION_MIN
				  <<SEP<<"PROPORTION_MAX "<<SEP<<PROPORTION_MAX<<SEP<<"Batch in ratio mode"<<SEP<<(Compensation_mode?"true":"false")<<ENDL;
		o<<"FLOAT"<<SEP<<"SELECTION_STEP"<<SEP<<SELECTION_STEP//Selection exploration step=0.1;
				  <<SEP<<"SELECTION_MIN "<<SEP<<SELECTION_MIN//Selection exploration minimum=0;
				  <<SEP<<"SELECTION_MAX "<<SEP<<SELECTION_MAX<<ENDL;//Selection exploration maximum=1;
		o<<"uint "<<SEP<<"TYPE OF BATCH JOB:"<<SEP<<batch_sele<<SEP<<batch_names[batch_sele]<<ENDL;
		o<<"FLOAT"<<SEP<<"Default POLICE_EFFIC "<<SEP<<POLICE_EFFIC<<ENDL;	//Z jakim prawdopodobie�stwem wezwana policja obroni agenta
		o<<"FLOAT"<<SEP<<"Default USE_SELECTION"<<SEP<<USED_SELECTION<<ENDL; //Jak bardzo przegrani umieraj� (0 - brak selekcji w og�le)
		o<<"FLOAT"<<SEP<<"Default BULLY_POPUL  "<<SEP<<BULLI_POPUL<<ENDL;
		o<<"FLOAT"<<SEP<<"Default HONOR_POPUL  "<<SEP<<HONOR_POPUL<<ENDL;
		if(ONLY3STRAT)
		{
			o<<"FLOAT"<<SEP<<"Default CALLP_POPUL"<<SEP<<(1.0-abs(BULLI_POPUL)-abs(HONOR_POPUL))<<ENDL;
		}
		else
		{
			o<<"FLOAT"<<SEP<<"Default CALLP_POPUL  "<<SEP<<CALLER_POPU<<ENDL;
			o<<"FLOAT"<<SEP<<"always give up"<<SEP<<(1.0-abs(CALLER_POPU)-abs(BULLI_POPUL)-abs(HONOR_POPUL))<<ENDL;//not realy "loosers"
		}
		o<<"BOOL"<<SEP<<"FAMILIES"<<SEP<<(MAFIAHONOR?"Yes":"No")<<SEP<<MAFIAHONOR<<ENDL;
	}
	else
	{
	 o<<"FLOAT"<<SEP<<"POLICE_EFFIC"<<SEP<<POLICE_EFFIC<<ENDL;	//Z jakim prawdopodobie�stwem wezwana policja obroni agenta
	 o<<"FLOAT"<<SEP<<"USE_SELECTION"<<SEP<<USED_SELECTION<<ENDL; //Jak bardzo przegrani umieraj� (0 - brak selekcji w og�le)
	 o<<"BOOL"<<SEP<<"FAMILIES"<<SEP<<(MAFIAHONOR?"Yes":"No")<<SEP<<MAFIAHONOR<<ENDL;

	 if(BULLI_POPUL>=1) //... CO TO?         TODO?
	 {                                                                          assert("Dead code called?"==NULL);
	  o<<"FLOAT"<<SEP<<"BULLISM_LIMIT"<<SEP<<BULLISM_LIMIT<<ENDL;//Maksymalny mo�liwy bulizm.
	 }
	 else
	 {
	  o<<"FLOAT"<<SEP<<"BULLY_POPUL"<<SEP<<BULLI_POPUL<<ENDL;//Albo zero-jedynkowo
	  o<<"FLOAT"<<SEP<<"HONOR_POPUL"<<SEP<<HONOR_POPUL<<ENDL;//Jaka cz�� agent�w populacji jest �ci�le honorowa
	  if(ONLY3STRAT)
	  {
			o<<"FLOAT"<<SEP<<"CALLP_POPUL"<<SEP<<(1.0-abs(BULLI_POPUL)-abs(HONOR_POPUL))<<ENDL;
	  }
	  else
	  {
			o<<"FLOAT"<<SEP<<"CALLP_POPUL"<<SEP<<CALLER_POPU<<ENDL;//Jaka cz�� wzywa policje zamiast si� poddawa�
			o<<"FLOAT"<<SEP<<"RATIONAL"<<SEP<<(1.0-abs(CALLER_POPU)-abs(BULLI_POPUL)-abs(HONOR_POPUL))<<ENDL;//not realy "loosers"
	  }
	 }
	 o<<"FLOAT"<<SEP<<"RATIONALITY"<<SEP<<RATIONALITY<<ENDL<<ENDL; //Jak realistycznie ocenia w�asn� si�� (vs. wg. w�asnej reputacji)
	}

	o<<"REPET"<<SEP<<"STOPAFER"<<SEP<<"VISFREQ";
	if(batch_mode)
		o<<SEP<<"STATSTEP"<<SEP<<"PREVSTEP"<<SEP<<"STATSTART";o<<ENDL;//Tak ma by� to ENDL!
	o<<REPETITION_LIMIT<<SEP<<STOP_AFTER<<SEP<<EveryStep;
	if(batch_mode)
		o<<SEP<<(max(EveryStep,100u))<<SEP<<PREVSTEP<<SEP<<STAT_AFTER;o<<ENDL<<ENDL;
	if(FL) o.flush();
}



ofstream OutLog;
ofstream Dumps;

unsigned long  step_counter=0;
unsigned long  LastStep=-1;//Ostatnie wypisany krok

/*
void social_impact_step(wb_dynmatrix<HonorAgent>& World,double percent_of_MC=100)
//Powolna dynamika wp�ywu spo�ecznego - w losowej pr�bce s�siedztwa Moora
{

	unsigned N=(SIDE*SIDE*percent_of_MC)/100;//Ile losowa� w kroku MC
	for(unsigned i=0;i<N;i++)
	{
		int v1=RANDOM(SIDE);
		int h1=RANDOM(SIDE);
		HonorAgent& Ag=World[v1][h1];	//Zapami�tanie referencji do agenta, �eby ci�gle nie liczy� indeks�w
		//...
	}
}
*/

//  Liczenie statystyk
///////////////////////////////////////////////////////////////////////////////
double MeanFeiReputation=0;
double MeanCallPolice=0;
double MeanPower=0;
double MeanAgres=0;
double MeanHonor=0;

extern unsigned LiczbaTrupow;
extern unsigned LiczbaTrupowDzis;

struct zliczacz
{
  double summ;
  unsigned N;
  void Reset(){summ=0;N=0;}
  zliczacz():summ(0),N(0){}
  static void Reset(zliczacz t[],unsigned N)
  {
	  for(unsigned i=0;i<N;i++)
		t[i].Reset();
  }
};

const int NumOfCounters=4; //I RACZEJ TYLE POWINNO ZOPSTA� ZE WZGL�DU NA WIZUALIZACJE ITP
zliczacz  MnStrenght[NumOfCounters];//Liczniki sily skrajnych typ�w agent�w
const char* MnStrNam[NumOfCounters]={"MnAgresPw","MnHonorPw","MnPolicPw","MnOthrPwr"};

//Liczniki akcji dla wszystkich i dla grup
HonorAgent::Actions CoutersForAll;
HonorAgent::Actions CoutersForBullys;
HonorAgent::Actions CoutersForHonors;
HonorAgent::Actions CoutersForCPolice;
HonorAgent::Actions CoutersForRationals;

void CalculateStatistics(wb_dynmatrix<HonorAgent>& World)
{
	unsigned N=SIDE*SIDE;
	double ForFeiRep=0;
	double ForCallRe=0;
	double ForPower=0;
	double ForAgres=0;
	double ForHonor=0;

	//Liczniki akcji dla wszystkich i dla grup
	CoutersForAll.Reset();
	CoutersForBullys.Reset();
	CoutersForHonors.Reset();
	CoutersForCPolice.Reset();
	CoutersForRationals.Reset();

	zliczacz::Reset(MnStrenght,NumOfCounters);

	for(unsigned v=0;v<SIDE;v++)
	{
		for(unsigned h=0;h<SIDE;h++)
		{
			HonorAgent& Ag=World[v][h];			//Zapami�tanie referencji do agenta, �eby ci�gle nie liczy� indeks�w

			ForFeiRep+=Ag.GetFeiReputation();
			//Przynale�no�c do grup jest wa�ona waro�ci� prawdopodobie�stwa zachowania
			ForCallRe+=Ag.CallPolice;//... ale jak wsyztsko jest =1 albo 0
			ForPower+=Ag.Power;      //to zwyk�a proporcja w populacji
			ForAgres+=Ag.Agres;
			ForHonor+=Ag.Honor;

			if(0.9<Ag.Agres) //AGRESYWNI (BULLY) - Tylko najwy�sze warto�ci
			{
			   MnStrenght[0].summ+=Ag.Power;MnStrenght[0].N++;
			}
			else
			if(0.9<Ag.Honor) //Jak nie, to jako HONORowi
			{
			   MnStrenght[1].summ+=Ag.Power;MnStrenght[1].N++;
			}
			else
			if(0.9<Ag.CallPolice) //Jak nie to mo�e zawsze wzywaj�cy POLICEje
			{
				MnStrenght[2].summ+=Ag.Power;MnStrenght[2].N++;
			}
			else //W ostateczno�ci zwykli luserzy
			{
             	MnStrenght[3].summ+=Ag.Power;MnStrenght[3].N++;
			}

			//ZLICZANIE DECYZJI
			HonorAgent::Decision Deci=Ag.LastDecision(false);

			CoutersForAll.Count(Deci); //Zliczanie dla wszystkich
			if(Ag.Agres!=0) CoutersForBullys.Count(Deci);// i dla agresywnych
			else
			if(Ag.Honor!=0) CoutersForHonors.Count(Deci);// i dla honorowych
			else
			if(Ag.CallPolice!=0) CoutersForCPolice.Count(Deci);// i dla policyjnych
			else                 CoutersForRationals.Count(Deci);// i dla racjonalnych

			}
	}

   //Ostateczne u�rednienie
	MeanFeiReputation=ForFeiRep/N;
	MeanCallPolice=ForCallRe/N;
	MeanPower=ForPower/N;
	MeanAgres=ForAgres/N;
	MeanHonor=ForHonor/N;
}

void save_stat()
{
	if(step_counter==0 && LastStep!=0 && RepetNum==1 )//Tylko za pierwszym razem
	{
		LastStep=0;
		Parameters_dump(OutLog);
		if(REPETITION_LIMIT>1)//Gdy ma zrobi� wi�cej powt�rze� tego samego eksperymentu
			OutLog<<"REPET N#"<<'\t'; //Kt�ra to kolejna repetycja?

		OutLog<<"MC_STEP";
		//MnStrenght&MnStrNam[NumOfCounters]
		for(unsigned i=0;i<NumOfCounters;i++)
			OutLog<<'\t'<<MnStrNam[i];
		for(unsigned i=0;i<NumOfCounters;i++)
			OutLog<<"\tNof"<<MnStrNam[i];
		OutLog<<'\t'<<"MeanAgres"<<'\t'<<"MeanHonor"<<'\t'<<"MeanCallPolice"<<'\t'
			  <<"MeanFeiReputation"<<'\t'<<"MeanPower"<<'\t'<<"All killed"
			  <<'\t'<<"NOTHING"<<  '\t'<<"WITHDRAW"<<  '\t'<<"GIVEUP"<<  '\t'<<"HOOK"<<  '\t'<<"FIGHT"<<  '\t'<<"CALLAUTH"
			  <<'\t'<<"A.NOTHING"<<'\t'<<"A.WITHDRAW"<<'\t'<<"A.GIVEUP"<<'\t'<<"A.HOOK"<<'\t'<<"A.FIGHT"<<'\t'<<"A.CALLAUTH"
			  <<'\t'<<"H.NOTHING"<<'\t'<<"H.WITHDRAW"<<'\t'<<"H.GIVEUP"<<'\t'<<"H.HOOK"<<'\t'<<"H.FIGHT"<<'\t'<<"H.CALLAUTH"
			  <<'\t'<<"C.NOTHING"<<'\t'<<"C.WITHDRAW"<<'\t'<<"C.GIVEUP"<<'\t'<<"C.HOOK"<<'\t'<<"C.FIGHT"<<'\t'<<"C.CALLAUTH"
			  <<'\t'<<"R.NOTHING"<<'\t'<<"R.WITHDRAW"<<'\t'<<"R.GIVEUP"<<'\t'<<"R.HOOK"<<'\t'<<"R.FIGHT"<<'\t'<<"R.CALLAUTH"
			  //RATIOS
			  <<'\t'<<"R%:A.NOTHING"<<'\t'<<"R%:A.WITHDRAW"<<'\t'<<"R%:A.GIVEUP"<<'\t'<<"R%:A.HOOK"<<'\t'<<"R%:A.FIGHT"<<'\t'<<"R%:A.CALLAUTH"
			  <<'\t'<<"R%:H.NOTHING"<<'\t'<<"R%:H.WITHDRAW"<<'\t'<<"R%:H.GIVEUP"<<'\t'<<"R%:H.HOOK"<<'\t'<<"R%:H.FIGHT"<<'\t'<<"R%:H.CALLAUTH"
			  <<'\t'<<"R%:C.NOTHING"<<'\t'<<"R%:C.WITHDRAW"<<'\t'<<"R%:C.GIVEUP"<<'\t'<<"R%:C.HOOK"<<'\t'<<"R%:C.FIGHT"<<'\t'<<"R%:C.CALLAUTH"
			  <<'\t'<<"R%:R.NOTHING"<<'\t'<<"R%:R.WITHDRAW"<<'\t'<<"R%:R.GIVEUP"<<'\t'<<"R%:R.HOOK"<<'\t'<<"R%:R.FIGHT"<<'\t'<<"R%:R.CALLAUTH";
		OutLog<<endl;

		if(REPETITION_LIMIT>1)//Gdy ma zrobi� wi�cej powt�rze� tego samego eksperymentu
			OutLog<<RepetNum<<'\t'; //Kt�ra to kolejna repetycja?
		OutLog<<"0.9"; //????

		//MnStrenght&MnStrNam[NumOfCounters]
		for(unsigned i=0;i<NumOfCounters;i++)
			OutLog<<'\t'<<(MnStrenght[i].N>0?MnStrenght[i].summ/MnStrenght[i].N:-9999);
		for(unsigned i=0;i<NumOfCounters;i++)
			OutLog<<'\t'<<MnStrenght[i].N;

		OutLog<<'\t'<<MeanAgres<<'\t'<<MeanHonor<<'\t'<<MeanCallPolice<<'\t'
			  <<MeanFeiReputation<<'\t'<<MeanPower<<'\t'<<LiczbaTrupow<<'\t'
			  <<      CoutersForAll.NOTHING<<'\t'<<      CoutersForAll.WITHDRAW<<'\t'<<      CoutersForAll.GIVEUP<<'\t'<<CoutersForAll.HOOK<<'\t'<<CoutersForAll.FIGHT<<'\t'<<CoutersForAll.CALLAUTH<<'\t'
			  <<   CoutersForBullys.NOTHING<<'\t'<<   CoutersForBullys.WITHDRAW<<'\t'<<   CoutersForBullys.GIVEUP<<'\t'<<CoutersForBullys.HOOK<<'\t'<<CoutersForBullys.FIGHT<<'\t'<<CoutersForBullys.CALLAUTH<<'\t'
			  <<   CoutersForHonors.NOTHING<<'\t'<<   CoutersForHonors.WITHDRAW<<'\t'<<   CoutersForHonors.GIVEUP<<'\t'<<CoutersForHonors.HOOK<<'\t'<<CoutersForHonors.FIGHT<<'\t'<<CoutersForHonors.CALLAUTH<<'\t'
			  <<  CoutersForCPolice.NOTHING<<'\t'<<  CoutersForCPolice.WITHDRAW<<'\t'<<  CoutersForCPolice.GIVEUP<<'\t'<<CoutersForCPolice.HOOK<<'\t'<<CoutersForCPolice.FIGHT<<'\t'<<CoutersForCPolice.CALLAUTH<<'\t'
			  <<CoutersForRationals.NOTHING<<'\t'<<CoutersForRationals.WITHDRAW<<'\t'<<CoutersForRationals.GIVEUP<<'\t'<<CoutersForRationals.HOOK<<'\t'<<CoutersForRationals.FIGHT<<'\t'<<CoutersForRationals.CALLAUTH<<'\t'
			  //RATIOS
			  <<   CoutersForBullys.R_NOTHING()<<'\t'<<   CoutersForBullys.R_WITHDRAW()<<'\t'<<   CoutersForBullys.R_GIVEUP()<<'\t'<<CoutersForBullys.R_HOOK()<<'\t'<<CoutersForBullys.R_FIGHT()<<'\t'<<CoutersForBullys.R_CALLAUTH()<<'\t'
			  <<   CoutersForHonors.R_NOTHING()<<'\t'<<   CoutersForHonors.R_WITHDRAW()<<'\t'<<   CoutersForHonors.R_GIVEUP()<<'\t'<<CoutersForHonors.R_HOOK()<<'\t'<<CoutersForHonors.R_FIGHT()<<'\t'<<CoutersForHonors.R_CALLAUTH()<<'\t'
			  <<  CoutersForCPolice.R_NOTHING()<<'\t'<<  CoutersForCPolice.R_WITHDRAW()<<'\t'<<  CoutersForCPolice.R_GIVEUP()<<'\t'<<CoutersForCPolice.R_HOOK()<<'\t'<<CoutersForCPolice.R_FIGHT()<<'\t'<<CoutersForCPolice.R_CALLAUTH()<<'\t'
			  <<CoutersForRationals.R_NOTHING()<<'\t'<<CoutersForRationals.R_WITHDRAW()<<'\t'<<CoutersForRationals.R_GIVEUP()<<'\t'<<CoutersForRationals.R_HOOK()<<'\t'<<CoutersForRationals.R_FIGHT()<<'\t'<<CoutersForRationals.R_CALLAUTH()<<'\t';
		OutLog<<endl; //KONIEC LINII LOGU

		if(ConsoleLog)
		{
		  cout<<"STEP"<<setw(4)<<'\t'<<"MnAgres"<<'\t'<<"MnHonor"<<'\t'<<"MnFeiRep"<<'\t'<<"MnCallRep"<<'\t'<<"MnPower";
		  for(unsigned i=0;i<NumOfCounters;i++)
					cout<<'\t'<<MnStrNam[i];
		  cout<<endl;
		}
		//MnStrenght&MnStrNam[NumOfCounters]
	}
	else
	if(LastStep!=step_counter)
	{
		if(REPETITION_LIMIT>1)//Gdy ma zrobi� wi�cej powt�rze� tego samego eksperymentu
			OutLog<<RepetNum<<'\t'; //Kt�ra to kolejna repetycja?

		OutLog<<(step_counter>0?step_counter:0.1);
		//MnStrenght&MnStrNam[NumOfCounters]
		for(unsigned i=0;i<NumOfCounters;i++)
			OutLog<<'\t'<<(MnStrenght[i].N>0?MnStrenght[i].summ/MnStrenght[i].N:-9999);
		for(unsigned i=0;i<NumOfCounters;i++)
			OutLog<<'\t'<<MnStrenght[i].N;

		OutLog<<'\t'<<MeanAgres<<'\t'<<MeanHonor<<'\t'<<MeanCallPolice<<'\t'<<MeanFeiReputation<<'\t'<<MeanPower<<'\t'<<LiczbaTrupow<<'\t'
			  <<CoutersForAll.NOTHING<<'\t'<<CoutersForAll.WITHDRAW<<'\t'<<CoutersForAll.GIVEUP<<'\t'<<CoutersForAll.HOOK<<'\t'<<CoutersForAll.FIGHT<<'\t'<<CoutersForAll.CALLAUTH<<'\t'
			  <<CoutersForBullys.NOTHING<<'\t'<<CoutersForBullys.WITHDRAW<<'\t'<<CoutersForBullys.GIVEUP<<'\t'<<CoutersForBullys.HOOK<<'\t'<<CoutersForBullys.FIGHT<<'\t'<<CoutersForBullys.CALLAUTH<<'\t'
			  <<CoutersForHonors.NOTHING<<'\t'<<CoutersForHonors.WITHDRAW<<'\t'<<CoutersForHonors.GIVEUP<<'\t'<<CoutersForHonors.HOOK<<'\t'<<CoutersForHonors.FIGHT<<'\t'<<CoutersForHonors.CALLAUTH<<'\t'
			  <<CoutersForCPolice.NOTHING<<'\t'<<CoutersForCPolice.WITHDRAW<<'\t'<<CoutersForCPolice.GIVEUP<<'\t'<<CoutersForCPolice.HOOK<<'\t'<<CoutersForCPolice.FIGHT<<'\t'<<CoutersForCPolice.CALLAUTH<<'\t'
			  <<CoutersForRationals.NOTHING<<'\t'<<CoutersForRationals.WITHDRAW<<'\t'<<CoutersForRationals.GIVEUP<<'\t'<<CoutersForRationals.HOOK<<'\t'<<CoutersForRationals.FIGHT<<'\t'<<CoutersForRationals.CALLAUTH<<'\t'
			  //RATIOS
			  <<   CoutersForBullys.R_NOTHING()<<'\t'<<   CoutersForBullys.R_WITHDRAW()<<'\t'<<   CoutersForBullys.R_GIVEUP()<<'\t'<<CoutersForBullys.R_HOOK()<<'\t'<<CoutersForBullys.R_FIGHT()<<'\t'<<CoutersForBullys.R_CALLAUTH()<<'\t'
			  <<   CoutersForHonors.R_NOTHING()<<'\t'<<   CoutersForHonors.R_WITHDRAW()<<'\t'<<   CoutersForHonors.R_GIVEUP()<<'\t'<<CoutersForHonors.R_HOOK()<<'\t'<<CoutersForHonors.R_FIGHT()<<'\t'<<CoutersForHonors.R_CALLAUTH()<<'\t'
			  <<  CoutersForCPolice.R_NOTHING()<<'\t'<<  CoutersForCPolice.R_WITHDRAW()<<'\t'<<  CoutersForCPolice.R_GIVEUP()<<'\t'<<CoutersForCPolice.R_HOOK()<<'\t'<<CoutersForCPolice.R_FIGHT()<<'\t'<<CoutersForCPolice.R_CALLAUTH()<<'\t'
			  <<CoutersForRationals.R_NOTHING()<<'\t'<<CoutersForRationals.R_WITHDRAW()<<'\t'<<CoutersForRationals.R_GIVEUP()<<'\t'<<CoutersForRationals.R_HOOK()<<'\t'<<CoutersForRationals.R_FIGHT()<<'\t'<<CoutersForRationals.R_CALLAUTH()<<'\t';


		OutLog<<endl;

		if(ConsoleLog)
		{
		  cout<<step_counter<<setw(4)<<setprecision(3)<<'\t'<<MeanAgres<<'\t'<<MeanHonor<<'\t'<<MeanFeiReputation<<'\t'<<MeanCallPolice<<'\t'<<MeanPower;
		  for(unsigned i=0;i<NumOfCounters;i++)
			cout<<'\t'<<setw(4)<<(MnStrenght[i].N>0?MnStrenght[i].summ/MnStrenght[i].N:-1);
		  cout<<endl;
		}

		LastStep=step_counter;//Zapisuje �eby nie wypisywa� podw�jnie do logu
	}
}

void dump_step(wb_dynmatrix<HonorAgent>& World,unsigned step)
{
	const char TAB='\t';
	if(RepetNum==1 && step==0) //W kroku zerowym metryczka i nag��wek
	{
	   Parameters_dump(Dumps);
	   Dumps<<"Repet"<<TAB<<"Step"<<TAB<<"Vert"<<TAB<<"Hori"<<TAB;
	   Dumps<<"Ag_ID"<<TAB<<"Power"<<TAB<<"PowLimit"<<TAB
				 <<"Reputation"<<TAB

				 <<"ProAgresion"<<TAB
				 <<"ProHonor"<<TAB
				 <<"ProCallPolice"<<TAB

				 <<"CULTURE_TXT"<<TAB
				 <<"CULTURE_NUM"<<TAB

				 <<"LastDec_TXT"<<TAB
				 <<"LastDec_NUM"<<TAB

				 <<"NLifeTime"<<TAB
				 <<"NumOfActions"<<TAB
				 <<"R_NOTHING"<<TAB
				 <<"R_WITHDRAW"<<TAB
				 <<"R_GIVEUP"<<TAB
				 <<"R_HOOK"<<TAB
				 <<"R_FIGHT" <<TAB
				 <<"R_CALLAUTH"<<TAB

				 <<"NeighSize"<<endl;
	}
	for(unsigned v=0;v<SIDE;v++)
	{
		for(unsigned h=0;h<SIDE;h++)
		{
			HonorAgent& Ag=World[v][h];	//Zapami�tanie referencji
			Dumps<< scientific         //Wszystkie inty w double �eby SPSS �atwo czyta� (swoj� drog� co za g�upi program!)
				 <<(double)RepetNum<<TAB
				 <<(double)step<<TAB
				 <<(double)v<<TAB
				 <<(double)h<<TAB;
			Dumps<<(double)Ag.ID<<TAB
				 << setprecision(5)
				 <<(double)Ag.Power<<TAB
				 <<(double)Ag.PowLimit<<TAB
				 <<(double)Ag.GetFeiReputation()<<TAB;

			Dumps<<(double)Ag.Agres<<TAB
				 <<(double)Ag.Honor<<TAB
				 <<(double)Ag.CallPolice<<TAB;

			Dumps<<Ag.AgentCultureStr().get()<<TAB
				 << scientific << setprecision(5)
				 <<(double)Ag.AgentCultureMask()<<TAB;

			Dumps<<HonorAgent::Decision2str(Ag.LastDecision(false))<<TAB//<<setw(4)<< setprecision(2)?
				 <<(double)Ag.LastDecision(false)<<TAB;

			Dumps<< scientific << setprecision(7)
				 <<(double)Ag.HisLifeTime<<TAB
				 <<(double)Ag.HisActions.Counter<<TAB;

			Dumps<< scientific << setprecision(2)
				 <<Ag.HisActions.R_NOTHING()<<TAB
				 <<Ag.HisActions.R_WITHDRAW()<<TAB
				 <<Ag.HisActions.R_GIVEUP()<<TAB
				 <<Ag.HisActions.R_HOOK() <<TAB
				 <<Ag.HisActions.R_FIGHT() <<TAB
				 <<Ag.HisActions.R_CALLAUTH()<<TAB;

			Dumps<<(double)Ag.NeighSize()<<endl;
		}
	}
	Dumps<<endl;//Po ca�o�ci
}

void PrintHonorAgentInfo(ostream& o,const HonorAgent& H)
// Drukowanie agenta do inspekcji
{
	o<<"Agent type:"<<H.AgentCultureStr().get()<<"\t\t";
	o<<"Agression prob.: "<<H.Agres<<"\t";// Bulizm (0..1) sk�onno�� do atakowania
	o<<"Honor: "<<H.Honor<<"\t";// Bezwarunkowa honorowo�� (0..1) sk�onno�� podj�cia obrony
	o<<"Police call.: "<<H.CallPolice<<endl;//Odium wzywacza policji - prawdopodobie�stwo wzywania policji (0..1) jako �w�drowna �rednia� wezwa�
	o<<endl;
	o<<"Curr. strenght: "<<H.Power<<" streght limit: "<<H.PowLimit<<endl;//	Si�a (0..1) i jak� si�� mo�e osi�gn�� maksymalnie, gdy nie traci
	o<<"Feighter reputation: "<<H.GetFeiReputation()<<endl;//Reputacja wojownika jako �w�drowna �rednia� z konfrontacji (0..1)
	o<<endl;
	o<<"Life time: "<<"\t Succeses"<<"\t Fails"<<"\t Action counter"<<endl;
	o<<'\t'<<H.HisLifeTime<<'\t'<<H.HisActions.Succeses<<'\t'<<H.HisActions.Fails<<'\t'<<H.HisActions.Counter<<endl;
	o<<HonorAgent::Decision2str(HonorAgent::NOTHING)<<'\t'<<HonorAgent::Decision2str(HonorAgent::WITHDRAW)<<'\t'<<HonorAgent::Decision2str(HonorAgent::GIVEUP)<<'\t'<<HonorAgent::Decision2str(HonorAgent::HOOK)<<'\t'<<HonorAgent::Decision2str(HonorAgent::FIGHT)<<'\t'<<HonorAgent::Decision2str(HonorAgent::CALLAUTH)<<endl;
	o<<'\t'<<H.HisActions.R_NOTHING()*100<<"%\t"<<H.HisActions.R_WITHDRAW()*100<<"%\t"<<H.HisActions.R_GIVEUP()*100<<"%\t"<<H.HisActions.R_HOOK()*100<<"%\t"<<H.HisActions.R_FIGHT()*100<<"%\t"<<H.HisActions.R_CALLAUTH()*100<<endl;
	o<<endl;
	o<<"Individual color: rgb("<<unsigned(H.GetColor().r)<<','<<unsigned(H.GetColor().g)<<','<<unsigned(H.GetColor().b)<<')'<<endl;	//Obliczony, kt�r�� z funkcji koduj�cych kolor

	o<<endl;
}

// WIZUALIZACJA
/////////////////////////////////////////////////////////////////////////////

void replot(wb_dynmatrix<HonorAgent>& World)
{
	int old=mouse_activity(0);
	//clear_screen();
	unsigned spw=screen_width()-SIDE*SSIZ;
	unsigned StartDyn=(SIDE+1)*SSIZ+char_height('X')+1;//Gdzie si� zaczyna wizualizacja pomocnicza
	unsigned StartPow=StartDyn+(SIDE+1)*SSIZ+char_height('X')+1;//Gdzie si� zaczyna wizualizacja aktywno�ci
	unsigned StartPro=StartPow+(SIDE+1)*SSIZ+char_height('X')+1;

	//double RealMaxReputation=0;
	//double SummReputation=0;

	//DRUKOWANIE POWI�ZA�
	int VSIZ2=VSIZ/2;

	if(VisFarLinks || VisShorLinks)
	for(unsigned n=0;n<SIDE*SIDE;n++)
	{
		unsigned v=RANDOM(SIDE);
		unsigned h=RANDOM(SIDE);

		HonorAgent& Ag=World[v][h];			//Zapami�tanie referencji do agenta, �eby ci�gle nie liczy� indeks�w
		unsigned NofL=Ag.NeighSize();
		ssh_rgb c=Ag.GetColor();

		for(unsigned x,y,l=0;l<NofL;l++)
		 if(Ag.getNeigh(l,x,y))
		 {
			if(l<MOORE_SIZE && VisShorLinks)
			{
			set_pen_rgb(c.r,c.g,c.b,0,SSH_LINE_SOLID);// Ustala aktualny kolor linii za pomoca skladowych RGB
			line_d(h*VSIZ+VSIZ2,v*VSIZ+VSIZ2,x*VSIZ+VSIZ2,y*VSIZ+VSIZ2);
			}
			if(l>=MOORE_SIZE && VisFarLinks)
			{
			set_pen_rgb(c.r,c.g,c.b,1,SSH_LINE_SOLID);// Dla dalekich nieco grubsze
			line_d(h*VSIZ+VSIZ2,v*VSIZ+VSIZ2,x*VSIZ+VSIZ2,y*VSIZ+VSIZ2);
			}
		 }
	}

	set_pen_rgb(75,75,75,0,SSH_LINE_SOLID); // Ustala aktualny kolor linii za pomoca skladowych RGB

	//DRUKOWANIE DANYCH DLA W�Z��W
	for(unsigned v=0;v<SIDE;v++)
	{
		for(unsigned h=0;h<SIDE;h++)
		{

			HonorAgent& Ag=World[v][h];			//Zapami�tanie referencji do agenta, �eby ci�gle nie liczy� indeks�w
			ssh_rgb Color=Ag.GetColor();

			if(VisAgents)
			{
				if(!(VisFarLinks || VisShorLinks))
					fill_rect(h*VSIZ,v*VSIZ,h*VSIZ+VSIZ,v*VSIZ+VSIZ,255+128); //Generowanie nieprzezroczystego t�a

				//set_brush_rgb(Ag.GetFeiReputation()*255,0,Ag.CallPolice*255);
				//set_pen_rgb(  Ag.GetFeiReputation()*255,0,Ag.CallPolice*255,1,SSH_LINE_SOLID);
				set_brush_rgb(Ag.Agres*255,Ag.Honor*255,Ag.CallPolice*255);
				if(VisReputation)
				{   //Ag.GetFeiReputation()*255,0,Ag.CallPolice*255
					//Ag.GetFeiReputation()*255,Ag.GetFeiReputation()*255,0
					set_pen_rgb(Ag.GetFeiReputation()*255,Ag.GetFeiReputation()*255,Ag.CallPolice*255,1,SSH_LINE_SOLID);
				}
				else
					set_pen_rgb(Ag.Agres*255,Ag.Honor*255,Ag.CallPolice*255,1,SSH_LINE_SOLID);

				unsigned ASiz=1+/*sqrt*/(Ag.Power)*VSIZ;
				fill_rect_d(h*VSIZ,v*VSIZ,h*VSIZ+ASiz,v*VSIZ+ASiz);
				fill_rect_d(spw+h*SSIZ,StartPro+v*SSIZ,spw+(h+1)*SSIZ,StartPro+(v+1)*SSIZ);

				line_d(h*VSIZ,v*VSIZ+ASiz,h*VSIZ+ASiz,v*VSIZ+ASiz);
				line_d(h*VSIZ+ASiz,v*VSIZ,h*VSIZ+ASiz,v*VSIZ+ASiz);
				//if(Ag.Agres>0 || Ag.Honor>0)
				//	plot_rgb(h*VSIZ+ASiz/2,v*VSIZ+ASiz/2,Ag.Agres*255,Ag.Agres*255,Ag.Honor*255);
				//set_brush_rgb(Ag.Power*Ag.Agres*255,Ag.Power*Ag.Honor*255,0);//Wzpenienie jako kombinacje si�y i sk�onno�ci
				//fill_circle_d(h*VSIZ+VSIZ2,v*VSIZ+VSIZ2,);   // G��wny rysunek HonorAgenta ...
			}

			//Wizualizacja "Reputacji" w odcieniach szaro�ci?
			set_pen_rgb(  Ag.GetFeiReputation()*255,0,Ag.CallPolice*255,1,SSH_LINE_SOLID);
			set_brush_rgb(Ag.GetFeiReputation()*255,0,Ag.CallPolice*255);
			fill_rect_d(spw+h*SSIZ,v*SSIZ,spw+(h+1)*SSIZ,(v+1)*SSIZ);

			//Wizualizacja si�y w odcieniach szaro�ci
			ssh_color ColorPow=257+unsigned(Ag.Power*254); //
			fill_rect(spw+h*SSIZ,StartPow+v*SSIZ,spw+(h+1)*SSIZ,StartPow+(v+1)*SSIZ,ColorPow);

			//Dynamika proces�w - punkty w r�nych kolorach co si� dzie�o w ostatnim kroku
			ssh_color ColorDyn=0;
			switch(Ag.LastDecision(false)){
			case HonorAgent::WITHDRAW:
								ColorDyn=180;  break;
			case HonorAgent::GIVEUP:
								ColorDyn=255;  break;
			case HonorAgent::HOOK:
								ColorDyn=254;  break;
			case HonorAgent::FIGHT:
								ColorDyn=70;  break;
			case HonorAgent::CALLAUTH:
								ColorDyn=128;  break;
			default:
								ColorDyn=0;	 break;
			}
			fill_rect(spw+h*SSIZ,StartDyn+v*SSIZ,spw+(h+1)*SSIZ,StartDyn+(v+1)*SSIZ,ColorDyn);
			if(VisDecision)
			{
			 unsigned ASiz=1+/*sqrt*/(Ag.Power)*VSIZ;
			 plot(h*VSIZ+ASiz/2,v*VSIZ+ASiz/2,ColorDyn);
			}
		}
	}

	printc(1,screen_height()-char_height('X'),100,255,"%s MnAgres=%f  MnHonor=%f  MnPoli=%f Killed=%u  AllKilled=%u  ",
										"COMPONENT VIEW",
									double(MeanAgres),double(MeanHonor),double(MeanCallPolice),LiczbaTrupowDzis,LiczbaTrupow);
	/*double MeanFeiReputation=0;
double MeanCallPolice=0;
double MeanPower=0;*/
	printc(spw,(SIDE+1)*SSIZ,150,255,        "%s Mn.Fe=%g Mn.Cp=%g    ","Reput.",double(MeanFeiReputation),double(MeanCallPolice));   // HonorAgent::MaxReputation
	printc(spw,StartPow+(SIDE+1)*SSIZ,50,255,"%s mn=%f                ","Power",double( MeanPower ));
	printc(spw,StartDyn+(SIDE+1)*SSIZ,50,255,"%s H:%u F:%u C:%u       ","Local interactions",CoutersForAll.HOOK,CoutersForAll.FIGHT,CoutersForAll.CALLAUTH);
	printc(spw,StartPro+(SIDE+1)*SSIZ,50,255,"%s A:%u H:%u P:%u O:%u      ","Counters",MnStrenght[0].N,MnStrenght[1].N,MnStrenght[2].N,MnStrenght[3].N);
	printc(spw,StartPro+(SIDE+1)*SSIZ+char_height('X'),128,255,"%u MC ",step_counter);
	//HonorAgent::Max...=Real...;//Aktualizacja max-�w do policzonych przed chwil� realnych
	flush_plot();

	save_stat();

	mouse_activity(old);
}



/*  OGOLNA FUNKCJA MAIN I TO CO JEJ POTRZEBNE */
/**********************************************/
void Help();
void SaveScreen(unsigned step);
void mouse_check(wb_dynmatrix<HonorAgent>& World);
void fixed_params_mode();//Tryb interakcyjny z pe�n� wizualizacj�
//Tryby analizy przestrzeni parametr�w
void walk_params_sele(); //--> selekcja vs efektywno�� policji
void walk_params_prop(); //--> honorowi/policyjni vs efektywno�� policji
void walk_honor_vs_agrr();//--> honorowi/agresywni vs selekcja

int main(int argc,const char* argv[])
{
	cout<<MODELNAME<<" v.:"<<VERSIONNUM<<" PID:"<<(getpid())<<" "<<endl<<
		"======================================================================"<<endl<<
  //		"(programmed by Wojciech Borkowski from University of Warsaw)\n"
		"        "<<endl
		<<endl;
	cout<<"Use -help for graphics setup information,\nor HELP for information about available parameters."<<endl;
	if(OptionalParameterBase::parse_options(argc,argv,Parameters,sizeof(Parameters)/sizeof(Parameters[0])))
			return 1;

	mouse_activity(1);
	set_background(255);
	buffering_setup(1);// Czy wlaczona animacja
	if(batch_mode) fix_size(0);
		else fix_size(1);

	char bufornazwy[128];
	sprintf(bufornazwy,"%s %s",MODELNAME,VERSIONNUM);
	shell_setup(bufornazwy,argc,argv);

	cout<<"\n MODEL CONFIGURATION: "<<endl;

	Parameters_dump(cout);

	if(!init_plot(SIDE*VSIZ+20+SIDE*SSIZ,SIDE*VSIZ,0,1)) //Na g��wn� wizualizacj� swiata i jakie� boki
	{
		cerr<<"Can't initialize graphics"<<endl;
		exit(-1);
	}

	char* SPom;
	if((SPom=strstr(LogName.get_ptr_val(),"XXX"))!=0)//Nie by�o zmiany nazwy z linii komend
	{
		*SPom='\0'; //Obci�cie
		LogName=MakeFileName(LogName.get());//?
	}
	if((SPom=strstr(DumpNam.get_ptr_val(),"XXX"))!=0)//Nie by�o zmiany nazwy z linii komend
	{
		*SPom='\0'; //Obci�cie
		DumpNam=MakeFileName(DumpNam.get());//?
	}

	string tmp=LogName.get(); tmp+=".log";
	OutLog.open(tmp.c_str());//"Honor.log";
	tmp=DumpNam.get(); tmp+=".txt";
	Dumps.open(tmp.c_str()); //"Honor_dump.txt";

	if(!OutLog.is_open())
			{ perror(tmp.c_str());exit(-2);}
	if(!Dumps.is_open())
			{ perror(tmp.c_str());exit(-3);}

	if(RandSeed>0)
	{
	 SRAND(RandSeed); //Właśnie żeby było powtarzalnie
	}
	else
	RANDOMIZE();//"Niepowtarzalnie" musi być z czasu (ale to jest z dokładnością do sekundy!!!)


	if(batch_mode)//Tryb analizy przestrzeni parametr�w
	{
		switch(batch_sele){ //Czy tryb przeszukiwania szuka po proporcjach czy po sile selekcji?
		case BAT_SELECTION:walk_params_sele();break;//=1,
		case BAT_HONORvsCPOLL:walk_params_prop();break;//=2,
		case BAT_HONORvsAGRR:walk_honor_vs_agrr();break;//=3
		case NO_BAT://=0
		default:
			cerr<<"\n\aInvalid batch mode!\n"<<endl;
			exit(-111);
		}
	}
	else
		fixed_params_mode();//Tryb interakcyjny z pe�n� wizualizacj�

	cout<<endl<<"Bye, bye!"<<endl;
	close_plot();
	return 0;
}

void PlotTables(const char* Name1,wb_dynmatrix<FLOAT>& Tab1,
				const char* Name2,wb_dynmatrix<FLOAT>& Tab2,
				const char* Name3,wb_dynmatrix<FLOAT>& Tab3,
				const char* Name4,wb_dynmatrix<FLOAT>& Tab4,bool true_color=false) //enum  {NO_BAT=0,BAT_SELECTION=1,BAT_HONORvsCPOLL=2,BAT_HONORvsAGRR=3} batch_sele;//Czy tryb przeszukiwania szuka po proporcjach czy po sile selekcji?
{
	 unsigned W=screen_width()/2;
	 unsigned Ws=(W-2)/Tab1[0].get_size();
	 unsigned H=(screen_height()-char_height('X'))/2;
	 unsigned Hs=(H-1-char_height('X'))/Tab1.get_size();
	 unsigned H2=Tab1.get_size()*Hs+char_height('X')+1;

	 if(true_color)
	 {
		for(unsigned i=0;i<256;i++)
		{
			set_pen_rgb(i,i,i,2,SSH_LINE_SOLID);
			line_d(screen_width()-10,screen_height()-i,screen_width(),screen_height()-i);
		}
	 }
	 else
	 {
		 for(unsigned i=0;i<256;i++)
			line(screen_width()-10,screen_height()-i,screen_width(),screen_height()-i,i);
         }

	 for(unsigned Y=0;Y<Tab1.get_size();Y++)
	  for(unsigned X=0;X<Tab1[Y].get_size();X++)
	  {
		  int col1=(Tab1[Y][X]>=0?Tab1[Y][X] * 255: -128);
		  int col2=(Tab2[Y][X]>=0?Tab2[Y][X] * 255: -128);
		  int col3=(Tab3[Y][X]>=0?Tab3[Y][X] * 255: -128);
		  int col4=(Tab4[Y][X]>=0?Tab4[Y][X] * 255: -128);

		  if(true_color)
		  {
			if(col1>=0){set_brush_rgb(col1,0,0);
			fill_rect_d(X*Ws,Y*Hs,(X+1)*Ws,(Y+1)*Hs); }

			if(col2>=0){set_brush_rgb(0,col2,0);
			fill_rect_d(W+X*Ws,Y*Hs,W+(X+1)*Ws,(Y+1)*Hs);}

			if(col3>=0){set_brush_rgb(0,0,col3);
			fill_rect_d(X*Ws,H2+Y*Hs,(X+1)*Ws,H2+(Y+1)*Hs);}

			if(col4>=0){set_brush_rgb(col4,col4,col4);
			fill_rect_d(W+X*Ws,H2+Y*Hs,W+(X+1)*Ws,H2+(Y+1)*Hs);}
		  }
		  else
		  {
			if(col1>=0)fill_rect(X*Ws,Y*Hs,(X+1)*Ws,(Y+1)*Hs,col1);
			if(col2>=0)fill_rect(W+X*Ws,Y*Hs,W+(X+1)*Ws,(Y+1)*Hs,col2);
			if(col3>=0)fill_rect(X*Ws,H2+Y*Hs,(X+1)*Ws,H2+(Y+1)*Hs,col3);
			if(col4>=0)fill_rect(W+X*Ws,H2+Y*Hs,W+(X+1)*Ws,H2+(Y+1)*Hs,col4);
                  }
	  }

	 //print_transparently(1);
	 printbw(0,Tab1.get_size()*Hs,"%s",Name1);
	 printbw(W,Tab1.get_size()*Hs,"%s",Name2);
	 printbw(0,2*H2-char_height('X'),"%s",Name3);
	 printbw(W,2*H2-char_height('X'),"%s",Name4);

	 print_transparently(0);
	 switch(batch_sele){ //Czy tryb przeszukiwania szuka po proporcjach czy po sile selekcji?
		case BAT_SELECTION:printc(0,screen_height()-char_height('X'),55,255,
			"POL.EFF:%g-%g:%g SEL.:%g-%g:%g (last: B=%g H=%g C=%g) STEPS:%u NREP:%u STATSTART:%u  ",
						POLICE_EFFIC_MIN,POLICE_EFFIC_MAX,POLICE_EFFIC_STEP,
						SELECTION_MIN,SELECTION_MAX,SELECTION_STEP,
						BULLI_POPUL,HONOR_POPUL,CALLER_POPU,
						STOP_AFTER,
						REPETITION_LIMIT,
						STAT_AFTER
						);
				break;//=1,        walk_params_sele();
		case BAT_HONORvsCPOLL:printc(0,screen_height()-char_height('X'),55,255,
			"POL.EFF:%g-%g:%g PROP:%g-%g:%g (last: B=%g H=%g C=%g Sel=%g) STEPS:%u NREP:%u STATSTART:%u  ",
						POLICE_EFFIC_MIN,POLICE_EFFIC_MAX,POLICE_EFFIC_STEP,
						PROPORTION_MIN,PROPORTION_MAX,PROPORTION_STEP,
						BULLI_POPUL,HONOR_POPUL,CALLER_POPU,
						USED_SELECTION,
						STOP_AFTER,
						REPETITION_LIMIT,
						STAT_AFTER
						);
				break;//=2,    walk_params_prop();
		case BAT_HONORvsAGRR:printc(0,screen_height()-char_height('X'),55,255,
			"PROP:%g-%g:%g SEL.:%g-%g:%g (last: B=%g H=%g C=%g Sel=%g) STEPS:%u NREP:%u STATSTART:%u  ",
						PROPORTION_MIN,PROPORTION_MAX,PROPORTION_STEP,
						SELECTION_MIN,SELECTION_MAX,SELECTION_STEP,
						BULLI_POPUL,HONOR_POPUL,CALLER_POPU,
						USED_SELECTION,
						STOP_AFTER,
						REPETITION_LIMIT,
						STAT_AFTER
						);
				break;//=3    walk_honor_vs_agrr();
		case NO_BAT://=0
		default:
			cerr<<"\n\aInvalid batch mode!\n"<<endl;
			exit(-111);
		}
	 flush_plot();
}

void Write_tables(ostream& o,const char* Name1,wb_dynmatrix<FLOAT>& Tab1,
							const char* Name2,wb_dynmatrix<FLOAT>& Tab2,const char* TAB="\t")//enum  {NO_BAT=0,BAT_SELECTION=1,BAT_HONORvsCPOLL=2,BAT_HONORvsAGRR=3} batch_sele;//Czy tryb przeszukiwania szuka po proporcjach czy po sile selekcji?
{
	unsigned X=0,Y=0;
	wb_pchar pom(128);
	o<<endl;
	o<<TAB;

	//NAG��WKI TABEL
	switch(batch_sele){ //Czy tryb przeszukiwania szuka po proporcjach czy po sile selekcji?
	case BAT_HONORvsCPOLL:
	case BAT_SELECTION:X=0;
	for(FLOAT effic=POLICE_EFFIC_MIN;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
	{
		pom.prn("P.Ef%g",effic);
		o<<pom.get()<<TAB;
	}
	o<<TAB<<TAB;X=0;
	for(FLOAT effic=POLICE_EFFIC_MIN;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
	{
		pom.prn("P.Ef%g",effic);
		o<<pom.get()<<TAB;
	}
	break;
	case BAT_HONORvsAGRR:X=0;
	for(FLOAT prop=PROPORTION_MIN;prop<=PROPORTION_MAX;prop+=PROPORTION_STEP,X++)
	{
		pom.prn("Prop%g",prop);
		o<<pom.get()<<TAB;
	}
	o<<TAB<<TAB;X=0;
	for(FLOAT prop=PROPORTION_MIN;prop<=PROPORTION_MAX;prop+=PROPORTION_STEP,X++)
	{
		pom.prn("Prop%g",prop);
		o<<pom.get()<<TAB;
	}
	break;//=3    walk_honor_vs_agrr();
	}
	o<<X<<TAB<<"!!!"<<endl;

	//NAG��WKI WIERSZY i ZAWARTO�� TABEL
	Y=0;
	switch(batch_sele){ //Czy tryb przeszukiwania szuka po proporcjach czy po sile selekcji?
		case BAT_HONORvsCPOLL:
			for(FLOAT prop=PROPORTION_MIN;prop<=PROPORTION_MAX;prop+=PROPORTION_STEP,Y++)
			{
			   pom.prn("Prop%g",prop);
			   o<<pom.get()<<TAB;
			   X=0;
			   for(FLOAT effic=POLICE_EFFIC_MIN;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
			   {
				  o<<Tab1[Y][X]<<TAB;
			   }
			   o<<TAB;
			   pom.prn("Prop%g",prop);
			   o<<pom.get()<<TAB;
			   X=0;
			   for(FLOAT effic=POLICE_EFFIC_MIN;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
			   {
				  o<<Tab2[Y][X]<<TAB;
			   }
			   o<<endl;
			}
				break;//=1,        walk_params_sele();
		case BAT_SELECTION:  //Ma selekcj� w pionie
			for(FLOAT selec=SELECTION_MIN;selec<=SELECTION_MAX;selec+=SELECTION_STEP,Y++)
			{
				pom.prn("Sel_%g",selec);
				o<<pom.get()<<TAB;
				X=0;
				for(FLOAT effic=POLICE_EFFIC_MIN;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
					{ o<<Tab1[Y][X]<<TAB; }
				o<<TAB;
				pom.prn("Sel_%g",selec);
				o<<pom.get()<<TAB;
				X=0;
				for(FLOAT effic=POLICE_EFFIC_MIN;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
					{o<<Tab2[Y][X]<<TAB;}
				o<<endl;
			}
				break;//=2,    walk_params_prop();
		case BAT_HONORvsAGRR://Te� ma selekcj� w pionie
			for(FLOAT selec=SELECTION_MIN;selec<=SELECTION_MAX;selec+=SELECTION_STEP,Y++)
			{
				pom.prn("Sel_%g",selec);
				o<<pom.get()<<TAB;
				X=0;
				for(FLOAT prop=PROPORTION_MIN;prop<=PROPORTION_MAX;prop+=PROPORTION_STEP,X++)
				//for(FLOAT effic=POLICE_EFFIC_MIN;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
					{ o<<Tab1[Y][X]<<TAB; }
				o<<TAB;
				pom.prn("Sel_%g",selec);
				o<<pom.get()<<TAB;
				X=0;
				for(FLOAT prop=PROPORTION_MIN;prop<=PROPORTION_MAX;prop+=PROPORTION_STEP,X++)
				//for(FLOAT effic=POLICE_EFFIC_MIN;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
					{o<<Tab2[Y][X]<<TAB;}
				o<<endl;
			}
				break;//=3    walk_honor_vs_agrr();
		case NO_BAT: default: cerr<<"\n\aInvalid batch mode!\n"<< endl; exit(-111);
		}

		//Dwie ostatnie nazwy pod spodem
		o << TAB << Name1;
		for (unsigned i = 0; i < X; i++)
			o << TAB;
		o << TAB << TAB << Name2 << endl;
}

void walk_params_prop()
// Tryb analizy przestrzeni parametr�w - wydajno�� policji (POLL_EFF) i PROPORTION
{
   unsigned X = (POLICE_EFFIC_MAX - POLICE_EFFIC_MIN)/ POLICE_EFFIC_STEP + 1;

   if (!(POLICE_EFFIC_MIN + POLICE_EFFIC_STEP * X < POLICE_EFFIC_MAX))
		// Operacja == nie dzia�a poprawnie na float
			X++;
   unsigned Y = (PROPORTION_MAX - PROPORTION_MIN) / PROPORTION_STEP + 1;
   if(! (PROPORTION_MIN+PROPORTION_STEP*Y < PROPORTION_MAX) )  //Operacja == nie dzia�a poprawnie na float
										Y++;
   cout<<"Police efficiency vs proportion batch job: "<<Y<<" vs. "<<X<<" cells."<<endl;
   Parameters_dump(OutLog);
   OutLog<<"Police efficiency vs proportion batch job: "<<Y<<" vs. "<<X<<" cells."<<endl;

   wb_dynmatrix<FLOAT> MeanPowerOfAgres(Y,X);MeanPowerOfAgres.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfHonor(Y,X);MeanPowerOfHonor.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfPCall(Y,X);MeanPowerOfPCall.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfOther(Y,X);MeanPowerOfOther.fill(-9999.0);

   wb_dynmatrix<FLOAT> PropMnDiffOfAgres(Y,X);PropMnDiffOfAgres.fill(-9999.0);
   wb_dynmatrix<FLOAT> PropMnDiffOfHonor(Y,X);PropMnDiffOfHonor.fill(-9999.0);
   wb_dynmatrix<FLOAT> PropMnDiffOfPCall(Y,X);PropMnDiffOfPCall.fill(-9999.0);
   wb_dynmatrix<FLOAT> PropMnDiffOfOther(Y,X);PropMnDiffOfOther.fill(-9999.0);

   wb_dynmatrix<FLOAT> MeanPropOfAgres(Y,X);MeanPropOfAgres.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfHonor(Y,X);MeanPropOfHonor.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfPCall(Y,X);MeanPropOfPCall.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfOther(Y,X);MeanPropOfOther.fill(-9999.0);

   for(FLOAT prop=PROPORTION_MIN,Y=0;prop<=PROPORTION_MAX;prop+=PROPORTION_STEP,Y++)
   {
	   //if(BULLI_POPUL>=0)  //????
	   //			BULLI_POPUL=prop;

	   if(Compensation_mode)
	   {                             assert(fabs(BULLI_POPUL)>=0);
		  HONOR_POPUL=PROPORTION_MAX-prop;
		  CALLER_POPU=prop;
	   }
	   else
	   {
		if(HONOR_POPUL>=0)
				HONOR_POPUL=prop;
		if(CALLER_POPU>=0 )
				CALLER_POPU=prop;
	   }

	   for(FLOAT effic=POLICE_EFFIC_MIN,X=0;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
	   {
		   cout<<endl<<"SYM: "<<Y<<' '<<X<<" Prop(H:C): "<<HONOR_POPUL<<":"<<CALLER_POPU<<"\tPEffic: "<<effic<<"\tB:"<<BULLI_POPUL<<endl;
		   POLICE_EFFIC=effic;

		   //P�TLA POWT�RZE� SYMULACJI
		   FLOAT MnPowOfAgres=0;
		   FLOAT MnPowOfHonor=0;
		   FLOAT MnPowOfPCall=0;
		   FLOAT MnPowOfOther=0;
		   FLOAT MnPropOfAgres=0;
		   FLOAT MnPropOfHonor=0;
		   FLOAT MnPropOfPCall=0;
		   FLOAT MnPropOfOther=0;
		   FLOAT PrevPropOfAgres=abs(BULLI_POPUL);
		   FLOAT PrevPropOfHonor=abs(HONOR_POPUL);
		   FLOAT PrevPropOfPCall=abs(CALLER_POPU);
		   FLOAT PrevPropOfOther=1-abs(BULLI_POPUL)-abs(HONOR_POPUL)-abs(CALLER_POPU);

		   unsigned StatSteps=0;

		   for(unsigned rep=0;rep<REPETITION_LIMIT;rep++)
		   {
			 HonorAgent::World.alloc(SIDE,SIDE);//Pocz�tek - alokacja agent�w �wiata
			 double POPULATION=double(SIDE)*SIDE;   //Ile ich w og�le jest?
			 InitConnections(SIDE*SIDE*OUTFAR_LINKS_PER_AGENT);//Tworzenie sieci
			 InitAtributes(SIDE*SIDE); //Losowanie atrybut�w dla agent�w
			 //CalculateStatistics(World); //Po raz pierwszy dla tych parametr�w

			 for(step_counter=1;step_counter<=STOP_AFTER;/*step_counter++*/)
			 {
				Reset_action_memories();//Czyszczenie, mo�e niepotrzebne
				power_recovery_step(); // Krok procesu regeneracji si�
				one_step(step_counter); // Krok dynamiki interakcji agresywnych
				/*                      // step_counter++ JEST W �RODKU!
				if (SOCIAL_IMPACT_INTENSITY_PERCENT > 0)
					// Opcjonalnie krok wp�ywu spo�ecznego
						social_impact_step(World,
						SOCIAL_IMPACT_INTENSITY_PERCENT);
				*/
				if (step_counter % max(EveryStep,100u) == 0)
				{
					CalculateStatistics(HonorAgent::World);
					cout <<"\r["<<rep<<"] "<< step_counter<<"  ";

					if(step_counter>=STAT_AFTER) //Mo�e tylko koncowy stan r�wnowagi?
					{
						MnPowOfAgres += MnStrenght[0].N > 0 ? MnStrenght[0].summ / MnStrenght[0].N : 0;
						MnPowOfHonor += MnStrenght[1].N > 0 ? MnStrenght[1].summ / MnStrenght[1].N : 0;
						MnPowOfPCall += MnStrenght[2].N > 0 ? MnStrenght[2].summ / MnStrenght[2].N : 0;
						MnPowOfOther += MnStrenght[3].N > 0 ? MnStrenght[3].summ / MnStrenght[3].N : 0;
						MnPropOfAgres += MnStrenght[0].N / POPULATION;
						MnPropOfHonor += MnStrenght[1].N / POPULATION;
						MnPropOfPCall += MnStrenght[2].N / POPULATION;
						MnPropOfOther += MnStrenght[3].N / POPULATION;
						StatSteps++;
					}

					if (input_ready())
						{
							int znak = get_char();
							if (znak == 'v' || znak == 'c')
							{
								   BatchPlotPower=!BatchPlotPower;
								   clear_screen();
							}
							else
							if (znak == '\n')
								if(BatchPlotPower)
									PlotTables("MnPowOfAgres",MeanPowerOfAgres,"MnPowOfHonor",MeanPowerOfHonor,
									"MnPowOfPCall",MeanPowerOfPCall,"MnPowOfOther",MeanPowerOfOther,Batch_true_color);
								else
									PlotTables("MnPropOfAgres",MeanPropOfAgres,"MnPropOfHonor",MeanPropOfHonor,
									"MnPropOfPCall",MeanPropOfPCall,"MnPropOfOther",MeanPropOfOther,Batch_true_color);
						}
				}
			 } // KONIEC P�TLI POJEDYNCZEJ SYMULACJI
			 DeleteAllConnections();  //Koniec tej symulacji
			} // KONIEC P�TLI POWTORZEN
																				assert(StatSteps>0);
			// PODLICZENIE WYNIK�W I ZAPAMI�TANIE W TABLICACH PRZESTRZENI PARAMETR�W
			MnPowOfAgres /= StatSteps;
			MnPowOfHonor /= StatSteps;
			MnPowOfPCall /= StatSteps;
			MnPowOfOther /= StatSteps;
			MnPropOfAgres /= StatSteps;
			MnPropOfHonor /= StatSteps;
			MnPropOfPCall /= StatSteps;
			MnPropOfOther /= StatSteps;
			cout << '\r' << setw(6) << setprecision(4)
				<< MnPowOfAgres << '\t' << MnPowOfHonor << '\t'
				<< MnPowOfPCall << '\t' << MnPowOfOther;
			MeanPowerOfAgres[Y][X] = MnPowOfAgres;
			MeanPowerOfHonor[Y][X] = MnPowOfHonor;
			MeanPowerOfPCall[Y][X] = MnPowOfPCall;
			MeanPowerOfOther[Y][X]=MnPowOfOther;
			MeanPropOfAgres[Y][X]=MnPropOfAgres;
			MeanPropOfHonor[Y][X]=MnPropOfHonor;
			MeanPropOfPCall[Y][X]=MnPropOfPCall;
			MeanPropOfOther[Y][X]=MnPropOfOther;

			if(BatchPlotPower)
				PlotTables("MnPowOfAgres",MeanPowerOfAgres,"MnPowOfHonor",MeanPowerOfHonor,
						"MnPowOfPCall",MeanPowerOfPCall,"MnPowOfOther",MeanPowerOfOther,Batch_true_color);
			else
				PlotTables("MnPropOfAgres",MeanPropOfAgres,"MnPropOfHonor",MeanPropOfHonor,
						"MnPropOfPCall",MeanPropOfPCall,"MnPropOfOther",MeanPropOfOther,Batch_true_color);
	   }
   }
   //Zrobienie u�ytku z wynik�w
   clear_screen();
   PlotTables("MnPowOfAgres",MeanPowerOfAgres,"MnPowOfHonor",MeanPowerOfHonor,
						"MnPowOfPCall",MeanPowerOfPCall,"MnPowOfOther",MeanPowerOfOther,Batch_true_color);
   SaveScreen(STOP_AFTER);
   clear_screen();
   PlotTables("MnPropOfAgres",MeanPropOfAgres,"MnPropOfHonor",MeanPropOfHonor,
						"MnPropOfPCall",MeanPropOfPCall,"MnPropOfOther",MeanPropOfOther,Batch_true_color);
   SaveScreen(STOP_AFTER+1);

   Write_tables(OutLog,"MnPowOfAgres",MeanPowerOfAgres,"MnPropOfAgres",MeanPropOfAgres);
   Write_tables(OutLog,"MnPowOfHonor",MeanPowerOfHonor,"MnPropOfHonor",MeanPropOfHonor);
   Write_tables(OutLog,"MnPowOfPCall",MeanPowerOfPCall,"MnPropOfPCall",MeanPropOfPCall);
   Write_tables(OutLog,"MnPowOfOther",MeanPowerOfOther,"MnPropOfOther",MeanPropOfOther);

   WB_error_enter_before_clean=0;//Juz sie bledow nie spodziewamy
   while(1)
   {
		int znak=get_char();
		if(znak==-1 || znak==27) break;
   }
}

void walk_params_sele()
//Tryb analizy przestrzeni parametr�w - wydajno�� policji (POLL_EFF) i SELECTION
{
   unsigned X=(POLICE_EFFIC_MAX-POLICE_EFFIC_MIN)/POLICE_EFFIC_STEP + 1;
   if(! (POLICE_EFFIC_MIN+POLICE_EFFIC_STEP*X < POLICE_EFFIC_MAX) ) //Operacja == nie dzia�a poprawnie na float
										X++;
   unsigned Y=(SELECTION_MAX-SELECTION_MIN)/SELECTION_STEP   + 1;
   if(! (SELECTION_MIN+SELECTION_STEP*Y < SELECTION_MAX) )  //Operacja == nie dzia�a poprawnie na float
										Y++;
   cout<<"Police efficiency vs selection batch job: "<<Y<<" vs. "<<X<<" cells."<<endl;
   Parameters_dump(OutLog);
   OutLog<<"Police efficiency vs selection batch job: "<<Y<<" vs. "<<X<<" cells."<<endl;

   wb_dynmatrix<FLOAT> MeanPowerOfAgres(Y,X);MeanPowerOfAgres.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfHonor(Y,X);MeanPowerOfHonor.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfPCall(Y,X);MeanPowerOfPCall.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfOther(Y,X);MeanPowerOfOther.fill(-9999.0);

   wb_dynmatrix<FLOAT> PropMnDiffOfAgres(Y,X);PropMnDiffOfAgres.fill(-9999.0);
   wb_dynmatrix<FLOAT> PropMnDiffOfHonor(Y,X);PropMnDiffOfHonor.fill(-9999.0);
   wb_dynmatrix<FLOAT> PropMnDiffOfPCall(Y,X);PropMnDiffOfPCall.fill(-9999.0);
   wb_dynmatrix<FLOAT> PropMnDiffOfOther(Y,X);PropMnDiffOfOther.fill(-9999.0);

   wb_dynmatrix<FLOAT> MeanPropOfAgres(Y,X);MeanPropOfAgres.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfHonor(Y,X);MeanPropOfHonor.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfPCall(Y,X);MeanPropOfPCall.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfOther(Y,X);MeanPropOfOther.fill(-9999.0);

   for(FLOAT selec=SELECTION_MIN,Y=0;selec<=SELECTION_MAX;selec+=SELECTION_STEP,Y++)
   {
	   USED_SELECTION=selec;
	   for(FLOAT effic=POLICE_EFFIC_MIN,X=0;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
	   {
		   cout<<endl<<"SYM "<<Y<<' '<<X<<" Select: "<<selec<<"\tPEffic: "<<effic<<endl;
		   POLICE_EFFIC=effic;

		   //P�TLA POWT�RZE� SYMULACJI
		   FLOAT MnPowOfAgres=0;
		   FLOAT MnPowOfHonor=0;
		   FLOAT MnPowOfPCall=0;
		   FLOAT MnPowOfOther=0;
		   FLOAT MnPropOfAgres=0;
		   FLOAT MnPropOfHonor=0;
		   FLOAT MnPropOfPCall=0;
		   FLOAT MnPropOfOther=0;
		   FLOAT VariPropOfAgres=0;
		   FLOAT VariPropOfHonor=0;
		   FLOAT VariPropOfPCall=0;
		   FLOAT VariPropOfOther=0;

		   unsigned StatSteps=0;   //Zlicza wszystkie kroki u�yte do statystyk, czasem przez kilka powt�rze�

		   for(unsigned rep=0;rep<REPETITION_LIMIT;rep++)  //POWTARZANIE SYMULACJI
		   {
			 HonorAgent::World.alloc(SIDE,SIDE);//Pocz�tek - alokacja agent�w �wiata
			 double POPULATION=double(SIDE)*SIDE;   //Ile ich w og�le jest?
			 InitConnections(SIDE*SIDE*OUTFAR_LINKS_PER_AGENT);//Tworzenie sieci
			 InitAtributes(SIDE*SIDE); //Losowanie atrybut�w dla agent�w
			 FLOAT PrevPropOfAgres=abs(BULLI_POPUL);
			 FLOAT PrevPropOfHonor=abs(HONOR_POPUL);
			 FLOAT PrevPropOfPCall=abs(CALLER_POPU);
			 FLOAT PrevPropOfOther=1-abs(BULLI_POPUL)-abs(HONOR_POPUL)-abs(CALLER_POPU);

			 //CalculateStatistics(World); //Po raz pierwszy dla tych parametr�w   ???
			 for(step_counter=1;step_counter<=STOP_AFTER;/*step_counter++*/)
			 {
				Reset_action_memories();//Czyszczenie, mo�e niepotrzebne
				power_recovery_step(); // Krok procesu regeneracji si�
				one_step(step_counter); // Krok dynamiki interakcji agresywnych
										// step_counter++ JEST W �RODKU!
				/*
				if (SOCIAL_IMPACT_INTENSITY_PERCENT > 0)
					// Opcjonalnie krok wp�ywu spo�ecznego
						social_impact_step(World,
						SOCIAL_IMPACT_INTENSITY_PERCENT);
				*/
				int StepSpecific = step_counter % max(EveryStep,100u);
				if ( StepSpecific == max(EveryStep,100u)- PREVSTEP  ) //poprzedni krok przed liczeniem g��wnym
				{
					 CalculateStatistics(HonorAgent::World);   //Potrzebne s� proporcje poprzednie dla tego co potem
					 PrevPropOfAgres = MnStrenght[0].N / POPULATION;
					 PrevPropOfHonor = MnStrenght[1].N / POPULATION;
					 PrevPropOfPCall = MnStrenght[2].N / POPULATION;
					 PrevPropOfOther = MnStrenght[3].N / POPULATION;
				}

				if ( StepSpecific == 0)
				{
					CalculateStatistics(HonorAgent::World);   //Teraz wrzystkie statystyki...
					cout <<"\r["<<rep<<"] "<< step_counter<<"  ";

					if(step_counter>=STAT_AFTER) //Mo�e tylko koncowy stan r�wnowagi?
					{
						MnPowOfAgres += MnStrenght[0].N > 0 ? MnStrenght[0].summ / MnStrenght[0].N : 0;
						MnPowOfHonor += MnStrenght[1].N > 0 ? MnStrenght[1].summ / MnStrenght[1].N : 0;
						MnPowOfPCall += MnStrenght[2].N > 0 ? MnStrenght[2].summ / MnStrenght[2].N : 0;
						MnPowOfOther += MnStrenght[3].N > 0 ? MnStrenght[3].summ / MnStrenght[3].N : 0;
						FLOAT CurrPropOfAgres=0;
						FLOAT CurrPropOfHonor=0;
						FLOAT CurrPropOfPCall=0;
						FLOAT CurrPropOfOther=0;
						MnPropOfAgres += CurrPropOfAgres = MnStrenght[0].N / POPULATION;
						MnPropOfHonor += CurrPropOfHonor = MnStrenght[1].N / POPULATION;
						MnPropOfPCall += CurrPropOfPCall = MnStrenght[2].N / POPULATION;
						MnPropOfOther += CurrPropOfOther = MnStrenght[3].N / POPULATION;
						VariPropOfAgres+=abs(PrevPropOfAgres-CurrPropOfAgres);//(MnPropOfAgres>PrevPropOfAgres?MnPropOfAgres-PrevPropOfAgres:PrevPropOfAgres-MnPropOfAgres);
						VariPropOfHonor+=abs(PrevPropOfHonor-CurrPropOfHonor);//(MnPropOfHonor>PrevPropOfHonor?MnPropOfHonor-PrevPropOfHonor:PrevPropOfHonor-MnPropOfHonor);
						VariPropOfPCall+=abs(PrevPropOfPCall-CurrPropOfPCall);//(MnPropOfPCall>PrevPropOfPCall?MnPropOfPCall-PrevPropOfPCall:PrevPropOfPCall-MnPropOfPCall);
						VariPropOfOther+=abs(PrevPropOfOther-CurrPropOfOther);//(MnPropOfOther>PrevPropOfOther?MnPropOfOther-PrevPropOfOther:PrevPropOfOther-MnPropOfOther);

						StatSteps++;
					}

					if (input_ready())
						{
							int znak = get_char();
							if (znak == 'v' || znak == 'c')
							{
								   BatchPlotPower=!BatchPlotPower;
								   clear_screen();
							}
							else
							if (znak == '\n')
								if(BatchPlotPower)
									PlotTables("MnPowOfAgres",MeanPowerOfAgres,"MnPowOfHonor",MeanPowerOfHonor,
									"MnPowOfPCall",MeanPowerOfPCall,"MnPowOfOther",MeanPowerOfOther,Batch_true_color);
								else
								{
									//PlotTables("MnPropOfAgres",MeanPropOfAgres,"MnPropOfHonor",MeanPropOfHonor,
									//		"MnPropOfPCall",MeanPropOfPCall,"MnPropOfOther",MeanPropOfOther,Batch_true_color);
									PlotTables("PropMnDiffOfAgres",PropMnDiffOfAgres,"PropMnDiffOfHonor",PropMnDiffOfHonor,
											"PropMnDiffOfPCall",PropMnDiffOfPCall,"PropMnDiffOfOther",PropMnDiffOfOther,Batch_true_color);
								}
						}
				}
			 } // KONIEC P�TLI SYMULACJI
			 DeleteAllConnections();  //Koniec tej symulacji
			} // KONIEC P�TLI POWTORZEN
																				assert(StatSteps>0);
			// PODLICZENIE WYNIK�W I ZAPAMI�TANIE W TABLICACH PRZESTRZENI PARAMETR�W
			MnPowOfAgres /= StatSteps;
			MnPowOfHonor /= StatSteps;
			MnPowOfPCall /= StatSteps;
			MnPowOfOther /= StatSteps;
			MnPropOfAgres /= StatSteps;
			MnPropOfHonor /= StatSteps;
			MnPropOfPCall /= StatSteps;
			MnPropOfOther /= StatSteps;
			VariPropOfAgres/= StatSteps;
			VariPropOfHonor/= StatSteps;
			VariPropOfPCall/= StatSteps;
			VariPropOfOther/= StatSteps;

			cout << '\r' << setw(6) << setprecision(4)
				<< MnPowOfAgres << '\t' << MnPowOfHonor << '\t'
				<< MnPowOfPCall << '\t' << MnPowOfOther;
			MeanPowerOfAgres[Y][X] = MnPowOfAgres;
			MeanPowerOfHonor[Y][X] = MnPowOfHonor;
			MeanPowerOfPCall[Y][X] = MnPowOfPCall;
			MeanPowerOfOther[Y][X]=MnPowOfOther;
			MeanPropOfAgres[Y][X]=MnPropOfAgres;
			MeanPropOfHonor[Y][X]=MnPropOfHonor;
			MeanPropOfPCall[Y][X]=MnPropOfPCall;
			MeanPropOfOther[Y][X]=MnPropOfOther;
			PropMnDiffOfAgres[Y][X]=VariPropOfAgres;
			PropMnDiffOfHonor[Y][X]=VariPropOfHonor;
			PropMnDiffOfPCall[Y][X]=VariPropOfPCall;
			PropMnDiffOfOther[Y][X]=VariPropOfOther;

			if(BatchPlotPower)
				PlotTables("MnPowOfAgres",MeanPowerOfAgres,"MnPowOfHonor",MeanPowerOfHonor,
						"MnPowOfPCall",MeanPowerOfPCall,"MnPowOfOther",MeanPowerOfOther,Batch_true_color);
			else
				PlotTables("MnPropOfAgres",MeanPropOfAgres,"MnPropOfHonor",MeanPropOfHonor,
						"MnPropOfPCall",MeanPropOfPCall,"MnPropOfOther",MeanPropOfOther,Batch_true_color);
	   }
   }
   //Zrobienie u�ytku z wynik�w
   Write_tables(OutLog,"MnPowOfAgres",MeanPowerOfAgres,"MnPropOfAgres",MeanPropOfAgres);
   Write_tables(OutLog,"MnPowOfHonor",MeanPowerOfHonor,"MnPropOfHonor",MeanPropOfHonor);
   Write_tables(OutLog,"MnPowOfPCall",MeanPowerOfPCall,"MnPropOfPCall",MeanPropOfPCall);
   Write_tables(OutLog,"MnPowOfOther",MeanPowerOfOther,"MnPropOfOther",MeanPropOfOther);
   Write_tables(OutLog,"PropMnDiffOfAgres",PropMnDiffOfAgres,"PropMnDiffOfHonor",PropMnDiffOfHonor);
   Write_tables(OutLog,"PropMnDiffOfPCall",PropMnDiffOfPCall,"PropMnDiffOfOther",PropMnDiffOfOther);

   clear_screen();
   PlotTables("MnPowOfAgres",MeanPowerOfAgres,"MnPowOfHonor",MeanPowerOfHonor,
              "MnPowOfPCall",MeanPowerOfPCall,"MnPowOfOther",MeanPowerOfOther,Batch_true_color);
   SaveScreen(STOP_AFTER);

   clear_screen();
   PlotTables("PropMnDiffOfAgres",PropMnDiffOfAgres,"PropMnDiffOfHonor",PropMnDiffOfHonor,
			  "PropMnDiffOfPCall",PropMnDiffOfPCall,"PropMnDiffOfOther",PropMnDiffOfOther,Batch_true_color);
   SaveScreen(STOP_AFTER+2);

   clear_screen();
   PlotTables("MnPropOfAgres",MeanPropOfAgres,"MnPropOfHonor",MeanPropOfHonor,
			  "MnPropOfPCall",MeanPropOfPCall,"MnPropOfOther",MeanPropOfOther,Batch_true_color);
   SaveScreen(STOP_AFTER+1);

   WB_error_enter_before_clean=0;//Juz sie bledow nie spodziewamy
   while(1)
   {
		int znak=get_char();
		if(znak==-1 || znak==27) break;
   }
}

void walk_honor_vs_agrr()
//Tryb analizy przestrzeni parametr�w - SELECTION i RATIO Agresywnych do Honorowych
{
   unsigned X = (PROPORTION_MAX - PROPORTION_MIN) / PROPORTION_STEP + 1;
   if(! (PROPORTION_MIN+PROPORTION_STEP*X < PROPORTION_MAX) )  //Operacja == nie dzia�a poprawnie na float
										X++;
   unsigned Y=(SELECTION_MAX-SELECTION_MIN)/SELECTION_STEP   + 1;
   if(! (SELECTION_MIN+SELECTION_STEP*Y < SELECTION_MAX) )  //Operacja == nie dzia�a poprawnie na float
										Y++;
   cout<<"Agressive to honor proportion vs selection batch job: "<<Y<<" vs. "<<X<<" cells."<<endl;
   Parameters_dump(OutLog);
   OutLog<<"Agressive to honor proportion vs selection batch job: "<<Y<<" vs. "<<X<<" cells."<<endl;

   wb_dynmatrix<FLOAT> MeanPowerOfAgres(Y,X);MeanPowerOfAgres.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfHonor(Y,X);MeanPowerOfHonor.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfPCall(Y,X);MeanPowerOfPCall.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfOther(Y,X);MeanPowerOfOther.fill(-9999.0);

   wb_dynmatrix<FLOAT> PropMnDiffOfAgres(Y,X);PropMnDiffOfAgres.fill(-9999.0);
   wb_dynmatrix<FLOAT> PropMnDiffOfHonor(Y,X);PropMnDiffOfHonor.fill(-9999.0);
   wb_dynmatrix<FLOAT> PropMnDiffOfPCall(Y,X);PropMnDiffOfPCall.fill(-9999.0);
   wb_dynmatrix<FLOAT> PropMnDiffOfOther(Y,X);PropMnDiffOfOther.fill(-9999.0);

   wb_dynmatrix<FLOAT> MeanPropOfAgres(Y,X);MeanPropOfAgres.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfHonor(Y,X);MeanPropOfHonor.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfPCall(Y,X);MeanPropOfPCall.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfOther(Y,X);MeanPropOfOther.fill(-9999.0);

   for(FLOAT selec=SELECTION_MIN,Y=0;selec<=SELECTION_MAX;selec+=SELECTION_STEP,Y++)
   {
	   USED_SELECTION=selec;

	   for(FLOAT prop=PROPORTION_MIN,X=0;prop<=PROPORTION_MAX;prop+=PROPORTION_STEP,X++)
	   {
			//if(BULLI_POPUL>=0)  //????
			//			BULLI_POPUL=prop;

		   if(Compensation_mode)
			{
			  BULLI_POPUL=PROPORTION_MAX-prop;
			  HONOR_POPUL=prop;
			}
			else
			{
			 if(HONOR_POPUL>=0)
					HONOR_POPUL=prop;
			 if(BULLI_POPUL>=0 )
					BULLI_POPUL=prop;
			}

		   cout<<endl<<"SYM "<<Y<<' '<<X<<" Prop: "<<prop<<"\tSelec: "<<selec<<endl;

		   //P�TLA POWT�RZE� SYMULACJI
		   FLOAT MnPowOfAgres=0;
		   FLOAT MnPowOfHonor=0;
		   FLOAT MnPowOfPCall=0;
		   FLOAT MnPowOfOther=0;
		   FLOAT MnPropOfAgres=0;
		   FLOAT MnPropOfHonor=0;
		   FLOAT MnPropOfPCall=0;
		   FLOAT MnPropOfOther=0;
		   unsigned StatSteps=0;

		   for(unsigned rep=0;rep<REPETITION_LIMIT;rep++)
		   {
			 HonorAgent::World.alloc(SIDE,SIDE);//Pocz�tek - alokacja agent�w �wiata
			 double POPULATION=double(SIDE)*SIDE;   //Ile ich w og�le jest?
			 InitConnections(SIDE*SIDE*OUTFAR_LINKS_PER_AGENT);//Tworzenie sieci
			 InitAtributes(SIDE*SIDE); //Losowanie atrybut�w dla agent�w
			 //CalculateStatistics(World); //Po raz pierwszy dla tych parametr�w
			 for(step_counter=1;step_counter<=STOP_AFTER;/*step_counter++*/)
			 {
				Reset_action_memories();//Czyszczenie, mo�e niepotrzebne
				power_recovery_step(); // Krok procesu regeneracji si�

				one_step(step_counter); // Krok dynamiki interakcji agresywnych
				/*                      // step_counter++ JEST W �RODKU!
				if (SOCIAL_IMPACT_INTENSITY_PERCENT > 0)
					// Opcjonalnie krok wp�ywu spo�ecznego
						social_impact_step(World,
						SOCIAL_IMPACT_INTENSITY_PERCENT);
				*/
				if (step_counter % max(EveryStep,100u) == 0)
				{
					CalculateStatistics(HonorAgent::World);
					cout <<"\r["<<rep<<"] "<< step_counter<<"  ";

					if(step_counter>=STAT_AFTER) //Mo�e tylko koncowy stan r�wnowagi?
					{
						MnPowOfAgres += MnStrenght[0].N > 0 ? MnStrenght[0].summ / MnStrenght[0].N : 0;
						MnPowOfHonor += MnStrenght[1].N > 0 ? MnStrenght[1].summ / MnStrenght[1].N : 0;
						MnPowOfPCall += MnStrenght[2].N > 0 ? MnStrenght[2].summ / MnStrenght[2].N : 0;
						MnPowOfOther += MnStrenght[3].N > 0 ? MnStrenght[3].summ / MnStrenght[3].N : 0;
						MnPropOfAgres += MnStrenght[0].N / POPULATION;
						MnPropOfHonor += MnStrenght[1].N / POPULATION;
						MnPropOfPCall += MnStrenght[2].N / POPULATION;
						MnPropOfOther += MnStrenght[3].N / POPULATION;
						StatSteps++;
					}

					if (input_ready())
						{
							int znak = get_char();
							if (znak == 'v' || znak == 'c')
							{
								   BatchPlotPower=!BatchPlotPower;
								   clear_screen();
							}
							else
							if (znak == '\n')
								if(BatchPlotPower)
									PlotTables("MnPowOfAgres",MeanPowerOfAgres,"MnPowOfHonor",MeanPowerOfHonor,
									"MnPowOfPCall",MeanPowerOfPCall,"MnPowOfOther",MeanPowerOfOther,Batch_true_color);
								else
									PlotTables("MnPropOfAgres",MeanPropOfAgres,"MnPropOfHonor",MeanPropOfHonor,
									"MnPropOfPCall",MeanPropOfPCall,"MnPropOfOther",MeanPropOfOther,Batch_true_color);
						}
				}
			 } // KONIEC P�TLI SYMULACJI
			 DeleteAllConnections();  //Koniec tej symulacji
			} // KONIEC P�TLI POWTORZEN
																				assert(StatSteps>0);
			// PODLICZENIE WYNIK�W I ZAPAMI�TANIE W TABLICACH PRZESTRZENI PARAMETR�W
			MnPowOfAgres /= StatSteps;
			MnPowOfHonor /= StatSteps;
			MnPowOfPCall /= StatSteps;
			MnPowOfOther /= StatSteps;
			MnPropOfAgres /= StatSteps;
			MnPropOfHonor /= StatSteps;
			MnPropOfPCall /= StatSteps;
			MnPropOfOther /= StatSteps;
			cout << '\r' << setw(6) << setprecision(4)
				<< MnPowOfAgres << '\t' << MnPowOfHonor << '\t'
				<< MnPowOfPCall << '\t' << MnPowOfOther;
			MeanPowerOfAgres[Y][X] = MnPowOfAgres;
			MeanPowerOfHonor[Y][X] = MnPowOfHonor;
			MeanPowerOfPCall[Y][X] = MnPowOfPCall;
			MeanPowerOfOther[Y][X]=MnPowOfOther;
			MeanPropOfAgres[Y][X]=MnPropOfAgres;
			MeanPropOfHonor[Y][X]=MnPropOfHonor;
			MeanPropOfPCall[Y][X]=MnPropOfPCall;
			MeanPropOfOther[Y][X]=MnPropOfOther;

			if(BatchPlotPower)
				PlotTables("MnPowOfAgres",MeanPowerOfAgres,"MnPowOfHonor",MeanPowerOfHonor,
						"MnPowOfPCall",MeanPowerOfPCall,"MnPowOfOther",MeanPowerOfOther,Batch_true_color);
			else
				PlotTables("MnPropOfAgres",MeanPropOfAgres,"MnPropOfHonor",MeanPropOfHonor,
						"MnPropOfPCall",MeanPropOfPCall,"MnPropOfOther",MeanPropOfOther,Batch_true_color);
	   }
   }
   //Zrobienie u�ytku z wynik�w
   clear_screen();
   PlotTables("MnPowOfAgres",MeanPowerOfAgres,"MnPowOfHonor",MeanPowerOfHonor,
						"MnPowOfPCall",MeanPowerOfPCall,"MnPowOfOther",MeanPowerOfOther,Batch_true_color);
   SaveScreen(STOP_AFTER);
   clear_screen();
   PlotTables("MnPropOfAgres",MeanPropOfAgres,"MnPropOfHonor",MeanPropOfHonor,
						"MnPropOfPCall",MeanPropOfPCall,"MnPropOfOther",MeanPropOfOther,Batch_true_color);
   SaveScreen(STOP_AFTER+1);
   Write_tables(OutLog,"MnPowOfAgres",MeanPowerOfAgres,"MnPropOfAgres",MeanPropOfAgres);
   Write_tables(OutLog,"MnPowOfHonor",MeanPowerOfHonor,"MnPropOfHonor",MeanPropOfHonor);
   Write_tables(OutLog,"MnPowOfPCall",MeanPowerOfPCall,"MnPropOfPCall",MeanPropOfPCall);
   Write_tables(OutLog,"MnPowOfOther",MeanPowerOfOther,"MnPropOfOther",MeanPropOfOther);

   WB_error_enter_before_clean=0;//Juz sie bledow nie spodziewamy
   while(1)
   {
		int znak=get_char();
		if(znak==-1 || znak==27) break;
   }
}

void fixed_params_mode()
//Tryb interakcyjny z pe�n� wizualizacj�
{
	HonorAgent::World.alloc(SIDE,SIDE);//Pocz�tek - alokacja agent�w �wiata w static tablicy agent�w �wiata

	InitConnections(SIDE*SIDE*OUTFAR_LINKS_PER_AGENT);
	InitAtributes(SIDE*SIDE);
	dump_step(HonorAgent::World,0);//Po raz pierwszy
	CalculateStatistics(HonorAgent::World); //Po raz pierwszy

	//Prowizoryczna p�tla g��wna
	Help();
	int cont=1;//flaga kontynuacji programu
	int runs=0;//Flaga wykonywania symulacji
	while(cont)
	{
		char tab[2];
		tab[1]=0;
		if(input_ready())
		{
			tab[0]=get_char();
			switch(tab[0])
			{
			case '1':EveryStep=1;cout<<"Visualisation at every "<<EveryStep<<" step."<<endl;break;
			case '2':EveryStep=2;cout<<"Visualisation at every "<<EveryStep<<" step."<<endl;break;
			case '3':EveryStep=3;cout<<"Visualisation at every "<<EveryStep<<" step."<<endl;break;
			case '4':EveryStep=4;cout<<"Visualisation at every "<<EveryStep<<" step."<<endl;break;
			case '5':EveryStep=5;cout<<"Visualisation at every "<<EveryStep<<" step."<<endl;break;
			case '6':EveryStep=10;cout<<"Visualisation at every "<<EveryStep<<" step."<<endl;break;
			case '7':EveryStep=20;cout<<"Visualisation at every "<<EveryStep<<" step."<<endl;break;
			case '8':EveryStep=25;cout<<"Visualisation at every "<<EveryStep<<" step."<<endl;break;
			case '9':EveryStep=50;cout<<"Visualisation at every "<<EveryStep<<" step."<<endl;break;
			case '0':EveryStep=100;cout<<"Visualisation at every "<<EveryStep<<" step."<<endl;break;

			case 'b':SaveScreen(step_counter);break;
			case 'd':dump_screens=!dump_screens;cout<<"\n From now screen will"<<(dump_screens?"be":"not be")<<" dumped..."<<endl;break;

			case '>'://Next step
			case 'n':runs=0;one_step(step_counter);CalculateStatistics(HonorAgent::World);replot(HonorAgent::World);break;
			case 'p':runs=0;replot(HonorAgent::World);break;
			case 'r':runs=1;break;

			case 'c':ConsoleLog=!ConsoleLog;break;
			case 's':VisShorLinks=!VisShorLinks;clear_screen();replot(HonorAgent::World);break; //Wizualizacja bliskich link�w
			case 'f':VisFarLinks=!VisFarLinks;clear_screen();replot(HonorAgent::World);break;	  //Wizualizacja dalekich
			case 'a':VisAgents=!VisAgents;clear_screen();replot(HonorAgent::World);break;     //Wizualizacja w�a�ciwo�ci agent�w
			case 'e':VisDecision=!VisDecision;clear_screen();replot(HonorAgent::World);break; //Czy wizualizacja g��wna z decyzjami
			case 'u':VisReputation=!VisReputation;clear_screen();replot(HonorAgent::World);break;//Czy ----//---------- z reputacj�
			case 'v'://if(CalculateColorDefault==  //Wybranie typu wizualizacji
					replot(HonorAgent::World);break;

			case '\b':mouse_check(HonorAgent::World);break;
			case '@':
			case '\r':clear_screen();
			case '\n':replot(HonorAgent::World);if(ConsoleLog)cout<<endl<<endl;break;

			case 'q':
			case EOF:  WB_error_enter_before_clean=0;//Juz sie bledow nie spodziewamy
					   cont=0;break;
			case 'h':
			default:
					Help();break;
			}
			flush_plot();
		}
		else
			if(runs)
			{
				Reset_action_memories();

				//Krok procesu regeneracji si�
				power_recovery_step();

				//Krok dynamiki interakcji agresywnych
				one_step(step_counter);

				//Opcjonalnie krok wp�ywu spo�ecznego
				/*
				if(SOCIAL_IMPACT_INTENSITY_PERCENT>0)
					social_impact_step(World,SOCIAL_IMPACT_INTENSITY_PERCENT);
				*/
				if(step_counter%EveryStep==0)
				{
					CalculateStatistics(HonorAgent::World);
					replot(HonorAgent::World);

					if(dump_screens)
					{
						SaveScreen(step_counter);
						cout<<"\nScreen for step "<<step_counter<<" dumped."<<endl;
					}

					if(step_counter>=STOP_AFTER)
					{
						if(RepetNum<REPETITION_LIMIT )//Kolejna repetycja?
						{
							dump_step(HonorAgent::World,step_counter);//Po raz ostatni
							RepetNum++;
							cout<<"\nStop after "<<STOP_AFTER<<" steps\a, and start iteration n# "<<RepetNum<<endl;
							Reset_action_memories();
							DeleteAllConnections();
							InitConnections(SIDE*SIDE*OUTFAR_LINKS_PER_AGENT);
							InitAtributes(SIDE*SIDE);
							step_counter=0;
							LiczbaTrupow=0; //Troch� to partyzantka
							LiczbaTrupowDzis=0;
							CalculateStatistics(HonorAgent::World); //Po raz pierwszy dla tej iteracji
							OutLog<<"next"<<endl;
							save_stat();
							dump_step(HonorAgent::World,0);//Po raz pierwszy
						}
						else
						{
							dump_step(HonorAgent::World,step_counter);//Po raz ostatni
							cout<<"\nStop because of limit "<<STOP_AFTER<<" steps\a\a"<<endl;
							runs=false;
						}
					}
					else
					{
					   if(step_counter%DumpStep==0)//10-100x rzadziej ni� statystyka
					   {
							dump_step(HonorAgent::World,step_counter);//Co jaki� czas
					   }
					}
				}
			}
	}
}

void Help()
{
	cout<<"POSSIBLE COMMANDS FOR GRAPHIC WINDOW:"<<endl
		<<"q - q-uit or ESC\n"
		"\n"
		"n - n-ext MC step\n"
		"p - p-ause simulation\n"
		"r - r-un simulation\n"
		"\n"
		"b - save screen to B-MP\n"
		"d - d-ump on every visualisation\n"
		"\n"
		"1..0 visualisation freq.\n"
		"c - swich c-onsole on/off\n"
		"s - visualise s-hort links\n"
		"f - visualise f-ar links\n"
		"a - visualise a-gents\n"
		"e - visualise d-e-cisions\n"
		"p - visualise rep-u-tations\n"
		"\n"
		"mouse left or right - \n"
		"     means inspection\n"
		"ENTER - replot screen\n";
}

void mouse_check(wb_dynmatrix<HonorAgent>& World)
{
	int xpos=0;
	int ypos=0;
	int click=0;
	get_mouse_event(&xpos,&ypos,&click);
	xpos=xpos/VSIZ;
	ypos=ypos/VSIZ;
	set_pen_rgb(255,255,255,0,SSH_LINE_SOLID); // Ustala aktualny kolor linii za pomoca skladowych RGB

	if(xpos<SIDE && ypos<SIDE)//Trafiono agenta
	{
		HonorAgent& Ag=World[ypos][xpos];	//Zapami�tanie referencji do agenta, �eby ci�gle nie liczy� indeks�w

		cout<<"INSPECTION OF HonorAgent: "<<xpos<<' '<<ypos<<endl;
		PrintHonorAgentInfo(cout,Ag);

		unsigned x2pos=0;
		unsigned y2pos=0;
		for(unsigned i=0;i<Ag.NeighSize();i++)
		{
		   Ag.getNeigh(i,x2pos,y2pos);
		   if(Ag.IsChild(i))
				cout<<x2pos<<' '<<y2pos<<" is child; ";
		   if(Ag.IsParent(i))
				cout<<x2pos<<' '<<y2pos<<" is parent; ";
		   if(i>MOORE_SIZE)
				cout<<x2pos<<' '<<y2pos<<" is far contact; ";
		}
		cout<<endl<<endl;
		if(click==2) //By�o z rysowaniem
			flush_plot();
	}
}

void SaveScreen(unsigned step)
{
	wb_pchar Filename;
	Filename.alloc(128);
	Filename.prn("D_%06u_%s",step,LogName.get());
	dump_screen(Filename.get_ptr_val());
}

int   WB_error_enter_before_clean=1; //Czy da� szanse operatorowi na poczytanie komunikat�w ko�cowych
extern "C" int   basic_line_width=1; //Bo X11 z jakiegoś powodu nie chce tej zmiennej jako static