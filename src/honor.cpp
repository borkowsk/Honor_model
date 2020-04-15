// Prototypowy program "ewolucji" kultury honory w kulturê policyjn¹
//                                                             wersja 01-06-2012
////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <fstream>
#include <iomanip>
#define _USE_MATH_DEFINES //bo M_PI
#include <cmath>

#define HIDE_WB_PTR_IO 1
#include "INCLUDE/wb_ptr.hpp"
using namespace wbrtm;
//#include "INCLUDE/wb_bitarray.h" //Mo¿e siê przyda póŸniej
#define USES_STDC_RAND
#include "INCLUDE/Random.h"
#include "SYMSHELL/symshell.h"
#include "SYMSHELL/sshutils.hpp"
#include "MISCCLASSES/Wieloboki.hpp"


const char* MODELNAME="Culture of honor";
const char* VERSIONNUM="0.26 (03-06-2012)";
bool  batch_mode=true;       //Czy tryb pracy przeszukiwania przestrzeni parametrów?
bool  Compansation_mode=true;//Czy przestrzeñ gdzie Honorowi i CallPolice uzupe³niaj¹ siê do PROP_MAX
							 //Ma sens tylko dla zafiksowanych bulli (agression)
bool  BatchPlotPower=true;   //Czy w trakcie wyœwietlaæ MnPower czy MnProportions
bool  Batch_true_color=false;//Czy skale kolorów true-color czy 256 kolorów têczy
unsigned population_growth=1;//SPOSOBY ROZMNA¯ANIA
							 // 0 - wg. inicjalnych proporcji
							 // 1 - lokalne rozmazanie losowe s¹siad
							 //	2 - lokalne rozmazanie proporcjonalne do sily
							 //	3 - losowy ziutek z ca³oœci
typedef double FLOAT;

//ZMIANY:
//0.26 -  Wykres przestrzeni - w proporcji honorowych i policyjnych do siebie dziel¹cych 1/2 lub 2/3 ca³oœci.
//		  Inny sposób kolorowania "kultur" na obrazie œwiata symulacji (czerwony-agresja, zielony-honor, niebieski-callPolice)
//		  Próba zmiany rozk³adu ³omotu dla GIVEUP - ¿eby mogli zgin¹æ, ale bardziej niszczy bullys!?!?
//		  Do ewolucji potrzebny szumowe œmierci - parametr NOISE_KILL
//		  ewolucja:
//		  - lokalne rozmazanie losowe s¹siad
//		  TODO  - losowy ziutek z ca³oœci
//		  TODO  - lokalne rozmazanie proporcjonalne do sily
//		  Statystyki: ile razy agent jest atakowany i ile razy wygra³ ,
//						klastering index dla kultur
//        Czestoœci ataków na poszczególne grupy w czasie - bez selekcji, zysk indywidualny
//        Du¿a mapa z gradientem policji

//		  ewentualnie ró¿na aktywnosc honorowych. Nie zawsze musz¹ stawaæ, ale z prawdopodobienstwem
//		 NA POZNIEJ: iloœæ policji zale¿y od iloœci ataków? Inny model - podatkowy.
//
//0.251 - poprawienie b³êdu w liczeniu "always give up" na metryczce pliku log
//
//0.25 - wprowadzenie statystyk z si³y ró¿nych grup (najwy¿szy decyl tylko)
//		 i ich drukowania do pliku i na konsolê
//		 Uelastycznienie wydruku metryczki, ¿eby nadawa³a siê i do pionu i do poziomu ...
//		 WPROWADZENIE TRYBU PRZESZUKIWANIA PRZESTRZENI PARAMETRÓW
//		 W tym ZAPISU BITMAP Z WYNIKAMI
//0.24 - wprowadzenie parametru RATIONALITY, bo u¿ycie w pe³ni realistycznej oceny si³y psuje selekcje
//		 Zmiany kosmetyczne w nag³ówku pliku wyjœciowego
//0.23 - znaj¹ swoj¹ realn¹ si³ê i j¹ porównuj¹ z reputacj¹ drugiego gdy maj¹ atakowaæ lub siê broniæ
//		 Ale to nie dzia³a³o dobrze...
//0.20 - pierwsza wersja w pe³ni dzia³aj¹ca

const FLOAT POLICE_EFFIC_STEP=0.01;
const FLOAT POLICE_EFFIC_MAX=1;
const FLOAT POLICE_EFFIC_MIN=0;
const FLOAT PROPORTION_STEP=0.01;
const FLOAT PROPORTION_MAX=1.0/3.0;
const FLOAT PROPORTION_MIN=0.01;

const unsigned SIDE=50;//SIDE*SIDE to rozmiar œwiata symulacji

const FLOAT    USED_SELECTION=0.05; //Jak bardzo przegrani umieraj¹ (0 - brak selekcji w ogóle)
const FLOAT    NOISE_KILL=0.001; //Jakie jest prawdopodobienstwo przypadkowej œmierci
const FLOAT    RECOVERY_POWER=0.005;//Jak¹ czêœæ si³y odzyskuje w kroku
const FLOAT    RATIONALITY=0.0; //Jak realistycznie ocenia w³asn¹ si³ê (vs. wg. w³asnej reputacji)
const FLOAT    RANDOM_AGRESSION=0.015;//Bazowy poziom agresji dla honorowych

FLOAT    POLICE_EFFIC=0.050;//0.950; //Z jakim prawdopodobieñstwem wezwana policja obroni agenta

//LINKOWANIE
const bool 	   TORUS=false; //Czy geometria torusa czy wyspy z brzegami
const int	   MOORE_RAD=3; //Nie zmieniaæ na unsigned bo siê psuje losowanie!
const FLOAT    OUTFAR_LINKS_PER_AGENT=0.5; //Ile jest dodatkowych linków jako u³amek liczby agentów
const unsigned MOORE_SIZE=(4*MOORE_RAD*MOORE_RAD)+4*MOORE_RAD;//S¹siedzi Moora - cztery kwadranty + 4 osie
const unsigned FAR_LINKS=unsigned(SIDE*SIDE*OUTFAR_LINKS_PER_AGENT*2);
const unsigned MAX_LINKS=MOORE_SIZE + 2/*ZAPAS*/ + (FAR_LINKS/(SIDE*SIDE)? FAR_LINKS/(SIDE*SIDE):2); //Ile maksymalnie mo¿e miec agent linków

//CECHY WRODZONE
const FLOAT    BULLISM_LIMIT=-1;//0.2;//0.66;//0.10;//Maksymalny mo¿liwy bulizm.
								//Jak ujemne to rozk³ad Pareto, jak dodatnie to dzwonowy

FLOAT    BULLI_POPUL=-0.33;//0.2;//0.100;//Albo zero-jedynkowo. Jak 1 to decyduje rozk³ad sterowany BULLISM_LIMIT ("-" jest sygna³em zafiksowania w trybie batch
FLOAT	 HONOR_POPUL=0.125;//0.3333;//Jaka czêœæ agentów populacji jest œciœle honorowa
FLOAT    CALLER_POPU=0.125;//Jaka czêœæ wzywa policje zamiast siê poddawaæ
								 //- musi byæ dwa razy wiêcej bo to udzia³ reszcie tych co nie s¹ agresywni i honorowi
								 //Mo¿e te¿ jako „wêdrowna œrednia” wezwañ, ale na razie nie

//const unsigned MAX_INIT_REPUTATION=100; //Maksymalna mo¿liwa pocz¹tkowa reputacja
const double   SOCIAL_IMPACT_INTENSITY_PERCENT=0; //Jaka jest wzglêdna intensywnoœæ wp³ywu spo³ecznego. W procentach kroku MC g³ównej dynamiki. 0 oznacza brak

//Sterowanie statystykami i powtórzeniami
unsigned REPETITION_LIMIT=10;//10//Ile ma zrobiæ powtórzeñ tego samego eksperymentu
unsigned RepetNum=1; //Która to kolejna repetycja?  - NIE ZMIENIAÆ RÊCZNIE!
unsigned STOP_AFTER=10000;//Po jakim czasie staje automatycznie
unsigned EveryStep=10;//Czêstotliwoœæ wizualizacji i zapisu do logu
					 //Ujemne oznacza tryb automatycznej inkrementacji
unsigned DumpStep=1000;//Czêstoœæ zrzutów stanów agentów

//Parametry techniczne steruj¹ce wizualizacj¹ i wydrukami
const unsigned VSIZ=10; //Maksymalny rozmiar boku agenta w wizualizacji kompozytowej
const unsigned SSIZ=2; //Bok agenta w wizualizacji uzupe³niaj¹cej (ma³ej)

bool  ConsoleLog=1;//Czy u¿ywa logowania zdarzeñ ma konsoli. Wa¿ne dla startu, potem siê da prze³¹czaæ
bool  VisShorLinks=false; //Wizualizacja bliskich linków
bool  VisFarLinks=false;  //Wizualizacja dalekich
bool  VisAgents=true;     //Wizualizacja w³aœciwoœci agentów
bool  dump_screens=false;

int   WB_error_enter_before_clean=0; //Czy daæ szanse operatorowi na poczytanie komunikatów koñcowych

void Parameters_dump(ostream& o,const char* SEP="\t",const char* ENDL="\n",bool FL=true)
{
	o<<MODELNAME<<"\tv.:"<<SEP<<VERSIONNUM<<ENDL;
	o<<"uint"<<SEP<<"SIDE"<<SEP<<SIDE<<ENDL;//SIDE*SIDE to rozmiar œwiata symulacji
	o<<"bool"<<SEP<<"TORUS"<<SEP<<(TORUS?"true":"false")<<ENDL;

	o<<"uint"<<SEP<<"MOORE_RAD"<<SEP<<MOORE_RAD<<ENDL;
	o<<"FLOAT"<<SEP<<"OUT_FAR_LINKS_PER_AGENT"<<SEP<<OUTFAR_LINKS_PER_AGENT<<ENDL;//Ile jest dodatkowych linków jako u³amek liczby agentów
	o<<"FLOAT"<<SEP<<"USE_SELECTION"<<SEP<<USED_SELECTION<<SEP<<"NOISE_KILL"<<SEP<<NOISE_KILL<<SEP<<"POP_GROWTH_MODE"<<SEP<<population_growth<<ENDL;  //
	o<<"FLOAT"<<SEP<<"RECOVERY_POWER"<<SEP<<RECOVERY_POWER<<ENDL;//Jak¹ czêœæ si³y odzyskuje w kroku

	if(batch_mode)
	{
		o<<"FLOAT"<<SEP<<"POLICE_EFFIC_STEP"<<SEP<<POLICE_EFFIC_STEP<<SEP<<"POLICE_EFFIC_MIN"<<SEP<<POLICE_EFFIC_MIN<<SEP<<"POLICE_EFFIC_MAX"<<SEP<<POLICE_EFFIC_MAX<<ENDL;
		o<<"FLOAT"<<SEP<<"PROPORTION_STEP"<<SEP<<PROPORTION_STEP<<SEP<<"PROPORTION_MIN"<<SEP<<PROPORTION_MIN<<SEP<<"PROPORTION_MAX"<<SEP<<PROPORTION_MAX<<ENDL;
		o<<"FLOAT"<<SEP<<"BULLY_POPUL"<<SEP;if(BULLI_POPUL>0)o<<"*";else o<<-BULLI_POPUL;o<<ENDL;
		o<<"FLOAT"<<SEP<<"HONOR_POPUL"<<SEP;if(HONOR_POPUL>0)o<<"*";else o<<-HONOR_POPUL;
		    o<<SEP<<"Compansation_mode"<<SEP<<(Compansation_mode?"true":"false")<<ENDL;
		o<<"FLOAT"<<SEP<<"CALLP_POPUL"<<SEP;if(CALLER_POPU>0)o<<"*";else o<<-CALLER_POPU;o<<ENDL;
	}
	else
	{
	 if(BULLI_POPUL>=1) //...
	  o<<"FLOAT"<<SEP<<"BULLISM_LIMIT"<<SEP<<BULLISM_LIMIT<<ENDL;//Maksymalny mo¿liwy bulizm.
	 else
	 {
	  o<<"FLOAT"<<SEP<<"BULLY_POPUL"<<SEP<<BULLI_POPUL<<ENDL;//Albo zero-jedynkowo
	  o<<"FLOAT"<<SEP<<"HONOR_POPUL"<<SEP<<HONOR_POPUL<<ENDL;//Jaka czêœæ agentów populacji jest œciœle honorowa
	  o<<"FLOAT"<<SEP<<"CALLP_POPUL"<<SEP<<CALLER_POPU<<ENDL;//Jaka czêœæ wzywa policje zamiast siê poddawaæ
	 }
	 o<<"FLOAT"<<SEP<<"POLICE_EFFIC"<<SEP<<POLICE_EFFIC<<ENDL;
	 o<<"FLOAT"<<SEP<<"always give up"<<SEP<<(1.0-CALLER_POPU-CALLER_POPU-HONOR_POPUL)<<ENDL;//"loosers"
	 o<<"FLOAT"<<SEP<<"RANDOM_AGRESSION"<<SEP<<RANDOM_AGRESSION<<ENDL;//0.950; //Z jakim prawdopodobieñstwem wezwana policja obroni agenta
	 o<<"FLOAT"<<SEP<<"RATIONALITY"<<SEP<<RATIONALITY<<ENDL<<ENDL; //Jak realistycznie ocenia w³asn¹ si³ê (vs. wg. w³asnej reputacji)
	}
	o<<"REPET:"<<SEP<<"STOP:"<<SEP<<"VFREQ:"<<ENDL;
	o<<REPETITION_LIMIT<<SEP<<STOP_AFTER<<SEP<<EveryStep<<ENDL<<ENDL;
	if(FL) o.flush();
}

class HonorAgent// agent tego modelu
{
 public: //WEWNÊTRZNE TYPYT POMOCNICZE
	struct LinkTo {unsigned X,Y;};
	enum Decision {NOTHING=-1,WITHDRAW=0,GIVEUP=1,HOOK=2,FIGHT=3,CALLAUTH=4};

 public: //Na razie g³ówne w³aœciwoœci dostêpne zewnêtrznie, potem mo¿na pomysleæ czy je schowaæ
	unsigned ID; //Kolejni agenci
	static unsigned licznik_zyc;//Do tworzeni unikalnych identyfikatorów agentów
	double Power;//	Si³a (0..1)
	double PowLimit;// Jak¹ si³ê mo¿e osi¹gn¹æ maksymalnie, gdy nie traci

	double Agres;// Bulizm (0..1) sk³onnoœæ do atakowania
	double Honor;// Bezwarunkowa honorowoœæ (0..1) sk³onnoœæ podjêcia obrony

	double CallPolice;//Odium wzywacza policji - prawdopodobieñstwo wzywania policji (0..1) jako „wêdrowna œrednia” wezwañ
	//double Resources;// Zasoby (0..inf)

	HonorAgent();  //KONSTRUKTOR

	void RandomReset(); //Losowanie wartoœci atrubutów

	//Obsluga po³¹czeñ
	bool addNeigh(unsigned x,unsigned y);//Dodaje sasiada o okreslonych wspó³rzêdnych w œwiecie, o ile zmieœci
	bool getNeigh(unsigned i,unsigned& x,unsigned& y) const;//Czyta wspó³rzedne s¹siada, o ile jest
	unsigned NeighSize() const; //Ile ma zarejestrowanych s¹siadów
	void forgetAllNeigh(); //Zapomina wszystkich dodanych s¹siadów, co nie znaczy ¿e oni zapominaj¹ jego

	//Funkcje decyzyjne
	Decision  check_partner(wb_dynmatrix<HonorAgent>& World,unsigned& x,unsigned& y);//Wybór partnera interakcji
	Decision  answer_if_hooked(wb_dynmatrix<HonorAgent>& World,unsigned x,unsigned y);//OdpowiedŸ na zaczepkê
	void      change_reputation(double delta);//Wzrost lub spadek reputacji z zabezpieczeniem zakresu
	void      lost_power(double delta);  //Spadek, zu¿ycie si³y z zabezpieczeniem zera
	static
	bool      firstWin(HonorAgent& In,HonorAgent& Ho);//Ustala czy pierwszy czy drugi agent zwyciê¿y³ w konfrontacji

	//Inne
	double    GetFeiReputation() const { return HonorFeiRep;}
	Decision  LastDecision(bool clean=false); //Ostatnia decyzja z kroku MC do celów wizualizacyjnych i statystycznych
	ssh_rgb   GetColor() const {return Color;} //Nie modyfikowalny z zewn¹trz indywidualny kolor wêz³a

 private:
	ssh_rgb   Color;	//Indywidualny i niezmienny lub obliczony któr¹œ z funkcji koduj¹cych kolor
	Decision  MemOfLastDecision;
	wb_dynarray<LinkTo> Neighbourhood;//Lista wspó³rzêdnych s¹siadów
	unsigned HowManyNeigh; //Liczba posiadanych s¹siadów

	double HonorFeiRep;//Reputacja wojownika jako „wêdrowna œrednia” z konfrontacji (0..1)


	const LinkTo* Neigh(unsigned i); //Dostêp do rekordu linku do kolejnego s¹siada
};
unsigned HonorAgent::licznik_zyc=0;//Do tworzenia unikalnych identyfikatorów agentów
ofstream OutLog("Honor.log");
ofstream Dumps("Honor_dump.txt");
unsigned long  step_counter=0;
unsigned long  LastStep=-1;//Ostatnie wypisany krok

HonorAgent::HonorAgent():
	   Neighbourhood(MAX_LINKS),HowManyNeigh(0)
	   ,Power(0),PowLimit(0)
	   ,Agres(0),HonorFeiRep(0.5),CallPolice(0.25)
	   ,MemOfLastDecision(HonorAgent::NOTHING),ID(0)
	 //  ,Resources(0),
{
	Color.r=25+RANDOM(220);Color.g=25+RANDOM(220);Color.b=25+RANDOM(220);
}

void HonorAgent::RandomReset()
//Losowanie wartoœci atrubutów
{
        ID=++licznik_zyc;
		PowLimit=(DRAND()+DRAND()+DRAND()+DRAND()+DRAND()+DRAND())/6;  // Jak¹ si³ê mo¿e osi¹gn¹æ maksymalnie, gdy nie traci
		Power=(0.5+DRAND()*0.5)*PowLimit; //	Si³a (0..inf)
		HonorFeiRep=Power;//DRAND()*0.5;  //Reputacja obrony jako „wêdrowna œrednia” z konfrontacji
													assert(0<=HonorFeiRep);
													assert(HonorFeiRep<=1);
/*
		if(BULLI_POPUL<1)
		{
		  Agres=(DRAND()<BULLI_POPUL?1:0); //Albo jest albo nie jest
		}
		else
		{
		 if(BULLISM_LIMIT>0)
			Agres=(DRAND()+DRAND()+DRAND()+DRAND()+DRAND()+DRAND())/6*BULLISM_LIMIT; // Bulizm (0..1) sk³onnoœæ do atakowania,  const FLOAT    BULLISM_LIMIT=1;//Maksymalny mo¿liwy bulizm.
			else
			Agres=(DRAND()*DRAND()*DRAND()*DRAND())*fabs(BULLISM_LIMIT);
		}

		Honor=(DRAND()<HONOR_POPUL?1:0); //Albo jest albo nie jest -  Bezwarunkowa honorowoœæ (0..1) sk³onnoœæ podjêcia obrony,  const FLOAT	   HONOR_POPUL=0.9;//Jaka czêœæ agentów populacji jest œciœle honorowa


		if(Honor<0.99 && Agres<0.99 && DRAND()<CALLER_POPU) //??? Agres XOR HighHonor XOR CallPolice
			CallPolice=1;//Prawdopodobieñstwo wzywania policji (0..1) Mo¿e te¿ jako „wêdrowna œrednia” wezwañ, ale na razie nie
			else
			CallPolice=0;
*/
		if(fabs(BULLI_POPUL)<1) //
		{
		   Honor=0;CallPolice=0;//Wartoœci domyœlne dla agresywnych i wzywaj¹cych policje
		   Agres=(DRAND()<fabs(BULLI_POPUL)?1:0); //Albo jest albo nie jest
		   if(Agres!=1)
		   {
			 Honor=(DRAND()*(1-fabs(BULLI_POPUL))<fabs(HONOR_POPUL)?1:0);
			 if(Honor!=1)
			   CallPolice=(DRAND()*(1-fabs(BULLI_POPUL)-fabs(HONOR_POPUL))<fabs(CALLER_POPU)?1:0);
			   else
			   { /* LOOSER */ }
		   }
		}
	 //	Resources=(DRAND()*DRAND()*DRAND()*DRAND()*DRAND()*DRAND())*10000;  //Zasoby (0..inf)
}

bool      HonorAgent::firstWin(HonorAgent& In,HonorAgent& Ho)
//Ustala czy pierwszy czy drugi agent zwyciê¿y³ w konfrontacji
//Zawsze któryœ musi zwyciê¿yæ, co znowu nie jest realistyczne
{                                                           ;
	if( In.Power*(1+DRAND())  >  Ho.Power*(1+DRAND()) )
			return true;
			else
			return false;
}

void      HonorAgent::change_reputation(double delta)
//Wzrost lub spadek reputacji z zabezpieczeniem zakresu
{
													assert(0<=HonorFeiRep);
													assert(HonorFeiRep<=1);
	if(delta>0) //Wzrost
	{
		HonorFeiRep+=(1-HonorFeiRep)*delta;       	assert(HonorFeiRep<=1);
	}
	else
	{
		HonorFeiRep-=HonorFeiRep*fabs(delta);       assert(0<=HonorFeiRep);
													assert(HonorFeiRep<=1);
	}
}

void      HonorAgent::lost_power(double delta)
//Spadek si³y z zabezpieczeniem zakresu
{                                  					assert(Power<=1);
	 delta=fabs(delta);
	 Power*=(1-delta);             					assert(Power>0);
	 //if(Power<0) Power=0;
}

HonorAgent::Decision  HonorAgent::check_partner(wb_dynmatrix<HonorAgent>& World,unsigned& x,unsigned& y)
//Wybór partnera interakcji przez agenta, który dosta³ losow¹ inicjatywê
{
	this->MemOfLastDecision=WITHDRAW; //DOMYŒLNA DECYZJA
	unsigned L=RANDOM(HowManyNeigh);//Ustalenie który s¹siad
	if(getNeigh(L,x,y) ) //Pobranie wspó³rzêdnych s¹siada
	{
		HonorAgent& Ag=World[y][x];	//Zapamiêtanie referencji do s¹siada
		//BARDZO PROSTA REGU£A DECYZYJNA - wygl¹da, ¿e warto atakowaæ i jest chêæ
		if(this->Agres>0.0 && DRAND()<this->Agres           //Agresywnoœæ jako
		&& Ag.HonorFeiRep<(RATIONALITY*this->Power+(1-RATIONALITY)*HonorFeiRep)
		) //jako wyrachowanie
		{
			this->MemOfLastDecision=HOOK;//Nieprzypadkowa zaczepka bulliego
		}
		else if(
		//DRAND()<RANDOM_AGRESSION)					//+losowe przypadki burd po pijaku - ZACIEMNIA
		(Honor>0 && DRAND()<Honor*RANDOM_AGRESSION) //A mo¿e tylko honorowi maj¹ niezer¹ agresjê z powodu nieporozumieñ?
		)
		{
			this->MemOfLastDecision=HOOK;//Ewentualna zaczepka losowa lub honorowa
		}
	}
	return this->MemOfLastDecision;
}

HonorAgent::Decision  HonorAgent::answer_if_hooked(wb_dynmatrix<HonorAgent>& World,unsigned x,unsigned y)
//OdpowiedŸ na zaczepkê
{
	 this->MemOfLastDecision=GIVEUP;//DOMYŒLNA DECYZJA
	 HonorAgent& Ag=World[y][x];	//Zapamiêtanie referencji do s¹siada

	 //REGU£A ZALE¯NA WYCHOWANIA POLICYJNEGO
	 //LUB OD HONORU i W£ASNEGO POCZUCIA SI£Y
	 if(DRAND()<this->CallPolice)  //TOCHECK CallPolice==0
	 {
		this->MemOfLastDecision=CALLAUTH;
	 }
	 else
	 if(DRAND()<this->Honor
	 || Ag.HonorFeiRep<this->Power /*HonorFeiRep*/)   //TOCHECK? Honor==1
	 {
		this->MemOfLastDecision=FIGHT;
	 }

	 return this->MemOfLastDecision;
}

void one_step(wb_dynmatrix<HonorAgent>& World)
//G³ówna dynamika kontaktów
{
	unsigned N=(SIDE*SIDE)/2;//Ile losowañ w kroku MC? Po³owa, bo w ka¿dej interakcji dwaj
	for(unsigned i=0;i<N;i++)
	{
		unsigned x1=RANDOM(SIDE);
		unsigned y1=RANDOM(SIDE);
		HonorAgent& AgI=World[y1][x1];  //Zapamiêtanie referencji do agenta inicjuj¹cego
		unsigned x2,y2;
		HonorAgent::Decision Dec1=AgI.check_partner(World,x2,y2);
		if(Dec1==HonorAgent::HOOK) //Jeœli zaczepi³
		{                                					assert(AgI.Agres>0 || AgI.Honor>0);
		   AgI.change_reputation(+0.05); // Od razu dostaje powzy¿szenie reputacji za samo wyzwanie
		   HonorAgent& AgH=World[y2][x2];//Zapamiêtanie referencji do agenta zaczepionego
		   HonorAgent::Decision Dec2=AgH.answer_if_hooked(World,x1,y1);
		   switch(Dec2){
		   case HonorAgent::FIGHT:
					   if(HonorAgent::firstWin(AgI,AgH))
					   {
						  AgI.change_reputation(+0.35);//Zyska³ bo wygra³
						  AgI.lost_power(-0.75*DRAND());//Straci³ bo walczy³

						  AgH.change_reputation(+0.1); //Zyska³ bo stan¹³
						  AgH.lost_power(-0.95*DRAND());//Straci³ bo przegra³ walkê
					   }
					   else
					   {
						  AgI.change_reputation(-0.35);//Straci³ bo zaczepi³ i dosta³ bêcki
						  AgI.lost_power(-0.95*DRAND()); //Straci³ bo przegra³ walkê

						  AgH.change_reputation(+0.75);//Zyska³ bo wygra³ choæ by³ zaczepiony
						  AgH.lost_power(-0.75*DRAND());	//Straci³ bo walczy³
					   }
					break;
		   case HonorAgent::GIVEUP:
					   {
						  AgI.change_reputation(+0.5); //Zyska³ bo wygra³ bez walki. Na pewno wiêcej ni¿ w walce???

						  AgH.change_reputation(-0.5); //Straci³ bo siê podda³
						  AgH.lost_power(-0.5*DRAND()/*DRAND()*/);//I trochê dosta³ ³omot dla przyk³adu   (!!!)
					   }
					break;
		   case HonorAgent::CALLAUTH:
					  if(DRAND()<POLICE_EFFIC) //Czy przyby³a
					  {
						  AgI.change_reputation(-0.35);//Straci³ bo zaczepi³ i dosta³ bêcki od policji
						  AgI.lost_power(-0.99*DRAND()); //Straci³ bo walczy³ i przegra³ z przewa¿aj¹c¹ si³¹

						  AgH.change_reputation(-0.75);//A zaczepiony straci³ bo wezwa³ policje zamiast walczyæ
					  }
					  else //A mo¿e nie
					  {
						  AgI.change_reputation(+0.5);//Zyska³ bo wygra³ bez walki. A¿ tyle?

						  AgH.change_reputation(-0.75);//Zaczepiony straci³ bo wezwa³ policje
						  AgH.lost_power(-0.99*DRAND()); //Straci³ bo siê nie broni³, a wkurzy³ wzywaj¹c
					  }
					break;
		   //Odpowiedzi na zaczepkê, które siê nie powinny zdarzaæ
		   case HonorAgent::WITHDRAW:/* TU NIE MO¯E BYÆ */
		   case HonorAgent::HOOK:	 /* TU NIE MO¯E BYÆ */
		   default:                  /* TU JU¯ NIE MO¯E BYÆ*/
				  cout<<"?";  //Podejrzane - nie powinno siê zdarzaæ
		   break;
		   }
		}
		else  //if WITHDRAW  //Jak siê wycofa³ z zaczepiania?
		{
		  	AgI.change_reputation(-0.0001);//Minimalnie traci w swoich oczach
		}
	}

	step_counter++;
}

//typedef ssh_rgb CalculateColorFunction(bitarray&); //Typ funkcji obliczaj¹cej kolor dla wirusa i hosta
//CalculateColorFunction* CalculateColorDefault=NULL; //WskaŸnik do domyœlnej funkcji koloruj¹cej
	//Po³¹czenia z najbli¿szymi s¹siadami
void DeleteAllConnections(wb_dynmatrix<HonorAgent>& World)
{
	for(int x=0;x<SIDE;x++)
		for(int y=0;y<SIDE;y++)
		{
		   HonorAgent& Ag=World[y][x];	//Zapamiêtanie referencji do agenta, ¿eby ci¹gle nie liczyæ indeksów
		   Ag.forgetAllNeigh(); //Bezwarunkowe zapomnienie
		}
}

void InitConnections(wb_dynmatrix<HonorAgent>& World,FLOAT HowManyFar)
{
	//Po³¹czenia z najbli¿szymi s¹siadami
	for(int x=0;x<SIDE;x++)
		for(int y=0;y<SIDE;y++)
		{
		   HonorAgent& Ag=World[y][x];	//Zapamiêtanie referencji do agenta, ¿eby ci¹gle nie liczyæ indeksów
		   for(int xx=x-MOORE_RAD;xx<=x+MOORE_RAD;xx++)
			  for(int yy=y-MOORE_RAD;yy<=y+MOORE_RAD;yy++)
			  if(!(xx==x && yy==y))//Wyciêcie samego siebie
			  {
				 if(TORUS)
				   Ag.addNeigh((xx+SIDE)%SIDE,(yy+SIDE)%SIDE);//Zamkniête w torus
				   else
				   if(0<=xx && xx<SIDE && 0<=yy && yy<SIDE)
					 Ag.addNeigh(xx,yy);//bez boków
			  }
		}

	//Dalekie po³¹czenia
	for(int f=0;f<HowManyFar;f++)
	{
		unsigned x1,y1,x2,y2;
		//POszukanie agentów z wolnymi slotami
		do{
		x1=RANDOM(SIDE);
		y1=RANDOM(SIDE);
		}while(World[y1][x1].NeighSize()>=MAX_LINKS);
		do{
		x2=RANDOM(SIDE);
		y2=RANDOM(SIDE);
		}while(World[y2][x2].NeighSize()>=MAX_LINKS);
		//Po³¹czenie ich linkami w obie strony
		World[y1][x1].addNeigh(x2,y2);
		World[y2][x2].addNeigh(x1,y1);
	}
}

void InitAtributes(wb_dynmatrix<HonorAgent>& World,FLOAT HowMany)
{
	for(int y=0;y<SIDE;y++)
	  for(int x=0;x<SIDE;x++)
		World[y][x].RandomReset();
}


void social_impact_step(wb_dynmatrix<HonorAgent>& World,double percent_of_MC=100)
//Powolna dynamika wp³ywu spo³ecznego - w losowej próbce s¹siedztwa Moora
{

	unsigned N=(SIDE*SIDE*percent_of_MC)/100;//Ile losowañ w kroku MC
	for(unsigned i=0;i<N;i++)
	{
		int v1=RANDOM(SIDE);
		int h1=RANDOM(SIDE);
		HonorAgent& Ag=World[v1][h1];	//Zapamiêtanie referencji do agenta, ¿eby ci¹gle nie liczyæ indeksów
		//...
	}
}

unsigned LiczbaTrupow=0;
unsigned LiczbaTrupowDzis=0;
void power_recovery_step(wb_dynmatrix<HonorAgent>& World)
{
  LiczbaTrupowDzis=0;
  for(int x=0;x<SIDE;x++)
	for(int y=0;y<SIDE;y++)
	{
		HonorAgent& Ag=World[y][x];	//Zapamiêtanie referencji do agenta, ¿eby ci¹gle nie liczyæ indeksów

		if(USED_SELECTION>0 && Ag.Power<USED_SELECTION)  //Chyba nie przezy³
		{
		   if(population_growth==0) //Tryb z prawdopodobienstwami inicjalnymi
		   {
			Ag.RandomReset();
		   }else
		   if(population_growth==1) //Tryb z losowym s¹siadem
		   {
			unsigned ktory=RANDOM(Ag.NeighSize()),xx,yy;
			bool pom=Ag.getNeigh(ktory,xx,yy);
			HonorAgent& Drugi=World[yy][xx];
			Ag.RandomReset();
			Ag.Agres=Drugi.Agres;
			Ag.Honor=Drugi.Honor;
			Ag.CallPolice=Drugi.CallPolice;
		   }
		   else
		   if(population_growth==3) //Tryb z losowym cz³onkiem populacji
		   {
			unsigned xx=RANDOM(SIDE),yy=RANDOM(SIDE);
			HonorAgent& Drugi=World[yy][xx];
			Ag.RandomReset();
			Ag.Agres=Drugi.Agres;
			Ag.Honor=Drugi.Honor;
			Ag.CallPolice=Drugi.CallPolice;
		   }
		   //	Ag.Power=0.1*Ag.PowLimit;
		   LiczbaTrupow++;
		   LiczbaTrupowDzis++;
		}
		else
		if(Ag.Power<Ag.PowLimit)//Mo¿e siê leczyæ lub "poprawiaæ"
		{
			Ag.Power+=(Ag.PowLimit-Ag.Power)*RECOVERY_POWER;
		}
	}

  //Losowe œmierci i przypadkowe narodzenia
  if(NOISE_KILL>0)
  {
	unsigned WypadkiLosow=SIDE*NOISE_KILL*SIDE;
	for(;WypadkiLosow>0;WypadkiLosow--)
	{
	  unsigned x=RANDOM(SIDE),y=RANDOM(SIDE);
	  HonorAgent& Ag=World[y][x];	//Zapamiêtanie referencji
	  Ag.RandomReset();
	}
  }
}

//  Liczenie statystyk
///////////////////////////////////////////////////////////////////////////////
double MeanFeiReputation=0;
double MeanCallPolice=0;
double MeanPower=0;
double MeanAgres=0;
double MeanHonor=0;

struct zliczacz
{
  double summ;
  unsigned N;
  Reset(){summ=0;N=0;}
  zliczacz():summ(0),N(0){}
  static Reset(zliczacz t[],unsigned N)
  {
	  for(unsigned i=0;i<N;i++)
		t[i].Reset();
  }
};

const NumOfCounters=4;
zliczacz  MnStrenght[NumOfCounters];//Liczniki sily skrajnych typów agentów
const char* MnStrNam[NumOfCounters]={"MnAgresPw","MnHonorPw","MnPolicPw","MnOthrPwr"};

union {
struct { unsigned NOTHING,WITHDRAW,GIVEUP,HOOK,FIGHT,CALLAUTH;};
		 unsigned Tab[6];
void Reset(){NOTHING=WITHDRAW=GIVEUP=HOOK=FIGHT=CALLAUTH=0;}
} Actions;

void CalculateStatistics(wb_dynmatrix<HonorAgent>& World)
{
	unsigned N=SIDE*SIDE;
	double ForFeiRep=0;
	double ForCallRe=0;
	double ForPower=0;
	double ForAgres=0;
	double ForHonor=0;
	Actions.Reset();
	zliczacz::Reset(MnStrenght,NumOfCounters);

	for(unsigned v=0;v<SIDE;v++)
	{
		for(unsigned h=0;h<SIDE;h++)
		{
			HonorAgent& Ag=World[v][h];			//Zapamiêtanie referencji do agenta, ¿eby ci¹gle nie liczyæ indeksów

			ForFeiRep+=Ag.GetFeiReputation();
			ForCallRe+=Ag.CallPolice;
			ForPower+=Ag.Power;
			ForAgres+=Ag.Agres;
			ForHonor+=Ag.Honor;

			if(0.9<Ag.Agres) //AGRESYWNI (BULLY) - Tylko najwy¿sze wartoœci
			{
			   MnStrenght[0].summ+=Ag.Power;MnStrenght[0].N++;
			}
			else
			if(0.9<Ag.Honor) //Jak nie, to jako HONORowi
			{
			   MnStrenght[1].summ+=Ag.Power;MnStrenght[1].N++;
			}
			else
			if(0.9<Ag.CallPolice) //Jak nie to mo¿e zawsze wzywaj¹cy POLICEje
			{
				MnStrenght[2].summ+=Ag.Power;MnStrenght[2].N++;
			}
			else //W ostatecznoœci zwykli luserzy
			{
             	MnStrenght[3].summ+=Ag.Power;MnStrenght[3].N++;
			}

			switch(Ag.LastDecision(false)){
			case HonorAgent::WITHDRAW: Actions.WITHDRAW++;  break;
			case HonorAgent::GIVEUP:   Actions.GIVEUP++;  break;
			case HonorAgent::HOOK:     Actions.HOOK++; break;
			case HonorAgent::FIGHT:    Actions.FIGHT++; break;
			case HonorAgent::CALLAUTH: Actions.CALLAUTH++;break;
			default:
								Actions.NOTHING++;	 break;
			}
		}
	}

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
		if(REPETITION_LIMIT>1)//Gdy ma zrobiæ wiêcej powtórzeñ tego samego eksperymentu
			OutLog<<"REPET N#"<<'\t'; //Która to kolejna repetycja?

		OutLog<<"MC_STEP"   <<'\t'<<"MeanAgres"<<'\t'<<"MeanHonor"<<'\t'<<"MeanCallPolice"<<'\t'<<"MeanFeiReputation"<<'\t'<<"MeanPower"<<'\t'<<"All killed"<<'\t'<<"NOTHING"<<'\t'<<"WITHDRAW"<<'\t'<<"GIVEUP"<<'\t'<<"HOOK"<<'\t'<<"FIGHT"<<'\t'<<"CALLAUTH";
		//MnStrenght&MnStrNam[NumOfCounters]
		for(unsigned i=0;i<NumOfCounters;i++)
			OutLog<<'\t'<<MnStrNam[i];
		for(unsigned i=0;i<NumOfCounters;i++)
			OutLog<<"\tNof"<<MnStrNam[i];
		OutLog<<endl;

		if(REPETITION_LIMIT>1)//Gdy ma zrobiæ wiêcej powtórzeñ tego samego eksperymentu
			OutLog<<RepetNum<<'\t'; //Która to kolejna repetycja?

		OutLog<<"0.9"<<'\t'<<MeanAgres<<'\t'<<MeanHonor<<'\t'<<MeanCallPolice<<'\t'<<MeanFeiReputation<<'\t'<<MeanPower<<'\t'<<LiczbaTrupow<<'\t'<<Actions.NOTHING<<'\t'<<Actions.WITHDRAW<<'\t'<<Actions.GIVEUP<<'\t'<<Actions.HOOK<<'\t'<<Actions.FIGHT<<'\t'<<Actions.CALLAUTH;
		//MnStrenght&MnStrNam[NumOfCounters]
		for(unsigned i=0;i<NumOfCounters;i++)
			OutLog<<'\t'<<(MnStrenght[i].N>0?MnStrenght[i].summ/MnStrenght[i].N:-9999);
		for(unsigned i=0;i<NumOfCounters;i++)
			OutLog<<'\t'<<MnStrenght[i].N;
		OutLog<<endl;

		if(ConsoleLog)
		{
		  cout<<"STEP"<<setw(4)<<'\t'<<"MnAgres"<<'\t'<<"MnHonor"<<'\t'<<"MnFeiRep"<<'\t'<<"MnCallRep"<<'\t'<<"MnPower";
		  for(unsigned i=0;i<NumOfCounters;i++) cout<<'\t'<<MnStrNam[i];
		  cout<<endl;
		}
		//MnStrenght&MnStrNam[NumOfCounters]
	}
	else
	if(LastStep!=step_counter)
	{
		if(REPETITION_LIMIT>1)//Gdy ma zrobiæ wiêcej powtórzeñ tego samego eksperymentu
			OutLog<<RepetNum<<'\t'; //Która to kolejna repetycja?

		OutLog<<(step_counter>0?step_counter:0.1)<<'\t'<<MeanAgres<<'\t'<<MeanHonor<<'\t'<<MeanCallPolice<<'\t'<<MeanFeiReputation<<'\t'<<MeanPower<<'\t'<<LiczbaTrupow<<'\t'<<Actions.NOTHING<<'\t'<<Actions.WITHDRAW<<'\t'<<Actions.GIVEUP<<'\t'<<Actions.HOOK<<'\t'<<Actions.FIGHT<<'\t'<<Actions.CALLAUTH;
		//MnStrenght&MnStrNam[NumOfCounters]
		for(unsigned i=0;i<NumOfCounters;i++)
			OutLog<<'\t'<<(MnStrenght[i].N>0?MnStrenght[i].summ/MnStrenght[i].N:-9999);
		for(unsigned i=0;i<NumOfCounters;i++)
			OutLog<<'\t'<<MnStrenght[i].N;
		OutLog<<endl;

		if(ConsoleLog)
		{
		  cout<<step_counter<<setw(4)<<setprecision(3)<<'\t'<<MeanAgres<<'\t'<<MeanHonor<<'\t'<<MeanFeiReputation<<'\t'<<MeanCallPolice<<'\t'<<MeanPower;
		  for(unsigned i=0;i<NumOfCounters;i++)
			cout<<'\t'<<setw(4)<<(MnStrenght[i].N>0?MnStrenght[i].summ/MnStrenght[i].N:-1);
		  cout<<endl;
		}

		LastStep=step_counter;//Zapisuje ¿eby nie wypisywaæ podwójnie do logu
	}
}

void Reset_action_memories(wb_dynmatrix<HonorAgent>& World)
{
	for(unsigned v=0;v<SIDE;v++)
	{
		for(unsigned h=0;h<SIDE;h++)
		{
			World[v][h].LastDecision(true);			//Zapamiêtanie referencji do agenta, ¿eby ci¹gle nie liczyæ indeksów
		}
	}
}

void dump_step(wb_dynmatrix<HonorAgent>& World,unsigned step)
{
	const char TAB='\t';
	if(RepetNum==1 && step==0) //W kroku zerowym metryczka i nag³ówek
	{
	   Parameters_dump(Dumps);
	   Dumps<<"Repet"<<TAB<<"step"<<TAB<<"v"<<TAB<<"h"<<TAB;
	   Dumps<<"Ag_ID"<<TAB<<"Power"<<TAB<<"PowLimit"<<TAB
				 <<"FeiReput"<<TAB
				 <<"Agresion"<<TAB
				 <<"Honor"<<TAB
				 <<"CallPolice"<<TAB
				 <<"LastDec"<<TAB
				 <<"NeighSize"<<endl;
	}
	for(unsigned v=0;v<SIDE;v++)
	{
		for(unsigned h=0;h<SIDE;h++)
		{
			Dumps<<RepetNum<<TAB<<step<<TAB<<v<<TAB<<h<<TAB;
			HonorAgent& Ag=World[v][h];	//Zapamiêtanie referencji
			Dumps<<Ag.ID<<TAB<<Ag.Power<<TAB<<Ag.PowLimit<<TAB
				 <<Ag.GetFeiReputation()<<TAB
				 <<Ag.Agres<<TAB
				 <<Ag.Honor<<TAB
				 <<Ag.CallPolice<<TAB
				 <<Ag.LastDecision(false)<<TAB
				 <<Ag.NeighSize()<<endl;
		}
	}
	Dumps<<endl;//Po ca³oœci
}

// WIZUALIZACJA
/////////////////////////////////////////////////////////////////////////////

void replot(wb_dynmatrix<HonorAgent>& World)
{
	int old=mouse_activity(0);
	//clear_screen();
	unsigned spw=screen_width()-SIDE*SSIZ;
	unsigned StartPow=(SIDE+1)*SSIZ+char_height('X')+1;//Gdzie siê zaczyna wizualizacja pomocnicza
	unsigned StartDyn=StartPow+(SIDE+1)*SSIZ+char_height('X')+1;//Gdzie siê zaczyna wizualizacja aktywnoœci

	//double RealMaxReputation=0;
	//double SummReputation=0;
	//bool  VisShorLinks=false; //Wizualizacja bliskich linków
	//bool  VisFarLinks=true;	//Wizualizacja dalekich
	//bool  VisAgents=true;     //Wizualizacja w³aœciwoœci agentów

	//DRUKOWANIE POWI¥ZAÑ
	int VSIZ2=VSIZ/2;

	if(VisFarLinks || VisShorLinks)
	for(unsigned n=0;n<SIDE*SIDE;n++)
	{
		unsigned v=RANDOM(SIDE);
		unsigned h=RANDOM(SIDE);

		HonorAgent& Ag=World[v][h];			//Zapamiêtanie referencji do agenta, ¿eby ci¹gle nie liczyæ indeksów
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
	//DRUKOWANIE DANYCH DLA WÊZ£ÓW
	for(unsigned v=0;v<SIDE;v++)
	{
		for(unsigned h=0;h<SIDE;h++)
		{

			HonorAgent& Ag=World[v][h];			//Zapamiêtanie referencji do agenta, ¿eby ci¹gle nie liczyæ indeksów
			ssh_rgb Color=Ag.GetColor();

			if(VisAgents)
			{
				if(!(VisFarLinks || VisShorLinks))
					fill_rect(h*VSIZ,v*VSIZ,h*VSIZ+VSIZ,v*VSIZ+VSIZ,255+128);

				//set_brush_rgb(Ag.GetFeiReputation()*255,0,Ag.CallPolice*255);
				//set_pen_rgb(  Ag.GetFeiReputation()*255,0,Ag.CallPolice*255,1,SSH_LINE_SOLID);
				set_brush_rgb(Ag.Agres*255,Ag.Honor*255,Ag.CallPolice*255);
				set_pen_rgb(Ag.GetFeiReputation()*255,Ag.GetFeiReputation()*255,0,1,SSH_LINE_SOLID);
				unsigned ASiz=1+/*sqrt*/(Ag.Power)*VSIZ;
				fill_rect_d(h*VSIZ,v*VSIZ,h*VSIZ+ASiz,v*VSIZ+ASiz);
				line_d(h*VSIZ,v*VSIZ+ASiz,h*VSIZ+ASiz,v*VSIZ+ASiz);
				line_d(h*VSIZ+ASiz,v*VSIZ,h*VSIZ+ASiz,v*VSIZ+ASiz);
				//if(Ag.Agres>0 || Ag.Honor>0)
				//	plot_rgb(h*VSIZ+ASiz/2,v*VSIZ+ASiz/2,Ag.Agres*255,Ag.Agres*255,Ag.Honor*255);
				//set_brush_rgb(Ag.Power*Ag.Agres*255,Ag.Power*Ag.Honor*255,0);//Wzpenienie jako kombinacje si³y i sk³onnoœci
				//fill_circle_d(h*VSIZ+VSIZ2,v*VSIZ+VSIZ2,);   // G³ówny rysunek HonorAgenta ...
			}

			//Wizualizacja "Reputacji" w odcieniach szaroœci?
			set_pen_rgb(  Ag.GetFeiReputation()*255,0,Ag.CallPolice*255,1,SSH_LINE_SOLID);
			set_brush_rgb(Ag.GetFeiReputation()*255,0,Ag.CallPolice*255);
			fill_rect_d(spw+h*SSIZ,v*SSIZ,spw+(h+1)*SSIZ,(v+1)*SSIZ);

			//Wizualizacja si³y w odcieniach szaroœci
			ssh_color ColorPow=257+unsigned(Ag.Power*254); //
			fill_rect(spw+h*SSIZ,StartPow+v*SSIZ,spw+(h+1)*SSIZ,StartPow+(v+1)*SSIZ,ColorPow);

			//Dynamika procesów - punkty w ró¿nych kolorach co siê dzie³o w ostatnim kroku
			ssh_color ColorDyn=0;
			switch(Ag.LastDecision(false)){
			case HonorAgent::WITHDRAW:
								ColorDyn=220;  break;
			case HonorAgent::GIVEUP:
								ColorDyn=190;  break;
			case HonorAgent::HOOK:
								ColorDyn=254;  break;
			case HonorAgent::FIGHT:
								ColorDyn=50;  break;
			case HonorAgent::CALLAUTH:
								ColorDyn=128;  break;
			default:
								ColorDyn=0;	 break;
			}
			fill_rect(spw+h*SSIZ,StartDyn+v*SSIZ,spw+(h+1)*SSIZ,StartDyn+(v+1)*SSIZ,ColorDyn);
			unsigned ASiz=1+/*sqrt*/(Ag.Power)*VSIZ;
			plot(h*VSIZ+ASiz/2,v*VSIZ+ASiz/2,ColorDyn);
		}
	}

	printc(1,screen_height()-char_height('X'),100,255,"%u MC %s MnAgres=%f  MnHonor=%f  MnPoli=%f Killed=%u  AllKilled=%u  ",
										step_counter,"COMPONENT VIEW",
									double(MeanAgres),double(MeanHonor),double(MeanCallPolice),LiczbaTrupowDzis,LiczbaTrupow);
	/*double MeanFeiReputation=0;
double MeanCallPolice=0;
double MeanPower=0;*/
	printc(spw,(SIDE+1)*SSIZ,150,255,"%s Mn.Fe=%g Mn.Cp=%g ","Reput.",double(MeanFeiReputation),double(MeanCallPolice));   // HonorAgent::MaxReputation
	printc(spw,StartPow+(SIDE+1)*SSIZ,50,255,"%s mn=%f ","Power",double( MeanPower ));
	printc(spw,StartDyn+(SIDE+1)*SSIZ,50,255,"%s H:%u F:%u C:%u ","Local interactions",Actions.HOOK,Actions.FIGHT,Actions.CALLAUTH);
	//HonorAgent::Max...=Real...;//Aktualizacja max-ów do policzonych przed chwil¹ realnych
	flush_plot();

	save_stat();

	mouse_activity(old);
}

//Pomocnicze metody agentów itp
//////////////////////////////////
HonorAgent::Decision HonorAgent::LastDecision(bool clean)
//Ostatnia decyzja z kroku MC do celów wizualizacyjnych i statystycznych
{
	Decision Tmp=MemOfLastDecision;
	if(clean) MemOfLastDecision=NOTHING;
	return Tmp;
}

unsigned HonorAgent::NeighSize()const
//Ile ma zarejestrowanych s¹siadów
{
	return HowManyNeigh;
}

void HonorAgent::forgetAllNeigh()
//Zapomina wszystkich dodanych s¹siadów, co nie znaczy ¿e oni zapominaj¹ jego
{
   HowManyNeigh=0;
}

bool HonorAgent::getNeigh(unsigned i,unsigned& x,unsigned& y) const
//Czyta wspó³rzedne s¹siada, o ile jest
{
  if(i<HowManyNeigh) //Jest taki
   {
	x=Neighbourhood[i].X;
	y=Neighbourhood[i].Y;
	return true;
   }
   else return false;
}

const HonorAgent::LinkTo* HonorAgent::Neigh(unsigned i)//Wspó³rzêdne kolejnego s¹siada
{
  if(i<HowManyNeigh)
		return &Neighbourhood[i];
		else return NULL;
}

bool HonorAgent::addNeigh(unsigned x,unsigned y)
//Dodaje sasiada o okreslonych wspó³rzêdnych w œwiecie
{
   if(HowManyNeigh<Neighbourhood.get_size()) //Jest jeszcze miejsce
   {
	Neighbourhood[HowManyNeigh].X=x;
	Neighbourhood[HowManyNeigh].Y=y;
	HowManyNeigh++;
	return true;
   }
   else return false;
}


/*  OGOLNA FUNKCJA MAIN I TO CO JEJ POTRZEBNE */
/**********************************************/
void Help();
void SaveScreen(unsigned step);
void mouse_check(wb_dynmatrix<HonorAgent>& World);
void fixed_params_mode();//Tryb interakcyjny z pe³n¹ wizualizacj¹
void walk_params_mode();//Tryb analizy przestrzeni parametrów


int main(int argc,const char* argv[])
{
	cout<<MODELNAME<<" v.:"<<VERSIONNUM<<endl<<
		"==============================="<<endl<<
  //		"(programmed by Wojciech Borkowski from University of Warsaw)\n"
		"        "<<endl
		<<endl;
	cout<<"\nuse -help for command line options for graphics\n"<<endl;

	mouse_activity(1);
	set_background(255);
	buffering_setup(1);// Czy wlaczona animacja
	if(batch_mode) fix_size(0);
		else fix_size(1);
	shell_setup(MODELNAME,argc,argv);

	cout<<"\n MODEL CONFIGURATION: "<<endl;

	Parameters_dump(cout);


	if(!init_plot(SIDE*VSIZ+20+SIDE*SSIZ,SIDE*VSIZ,0,1)) //Na g³ówn¹ wizualizacjê swiata i jakieœ boki
	{
		cerr<<"Can't initialize graphics"<<endl;
		exit(1);
	}

	RANDOMIZE();//Musi byæ chyba ¿e chce siê powtarzalnie debugowaæ!

	if(batch_mode)
		walk_params_mode();//Tryb analizy przestrzeni parametrów
	else
		fixed_params_mode();//Tryb interakcyjny z pe³n¹ wizualizacj¹

	cout<<endl<<"Bye, bye!"<<endl;
	close_plot();
	return 0;
}

void PlotTables(const char* Name1,wb_dynmatrix<FLOAT>& Tab1,
				const char* Name2,wb_dynmatrix<FLOAT>& Tab2,
				const char* Name3,wb_dynmatrix<FLOAT>& Tab3,
				const char* Name4,wb_dynmatrix<FLOAT>& Tab4,bool true_color=false)
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
			line_d(screen_width()-10,i,screen_width(),i);
		}
	 }
	 else
	 {
		 for(unsigned i=0;i<256;i++)
			line(screen_width()-10,i,screen_width(),i,i);
     }

	 for(unsigned Y=0;Y<Tab1.get_size();Y++)
	  for(unsigned X=0;X<Tab1[Y].get_size();X++)
	  {
		  int col1=(Tab1[Y][X]>=0?Tab1[Y][X]*255:-128);
		  int col2=(Tab2[Y][X]>=0?Tab2[Y][X]*255:-128);
		  int col3=(Tab3[Y][X]>=0?Tab3[Y][X]*255:-128);
		  int col4=(Tab4[Y][X]>=0?Tab4[Y][X]*255:-128);

		  if(true_color)
		  {
			if(col1>=0){set_brush_rgb(col1,col1,0);
			fill_rect_d(X*Ws,Y*Hs,(X+1)*Ws,(Y+1)*Hs); }

			if(col2>=0){set_brush_rgb(col2,0,col2);
			fill_rect_d(W+X*Ws,Y*Hs,W+(X+1)*Ws,(Y+1)*Hs);}

			if(col3>=0){set_brush_rgb(0,col3,col3);
			fill_rect_d(X*Ws,H2+Y*Hs,(X+1)*Ws,H2+(Y+1)*Hs);}

			if(col4>=0){set_brush_rgb(0,col4,0);
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

	 print_transparently(1);
	 printbw(0,Tab1.get_size()*Hs,"%s",Name1);
	 printbw(W,Tab1.get_size()*Hs,"%s",Name2);
	 printbw(0,2*H2-char_height('X'),"%s",Name3);
	 printbw(W,2*H2-char_height('X'),"%s",Name4);

	 printc(0,screen_height()-char_height('X'),55,255,
			"POL.EFF:%g-%g s:%g PROP:%g-%g s:%g (B=%g H=%g C=%g) SELECT:%g STEPS:%u NREP:%u ",
						POLICE_EFFIC_MIN,POLICE_EFFIC_MAX,POLICE_EFFIC_STEP,
						PROPORTION_MIN,PROPORTION_MAX,PROPORTION_STEP,
						(BULLI_POPUL<0?-BULLI_POPUL:0),(HONOR_POPUL<0?-HONOR_POPUL:0),(CALLER_POPU<0?-CALLER_POPU:0),
						USED_SELECTION,
						STOP_AFTER,
						REPETITION_LIMIT
						);
	 flush_plot();
}

void Write_tables(ostream o,const char* Name1,wb_dynmatrix<FLOAT>& Tab1,
							const char* Name2,wb_dynmatrix<FLOAT>& Tab2,const char* TAB="\t")
{
	unsigned X=0,Y=0;
	wb_pchar pom(128);
	o<<endl;
	o<<TAB;
	for(FLOAT effic=POLICE_EFFIC_MIN;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
	{
		pom.prn("P.Ef%g",effic);
		o<<pom.get()<<TAB;
	}
	o<<TAB<<TAB;
	for(FLOAT effic=POLICE_EFFIC_MIN;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP)
	{
		pom.prn("P.Ef%g",effic);
		o<<pom.get()<<TAB;
	}
	o<<X<<endl;

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
   o<<TAB<<Name1;for(unsigned i=0;i<X;i++) o<<TAB; o<<TAB<<TAB<<Name2<<endl;
}

void walk_params_mode()
//Tryb analizy przestrzeni parametrów
{
   unsigned X=(POLICE_EFFIC_MAX-POLICE_EFFIC_MIN)/POLICE_EFFIC_STEP + 1;
   if(! (POLICE_EFFIC_MIN+POLICE_EFFIC_STEP*X < POLICE_EFFIC_MAX) ) //Operacja == nie dzia³a poprawnie na float
										X++;
   unsigned Y=(PROPORTION_MAX-PROPORTION_MIN)/PROPORTION_STEP   + 1;
   if(! (PROPORTION_MIN+PROPORTION_STEP*Y < PROPORTION_MAX) )  //Operacja == nie dzia³a poprawnie na float
										Y++;
   cout<<"Batch mode: "<<Y<<" vs. "<<X<<" cells."<<endl;
   Parameters_dump(OutLog);
   OutLog<<"Batch mode: "<<Y<<" vs. "<<X<<" cells."<<endl;

   wb_dynmatrix<FLOAT> MeanPowerOfAgres(Y,X);MeanPowerOfAgres.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfHonor(Y,X);MeanPowerOfHonor.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfPCall(Y,X);MeanPowerOfPCall.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfOther(Y,X);MeanPowerOfOther.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfAgres(Y,X);MeanPropOfAgres.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfHonor(Y,X);MeanPropOfHonor.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfPCall(Y,X);MeanPropOfPCall.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfOther(Y,X);MeanPropOfOther.fill(-9999.0);

   for(FLOAT prop=PROPORTION_MIN,Y=0;prop<=PROPORTION_MAX;prop+=PROPORTION_STEP,Y++)
   {
	   if(BULLI_POPUL>=0)
				BULLI_POPUL=prop;

	   if(Compansation_mode)
	   {                             assert(BULLI_POPUL<0);
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
		   cout<<endl<<"SYM "<<Y<<' '<<X<<" Prop: "<<prop<<"\tPEffic: "<<effic<<endl;
		   POLICE_EFFIC=effic;

		   //PÊTLA POWTÓRZEÑ SYMULACJI
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
			 wb_dynmatrix<HonorAgent> World(SIDE,SIDE);//Pocz¹tek - alokacja agentów œwiata
			 double POPULATION=double(SIDE)*SIDE;   //Ile ich w ogóle jest?
			 InitConnections(World,SIDE*SIDE*OUTFAR_LINKS_PER_AGENT);//Tworzenie sieci
			 InitAtributes(World,SIDE*SIDE); //Losowanie atrybutów dla agentów
			 //CalculateStatistics(World); //Po raz pierwszy dla tych parametrów
			 for(step_counter=1;step_counter<=STOP_AFTER;step_counter++)
			 {
				Reset_action_memories(World);//Czyszczenie, mo¿e niepotrzebne
				power_recovery_step(World); // Krok procesu regeneracji si³
				one_step(World); // Krok dynamiki interakcji agresywnych
				if (SOCIAL_IMPACT_INTENSITY_PERCENT > 0)
					// Opcjonalnie krok wp³ywu spo³ecznego
						social_impact_step(World,
						SOCIAL_IMPACT_INTENSITY_PERCENT);

				if (step_counter % EveryStep == 0)
				{
					CalculateStatistics(World);
					cout <<"\r["<<rep<<"] "<< step_counter<<"  ";
					MnPowOfAgres += MnStrenght[0].N > 0 ? MnStrenght[0].summ / MnStrenght[0].N : 0;
					MnPowOfHonor += MnStrenght[1].N > 0 ? MnStrenght[1].summ / MnStrenght[1].N : 0;
					MnPowOfPCall += MnStrenght[2].N > 0 ? MnStrenght[2].summ / MnStrenght[2].N : 0;
					MnPowOfOther += MnStrenght[3].N > 0 ? MnStrenght[3].summ / MnStrenght[3].N : 0;
					MnPropOfAgres += MnStrenght[0].N / POPULATION;
					MnPropOfHonor += MnStrenght[1].N / POPULATION;
					MnPropOfPCall += MnStrenght[2].N / POPULATION;
					MnPropOfOther += MnStrenght[3].N / POPULATION;
					StatSteps++;
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
				} // KONIEC PÊTLI SYMULACJI
				DeleteAllConnections(World);  //Koniec tej symulacji
			} // KONIEC PÊTLI POWTORZEN

			// PODLICZENIE WYNIKÓW I ZAPAMIÊTANIE W TABLICACH PRZESTRZENI PARAMETRÓW
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
   //Zrobienie u¿ytku z wyników
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
   while(1)
   {
		int znak=get_char();
		if(znak==-1 || znak==27) break;
   }
}

void fixed_params_mode()
//Tryb interakcyjny z pe³n¹ wizualizacj¹
{
	wb_dynmatrix<HonorAgent> World(SIDE,SIDE);//Tablica agentów œwiata

	InitConnections(World,SIDE*SIDE*OUTFAR_LINKS_PER_AGENT);
	InitAtributes(World,SIDE*SIDE);
	dump_step(World,0);//Po raz pierwszy
	CalculateStatistics(World); //Po raz pierwszy

	//Prowizoryczna pêtla g³ówna
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
			case 'n':runs=0;one_step(World);CalculateStatistics(World);replot(World);break;
			case 'p':runs=0;replot(World);break;
			case 'r':runs=1;break;

			case 'c':ConsoleLog=!ConsoleLog;break;
			case 's':VisShorLinks=!VisShorLinks;clear_screen();replot(World);break; //Wizualizacja bliskich linków
			case 'f':VisFarLinks=!VisFarLinks;clear_screen();replot(World);break;	  //Wizualizacja dalekich
			case 'a':VisAgents=!VisAgents;clear_screen();replot(World);break;     //Wizualizacja w³aœciwoœci agentów
			case 'v'://if(CalculateColorDefault==  //Wybranie typu wizualizacji
					replot(World);break;

			case '\b':mouse_check(World);break;
			case '@':
			case '\r':clear_screen();
			case '\n':replot(World);if(ConsoleLog)cout<<endl<<endl;break;

			case 'q':
			case EOF:cont=0;break;
			case 'h':
			default:
					Help();break;
			}
			flush_plot();
		}
		else
			if(runs)
			{
				Reset_action_memories(World);

				//Krok procesu regeneracji si³
				power_recovery_step(World);

				//Krok dynamiki interakcji agresywnych
				one_step(World);

				//Opcjonalnie krok wp³ywu spo³ecznego
				if(SOCIAL_IMPACT_INTENSITY_PERCENT>0)
					social_impact_step(World,SOCIAL_IMPACT_INTENSITY_PERCENT);

				if(step_counter%EveryStep==0)
				{
					CalculateStatistics(World);
					replot(World);

					if(dump_screens)
					{
						SaveScreen(step_counter);
						cout<<"\nScreen for step "<<step_counter<<" dumped."<<endl;
					}

					if(step_counter>=STOP_AFTER)
					{
						if(RepetNum<REPETITION_LIMIT )//Kolejna repetycja?
						{
							dump_step(World,step_counter);//Po raz ostatni
							RepetNum++;
							cout<<"\nStop after "<<STOP_AFTER<<" steps\a, and start iteration n# "<<RepetNum<<endl;
							Reset_action_memories(World);
							DeleteAllConnections(World);
							InitConnections(World,SIDE*SIDE*OUTFAR_LINKS_PER_AGENT);
							InitAtributes(World,SIDE*SIDE);
							step_counter=0;
							LiczbaTrupow=0; //Trochê to partyzantka
							LiczbaTrupowDzis=0;
							CalculateStatistics(World); //Po raz pierwszy dla tej iteracji
                            OutLog<<"next"<<endl;
							save_stat();
							dump_step(World,0);//Po raz pierwszy
						}
						else
						{
							dump_step(World,step_counter);//Po raz ostatni
							cout<<"\nStop because of limit "<<STOP_AFTER<<" steps\a\a"<<endl;
							runs=false;
						}
					}
					else
					{
					   if(step_counter%DumpStep==0)//10-100x rzadziej ni¿ statystyka
					   {
							dump_step(World,step_counter);//Co jakiœ czas
                       }
					}
				}
			}
	}
}

void Help()
{
	cout<<"POSSIBLE COMMANDS FOR GRAPHIC WINDOW:"<<endl
		<<"q-quit or ESC\n"
		"\n"
		"n-next MC step\n"
		"p-pause simulation\n"
		"r-run simulation\n"
		"\n"
		"b-save screen to BMP\n"
		"d-dump on every visualisation\n"
		"\n"
		"1..0 visualisation freq.\n"
		"c-swich console on/off\n"
		"s - visualise short links\n"
		"f - visualise far links\n"
		"a - visualise agents\n"
		"\n"
		"mouse left or right - \n"
		"     inspection\n"
		"ENTER - replot screen\n";
}

void PrintHonorAgentInfo(ostream& o,const HonorAgent& H)
{
	//PrettyPrint(o,H.Pattern);o<<endl;	//Wzorzec bitowy - geny lub memy kultury - mo¿e póŸniej
	o<<"Individual color: rgb("<<unsigned(H.GetColor().r)<<','<<unsigned(H.GetColor().g)<<','<<unsigned(H.GetColor().b)<<')'<<endl;	//Obliczony, któr¹œ z funkcji koduj¹cych kolor
	o<<"Curr. strenght: "<<H.Power<<" streght limit: "<<H.PowLimit<<endl;//	Si³a (0..1) i jak¹ si³ê mo¿e osi¹gn¹æ maksymalnie, gdy nie traci
	o<<"Agression prob.: "<<H.Agres<<endl;// Bulizm (0..1) sk³onnoœæ do atakowania
	o<<"Feighter reputation: "<<H.GetFeiReputation()<<endl;//Reputacja wojownika jako „wêdrowna œrednia” z konfrontacji (0..1)
	o<<"Honor: "<<H.Honor<<endl;// Bezwarunkowa honorowoœæ (0..1) sk³onnoœæ podjêcia obrony
	o<<"Police call.: "<<H.CallPolice<<endl;//Odium wzywacza policji - prawdopodobieñstwo wzywania policji (0..1) jako „wêdrowna œrednia” wezwañ
	o<<endl;
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
		HonorAgent& Ag=World[ypos][xpos];	//Zapamiêtanie referencji do agenta, ¿eby ci¹gle nie liczyæ indeksów

		cout<<"INSPECTION OF HonorAgent: "<<xpos<<' '<<ypos<<endl;
		PrintHonorAgentInfo(cout,Ag);
		if(click==1)
		{

		}
		else
			if(click==2)
			{


			}
	}
}


void SaveScreen(unsigned step)
{
	wb_pchar Filename;
	Filename.alloc(128);
	Filename.prn("%u_honorwrld",step);
	dump_screen(Filename.get_ptr_val());
}
