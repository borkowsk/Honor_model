//  Agent do modelu kultury honoru
//      IMPLEMENTACJE
////////////////////////////////////////////////////////////////////////////////
#define _USE_MATH_DEFINES //bo M_PI
#include <cmath>
#include <iostream>
#include <fstream>
#include <iomanip>
#define HIDE_WB_PTR_IO 1
#include "INCLUDE/wb_ptr.hpp"
#include "SYMSHELL/symshell.h"

#include "HonorAgent.hpp"

using namespace wbrtm;
using namespace std;

bool 	HonorAgent::CzyTorus=false; //Czy geometria torusa czy wyspy z brzegami

unsigned HonorAgent::licznik_zyc=0;//Do tworzenia unikalnych identyfikatorów agentów

wb_dynmatrix<HonorAgent> HonorAgent::World;//Ca³y œwiat agentów - wspólny

//Pomocnicze metody i akcesory agentów
////////////////////////////////////////////////////////////////////////////////
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

const HonorAgent::LinkTo* HonorAgent::Neigh(unsigned i)
//Wspó³rzêdne kolejnego s¹siada
{
  if(i<HowManyNeigh)
		return &Neighbourhood[i];
		else return NULL;
}

bool HonorAgent::addNeigh(unsigned x,unsigned y)
//Dodaje sasiada o okreslonych wspó³rzêdnych w œwiecie
{
   if(HowManyNeigh<Neighbourhood.get_size()) //Czy jest jeszcze miejsce wokó³?
   {
	Neighbourhood[HowManyNeigh].X=x;
	Neighbourhood[HowManyNeigh].Y=y;
	HowManyNeigh++;
	return true;
   }
   else return false;
}

void HonorAgent::RandomReset()
//Losowanie wartoœci atrubutów
{
		this->ID=++licznik_zyc;
		this->HisActions.Reset();
		this->HisLifeTime=0;

		PowLimit=(DRAND()+DRAND()+DRAND()+DRAND()+DRAND()+DRAND())/6;  // Jak¹ si³ê mo¿e osi¹gn¹æ maksymalnie, gdy nie traci
		Power=(0.5+DRAND()*0.5)*PowLimit; //  zawsze przynajmniej 0.5 limitu si³y
		HonorFeiRep=Power;//DRAND()*Power, a kiedyœ by³o te¿ DRAND()*0.5 te¿. Potem uproszczone do Power;  //Reputacja wojownika wygl¹du i z realnych konfrontacji
													assert(0<=HonorFeiRep);
													assert(HonorFeiRep<=1);
/*
		// Wartoœci w Agres, Honor, CallPolice s¹ prawdopodobieñstwami innymi ni¿ 0 i 1 - WARIANT PORZUCONY NA RAZIE (?)
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
		//Agres, Honor, CallPolice alternatywnie, ale czli jeden z prawdopodobieñstwem 1 a reszta 0.   WA¯NE UPROSZCZENIE!
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
																				assert(fabs(BULLI_POPUL)<1);
		if(fabs(BULLI_POPUL)<1) // Oczywiœcie ma sens tylko wtedy  - jak 1 to sami agresywni - ALE NIGDYTAK NIE ROBIMY!
		{
		   Honor=0;CallPolice=0;//Wartoœci domyœlne dla agresywnych i wzywaj¹cych policje

		   Agres=(DRAND()<fabs(BULLI_POPUL)?1:0); //Albo jest AGRESYWNY albo nie jest
		   if(Agres!=1) //Jak nie jest
		   {
			 Honor=(DRAND()*(1-fabs(BULLI_POPUL)) < fabs(HONOR_POPUL)? 1 : 0 ); //Jest HONOROWY albo nie jest
			 if(Honor!=1)   //NIE AGRESYWNY i NIE HONOROWY
			 {              //policyjny albo racjonalny - mog³oby byæ CallPolice=1 i w ten sposób znikaj¹ RACJONALNI (czwarta strategia)
			   CallPolice=(DRAND()*(1-fabs(BULLI_POPUL)-fabs(HONOR_POPUL)) < fabs(CALLER_POPU)? 1 : 0); //Jest albo nie ma
			   if(ONLY3STRAT && CallPolice==0) //Racjonalni "zabijani"!
					this->Power=0;
			 }
			 else
			 {
			   /* NIE LOOSER tylko HONOROWY */    //cerr<<"*";
			   //cerr<<"?"; //  Kiedy tu trafia? - jak Honorowy! Niepotrzebne, ale nie szkodzi
			 }
		   }
		}
}

bool      HonorAgent::firstWin(HonorAgent& In,HonorAgent& Ho)
//Ustala czy pierwszy czy drugi agent zwyciê¿y³ w konfrontacji
//Zawsze któryœ musi zwyciê¿yæ, co nie jest realistyczne - WA¯NE UPROSZCZENIE
{
	if( In.Power*(1+DRAND())  >  Ho.Power*(1+DRAND()) )
			return true;
			else
			return false;
}

bool    HonorAgent::IsMyFamilyMember(HonorAgent& Inny,HonorAgent*& Cappo,int MaxLevel)//=2
//Czy ten "Inny" nale¿y do "rodziny" danego agenta
//Przy okazji sprawdzenia ustalamy "Ojca chrzestnego" (Cappo) na poŸniej
{
	if(MaxLevel>0)//Jak jeszcze nie szczyt zadanej hierarchii, to szuka ojca
	 for(unsigned i=0;i<NeighSize();i++)
	  if(Neighbourhood[i].Parent==1)//Jeœli ma ojca to idzie w górê
	  {
		 HonorAgent& Rodzic=World[Neighbourhood[i].Y][Neighbourhood[i].X];
		 Cappo=&Rodzic;//Mo¿e "Rodzic" to zmieni, ale jak nie to juz znaleziony
		 return  Rodzic.IsMyFamilyMember(Inny,Cappo,MaxLevel-1);  //REKURENCJA!
	  }

	//Jak ju¿ nie poszed³ z tego czy innego powodu w górê to mo¿e szukaæ
	if(this==&Inny) //On sam okaza³ siê szukanym
			return true;
	for(unsigned j=0;j<NeighSize();j++)//Jest najwy¿szym ojcem, teraz mo¿e szukaæ potomków
	 if(Neighbourhood[j].Child==1) //Jest dzieckiem
	 {
		HonorAgent& Dziecko=World[Neighbourhood[j].Y][Neighbourhood[j].X];
		bool res=Dziecko.IsMyFamilyMember(Inny,Cappo,-1);  //-1 zabezpiecza przed REKURENCJA
		if(res)
			return true;
	 }

	//Nigdzie nie znalaz³ - znaczy w jego pod-drzewie nie ma
	return false;
}

void    HonorAgent::change_reputation_thru_family(double Delta)
// Osobista zmiana reputacji, oraz ewentualnie rodzinna rekurencyjnie
{
	if(Delta>0) //Wzrost
	{
		HonorFeiRep+=(1-HonorFeiRep)*Delta;       	assert(HonorFeiRep<=1);
	}
	else       //Spadek
	{
		HonorFeiRep-=HonorFeiRep*fabs(Delta);       assert(0<=HonorFeiRep);
													assert(HonorFeiRep<=1);
	}
	//A teraz dla dzieci
	for(unsigned j=0;j<NeighSize();j++)//Czy jest mo¿e ojcem? Trzeba szukaæ potomków
	 if(Neighbourhood[j].Child==1) //Jest dzieckiem
	 {
		HonorAgent& Dziecko=World[Neighbourhood[j].Y][Neighbourhood[j].X];
		Dziecko.change_reputation_thru_family(Delta); //REKURENCJA W DÓ£ RODZINY
	 }
}

void    HonorAgent::change_reputation(double Delta,HonorAgent& Powod,int level)//=0
//Wzrost lub spadek reputacji z zabezpieczeniem zakresu
//Jak MAFIAHONOR i goœæ jest honorowy to zmienia siê tak samo ca³ej rodzinie!
{                                                   assert(this!=&Powod);//Sam siebie nie mo¿e atakowaæ!
													assert(0<=HonorFeiRep);
													assert(HonorFeiRep<=1);
	HonorAgent* Cappo=NULL;//Przy okazji sprawdzenia czy ktoœ jest w rodzinie ustalamy "Ojca chrestnego".
	if(&Powod!=NULL && MAFIAHONOR && !IsMyFamilyMember(Powod,Cappo) )//Je¿eli dzia³a honor rodzinny to ...
	{
		if(Cappo==NULL)//Nie ma ¿yj¹cego ojca - œlad siê urywa
				Cappo=this;
		Cappo->change_reputation_thru_family(Delta);//Ale mo¿e mieæ dzieci
	}
	else
	if(Delta>0) //Wzrost
	{
		HonorFeiRep+=(1-HonorFeiRep)*Delta;       	assert(HonorFeiRep<=1);
	}
	else       //Spadek
	{
		HonorFeiRep-=HonorFeiRep*fabs(Delta);       assert(0<=HonorFeiRep);
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

HonorAgent::Decision  HonorAgent::check_partner(unsigned& x,unsigned& y)
//Wybór partnera interakcji przez agenta, który dosta³ losow¹ inicjatywê
{
	this->MemOfLastDecision=WITHDRAW; //DOMYŒLNA DECYZJA

	unsigned L=RANDOM(HowManyNeigh);//Ustalenie który s¹siad
	if(getNeigh(L,x,y) ) //Pobranie wspó³rzêdnych s¹siada
	{
		HonorAgent& Ag=World[y][x];	//Zapamiêtanie referencji do s¹siada

		//BARDZO PROSTA REGU£A DECYZYJNA - gdy wygl¹da, ¿e warto atakowaæ i jest chêæ    RULE
		if(this->Agres>0.0 && DRAND()<this->Agres   //Agresywnoœæ jako  jako wyrachowanie
		&& Ag.HonorFeiRep<=(RATIONALITY*this->Power+(1-RATIONALITY)*HonorFeiRep)  //Gdy umownie S£ABSZY! Wiêc agresywni nigdy ze sob¹ nie walcz¹!     RULE
		)
		{
			this->MemOfLastDecision=HOOK;//Nieprzypadkowa zaczepka bulliego
		}
		else if(
		//DRAND()<RANDOM_AGRESSION)					//+losowe przypadki burd po pijaku - PODOBNO ZACIEMNIA
		(Honor>0 && DRAND()<Honor*RANDOM_AGRESSION) //Wiêc mo¿e tylko honorowi maj¹ niezer¹ agresjê z powodu nieporozumieñ?
		)
		{
			this->MemOfLastDecision=HOOK;//Ewentualna zaczepka losowa lub honorowa
		}
	}
	this->HisActions.Count(this->MemOfLastDecision);
	return this->MemOfLastDecision;
}

HonorAgent::Decision  HonorAgent::answer_if_hooked(unsigned x,unsigned y)
//OdpowiedŸ na zaczepkê
{
	 this->MemOfLastDecision=GIVEUP;//DOMYŒLNA DECYZJA

	 HonorAgent& Ag=World[y][x];	//Zapamiêtanie referencji do s¹siada

	 //REGU£A ZALE¯NA WYCHOWANIA POLICYJNEGO
	 //LUB OD HONORU i W£ASNEGO POCZUCIA SI£Y
	 if(this->CallPolice>0.999999
	 || (this->CallPolice>0 && DRAND()<this->CallPolice)
	 )
	 {
		this->MemOfLastDecision=CALLAUTH;
	 }
	 else
	 if(this->Honor>0.999999
	 || (this->Honor>0 && DRAND()<this->Honor)
	 || Ag.HonorFeiRep<this->HonorFeiRep
//	 || Ag.HonorFeiRep<this->Power /*HonorFeiRep*/)   //Wersja 2.60
	 )
	 {
		this->MemOfLastDecision=FIGHT;
	 }

	 this->HisActions.Count(this->MemOfLastDecision);
	 return this->MemOfLastDecision;
}

void InitAtributes(FLOAT HowMany)
{
	for(int y=0;y<SIDE;y++)
	  for(int x=0;x<SIDE;x++)
		HonorAgent::World[y][x].RandomReset();
}


void InitConnections(FLOAT HowManyFar)
{
	//Po³¹czenia z najbli¿szymi s¹siadami
	for(int x=0;x<SIDE;x++)
		for(int y=0;y<SIDE;y++)
		{
		   HonorAgent& Ag=HonorAgent::World[y][x];	//Zapamiêtanie referencji do agenta, ¿eby ci¹gle nie liczyæ indeksów
		   for(int xx=x-MOORE_RAD;xx<=x+MOORE_RAD;xx++)
			  for(int yy=y-MOORE_RAD;yy<=y+MOORE_RAD;yy++)
			  if(!(xx==x && yy==y))//Wyciêcie samego siebie
			  {
				 if(HonorAgent::CzyTorus)
				   Ag.addNeigh((xx+SIDE)%SIDE,(yy+SIDE)%SIDE);//Zamkniête w torus
				   else
				   if(0<=xx && xx<SIDE && 0<=yy && yy<SIDE)
					 Ag.addNeigh(xx,yy);//bez boków
			  }
		}

	//Dalekie po³¹czenia musz¹ byæ tworzone po lokalnych ¿eby siê nie dublowa³y
	for(int f=0;f<HowManyFar;f++)
	{
		unsigned x1,y1,x2,y2;

		//Poszukanie agentów z wolnymi slotami - inicjuj¹cych
		do{
		x1=RANDOM(SIDE);
		y1=RANDOM(SIDE);
		}while(HonorAgent::World[y1][x1].NeighSize()>=MAX_LINKS);

		do{//"Bierny" odbiorca dalekiego po³¹czenia nie mo¿e byæ tym samym agentem ani bliskim s¹siadem
		x2=RANDOM(SIDE);
		y2=RANDOM(SIDE);
		}while((x1==x2 && y1==y2)							//Nie sam na siebie
		|| HonorAgent::World[y2][x2].NeighSize()>=MAX_LINKS //Musi miec wolny slot
		|| HonorAgent::AreNeigh(x1,y1,x2,y2)); //UWAGA! KTOŒ MO¯E BYÆ DWA RAZY NA LIŒCIE!

		//Po³¹czenie ich linkami w obie strony
		HonorAgent::World[y1][x1].addNeigh(x2,y2);
		HonorAgent::World[y2][x2].addNeigh(x1,y1);
	}
}

void DeleteAllConnections()
//Usuwanie po³¹czenia z najbli¿szymi s¹siadami
{
	for(int x=0;x<SIDE;x++)
		for(int y=0;y<SIDE;y++)
		{
		   HonorAgent& Ag=HonorAgent::World[y][x];	//Zapamiêtanie referencji do agenta, ¿eby ci¹gle nie liczyæ indeksów
		   Ag.forgetAllNeigh(); //Bezwarunkowe zapomnienie
		}
}

void one_step(unsigned long& step_counter)
//G³ówna dynamika kontaktów
{
	unsigned N=(SIDE*SIDE)/2;//Ile losowañ w kroku MC? Po³owa, bo w ka¿dej interakcji dwaj
	for(unsigned i=0;i<N;i++)
	{
		unsigned x1=RANDOM(SIDE);
		unsigned y1=RANDOM(SIDE);
		HonorAgent& AgI=HonorAgent::World[y1][x1];  //Zapamiêtanie referencji do agenta inicjuj¹cego
		unsigned x2,y2;
		HonorAgent::Decision Dec1=AgI.check_partner(x2,y2);
		if(Dec1==HonorAgent::HOOK) //Jeœli zaczepi³
		{                                					assert(AgI.Agres>0 || AgI.Honor>0);
		   HonorAgent& AgH=HonorAgent::World[y2][x2];//Zapamiêtanie referencji do agenta zaczepionego
		   AgI.change_reputation(+0.05,AgH); // Od razu dostaje powzy¿szenie reputacji za samo wyzwanie
		   HonorAgent::Decision Dec2=AgH.answer_if_hooked(x1,y1);
		   switch(Dec2){
		   case HonorAgent::FIGHT:
					   if(HonorAgent::firstWin(AgI,AgH))
					   {
						  AgI.change_reputation(+0.35,AgH);//Zyska³ bo wygra³
						  AgI.HisActions.Succeses++;
						  AgI.lost_power(-0.75*DRAND());//Straci³ bo walczy³

						  AgH.change_reputation(+0.1,AgI); //Zyska³ bo stan¹³
						  AgH.lost_power(-0.95*DRAND());//Straci³ bo przegra³ walkê
						  AgH.HisActions.Fails++;
					   }
					   else
					   {
						  AgI.change_reputation(-0.35,AgH);//Straci³ bo zaczepi³ i dosta³ bêcki
						  AgI.HisActions.Fails++;
						  AgI.lost_power(-0.95*DRAND()); //Straci³ bo przegra³ walkê


						  AgH.change_reputation(+0.75,AgI);//Zyska³ bo wygra³ choæ by³ zaczepiony
						  AgH.HisActions.Succeses++;
						  AgH.lost_power(-0.75*DRAND());	//Straci³ bo walczy³

					   }
					break;
		   case HonorAgent::GIVEUP:
					   {
						  AgI.change_reputation(+0.5,AgH); //Zyska³ bo wygra³ bez walki. Na pewno wiêcej ni¿ w walce???
						  AgI.HisActions.Succeses++;

						  AgH.change_reputation(-0.5,AgI); //Straci³ bo siê podda³
						  AgH.HisActions.Fails++;
						  AgH.lost_power(-0.5*DRAND()/*DRAND()*/);//I trochê dosta³ ³omot dla przyk³adu   (!!!)
					   }
					break;
		   case HonorAgent::CALLAUTH:
					  if(DRAND()<POLICE_EFFIC) //Czy przyby³a
					  {
						  AgI.change_reputation(-0.35,AgH);//Straci³ bo zaczepi³ i dosta³ bêcki od policji
						  AgI.HisActions.Fails++;
						  AgI.lost_power(-0.99*DRAND()); //Straci³ bo walczy³ i przegra³ z przewa¿aj¹c¹ si³¹

						  AgH.HisActions.Succeses++; //Zaczepionemu uda³o siê unikn¹æ bêcek, ale...
						  AgH.change_reputation(-0.75,AgI);//A  straci³ reputacje bo wezwa³ policje zamiast walczyæ
					  }
					  else //A mo¿e nie
					  {
						  AgI.change_reputation(+0.5,AgH);//Zyska³ bo wygra³ bez walki. A¿ tyle?
						  AgI.HisActions.Succeses++;

						  AgH.change_reputation(-0.75,AgI);//Zaczepiony straci³ bo wezwa³ policje
						  AgH.lost_power(-0.99*DRAND()); //Straci³ bo siê nie broni³, a wkurzy³ wzywaj¹c
						  AgH.HisActions.Fails++;
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
			AgI.change_reputation(-0.0001,*(HonorAgent*)(NULL));//Minimalnie traci w swoich oczach
		}
	}

	step_counter++;
}

unsigned LiczbaTrupow=0;
unsigned LiczbaTrupowDzis=0;

void power_recovery_step()
{
  LiczbaTrupowDzis=0;
  for(int x=0;x<SIDE;x++)
	for(int y=0;y<SIDE;y++)
	{
		HonorAgent& Ag=HonorAgent::World[y][x];	//Zapamiêtanie referencji do agenta, ¿eby ci¹gle nie liczyæ indeksów

		if(Ag.LastDecision(false)==HonorAgent::NOTHING)//Nie by³ w tym kroku ani razu wylosowany!
				Ag.HisActions.Count(HonorAgent::NOTHING);//To mu trzeba doliczyæ do liczników, bo inaczej NOTHING nie bêdzie tam nigdy rejestrowane!

		Ag.HisLifeTime++;  //Zwiêkszenie licznika ¿ycia

		if(MORTALITY>0)   //Losowa œmieræ z horób, staroœci i wypadków
			{
			   if(DRAND()<MORTALITY)
				   Ag.Power=-1;
			}

		if(USED_SELECTION>=0 && Ag.Power<USED_SELECTION)  //Chyba nie przezy³, selekcja 0 oznacza œmieræ tylko od wypadków
		{
		   if(population_growth==0) //Tryb z prawdopodobienstwami inicjalnymi
		   {       																assert(MAFIAHONOR==false);
			Ag.RandomReset();
			LiczbaTrupow++;
			LiczbaTrupowDzis++;
		   }
		   else
		   if(population_growth==1) //Tryb z losowym s¹siadem
		   {
			unsigned ktory=RANDOM(Ag.NeighSize()),xx,yy;
			bool pom=Ag.getNeigh(ktory,xx,yy);
			HonorAgent& Rodzic=HonorAgent::World[yy][xx];
			if(Rodzic.Power>0) //Tylko wtedy mo¿e siê rodziæ! NEW TODO Check  - JAK NIE TO ROZLICZNIE NA PÓNIEJ
			{
			 if(MAFIAHONOR)//Je¿eli s¹ stosunki rodzinne to œmieræ ma ró¿ne konsekwencje
				Ag.SmiercDona();

			 Ag.RandomReset();
			 Ag.Agres=Rodzic.Agres;
			 Ag.Honor=Rodzic.Honor;
			 Ag.CallPolice=Rodzic.CallPolice;

			 if(MAFIAHONOR)//I urodziny tak¿e
			 {
				Ag.HonorFeiRep=Rodzic.HonorFeiRep;//Ma reputacje rodzica, bo on go chroni
				PowiazRodzicielsko(Rodzic,Ag);  //anty SmiercDona();
			 }
			 LiczbaTrupow++;
			 LiczbaTrupowDzis++;
			}
		   }
		   else
		   if(population_growth==3) //Tryb z losowym cz³onkiem populacji
		   {                                                                    assert(MAFIAHONOR==false);
			unsigned xx=RANDOM(SIDE),yy=RANDOM(SIDE);
			HonorAgent& Drugi=HonorAgent::World[yy][xx];
			if(Drugi.Power>0)  //Mo¿e postaæ do rozliczenia na póŸniej
			{
			 Ag.RandomReset();
			 Ag.Agres=Drugi.Agres;
			 Ag.Honor=Drugi.Honor;
			 Ag.CallPolice=Drugi.CallPolice;
			 LiczbaTrupow++;
			 LiczbaTrupowDzis++;
			}
		   }
		}
		else
		if(Ag.Power<Ag.PowLimit)//Mo¿e siê leczyæ lub "poprawiaæ"
		{
			Ag.Power+=(Ag.PowLimit-Ag.Power)*RECOVERY_POWER;
		}
	}

  //Losowa wymiana na przypadkowo narodzone (z domyœlnej proporcji)
  if(EXTERNAL_REPLACE>0)
  {
	unsigned WypadkiLosow=SIDE*EXTERNAL_REPLACE*SIDE;
	for(;WypadkiLosow>0;WypadkiLosow--)
	{
	  unsigned x=RANDOM(SIDE),y=RANDOM(SIDE);
	  HonorAgent& Ag=HonorAgent::World[y][x];	//Zapamiêtanie referencji
	  Ag.RandomReset();  //Wg. inicjalnej dystrybucji - mo¿e przywracac do istnienia ju¿ wymar³e strategie
	}
  }
}

void Reset_action_memories()
//Czyszczenie pamiêci zachowañ
{
	for(unsigned v=0;v<SIDE;v++)
	{
		for(unsigned h=0;h<SIDE;h++)
		{
			HonorAgent::World[v][h].LastDecision(true);			//Zapamiêtanie referencji do agenta, ¿eby ci¹gle nie liczyæ indeksów
		}
	}
}


