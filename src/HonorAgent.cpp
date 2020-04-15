//  Agent for"Culture of Honor" 
//  IMPLEMENTACJE METOD
////////////////////////////////////////////////////////////////////////////////
#define _USE_MATH_DEFINES //--> M_PI use
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

unsigned HonorRandomAttack=0;
unsigned AgressRandomAttack=0;

bool      HonorAgent::firstWin(HonorAgent& In,HonorAgent& Ho)
//Ustala czy pierwszy czy drugi agent zwycięży w konfrontacji
//WAŻNE UPROSZCZENIE: Zawsze któryś musi zwyciężyć, co nie jest realistyczne - 
{
	if( In.Power*(1+DRAND())  >  Ho.Power*(1+DRAND()) ) //DRAND() --> random number from 0 to 1
			return true;
			else
			return false;
}

void      HonorAgent::lost_power(double delta)
//Spadek siły z zabezpieczeniem zakresu
{                                  					assert(Power<=1);
	 delta=fabs(delta);
	 Power*=(1-delta);             					
	 if(Power<0)
			Power=0;     //To be sure...
}

void    HonorAgent::change_reputation(double Delta,HonorAgent& Powod,int level)//level==0
//Wzrost lub spadek reputacji z zabezpieczeniem zakresu
//Jak MAFIAHONOR i gość jest honorowy to zmienia się tak samo całej rodzinie!
{                                                   assert(this!=&Powod);//Sam siebie nie może atakować!
													assert(0<=HonorFeiRep);
													assert(HonorFeiRep<=1);
#ifdef HONOR_WITHOUT_REPUTATION //For "null hipothesis" tests
	 if(this->Honor==1) //Honor agent
	 {
		return; //nothing to do
	 }
#endif
	HonorAgent* Cappo=NULL;//Przy okazji sprawdzenia czy ktoś jest w rodzinie ustalamy "Ojca chrzestnego".
	if(&Powod!=NULL && MAFIAHONOR && !IsMyFamilyMember(Powod,Cappo) )//Jeżeli działa honor rodzinny to ...
	{
		if(Cappo==NULL)//Nie ma żyjącego ojca - ślad się urywa
				Cappo=this;
		Cappo->change_reputation_thru_family(Delta);//Ale może mieć dzieci
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



void HonorAgent::RandomReset(float iPOWLIMIT)//def iPOWLIMIT==0
//Losowanie wartosci atrubutow
{
		this->ID=++licznik_zyc;
		this->HisActions.Reset();
		this->HisLifeTime=0;

		if(iPOWLIMIT>0)
			PowLimit=iPOWLIMIT;//Ustawia jaką siłę może osiągnąć maksymalnie, gdy nie traci
		else
			PowLimit=(DRAND()+DRAND()+DRAND()+DRAND()+DRAND()+DRAND())/6;  //Losuje jaką siłę może osiągnąć maksymalnie

		Power=(0.5+DRAND()*0.5)*PowLimit; //Zawsze przynajmniej 0.5 limitu siły
		HonorFeiRep=Power; //Reputacja wojownika - potem z realnych konfrontacji
						   //Inicjalizacja: DRAND()*Power, a kiedyś było DRAND()*0.5 też. Potem uproszczone do Power;  
																				assert(0<=HonorFeiRep);
																				assert(HonorFeiRep<=1);

		//Agres, Honor, CallPolice INICJOWANE alternatywnie, czyli jeden z prawdopodobieństwem 1 a reszta 0.   POWAŻNE UPROSZCZENIE!
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		Honor=0;CallPolice=0;//Wartości domyślne
																				assert(fabs(BULLI_POPUL)<1);
		if(fabs(BULLI_POPUL)<1) //Ma sens tylko wtedy jak 1 to sami agresywni - ALE NIGDY TAK NIE ROBILISMY!
		{
		   Agres=(DRAND()<fabs(BULLI_POPUL)?1:0); //Albo jest AGRESYWNY albo nie jest
		   
		   if(Agres!=1) //Jak nie jest
		   {
			 Honor=(DRAND()*(1-fabs(BULLI_POPUL)) < fabs(HONOR_POPUL)? 1 : 0 ); //Jest HONOROWY albo nie jest
			 
			 if(Honor!=1)   //NIE AGRESYWNY i NIE HONOROWY
			 {              //policyjny albo racjonalny 
			   if(!ONLY3STRAT)//jeśli TRUE to znikają RACJONALNI (czyli "strategia resztowa")
					CallPolice=(DRAND()*(1-fabs(BULLI_POPUL)-fabs(HONOR_POPUL)) < fabs(CALLER_POPU)? 1 : 0); //Jest albo nie jest 
			   else
					CallPolice=1;//Nie ma strategii "racjonalnej"
			 }
			 else
			 {
#ifndef NDEBUG 
			  // cerr<<"?"; //  When? Not Aggressive but honor
#endif
			 }
		   }
		}
		else
		{
			Agres=1;
		}
}


HonorAgent::Decision  HonorAgent::check_partner(unsigned& x,unsigned& y)
//Wybór partnera interakcji przez agenta, który dostał losową inicjatywę
{
	this->MemOfLastDecision=WITHDRAW; //DOMYŚLNA DECYZJA

	unsigned L=RANDOM(HowManyNeigh);//Ustalenie który sąsiad
	if(getNeigh(L,x,y) ) //Pobranie współrzędnych sąsiada
	{
		HonorAgent& Ag=World[y][x];	//Zapamiętanie referencji do sąsiada
		
		// MAIN DECISIONS RULES START HERE
		  
		if( (this->Agres==1 //AGRESYWNY - gdy wygląda, że warto atakować i jest chęć 
		||
		(this->Agres>0.0 && DRAND()<this->Agres))  //NOT USED REALLY! //Jak nie w pełni agresywny to losowo
		&&                                          
		( Ag.HonorFeiRep<=(RATIONALITY*this->Power+(1-RATIONALITY)*HonorFeiRep) //Agresywność gdy przeciwnik SŁABSZY! Więc agresywni by nigdy by tak ze sobą nie walczyli!     RULE 2 !
		||( DRAND()< AGRES_AGRESSION   )  )           //Agresywność spontaniczna (bez kalkulacji)
		)
		{                                                                       assert(this->Agres>0.0);
			if(  Ag.HonorFeiRep>this->HonorFeiRep )
				AgressRandomAttack++;
			//cout<<"!"; //Zaatakować silniejszego!  
			this->MemOfLastDecision=HOOK;//Nieprzypadkowa zaczepka agresywnego
		}
		else if(
		
		(Honor>0 && DRAND()<Honor*HONOR_AGRESSION) //Więc poza garesywnymi może tylko honorowi mają niezerową agresję z powodu nieporozumień?
		)
		{
			this->MemOfLastDecision=HOOK;//Ewentualna zaczepka losowa lub honorowa
			   HonorRandomAttack++; //Zaatakować losowo/honorowo
		}
		//DRAND()<RANDOM_AGRESSION)					//+losowe przypadki agresji u wszystkich - PORZUCONE
	}
	this->HisActions.Count(this->MemOfLastDecision);
	return this->MemOfLastDecision;
}

HonorAgent::Decision  HonorAgent::answer_if_hooked(unsigned x,unsigned y)
//Odpowiedż na zaczepkę
{
	 this->MemOfLastDecision=GIVEUP;//DOMY�LNA DECYZJA

	 HonorAgent& Ag=World[y][x];	//Zapami�tanie referencji do s�siada

	 //REGU�A ZALE�NA WYCHOWANIA POLICYJNEGO
	 //LUB OD HONORU i W�ASNEGO POCZUCIA SI�Y
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
//	 || Ag.HonorFeiRep<this->Power /*HonorFeiRep*/)   //Tylko do wersji 0.2.60
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
	//Po��czenia z najbli�szymi s�siadami
	for(int x=0;x<SIDE;x++)
		for(int y=0;y<SIDE;y++)
		{
		   HonorAgent& Ag=HonorAgent::World[y][x];	//Zapami�tanie referencji do agenta, �eby ci�gle nie liczy� indeks�w
		   for(int xx=x-MOORE_RAD;xx<=x+MOORE_RAD;xx++)
			  for(int yy=y-MOORE_RAD;yy<=y+MOORE_RAD;yy++)
			  if(!(xx==x && yy==y))//Wyci�cie samego siebie
			  {
				 if(HonorAgent::CzyTorus)
				   Ag.addNeigh((xx+SIDE)%SIDE,(yy+SIDE)%SIDE);//Zamkni�te w torus
				   else
				   if(0<=xx && xx<SIDE && 0<=yy && yy<SIDE)
					 Ag.addNeigh(xx,yy);//bez bok�w
			  }
		}

	//Dalekie po��czenia musz� by� tworzone po lokalnych �eby si� nie dublowa�y
	for(int f=0;f<HowManyFar;f++)
	{
		unsigned x1,y1,x2,y2;

		//Poszukanie agent�w z wolnymi slotami - inicjuj�cych
		do{
		x1=RANDOM(SIDE);
		y1=RANDOM(SIDE);
		}while(HonorAgent::World[y1][x1].NeighSize()>=MAX_LINKS);

		do{//"Bierny" odbiorca dalekiego po��czenia nie mo�e by� tym samym agentem ani bliskim s�siadem
		x2=RANDOM(SIDE);
		y2=RANDOM(SIDE);
		}while((x1==x2 && y1==y2)							//Nie sam na siebie
		|| HonorAgent::World[y2][x2].NeighSize()>=MAX_LINKS //Musi miec wolny slot
		|| HonorAgent::AreNeigh(x1,y1,x2,y2)); //UWAGA! KTO� MO�E BY� DWA RAZY NA LI�CIE!

		//Po��czenie ich linkami w obie strony
		HonorAgent::World[y1][x1].addNeigh(x2,y2);
		HonorAgent::World[y2][x2].addNeigh(x1,y1);
	}
}

void one_step(unsigned long& step_counter)
//G��wna dynamika kontakt�w
{
	unsigned N=(SIDE*SIDE)/2;//Ile losowa� w kroku MC? Po�owa, bo w ka�dej interakcji dwaj
	for(unsigned i=0;i<N;i++)
	{
		unsigned x1=RANDOM(SIDE);
		unsigned y1=RANDOM(SIDE);
		HonorAgent& AgI=HonorAgent::World[y1][x1];  //Zapami�tanie referencji do agenta inicjuj�cego
		unsigned x2,y2;
		HonorAgent::Decision Dec1=AgI.check_partner(x2,y2);
		if(Dec1==HonorAgent::HOOK) //Je�li zaczepi�
		{                                					assert(AgI.Agres>0 || AgI.Honor>0);
		   HonorAgent& AgH=HonorAgent::World[y2][x2];//Zapami�tanie referencji do agenta zaczepionego
		   AgI.change_reputation(+0.05*TEST_DIVIDER,AgH); // Od razu dostaje powzy�szenie reputacji za samo wyzwanie
		   HonorAgent::Decision Dec2=AgH.answer_if_hooked(x1,y1);
		   switch(Dec2){
		   case HonorAgent::FIGHT:
					   if(HonorAgent::firstWin(AgI,AgH))
					   {
						  AgI.change_reputation(+0.35*TEST_DIVIDER,AgH);//Zyska� bo wygra�
						  AgI.HisActions.Successes++;
						  AgI.lost_power(-0.75*TEST_DIVIDER*DRAND());//Straci� bo walczy�

						  AgH.change_reputation(+0.1*TEST_DIVIDER,AgI); //Zyska� bo stan��
						  AgH.lost_power(-0.95*TEST_DIVIDER*DRAND());//Straci� bo przegra� walk�
						  AgH.HisActions.Fails++;
					   }
					   else
					   {
						  AgI.change_reputation(-0.35*TEST_DIVIDER,AgH);//Straci� bo zaczepi� i dosta� b�cki
						  AgI.HisActions.Fails++;
						  AgI.lost_power(-0.95*TEST_DIVIDER*DRAND()); //Straci� bo przegra� walk�

						  AgH.change_reputation(+0.75*TEST_DIVIDER,AgI);//Zyska� bo wygra� cho� by� zaczepiony
						  AgH.HisActions.Successes++;
						  AgH.lost_power(-0.75*TEST_DIVIDER*DRAND());	//Straci� bo walczy�
					   }
					break;
		   case HonorAgent::GIVEUP:
					   {
						  AgI.change_reputation(+0.5*TEST_DIVIDER,AgH); //Zyska� bo wygra� bez walki. Na pewno wi�cej ni� w walce???
						  AgI.HisActions.Successes++;

						  AgH.change_reputation(-0.5*TEST_DIVIDER,AgI); //Straci� bo si� podda�
						  AgH.HisActions.Fails++;
						  AgH.lost_power(-0.5*TEST_DIVIDER*DRAND()/*DRAND()*/);//I troch� dosta� �omot dla przyk�adu   (!!!)
					   }
					break;
		   case HonorAgent::CALLAUTH:
					  if(DRAND()<POLICE_EFFIC) //Czy przyby�a
					  {
						  AgI.change_reputation(-0.35*TEST_DIVIDER,AgH);//Straci� bo zaczepi� i dosta� b�cki od policji
						  AgI.HisActions.Fails++;
						  AgI.lost_power(-0.99*TEST_DIVIDER*DRAND()); //Straci� bo walczy� i przegra� z przewa�aj�c� si��

						  AgH.HisActions.Successes++; //Zaczepionemu uda�o si� unikn�� b�cek, ale...
						  AgH.change_reputation(-0.75*TEST_DIVIDER,AgI);//A  straci� reputacje bo wezwa� policje zamiast walczy�
					  }
					  else //A mo�e nie
					  {
						  AgI.change_reputation(+0.5*TEST_DIVIDER,AgH);//Zyska� bo wygra� bez walki. A� tyle?
						  AgI.HisActions.Successes++;

						  AgH.change_reputation(-0.75*TEST_DIVIDER,AgI);//Zaczepiony straci� bo wezwa� policje
						  AgH.lost_power(-0.99*TEST_DIVIDER*DRAND()); //Straci� bo si� nie broni�, a wkurzy� wzywaj�c
						  AgH.HisActions.Fails++;
					  }
					break;

		   //Odpowiedzi na zaczepk�, kt�re si� nie powinny zdarza�
		   case HonorAgent::WITHDRAW:/* TU NIE MO�E BY� */
		   case HonorAgent::HOOK:	 /* TU NIE MO�E BY� */
		   default:                  /* TU JU� NIE MO�E BY�*/
				  cout<<"?";  //Podejrzane - nie powinno si� zdarza�
		   break;
		   }
		}
		else  //if WITHDRAW  //Jak si� wycofa� z zaczepiania?
		{
			AgI.change_reputation(-0.0001*TEST_DIVIDER,*(HonorAgent*)(NULL));//Minimalnie traci w swoich oczach
		}
	}

	step_counter++;
}

unsigned NumberOfKilled=0;
unsigned NumberOfKilledToday=0;


void power_recovery_step()
{
  NumberOfKilledToday=0;
  for(int x=0;x<SIDE;x++)
	for(int y=0;y<SIDE;y++)
	{
		HonorAgent& Ag=HonorAgent::World[y][x];	//Zapami�tanie referencji do agenta, �eby ci�gle nie liczy� indeks�w

		if(Ag.LastDecision(false)==HonorAgent::NOTHING)//Nie by� w tym kroku ani razu wylosowany!
				Ag.HisActions.Count(HonorAgent::NOTHING);//To mu trzeba doliczy� do licznik�w, bo inaczej NOTHING nie b�dzie tam nigdy rejestrowane!

		Ag.HisLifeTime++;  //Zwi�kszenie licznika �ycia

		if(MORTALITY>0)   //Losowa �mier� z hor�b, staro�ci i wypadk�w
			{
			   if(DRAND()<MORTALITY)
				   Ag.Power=-1;
			}

		if(USED_SELECTION>=0 && Ag.Power<USED_SELECTION)  //Chyba nie przezy�, selekcja 0 oznacza �mier� tylko od wypadk�w
		{
		   if(population_growth==0) //Tryb z prawdopodobienstwami inicjalnymi
		   {       																assert(MAFIAHONOR==false);
			Ag.RandomReset();
			NumberOfKilled++;
			NumberOfKilledToday++;
		   }
		   else
		   if(population_growth==1) //Tryb z losowym s�siadem
		   {
			unsigned ktory=RANDOM(Ag.NeighSize()),xx,yy;
			bool pom=Ag.getNeigh(ktory,xx,yy);
			HonorAgent& Rodzic=HonorAgent::World[yy][xx];

			if(Rodzic.Power>0) //Tylko wtedy mo�e si� rodzi�! NEW TODO Check  - JAK NIE TO ROZLICZNIE NA PӏNIEJ
			{
			 if(MAFIAHONOR)//Je�eli s� stosunki rodzinne to �mier� ma r�ne konsekwencje
				Ag.SmiercDona();

			 if(InheritMAXPOWER)
			 {
				float NewLimit=Rodzic.PowLimit + LIMITNOISE *Rodzic.PowLimit*(( (DRAND()+DRAND()+DRAND()+DRAND()+DRAND()+DRAND())/6 ) - 0.5);
                																assert(NewLimit>0);
				if(NewLimit>1) NewLimit=1;//Zerowy nie będzie, ale może przekraczać 1
				Ag.RandomReset(NewLimit);
			 }
			 else
				Ag.RandomReset();

			 Ag.Agres=Rodzic.Agres;
			 Ag.Honor=Rodzic.Honor;
			 Ag.CallPolice=Rodzic.CallPolice;

			 if(MAFIAHONOR)//I urodziny tak�e
			 {
				Ag.HonorFeiRep=Rodzic.HonorFeiRep;//Ma reputacje rodzica, bo on go chroni
				PowiazRodzicielsko(Rodzic,Ag);  //anty SmiercDona();
			 }
			 NumberOfKilled++;
			 NumberOfKilledToday++;
			}
		   }
		   else
		   if(population_growth==3) //Tryb z losowym cz�onkiem populacji
		   {                                                                    assert(MAFIAHONOR==false);
			unsigned xx=RANDOM(SIDE),yy=RANDOM(SIDE);
			HonorAgent& Drugi=HonorAgent::World[yy][xx];
			if(Drugi.Power>0)  //Mo�e posta� do rozliczenia na p�niej
			{
			 Ag.RandomReset();
			 Ag.Agres=Drugi.Agres;
			 Ag.Honor=Drugi.Honor;
			 Ag.CallPolice=Drugi.CallPolice;
			 NumberOfKilled++;
			 NumberOfKilledToday++;
			}
		   }
		}
		else
		if(Ag.Power<Ag.PowLimit)//Mo�e si� leczy� lub "poprawia�"
		{
			Ag.Power+=(Ag.PowLimit-Ag.Power)*RECOVERY_POWER;
		}
	}

  //Losowa wymiana na przypadkowo narodzone (z domy�lnej proporcji)
  if(EXTERNAL_REPLACE>0)
  {
	unsigned WypadkiLosow=SIDE*EXTERNAL_REPLACE*SIDE;
	for(;WypadkiLosow>0;WypadkiLosow--)
	{
	  unsigned x=RANDOM(SIDE),y=RANDOM(SIDE);
	  HonorAgent& Ag=HonorAgent::World[y][x];	//Zapami�tanie referencji
	  Ag.RandomReset();  //Wg. inicjalnej dystrybucji - mo�e przywracac do istnienia ju� wymar�e strategie
	}
  }
}

void Reset_action_memories()
//Czyszczenie pami�ci zachowa�
{
	for(unsigned v=0;v<SIDE;v++)
	{
		for(unsigned h=0;h<SIDE;h++)
		{
			HonorAgent::World[v][h].LastDecision(true);			//Zapami�tanie referencji do agenta, �eby ci�gle nie liczy� indeks�w
		}
	}
}


void DeleteAllConnections()
//Usuwanie połączeń z zarejestrowanymi sąsiadami
{
	for(int x=0;x<SIDE;x++)
		for(int y=0;y<SIDE;y++)
		{
		   HonorAgent& Ag=HonorAgent::World[y][x];	//Zapami�tanie referencji do agenta, �eby ci�gle nie liczy� indeks�w
		   Ag.forgetAllNeigh(); //Bezwarunkowe zapomnienie
		}
}

//FAMILY RELATIONS
///////////////////////////////////////////////////////////////////////////////////////////
bool    HonorAgent::IsMyFamilyMember(HonorAgent& Inny,HonorAgent*& Cappo,int MaxLevel)//=2
//Czy ten "Inny" nale�y do "rodziny" danego agenta
//Przy okazji sprawdzenia ustalamy "Ojca chrzestnego" (Cappo) na po�niej
{
	if(MaxLevel>0)//Jak jeszcze nie szczyt zadanej hierarchii, to szuka ojca
	 for(unsigned i=0;i<NeighSize();i++)
	  if(Neighbourhood[i].Parent==1)//Je�li ma ojca to idzie w g�r�
	  {
		 HonorAgent& Rodzic=World[Neighbourhood[i].Y][Neighbourhood[i].X];
		 Cappo=&Rodzic;//Mo�e "Rodzic" to zmieni, ale jak nie to juz znaleziony
		 return  Rodzic.IsMyFamilyMember(Inny,Cappo,MaxLevel-1);  //REKURENCJA!
	  }

	//Jak ju� nie poszed� z tego czy innego powodu w g�r� to mo�e szuka�
	if(this==&Inny) //On sam okaza� si� szukanym
			return true;
	for(unsigned j=0;j<NeighSize();j++)//Jest najwy�szym ojcem, teraz mo�e szuka� potomk�w
	 if(Neighbourhood[j].Child==1) //Jest dzieckiem
	 {
		HonorAgent& Dziecko=World[Neighbourhood[j].Y][Neighbourhood[j].X];
		bool res=Dziecko.IsMyFamilyMember(Inny,Cappo,-1);  //-1 zabezpiecza przed REKURENCJA
		if(res)
			return true;
	 }

	//Nigdzie nie znalazł - znaczy w jego pod-drzewie nie ma
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
	for(unsigned j=0;j<NeighSize();j++)//Czy jest może ojcem? Trzeba szukać potomków
	 if(Neighbourhood[j].Child==1) //Jest dzieckiem
	 {
		HonorAgent& Dziecko=World[Neighbourhood[j].Y][Neighbourhood[j].X];
		Dziecko.change_reputation_thru_family(Delta); //REKURENCJA W DLA RODZINY
	 }
}


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
//Ile ma zarejestrowanych s�siad�w
{
	return HowManyNeigh;
}

void HonorAgent::forgetAllNeigh()
//Zapomina wszystkich dodanych s�siad�w, co nie znaczy �e oni zapominaj� jego
{
   HowManyNeigh=0;
}

bool HonorAgent::getNeigh(unsigned i,unsigned& x,unsigned& y) const
//Czyta wsp�rzedne s�siada, o ile jest
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
//Wsp�rz�dne kolejnego s�siada
{
  if(i<HowManyNeigh)
		return &Neighbourhood[i];
		else return NULL;
}

bool HonorAgent::addNeigh(unsigned x,unsigned y)
//Dodaje sasiada o okreslonych wsp�rz�dnych w �wiecie
{
   if(HowManyNeigh<Neighbourhood.get_size()) //Czy jest jeszcze miejsce wok�?
   {
	Neighbourhood[HowManyNeigh].X=x;
	Neighbourhood[HowManyNeigh].Y=y;
	HowManyNeigh++;
	return true;
   }
   else return false;
}

//Pola statyczne klasy HonorAgent
bool 	HonorAgent::CzyTorus=false; 
unsigned HonorAgent::licznik_zyc=0;
wb_dynmatrix<HonorAgent> HonorAgent::World;

/*
VERY OLD INITIALISATION:


		// Warto�ci w Agres, Honor, CallPolice s� prawdopodobie�stwami innymi ni� 0 i 1 - WARIANT PORZUCONY NA RAZIE (?)
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if(BULLI_POPUL<1)
		{
		  Agres=(DRAND()<BULLI_POPUL?1:0); //Albo jest albo nie jest
		}
		else
		{
		 if(BULLISM_LIMIT>0)
			Agres=(DRAND()+DRAND()+DRAND()+DRAND()+DRAND()+DRAND())/6*BULLISM_LIMIT; // Bulizm (0..1) sk�onno�� do atakowania,  const FLOAT    BULLISM_LIMIT=1;//Maksymalny mo�liwy bulizm.
			else
			Agres=(DRAND()*DRAND()*DRAND()*DRAND())*fabs(BULLISM_LIMIT);
		}

		Honor=(DRAND()<HONOR_POPUL?1:0); //Albo jest albo nie jest -  Bezwarunkowa honorowo�� (0..1) sk�onno�� podj�cia obrony,  const FLOAT	   HONOR_POPUL=0.9;//Jaka cz�� agent�w populacji jest �ci�le honorowa


		if(Honor<0.99 && Agres<0.99 && DRAND()<CALLER_POPU) //??? Agres XOR HighHonor XOR CallPolice
			CallPolice=1;//Prawdopodobie�stwo wzywania policji (0..1) Mo�e te� jako �w�drowna �rednia� wezwa�, ale na razie nie
			else
			CallPolice=0;
*/