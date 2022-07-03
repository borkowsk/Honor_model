//  Agent for "culture of honor" model
// (Version for publication accepted 07.2015)
////////////////////////////////////////////////////////////////////////////////

//#define TESTING_RULE_LITERALS 1   //VERY SLOW!!!
//#define HONOR_WITHOUT_REPUTATION 1  //Testing of "null hypothesis"

typedef double FLOAT;
#include "iostream"
#define USES_STDC_RAND
#include "../INCLUDE/Randoms.h"
#include "wb_ptr.hpp"


extern unsigned population_growth;//=1;//How population growth? (SPOSOBY ROZMNAŻANIA)
							 // 0-as initial distribution,(0 - wg. inicjalnych proporcji) 
							 // 1-as local distribution,  (1 - lokalne rozmazanie losowe sąsiad)
							 // 2-NOT IMPLEMENTED		  (2 - lokalne rozmazanie proporcjonalne do siły)
							 // 3-as global distribution  (3 - globalne, losowy agent z całości populacji)
							 // ONLY #1 IS USED FOR THE PAPER in 2015 
//World of agents:
/////////////////////////////
const unsigned 		SIDE = 100;//SIDE*SIDE is a size of matrix
const int	   		MOORE_RAD = 3; //Should be int not unsigned because of randomization formula
const unsigned 		MOORE_SIZE = (4*MOORE_RAD*MOORE_RAD)+4*MOORE_RAD;//Size of Moore neigh.

const FLOAT    		OUTFAR_LINKS_PER_AGENT=0.5;//0.5; //How many extra long links as a fraction of the number of agents (ile jest dodatkowych dalekich linków jako ułamek liczby agentów)
const unsigned 		FAR_LINKS = unsigned(SIDE*SIDE*OUTFAR_LINKS_PER_AGENT*2);

const unsigned 		MAX_LINKS = MOORE_SIZE + 2/*for sure*/ + (FAR_LINKS/(SIDE*SIDE)? FAR_LINKS/(SIDE*SIDE):2); //Max size of agent's link list (ile maksymalnie może miec agent linków)


//AGENT POPULATION
extern FLOAT    BULLI_POPUL;//=-0.25; Ratio of aggressive agents  ("-" jest sygnałem zafiksowania w trybie batch (?!?!?! TODO? }
extern FLOAT	HONOR_POPUL;//=0.25; Ratio of honor agents (Jaka część agentów populacji jest honorowa)
extern FLOAT    CALLER_POPU;//=0.25; Ratio of police callers (Jaka część wzywa policje zamiast walczyć)
extern bool		ONLY3STRAT;//=false; Without rational strategy (Czy tylko 3 strategie?)

extern FLOAT    POLICE_EFFIC;//=0.050; Probability of efficient interventions of authority (Z jakim prawdopodobieństwem wezwana policja obroni agenta)

extern bool     InheritMAXPOWER; //NOT USED //=false;//Czy nowi agenci dziedziczą (z szumem) max power po rodzicu?
extern FLOAT    LIMITNOISE;		 //NOT USED //=0.3; //Mnożnik szumu
const FLOAT     BULLISM_LIMIT=-1;//NOT USED (Nie używany. Jak ujemne to rozkład Pareto lub brak rozkładu, jak dodatnie to dzwonowy. Jak BULLI_POPUL 1 to decyduje rozkład sterowany BULLISM_LIMIT)
const FLOAT    	RATIONALITY=0.0; //NOT USED. How realistically the agent evaluate their own strength (Jak realistycznie ocenia własną siłę vs. według własnej reputacji)

//OTHER GLOBAL PARAMETRES OF THE MODEL
extern FLOAT    RECOVERY_POWER;//=0.005; What part of the agent force recovers after one step (Jaką część siły odzyskuje po jednym kroku)
extern FLOAT    USED_SELECTION;//=0.05; How easily losers die. (Jak łatwo przegrani umierają. Gdy 0 to brak selekcji w ogóle!)
extern FLOAT    MORTALITY;  //=0.01; How easy it is to die for reasons of chance or nonsocial - illness, accident, etc. WHEN ARE 0 = "ELVES" (Jak łatwo można zginąć z przyczyn losowych czyli niespołecznych - choroba, wypadek itp. JAK 0 TO SĄ "ELFY")
extern FLOAT    EXTERNAL_REPLACE;//=0.001; What is the probability of random exchange for a new agent with the initial distribution (Jakie jest prawdopodobieństwo wymiany na losowego agenta)

extern FLOAT    AGRES_AGRESSION;//=0.0250; Random aggression of AGGRESSIVE (POZIOM PRZYPADKOWEJ AGRESJI AGRESYWNYCH - bez kalkulacji kto silniejszy!)
extern FLOAT    HONOR_AGRESSION;//=0.0250; Random aggression of HONOR (Bazowy poziom agresji tylko dla HONOROWYCH)


extern bool     MAFIAHONOR;//NOT USED //=false; //Czy reputacja przenosi się na członków rodziny

#ifdef TESTING_RULE_LITERALS
extern FLOAT	TEST_DIVIDER;//=1.0; ONLY FOR ADVANCED ROBUSTNESS STUDIES (Służy do modyfikacji stałych liczbowych używanych w regułach reakcji agenta)
#else
const  FLOAT	TEST_DIVIDER=1;
#endif


class HonorAgent //DEFINITION OF AGENT FOR "EVOLUTION OF HONOR"
{
 public: 
    static  wbrtm::wb_dynmatrix<HonorAgent> World;//One common world of agents (Wspólny świat agentów)


	// INTERNAL TYPES
	//////////////////////////////////////
	enum Decision {NOTHING=-1,WITHDRAW=0,GIVEUP=1,HOOK=2,FIGHT=3,CALLAUTH=4};

	static const char* Decision2str(Decision Deci) //{NOTHING=-1,WITHDRAW=0,GIVEUP=1,HOOK=2,FIGHT=3,CALLAUTH=4};
	{
	   static const char* Names[]={"NOTHING","WITHDRAW","GIVEUP","HOOK","FIGHT","CALLAUTH","?????"};
	   return Names[Deci+1];
	}

	struct Actions 
	{
	unsigned Counter;  //How many times he decided (Ile razy decydował)
	unsigned Fails;    //How many times lost (Ile razy przegrał)
	unsigned Successes; //How many times he won (Ile razy wygrał)

	union {
	struct { unsigned NOTHING, WITHDRAW, GIVEUP, HOOK, FIGHT, CALLAUTH;};
	unsigned Tab[6]; };

	//METHODS OF STRUCT ACTIONS:
	/////////////////////////////////////
	Actions(){ Reset();}
	void Reset(){NOTHING=WITHDRAW=GIVEUP=HOOK=FIGHT=CALLAUTH=Fails=Successes=Counter=0;}

	void Count(HonorAgent::Decision Deci); // Counting of decisions
	//void operator () (HonorAgent::Decision Deci) { Count(Deci);} //NOT USED (Operator wywołania!)

	//Ratio of different actions
	double R_NOTHING()const{ return  (Counter==0?-1:(NOTHING==0?0:NOTHING/double(Counter)));}
	double R_WITHDRAW()const { return  (Counter==0?-1:(WITHDRAW==0?0:WITHDRAW/double(Counter)));}
	double R_GIVEUP()const{ return  (Counter==0?-1:(GIVEUP==0?0:GIVEUP/double(Counter)));}
	double R_HOOK()const  { return  (Counter==0?-1:(HOOK==0?0:HOOK/double(Counter)));}
	double R_FIGHT()const  { return  (Counter==0?-1:(FIGHT==0?0:FIGHT/double(Counter)));}
	double R_CALLAUTH()const { return  (Counter==0?-1:(CALLAUTH==0?0:CALLAUTH/double(Counter)));}

	};//Struct Actions

	struct LinkTo {unsigned X,Y;unsigned Parent:1;unsigned Child;
					LinkTo(){X=Y=-1;Parent=0;Child=0;}
					};

 public: 
	//ATTRIBUTES
	static unsigned licznik_zyc;//To create unique identifiers of agents (Do tworzenia unikalnych identyfikatorów agentów)
	static bool 	CzyTorus; // Is the geometry of the torus or the island with the banks (Czy geometria torusa czy wyspa z brzegami)

	unsigned ID; // The unique identifier, just subsequent agents in the particular course (Unikalny identyfikator, po prostu kolejni agenci  w danym przebiegu)
	unsigned HisLifeTime;//Life span (Czas życia)
	Actions  HisActions; // Counters of actions (Liczniki akcji)

	double Power;//Real power or strength (0..1)
	double PowLimit;//What strength can reach up (Jaką siłę może osiągnąć maksymalnie)
	double Agres;//Propensity to attack (0..1), ONLY 0 or 1 used currently (Agresywność czyli skłonność do atakowania)
	double Honor;//Propensity to be honor (0..1), ONLY 0 or 1 used currently (Skłonność do podjęcia obrony niezależnie od siły przeciwnika)
	double CallPolice;//The probability of calling the police (0..1), ONLY 0 or 1 used currently (Prawdopodobieństwo wzywania policji)
	//METHOD OF CLASS AGENT
	////////////////////////////////////////////////////////////////////////////
	HonorAgent(): //CONSTRUCTOR
	    Neighbourhood(MAX_LINKS),HowManyNeigh(0)
	   ,Power(0),PowLimit(0),HisLifeTime(0)
	   ,Agres(0),HonorFeiRep(0.5),CallPolice(0.25)
	   ,MemOfLastDecision(HonorAgent::NOTHING),ID(0)
	{
		Color.r=25+RANDOM(220);Color.g=25+RANDOM(220);Color.b=25+RANDOM(220);
	}

	void RandomReset(float POWLIMIT=0); //Drawing attribute values (Losowanie wartości atrybutów)

	// Neighbourhood Support ( Obsługa połączeń)
////////////////////////////////
unsigned NeighSize() const; //How many registered neighbors (Ile ma zarejestrowanych sąsiadów)
	bool addNeigh(unsigned x,unsigned y);//Adds the specified neighbor at coordinates in the world, as far as can fit on the list (Dodaje sąsiada o określonych współrzędnych w świecie, o ile zmieści na liście)
	bool getNeigh(unsigned i,unsigned& x,unsigned& y) const;//Reads the coordinates of the i-th neighbor (Odczytuje współrzędne sąsiada i-tego sąsiada)
	
	bool getNeigh(HonorAgent* Ptr,unsigned i,unsigned& x,unsigned& y) const;//Looking for a neighbor via the pointer (Szuka sąsiada po wskaźniku)
	void forgetAllNeigh(); // He forgets all added neighbors, which does not mean that they forget his (Zapomina wszystkich dodanych sąsiadów, co nie znaczy że oni zapominają jego)
	static bool AreNeigh(int x1,int y1,int x2,int y2);// Checking whether the two agents are adjacent in some way (Sprawdzanie czy dwaj agenci sąsiadują w jakiś sposób)

	//Method for making decision
	Decision  check_partner(unsigned& x, unsigned& y); // Choice of interaction partner  (Wybór partnera interakcji)
	Decision  answer_if_hooked(unsigned x, unsigned y); // The answer to provocation (Odpowiedź na zaczepkę)
	void      change_reputation(double Delta, HonorAgent& Powod, int level=0);//(Wzrost lub spadek reputacji z zabezpieczeniem zakresu, i ewentualnym dziedziczeniem)
	void      lost_power(double delta);  //An increase or decrease of the reputation with protection scope, and possible inheritance (Spadek, zużycie siły z zabezpieczeniem zera)
	static
	bool      firstWin(HonorAgent& In,HonorAgent& Ho); //Determines whether the first or second agent was the winner in the confrontation (Ustala czy pierwszy czy drugi agent zwyciężył w konfrontacji)
	
	// The main dynamics of the simulation
    	friend void InitAtributes(FLOAT HowMany);
	friend void InitConnections(FLOAT HowManyFar);
	friend void one_step(unsigned long& step_counter);
	friend void power_recovery_step();
	friend void DeleteAllConnections();
	friend void Reset_action_memories();
	
	//NOT USED! Support of family relationships (Obsługa stosunków rodzinnych)
	bool    IsParent(unsigned i);//Is this neighbor is the parent (Czy dany sąsiad jest rodzicem)
	bool    IsChild(unsigned i); //Is this neighbor is a child (Czy dany sąsiad jest dzieckiem)
	bool    IsMyFamilyMember(HonorAgent& Inny,HonorAgent*& Cappo,int MaxLevel=2);//Is another "belongs to the family". By the way, we set the "Godfather" (Czy inny "należy do rodziny". Przy okazji ustalamy "Ojca chrzestnego")
	void    change_reputation_thru_family(double Delta);//Recursive change of the reputation of the agent down the best of 'Cappo' of the family (Rodzinna, rekurencyjna zmiana reputacji od agenta w dół najlepiej z "Cappo" rodziny)
	void 	SmiercDona();//removes family links (Usuwa powiązania rodzinne)
	friend void PowiazRodzicielsko(HonorAgent& Rodzic,HonorAgent& Ag);  // builds a family connections (buduje powiązanie rodzinne)

	//Methods for statistics & visualization 
	ssh_rgb   GetColor() const {return Color;} //Individual color non modifiable from the outside (Nie modyfikowalny z zewnątrz indywidualny kolor) 
	double    GetFeiReputation() const { return HonorFeiRep;}
	wbrtm::wb_pchar  AgentCultureStr() const;// Returns textual representation of culture Agent (Zwraca tekstową reprezentacje kultury agenta)
	unsigned  AgentCultureMask() const;// (Returns bitwise representations of culture of the agent (Zwraca maskę 3 bitów, pokazującą gdzie jest różne od 0. Aggr:1. bit, Honor:2.bit CallPoll:3.bit)	
	Decision  LastDecision(bool clean=false); // The recent decision of the MC step (Ostatnia decyzja z kroku MC)
	friend void PrintHonorAgentInfo(std::ostream& o,const HonorAgent& H);//For inspection of the agent

 private:
	const LinkTo* Neigh(unsigned i); //Raw access to link data

	wbrtm::wb_dynarray<LinkTo> Neighbourhood; //List of neighbors
	unsigned HowManyNeigh; //Number  of neighbors	
Decision  MemOfLastDecision;

	double HonorFeiRep; //Reputation as a warrior (0..1) (Reputacja wojownika) 
	ssh_rgb   Color;	// Individual and unchangeable, or calculated  by any of the functions for color coding (Indywidualny i niezmienny lub obliczony którąś z funkcji kodujących kolor)
};


inline
bool HonorAgent::AreNeigh(int x1,int y1,int x2,int y2)
{
	HonorAgent& A=HonorAgent::World[y1][x1];
	for(unsigned i=0;i<A.NeighSize();i++)
	{
	   if(A.Neighbourhood[i].X==x2 && A.Neighbourhood[i].Y==y2) // Already on the list

				return true;
	}
	return false; // not found
}

//Global simulation functions defined in HonorAgent.cpp
void InitAtributes(FLOAT HowMany);
void InitConnections(FLOAT HowManyFar);
void DeleteAllConnections();
void one_step(unsigned long& step_counter);
void power_recovery_step();
void Reset_action_memories();


//Accesors for statistics & visualization
/////////////////////////////////////////////////////////////////////////////////

inline void HonorAgent::Actions::Count(HonorAgent::Decision Deci)
{
			Counter++; //Which count of actions
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

inline unsigned  HonorAgent::AgentCultureMask() const
//Zwraca maskę 3 bitów, pokazującą gdzie jest różne od 0. Aggr:1. bit, Honor:2.bit CallPoll:3.bit
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

inline wbrtm::wb_pchar  HonorAgent::AgentCultureStr()  const
//Zwraca reprezentacje tekstową kultury agenta
{
   wbrtm::wb_pchar Pom(128);
   if(Agres==0 && Honor==0 && CallPolice==0) //Racjonalny jest
		Pom.add("Rational");

   if(Agres>0)
		Pom.add("Aggressive");

   if(Honor>0)
		Pom.add("Honor");

   if(CallPolice>0)
		Pom.add("Dignity");

   return Pom;
}

// FOR FAMILIES VERSION - NOT USED FOR PAPER 2015
////////////////////////////////////////////////////////////////////////
inline bool    HonorAgent::IsParent(unsigned i)
//Czy dany sąsiad jest rodzicem
{
	if(i<this->NeighSize()
	&& this->Neighbourhood[i].Parent==1)
		return true;
	else
		return false;
}

inline
bool    HonorAgent::IsChild(unsigned i)
//Czy dany sąsiad jest dzieckiem
{
	if(i<this->NeighSize()
	&& this->Neighbourhood[i].Child==1)
		return true;
	else
		return false;
}

inline void 	HonorAgent::SmiercDona()//Usuwa powiązania
{
	for(unsigned i=0;i<this->NeighSize();i++)
	{
		if(Neighbourhood[i].Parent==1)//Rodzic agenta, jeśli żyje zostaje powiadomiony że stracił dziecko
		{
			Neighbourhood[i].Parent=0;//Dla porządku
			HonorAgent& Rodzic=World[Neighbourhood[i].Y][Neighbourhood[i].X];
			for(unsigned j=0;j<Rodzic.NeighSize();j++)
			if( this==&World[Rodzic.Neighbourhood[j].Y][Rodzic.Neighbourhood[j].X] ) //Się znalazł na liście
			{																								assert(Rodzic.Neighbourhood[j].Child==1);
				Rodzic.Neighbourhood[j].Child=0;
				//Rodzic.notifyChildDeath(j);
				break;
			}
		}
		else
		if(Neighbourhood[i].Child==1)//Dzieci agenta, jesli żyją, zostają powiadomione że straciły rodzica
		{
			Neighbourhood[i].Child=0;//Dla porządku
			HonorAgent& Dziecko=World[Neighbourhood[i].Y][Neighbourhood[i].X];
			for(unsigned j=0;j<Dziecko.NeighSize();j++)
			if( this==&World[Dziecko.Neighbourhood[j].Y][Dziecko.Neighbourhood[j].X] ) //Się znalazł na tej liście
			{																								assert(Dziecko.Neighbourhood[j].Parent==1);
				Dziecko.Neighbourhood[j].Parent=0;
				//Rodzic.notifyChildDeath(j);
				break;
			}
		}
	}
}

inline void PowiazRodzicielsko(HonorAgent& Rodzic,HonorAgent& NowyAgent)
//anty SmiercDona(); - buduje powiązania
{
	for(unsigned i=0;i<NowyAgent.NeighSize();i++)
	if( &Rodzic==&HonorAgent::World[NowyAgent.Neighbourhood[i].Y][NowyAgent.Neighbourhood[i].X] ) //Znalazł rodzica na swojej liście
{                                                                           assert(NowyAgent.Neighbourhood[i].Parent==0);
		NowyAgent.Neighbourhood[i].Parent=1;
		break;
	}

	for(unsigned j=0;j<Rodzic.NeighSize();j++)
	if( &NowyAgent==&HonorAgent::World[Rodzic.Neighbourhood[j].Y][Rodzic.Neighbourhood[j].X] ) //Się odnalazł na liście u rodzica
	{																										assert(Rodzic.Neighbourhood[j].Child==0);
		Rodzic.Neighbourhood[j].Child=1;
		//Rodzic.notifyChildBirdth(j);
		break;
	}
}

