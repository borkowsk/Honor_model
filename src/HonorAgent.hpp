//  Agent do modelu kultury honoru
//
////////////////////////////////////////////////////////////////////////////////
typedef double FLOAT;
#include "iostream"
#define USES_STDC_RAND
#include "INCLUDE/Random.h"
#include "INCLUDE/wb_ptr.hpp"


extern unsigned population_growth;//=1;//SPOSOBY ROZMNA�ANIA
							 // 0 - wg. inicjalnych proporcji
							 // 1 - lokalne rozmazanie losowe s�siad
							 // 2 - lokalne rozmazanie proporcjonalne do sily  //NIE ZAIMPLEMENTOWANE!
							 // 3 - globalne, losowy ziutek z ca�o�ci

//MACIE� I LINKOWANIE  AGENT�W
const unsigned 		SIDE=100;//SIDE*SIDE to rozmiar �wiata symulacji
const int	   		MOORE_RAD=3; //Nie zmienia� na unsigned bo si� psuje losowanie!      //2 dla DEBUG,zwykle 3
const unsigned 		MOORE_SIZE=(4*MOORE_RAD*MOORE_RAD)+4*MOORE_RAD;//S�siedzi Moora - cztery kwadranty + 4 osie
const FLOAT    		OUTFAR_LINKS_PER_AGENT=0.5; //Ile jest dodatkowych link�w jako u�amek liczby agent�w
const unsigned 		FAR_LINKS=unsigned(SIDE*SIDE*OUTFAR_LINKS_PER_AGENT*2);
const unsigned 		MAX_LINKS=MOORE_SIZE + 2/*ZAPAS*/ + (FAR_LINKS/(SIDE*SIDE)? FAR_LINKS/(SIDE*SIDE):2); //Ile maksymalnie mo�e miec agent link�w
const FLOAT    		RECOVERY_POWER=0.005;//Jak� cz�� si�y odzyskuje w kroku
const FLOAT    		RATIONALITY=0.0; //WAGA jak realistycznie ocenia w�asn� si�� (vs. wed��g w�asnej reputacji)

//INDYWIDUALNE CECHY AGENT�W
const FLOAT    BULLISM_LIMIT=-1;//0.2;//0.66;//0.10;//Maksymalny mo�liwy bulizm.
								//Jak ujemne to rozk�ad Pareto lub brak rozk�adu, jak dodatnie to dzwonowy
extern FLOAT    BULLI_POPUL;//=-0.25;//0.2;//0.100;//Albo zero-jedynkowo. Jak 1 to decyduje rozk�ad sterowany BULLISM_LIMIT
						   //("-" jest sygna�em zafiksowania w trybie batch (?!?!? USUN��! TODO }
extern FLOAT	HONOR_POPUL;//=0.18;//0.3333;//Jaka cz�� agent�w populacji jest �ci�le honorowa
extern FLOAT    CALLER_POPU;//=0.25;//Jaka cz�� wzywa policje zamiast si� poddawa�
extern FLOAT    POLICE_EFFIC;//=0.50;//0.650;//0.950; //Z jakim prawdopodobie�stwem wezwana policja obroni agenta
extern bool		ONLY3STRAT; //Czy tylko 3 strategie?

//INNE GLOBALNE WLASCIWOSCI SWIATA
extern bool     MAFIAHONOR;//=true; //Czy reputacja przenosi si� na cz�onk�w rodziny
extern FLOAT    USED_SELECTION;//=0.05;//0.10; //Jak bardzo przegrani umieraj� (0 - brak selekcji w og�le)
extern FLOAT    MORTALITY;  //=0.01 //Jak �atwo mo�na zgin�� z przyczyn niespo�ecznych - horoba, wypadek itp. JAK 0 TO S� "ELFY"
extern FLOAT    EXTERNAL_REPLACE;//=0.001; //Jakie jest prawdopodobienstwo wymiany na losowego agenta
//extern FLOAT    RANDOM_AGRESSION;//=0.05;//0.015;//Bazowy poziom agresji, najpierw by� dla wszystkich a potem zale�ny od honoru
extern FLOAT    HONOR_AGRESSION;//=0.0250;//0.015;//Bazowy poziom agresji tylko dla HONOROWYCH
extern FLOAT    AGRES_AGRESSION;//=0.0250;//POZIOM PRZYPADKOWEJ AGRESJI AGRESYWNYCH (bez calkulacji kto silniejszy!)

class HonorAgent
{
 public: // TYPY POMOCNICZE

	enum Decision {NOTHING=-1,WITHDRAW=0,GIVEUP=1,HOOK=2,FIGHT=3,CALLAUTH=4};
	static const char* Decision2str(Decision Deci) //{NOTHING=-1,WITHDRAW=0,GIVEUP=1,HOOK=2,FIGHT=3,CALLAUTH=4};
	{
	   static const char* Names[]={"NOTHING","WITHDRAW","GIVEUP","HOOK","FIGHT","CALLAUTH","?????"};
	   return Names[Deci+1];
	}

	struct Actions {
	//POLA
	unsigned Counter; //B�dzie redundantne, ale mo�e potrzebne do akcesor�w R*()
	unsigned Fails;    //Ile razy przegra�
	unsigned Succeses; //Ile razy wygra�

	union {
	struct { unsigned NOTHING,WITHDRAW,GIVEUP,HOOK,FIGHT,CALLAUTH;};
	unsigned Tab[6]; };

	//METODY KLASY ACTIONS
	/////////////////////////
	Actions(){ Reset();}
	void Reset(){NOTHING=WITHDRAW=GIVEUP=HOOK=FIGHT=CALLAUTH=Fails=Succeses=Counter=0;}
	void Count(HonorAgent::Decision Deci)
	{
			Counter++; //Kt�re zliczenie akcji
			switch(Deci)
			{
			case HonorAgent::WITHDRAW: this->WITHDRAW++;  break;
			case HonorAgent::GIVEUP:   this->GIVEUP++;  break;
			case HonorAgent::HOOK:     this->HOOK++; break;
			case HonorAgent::FIGHT:    this->FIGHT++; break;
			case HonorAgent::CALLAUTH: this->CALLAUTH++;break;
			default:
									   this->NOTHING++;	 break;
			}
	}
	//void operator () (HonorAgent::Decision Deci) { Count(Deci);} //Operator wywo�ania!

	//Obliczanie udzia��w poszczeg�lnych zachowa� w pr�bie
	double R_NOTHING()const{ return  (Counter==0?-1:(NOTHING==0?0:NOTHING/double(Counter)));}
	double R_WITHDRAW()const { return  (Counter==0?-1:(WITHDRAW==0?0:WITHDRAW/double(Counter)));}
	double R_GIVEUP()const{ return  (Counter==0?-1:(GIVEUP==0?0:GIVEUP/double(Counter)));}
	double R_HOOK()const  { return  (Counter==0?-1:(HOOK==0?0:HOOK/double(Counter)));}
	double R_FIGHT()const  { return  (Counter==0?-1:(FIGHT==0?0:FIGHT/double(Counter)));}
	double R_CALLAUTH()const { return  (Counter==0?-1:(CALLAUTH==0?0:CALLAUTH/double(Counter)));}
	};

	static  wbrtm::wb_dynmatrix<HonorAgent> World;//Wsp�lny swiat agent�w

	struct LinkTo {unsigned X,Y;unsigned Parent:1;unsigned Child;
					LinkTo(){X=Y=-1;Parent=0;Child=0;}
					};

 public: //Na razie g��wne w�a�ciwo�ci dost�pne zewn�trznie, potem mo�na pomysle� czy je schowa�
	static unsigned licznik_zyc;//Do tworzeni unikalnych identyfikator�w agent�w
	static bool 	CzyTorus; //Czy geometria torusa czy wyspy z brzegami

	unsigned ID; //Unikalny identyfikator, po prostu kolejni agenci  w danym przebiegu
	unsigned HisLifeTime;//Czas �ycia
	Actions  HisActions;//Liczniki

	double Power;//	Si�a (0..1)
	double PowLimit;// Jak� si�� mo�e osi�gn�� maksymalnie, gdy nie traci

	double Agres;// Bulizm (0..1) sk�onno�� do atakowania
	double Honor;// Bezwarunkowa honorowo�� (0..1) sk�onno�� podj�cia obrony
	double CallPolice;//Odium wzywacza policji - prawdopodobie�stwo wzywania policji (0..1) jako �w�drowna �rednia� wezwa�

	//WLA�CIWE METODY AGENT HONOROWEGO
	////////////////////////////////////////////////////////////////////////////
	HonorAgent();  //KONSTRUKTOR

	void RandomReset(); //Losowanie warto�ci atrubut�w

	//Obsluga po��cze�
	bool addNeigh(unsigned x,unsigned y);//Dodaje sasiada o okreslonych wsp�rz�dnych w �wiecie, o ile zmie�ci
	bool getNeigh(unsigned i,unsigned& x,unsigned& y) const;//Czyta wsp�rzedne s�siada, o ile jest
	unsigned NeighSize() const; //Ile ma zarejestrowanych s�siad�w
	bool getNeigh(HonorAgent* Ptr,unsigned i,unsigned& x,unsigned& y) const;//Szuka s�siada po wska�niku, o ile jest
	void forgetAllNeigh(); //Zapomina wszystkich dodanych s�siad�w, co nie znaczy �e oni zapominaj� jego
	static bool AreNeigh(int x1,int y1,int x2,int y2);//Sprawdzanie czy dwaj agenci s�siaduj� w jaki� spos�b

	//Inne akcesory
	wbrtm::wb_pchar  AgentCultureStr() const;//Zwraca reprezentacje tekstow� kultury agenta
	unsigned  AgentCultureMask() const;//Zwraca mask� 3 bitow�, pokazuj�c� gdzie jest r�ne od 0. Aggr:1. bit, Honor:2.bit CallPoll:3.bit
	ssh_rgb   GetColor() const {return Color;} //Nie modyfikowalny z zewn�trz indywidualny kolor w�z�a
	double    GetFeiReputation() const { return HonorFeiRep;}
	Decision  LastDecision(bool clean=false); //Ostatnia decyzja z kroku MC do cel�w wizualizacyjnych i statystycznych

	//Funkcje decyzyjne
	Decision  check_partner(unsigned& x,unsigned& y);//Wyb�r partnera interakcji
	Decision  answer_if_hooked(unsigned x,unsigned y);//Odpowied� na zaczepk�
	void      change_reputation(double Delta,HonorAgent& Powod,int level=0);//Wzrost lub spadek reputacji z zabezpieczeniem zakresu, i ewentualnym dziedziczeniem
	void      lost_power(double delta);  //Spadek, zu�ycie si�y z zabezpieczeniem zera
	static
	bool      firstWin(HonorAgent& In,HonorAgent& Ho);//Ustala czy pierwszy czy drugi agent zwyci�y� w konfrontacji

	//Obsluga stosunk�w rodzinnych
	bool    IsParent(unsigned i);//Czy dany s�siad jest rodzicem
	bool    IsChild(unsigned i); //Czy dany s�siad jest dzieckiem
	bool    IsMyFamilyMember(HonorAgent& Inny,HonorAgent*& Cappo,int MaxLevel=2);//Czy inny "nale�y do rodziny"
								//Przy okazji sprawdzenia ustalamy "Ojca chrzestnego" na po�niej
	void    change_reputation_thru_family(double Delta);//Rodzinna, rekurencyjna zmiana reputacji od agenta w d�
								//najlepiej u�y� z "Cappo" rodziny
	void 	SmiercDona();//Usuwa powi�zania
	friend void PowiazRodzicielsko(HonorAgent& Rodzic,HonorAgent& Ag);  //anty SmiercDona(); - buduje powi�zania

	//G��wna dynamika symulacji
    friend void InitAtributes(FLOAT HowMany);
	friend void InitConnections(FLOAT HowManyFar);
	friend void DeleteAllConnections();
	friend void one_step(unsigned long& step_counter);
	friend void power_recovery_step();
	friend void Reset_action_memories();
	friend void PrintHonorAgentInfo(std::ostream& o,const HonorAgent& H);

 private:
	ssh_rgb   Color;	//Indywidualny i niezmienny lub obliczony kt�r�� z funkcji koduj�cych kolor
	Decision  MemOfLastDecision;
	wbrtm::wb_dynarray<LinkTo> Neighbourhood;//Lista wsp�rz�dnych s�siad�w
	unsigned HowManyNeigh; //Liczba posiadanych s�siad�w

	double HonorFeiRep;//Reputacja wojownika jako �w�drowna �rednia� z konfrontacji (0..1)

	const LinkTo* Neigh(unsigned i); //Dost�p do rekordu linku do kolejnego s�siada
};



inline HonorAgent::HonorAgent():
	   Neighbourhood(MAX_LINKS),HowManyNeigh(0)
	   ,Power(0),PowLimit(0),HisLifeTime(0)
	   ,Agres(0),HonorFeiRep(0.5),CallPolice(0.25)
	   ,MemOfLastDecision(HonorAgent::NOTHING),ID(0)
	 //  ,Resources(0),
{
	Color.r=25+RANDOM(220);Color.g=25+RANDOM(220);Color.b=25+RANDOM(220);
}

inline
bool HonorAgent::AreNeigh(int x1,int y1,int x2,int y2)
//Sprawdzanie czy dwaj agenci s�siaduj� w jaki� spos�b. Np. �eby wykluczy� zdublowane linki
{
	HonorAgent& A=HonorAgent::World[y1][x1];
	for(unsigned i=0;i<A.NeighSize();i++)
	{
	   if(A.Neighbourhood[i].X==x2 && A.Neighbourhood[i].Y==y2) //Ju� taki jest
				return true;
	}
	return false;
}

inline
bool    HonorAgent::IsParent(unsigned i)
//Czy dany s�siad jest rodzicem
{
	if(i<this->NeighSize()
	&& this->Neighbourhood[i].Parent==1)
		return true;
	else
		return false;
}

inline
bool    HonorAgent::IsChild(unsigned i)
//Czy dany s�siad jest dzieckiem
{
	if(i<this->NeighSize()
	&& this->Neighbourhood[i].Child==1)
		return true;
	else
		return false;
}

inline
void 	HonorAgent::SmiercDona()//Usuwa powi�zania
{
	for(unsigned i=0;i<this->NeighSize();i++)
	{
		if(Neighbourhood[i].Parent==1)//Rodzic agenta, jesli �yje zostaje powiadomiony �e straci� dziecko
		{
			Neighbourhood[i].Parent=0;//Dla porz�dku
			HonorAgent& Rodzic=World[Neighbourhood[i].Y][Neighbourhood[i].X];
			for(unsigned j=0;j<Rodzic.NeighSize();j++)
			if( this==&World[Rodzic.Neighbourhood[j].Y][Rodzic.Neighbourhood[j].X] ) //Si� znalaz� na li�cie
			{																	assert(Rodzic.Neighbourhood[j].Child==1);
				Rodzic.Neighbourhood[j].Child=0;
				//Rodzic.notifyChildDeath(j);
				break;
			}
		}
		else
		if(Neighbourhood[i].Child==1)//Dzieci agenta, jesli �yj�, zostaj� powiadomione �e straci�y rodzica
		{
			Neighbourhood[i].Child=0;//Dla porz�dku
			HonorAgent& Dziecko=World[Neighbourhood[i].Y][Neighbourhood[i].X];
			for(unsigned j=0;j<Dziecko.NeighSize();j++)
			if( this==&World[Dziecko.Neighbourhood[j].Y][Dziecko.Neighbourhood[j].X] ) //Si� znalaz� na li�cie
			{																	assert(Dziecko.Neighbourhood[j].Parent==1);
				Dziecko.Neighbourhood[j].Parent=0;
				//Rodzic.notifyChildDeath(j);
				break;
			}
		}
	}
}

inline
void PowiazRodzicielsko(HonorAgent& Rodzic,HonorAgent& NowyAgent)
//anty SmiercDona(); - buduje powi�zania
{
	for(unsigned i=0;i<NowyAgent.NeighSize();i++)
	if( &Rodzic==&HonorAgent::World[NowyAgent.Neighbourhood[i].Y][NowyAgent.Neighbourhood[i].X] ) //Znalaz� rodzica na swojej li�cie
	{                                                                           assert(NowyAgent.Neighbourhood[i].Parent==0);
		NowyAgent.Neighbourhood[i].Parent=1;
		break;
	}

	for(unsigned j=0;j<Rodzic.NeighSize();j++)
	if( &NowyAgent==&HonorAgent::World[Rodzic.Neighbourhood[j].Y][Rodzic.Neighbourhood[j].X] ) //Si� znalaz� na li�cie rodzica
	{																			assert(Rodzic.Neighbourhood[j].Child==0);
		Rodzic.Neighbourhood[j].Child=1;
		//Rodzic.notifyChildBirdth(j);
		break;
	}
}

inline
wbrtm::wb_pchar  HonorAgent::AgentCultureStr()  const
//Zwraca reprezentacje tekstow� kultury agenta
{
   wbrtm::wb_pchar Pom(128);
  //double Agres;// Bulizm (0..1) sk�onno�� do atakowania
  //double Honor;// Bezwarunkowa honorowo�� (0..1) sk�onno�� podj�cia obrony
  //double CallPolice;//Odium wzywacza policji - prawdopodobie�stwo wzywania policji (0..1) jako �w�drowna �rednia� wezwa�
  //Prawdopodobie�stwa mog� by� naprawd� od 0..1 i moga by� "zmieszane", wi�c kultury te�. Cho� w podstawowym modelu same 1
   if(Agres==0 && Honor==0 && CallPolice==0) //Racjonalny jest
		Pom.add("Rational");

   if(Agres>0)
		Pom.add("Agressive");

   if(Honor>0)
		Pom.add("Honor");

   if(CallPolice>0)
		Pom.add("Dignity");

   return Pom;
}

inline
unsigned  HonorAgent::AgentCultureMask() const
//Zwraca mask� 3 bitow�, pokazuj�c� gdzie jest r�ne od 0. Aggr:1. bit, Honor:2.bit CallPoll:3.bit
{
	unsigned Pom=0;
    if(Agres>0)
		Pom|=0x01;

   if(Honor>0)
		Pom|=0x02;

   if(CallPolice>0)
		Pom|=0x04;

   return Pom;
}


//Global simulation functions used in honor.exe but defined in HonorAgent.cpp

void InitAtributes(FLOAT HowMany);
void InitConnections(FLOAT HowManyFar);
void DeleteAllConnections();
void one_step(unsigned long& step_counter);
void power_recovery_step();
void Reset_action_memories();