///  Agent for "Culture of Honor"
///  IMPLEMENTATIONS of METHODS
//*//////////////////////////////////////////////////////////////////////////////

#define _USE_MATH_DEFINES //--> M_PI use. Needed by MSVC?
#include <cmath>
//#include <fstream>
//#include <iomanip>
#include <iostream>

#include "symshell.h"
#define HIDE_WB_PTR_IO 1
#include "wb_ptr.hpp"

#include "HonorAgent.hpp"

using namespace wbrtm;
using namespace std;

/// Pola statyczne klasy HonorAgent
//*////////////////////////////////////////////////////////
bool 	HonorAgent::CzyTorus=false; ///< Topologia świata
unsigned HonorAgent::licznik_zyc=0; ///< Ile żywych agentów

wb_dynmatrix<HonorAgent> HonorAgent::World; ///< ŚWIAT AGENTÓW

/// Liczniki do statystyk "on run"
//*/////////////////////////////////////////////////////////////////////////////////
unsigned HonorRandomAttack=0;  ///< Honor Culture Random Attack Counter
unsigned AgressRandomAttack=0; ///< Counter for random aggressive "culture" attacks

// OBSŁUGA INICJALIZACJI ŚWIATA AGENTÓW
//*////////////////////////////////////////

/// Losowa inicjalizacja atrybutów agentów
void InitAtributes(FLOAT HowMany)
{
    for(int y=0;y<SIDE;y++)
        for(int x=0;x<SIDE;x++)
            HonorAgent::World[y][x].RandomReset();
}

/// Losowanie wartości atrybutów agenta.
///\default iPOWLIMIT==0
void HonorAgent::RandomReset(float iPOWLIMIT)
{
		this->ID=++licznik_zyc;
		this->HisActions.Reset();
		this->HisLifeTime=0;

		if(iPOWLIMIT>0)
			PowLimit=iPOWLIMIT; //Ustawia jaką siłę może osiągnąć maksymalnie, gdy nie traci
		else
			PowLimit=(DRAND()+DRAND()+DRAND()+DRAND()+DRAND()+DRAND())/6;  //Losuje jaką siłę może osiągnąć maksymalnie

		Power=(0.5+DRAND()*0.5)*PowLimit; //Zawsze przynajmniej 0.5 limitu siły
		HonorFeiRep=Power; //Reputacja wojownika. Najpierw "z wyglądu", a potem z realnych konfrontacji
						   //Inicjalizacja: DRAND()*Power, a kiedyś było DRAND()*0.5 też. Potem uproszczone do Power;  
																				                 assert(0<=HonorFeiRep);
																				                 assert(HonorFeiRep<=1);

		// Agres, Honor, CallPolice INICJOWANE alternatywnie, czyli jeden z prawdopodobieństwem 1 a reszta 0.
		//*///////////////////////////////////////////////////////////////////////////////////////////////////
		Honor=0;CallPolice=0;//Wartości domyślne
																				           assert(fabs(BULLI_POPUL)<1);

		if(fabs(BULLI_POPUL)<1)
		{
		   Agres=(DRAND()<fabs(BULLI_POPUL)?1:0); //Albo jest AGRESYWNY, albo nie jest
		   
		   if(Agres!=1) //Jak nie jest
		   {
			 Honor=(DRAND()*(1-fabs(BULLI_POPUL)) < fabs(HONOR_POPUL)? 1 : 0 ); //Jest HONOROWY albo nie jest
			 
			 if(Honor!=1)   //NIE AGRESYWNY i NIE HONOROWY
			 {              //"dignity" albo racjonalny
			   if(!ONLY3STRAT) //jeśli TRUE to znikają RACJONALNI (czyli wzywanie policji jest wtedy "strategią resztkową")
					CallPolice=(DRAND()*(1-fabs(BULLI_POPUL)-fabs(HONOR_POPUL)) < fabs(CALLER_POPU)? 1 : 0); //Jest albo nie jest 
			   else
					CallPolice=1; //Nie ma strategii "racjonalnej"
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
			Agres=1; //Ma sens tylko wtedy jak 1 to sami agresywni. ALE CZY TAK KIEDYŚ ROBILIŚMY! (TODO???)
		}
}

/// Losowo-matrycowa inicjalizacja połączeń (czyli są lokalne + dalekie)
void InitConnections(FLOAT HowManyFar)
{
    // Połączenia z najbliższymi sąsiadami
    for(int x=0;x<SIDE;x++)
        for(int y=0;y<SIDE;y++)
        {
            HonorAgent& Ag=HonorAgent::World[y][x];	//Zapamiętanie referencji do agenta, żeby ciągle nie liczyć indeksów
            for(int xx=x-MOORE_RAD;xx<=x+MOORE_RAD;xx++)
                for(int yy=y-MOORE_RAD;yy<=y+MOORE_RAD;yy++)
                    if(!(xx==x && yy==y))//Wyci�cie samego siebie
                    {
                        if(HonorAgent::CzyTorus) //JAKA TOPOLOGIA ŚWIATA?
                            Ag.addNeigh((xx+SIDE)%SIDE,(yy+SIDE)%SIDE); //Zamknięte w torus
                        else
                        if(0<=xx && xx<SIDE && 0<=yy && yy<SIDE)
                            Ag.addNeigh(xx,yy); //bez boków
                    }
        }

    // Dalekie połączenia muszą być tworzone po lokalnych, żeby się nie dublowały
    for(int f=0;f<HowManyFar;f++)
    {
        unsigned x1,y1,x2,y2;

        // Poszukanie agentów z wolnymi slotami, czyli inicjujących dalekie połączenia
        do{
            x1=RANDOM(SIDE);
            y1=RANDOM(SIDE);
        }while(HonorAgent::World[y1][x1].NeighSize()>=MAX_LINKS);

        //"Bierny" odbiorca dalekiego połączenia nie może być tym samym agentem ani bliskim sąsiadem!
        do{
            x2=RANDOM(SIDE);
            y2=RANDOM(SIDE);
        }while( (x1==x2 && y1==y2)							//Nie sam na siebie
                || HonorAgent::World[y2][x2].NeighSize()>=MAX_LINKS //Musi mieć wolny slot
                || HonorAgent::AreNeigh(x1,y1,x2,y2)); //CZY KTOŚ MOŻE BYĆ DWA RAZY NA LIŚCIE!?

        //Połączenie ich linkami w obie strony
        HonorAgent::World[y1][x1].addNeigh(x2,y2);
        HonorAgent::World[y2][x2].addNeigh(x1,y1);
    }
}

// OBSŁUGA DYNAMIKI SYMULACJI
//*///////////////////////////////////////////////////////////////////////

/// \brief
/// Wybór partnera interakcji przez agenta, który dostał losową inicjatywę, oraz decyzja co agent z inicjatywą ma zrobić.
/// \details
/// Zarówno Honorowi jak i agresywni mają spontaniczną agresję, która może być różnie prawdopodobna dla każdej z kultur.
/// W obu przypadkach pełni ona rolę "challengu" - weryfikuje czy istniejąca reputacja jest prawdziwa.
/// Bez tego ktoś "silny z natury" mógłby nigdy nie zostać sprowadzony do realiów.
/// Poza tym Agresywni mają swoją agresję strategiczną, czyli atakowanie słabszych.
/// Gdy atrybut Agresywności jest mniejszy niż 1 to atak może być przeprowadzany nie zawsze,
/// ale z tej możliwości nie korzystaliśmy w artykule,
/// podobnie jak w końcu z honoru rodzinnego/klanowego.
HonorAgent::Decision  HonorAgent::check_partner(unsigned& x,unsigned& y)
{
	this->MemOfLastDecision=WITHDRAW; //DOMYŚLNA DECYZJA

	unsigned L=RANDOM(HowManyNeigh); //Ustalenie, który sąsiad będzie partnerem

	if(getNeigh(L,x,y) ) //Pobranie współrzędnych sąsiada
	{
		HonorAgent& Ag=World[y][x];	//Zapamiętanie referencji do sąsiada
		
		// MAIN DECISIONS RULES START HERE
        //*///////////////////////////////////////////////
		if( (this->Agres==1 //AGRESYWNY: Atakuje, gdy wygląda, że warto atakować i ma chęć
		||
		(this->Agres>0.0 && DRAND()<this->Agres))  //NOT USED REALLY in PAPER 2015! Jak nie w pełni agresywny to losujemy chęć!
		&&                                          
		( Ag.HonorFeiRep<=(RATIONALITY*this->Power+(1-RATIONALITY)*HonorFeiRep) //Agresywność, gdy przeciwnik SŁABSZY! Więc agresywni by nigdy by tak ze sobą nie walczyli!     RULE 2 !
		||( DRAND()< AGRES_AGRESSION   )  )                                     //Agresywność spontaniczna (bez kalkulacji)
		)
		{                                                                                       assert(this->Agres>0.0);
			if(  Ag.HonorFeiRep>this->HonorFeiRep )
				AgressRandomAttack++;
			//cout<<"!"; //Zaatakować silniejszego!  
			this->MemOfLastDecision=HOOK;//Nieprzypadkowa zaczepka agresywnego
		}
		else if(
//TODO??? Czy coś tu było wcześniej?
		(Honor>0 && DRAND()<Honor*HONOR_AGRESSION) //Więc poza agresywnymi może tylko honorowi mają niezerową agresję z powodu nieporozumień?
		)
		{
			this->MemOfLastDecision=HOOK; //Ewentualna zaczepka losowa lub honorowa
			   HonorRandomAttack++; //Zaatakować losowo/honorowo
		}
		//DRAND()<RANDOM_AGRESSION)					//+losowe przypadki agresji u wszystkich. POMYSŁ PORZUCONY.
	}

	this->HisActions.Count(this->MemOfLastDecision);

    return this->MemOfLastDecision;
}

/// Odpowiedź pasywnego agenta na zaczepkę.
/// \details
/// To też zależy od kultury. Domyślną decyzją jest się poddać, i dotyczy zarówno "agresywnych" jak i racjonalnych.
/// Co jest o tyle sensowne że atakujący ma i tak ZAWSZE wyższą reputację niż atakowany. Albo tak mu się przynajmniej wydaje.
/// Jednakże:
/// Agenci kultury "DYGNITY" wzywają policję z prawdopodobieństwem zależnym od tego jak bardzo się z ta kulturą identyfikują.
/// Ale w artykule z 2015 ta identyfikacja zawsze wynosi 1 czyli policja jest zawsze przez nich wzywana.
/// Agenci kultury honoru ZAWSZE odpowiadają na zaczepkę (chyba że nie są w 100% honorowi, ale tego też nie testowaliśmy).
///
HonorAgent::Decision  HonorAgent::answer_if_hooked(unsigned x,unsigned y)
{
	 this->MemOfLastDecision=GIVEUP; //DOMYŚLNA DECYZJA

	 HonorAgent& Ag=World[y][x];	//Zapamiętanie referencji do sąsiada

	 // REGUŁA ZALEŻNA OD "WYCHOWANIA" POLICYJNEGO
	 // LUB OD HONORU i WŁASNEGO POCZUCIA SIŁY
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

/// Główna dynamika kontaktów.
/// \Details
/// W pętli Monte Carlo odbywa się:
///     * Losowanie agenta aktywnego,
///     * ustalanie z kim wchodzi w interakcje i jakiego rodzaju to interakcja.
///     * Sprawdzenie odpowiedzi agenta zaczepionego
///     * obsługa rezultatów interakcji.
///
void one_step(unsigned long& step_counter)
{
	unsigned N=(SIDE*SIDE)/2; //Ile losowań w kroku MC? Połowa (!), bo w każdej interakcji biorą udział dwaj agenci
	for(unsigned i=0;i<N;i++)
	{
		unsigned x1=RANDOM(SIDE);
		unsigned y1=RANDOM(SIDE);
		HonorAgent& AgI=HonorAgent::World[y1][x1];  //Zapamiętanie referencji do agenta inicjującego

		unsigned x2,y2; //Współrzędne sąsiada ustawiane w wywołaniu funkcji poniżej.
		HonorAgent::Decision Dec1=AgI.check_partner(x2,y2); //Kto jest partnerem i jaka akcja?

        if(Dec1==HonorAgent::HOOK) //Jeżeli zaczepia
		{                                					                         assert(AgI.Agres>0 || AgI.Honor>0);
		   HonorAgent& AgH=HonorAgent::World[y2][x2]; //Zapamiętanie referencji do agenta zaczepionego

		   AgI.change_reputation(+0.05*TEST_DIVIDER,AgH); // Od razu dostaje podwyższenie reputacji za samo wyzwanie
		   HonorAgent::Decision Dec2=AgH.answer_if_hooked(x1,y1);
		   switch(Dec2){
		   case HonorAgent::FIGHT:
					   if(HonorAgent::firstWin(AgI,AgH))
					   {
						  AgI.change_reputation(+0.35*TEST_DIVIDER,AgH); //Zyskał, bo wygrał
						  AgI.HisActions.Successes++;
						  AgI.lost_power(-0.75*TEST_DIVIDER*DRAND()); //Stracił siły, bo walczył

						  AgH.change_reputation(+0.1*TEST_DIVIDER,AgI); //Zyskał, bo stanął do walki
						  AgH.lost_power(-0.95*TEST_DIVIDER*DRAND()); //Stracił siły, bo przegrał walkę
						  AgH.HisActions.Fails++;
					   }
					   else
					   {
						  AgI.change_reputation(-0.35*TEST_DIVIDER,AgH); //Stracił, bo zaczepił i dostał bęcki
						  AgI.HisActions.Fails++;
						  AgI.lost_power(-0.95*TEST_DIVIDER*DRAND()); //Stracił siły, bo przegrał walkę

						  AgH.change_reputation(+0.75*TEST_DIVIDER,AgI); //Zyskał, bo wygrał choć był zaczepiony
						  AgH.HisActions.Successes++;
						  AgH.lost_power(-0.75*TEST_DIVIDER*DRAND());	//Stracił siłę, bo walczył
					   }
					break;
		   case HonorAgent::GIVEUP:
					   {
						  AgI.change_reputation(+0.5*TEST_DIVIDER,AgH); //Zyskał, bo wygrał bez walki.
                                                                                 //Czy na pewno więcej niż w walce???
						  AgI.HisActions.Successes++;

						  AgH.change_reputation(-0.5*TEST_DIVIDER,AgI); //Stracił, bo się poddał
						  AgH.HisActions.Fails++;

						  AgH.lost_power(-0.5*TEST_DIVIDER*DRAND()/*DRAND()*/); //Ale trochę dostał w ucho dla przykładu   (!!!)
					   }
					break;
		   case HonorAgent::CALLAUTH:
					  if(DRAND()<POLICE_EFFIC) //Czy pomoc przybyła?
					  {
						  AgI.change_reputation(-0.35*TEST_DIVIDER,AgH); //Stracił, bo zaczepił i dostał bęcki od policji
						  AgI.HisActions.Fails++;
						  AgI.lost_power(-0.99*TEST_DIVIDER*DRAND()); //Stracił siły, bo walczył i przegrał z przeważającą siłą

						  AgH.HisActions.Successes++; //Zaczepionemu udało się uniknąć bęcek, ale...
						  AgH.change_reputation(-0.75*TEST_DIVIDER,AgI); //...stracił reputację, bo wezwał policje, zamiast walczyć
					  }
					  else //A może nie przybyła?
					  {
						  AgI.change_reputation(+0.5*TEST_DIVIDER,AgH); //Zyskał, bo wygrał bez walki. Ale aż tyle?
						  AgI.HisActions.Successes++;

						  AgH.change_reputation(-0.75*TEST_DIVIDER,AgI); //Zaczepiony stracił, bo wezwał policje
						  AgH.lost_power(-0.99*TEST_DIVIDER*DRAND()); //Stracił, bo się nie bronił, a wkurzył, wzywając policję.
						  AgH.HisActions.Fails++;
					  }
					break;

		   //Odpowiedzi na zaczepkę, które się nie powinny zdarzać
		   case HonorAgent::WITHDRAW:/* TU NIE MOżE BYĆ TUTAJ! */
		   case HonorAgent::HOOK:	 /* TU NIE MOżE BYĆ TUTAJ! */
		   default:                  /* TU NIE MOżE BYĆ TUTAJ! */
				  cout<<"?";  //MOCNO Podejrzane! Nieznane akcje nie powinny się zdarzać!
		   break;
		   }
		}
		else  //if(WITHDRAW)  - Jak się wycofał z zaczepiania?
		{
			AgI.change_reputation(-0.0001*TEST_DIVIDER,*(HonorAgent*)(NULL)); //Minimalnie traci w swoich oczach
		}
	}

	step_counter++; // KONIEC KROKU M-C
}

/// Statystyki przeżyciowe
unsigned NumberOfKilled=0; ///< Counter of all deaths
unsigned NumberOfKilledToday=0; ///< Death counter per step


/// \brief
/// Odzyskiwanie sił, albo stwierdzanie zgonu. Także opcjonalna migracja.
///
/// \details
/// "Post-krok" czyli odpoczynek po ciężkim dniu ;-)
///
/// Jeśli agent nie przeżył bo zabrakło mu siły, to jest podmieniany na jeden z kilku sposobów.
/// Jego miejsce może zająć:
///     1) Nowy losowy agent
///     2) Potomek sąsiada
///     3) Potomek dowolnego agenta z populacji
///
/// PONADTO:
/// Jeżeli EXTERNAL_REPLACE jest niezerowe to zupełnie przypadkowy agent moze zniknąć i zostać zastapiony przez losowego.
/// To sposób zagwarantowania że wymarłe strategie mogą wracać i testować się w środowisku, w którym wcześniej przepadły.
///
void power_recovery_step()
{
  NumberOfKilledToday=0;
  for(int x=0;x<SIDE;x++)
	for(int y=0;y<SIDE;y++)
	{
		HonorAgent& Ag=HonorAgent::World[y][x];	//Zapamiętanie referencji do agenta

		if(Ag.LastDecision(false)==HonorAgent::NOTHING)  // Nie był w tym kroku ani razu wylosowany!
				Ag.HisActions.Count(HonorAgent::NOTHING); // To mu trzeba doliczyć do liczników,
                                                               // bo inaczej NOTHING nie byłoby tam nigdy rejestrowane!

		Ag.HisLifeTime++;  //Zwiększenie licznika długości życia

		if(MORTALITY>0)   //Losowa śmierć z "chorób, starości i wypadków losowych"
			{
			   if(DRAND()<MORTALITY)
				   Ag.Power=-1;
			}

		if(USED_SELECTION>=0 //selekcja 0 oznacza śmierć tylko od MORTALITY i EXTERNAL_REPLACE
        && Ag.Power<USED_SELECTION)  //Siła spadła poniżej progu przeżycia.
		{
		   if(population_growth==0) //Tryb z prawdopodobieństwami inicjalnymi
		   {       																              assert(MAFIAHONOR==false);
			Ag.RandomReset(); //Po prostu w jego miejsce losowy agent
			NumberOfKilled++;
			NumberOfKilledToday++;
		   }
		   else
		   if(population_growth==1) //Tryb lokalny. Z losowym sąsiadem jako rodzicem
		   {
			unsigned ktory=RANDOM(Ag.NeighSize()),xx,yy;
			bool pom=Ag.getNeigh(ktory,xx,yy);
			HonorAgent& Rodzic=HonorAgent::World[yy][xx];

			if(Rodzic.Power>0) //Tylko wtedy może się rodzić! NEW TODO Check  - JAK NIE TO ROZLICZENIE NA PÓŹNIEJ?
			{
			 if(MAFIAHONOR) //Jeżeli są stosunki rodzinne to śmierć ma różne konsekwencje społeczne
				Ag.SmiercDona();

			 if(InheritMAXPOWER)
			 {
				float NewLimit=Rodzic.PowLimit + LIMITNOISE
                              *Rodzic.PowLimit*(( (DRAND()+DRAND()+DRAND()+DRAND()+DRAND()+DRAND())/6 ) - 0.5);
                																                     assert(NewLimit>0);
				if(NewLimit>1) NewLimit=1; //Zerowy nie będzie, ale może przekraczać 1
				Ag.RandomReset(NewLimit);
			 }
			 else
				Ag.RandomReset();

			 Ag.Agres=Rodzic.Agres;
			 Ag.Honor=Rodzic.Honor;
			 Ag.CallPolice=Rodzic.CallPolice;

			 if(MAFIAHONOR) //I urodziny także mają konsekwencje rodzinne
			 {
				Ag.HonorFeiRep=Rodzic.HonorFeiRep; //Ma reputacje rodzica, bo on go chroni
				PowiazRodzicielsko(Rodzic,Ag);  //anty SmiercDona();
			 }

             // Aktualizacja licznków
			 NumberOfKilled++;
			 NumberOfKilledToday++;
			}
		   }
		   else
		   if(population_growth==3) //Tryb GLOBALNY z losowym członkiem populacji jako rodzicem
		   {                                                                                  assert(MAFIAHONOR==false);
			unsigned xx=RANDOM(SIDE),yy=RANDOM(SIDE);
			HonorAgent& Drugi=HonorAgent::World[yy][xx]; //Uchwyt do agenta
			if(Drugi.Power>0)  //Może postać do rozliczenia na później.
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
		else // Ewentualnie, jeżeli jednak nie umarł, to...
		if(Ag.Power<Ag.PowLimit) //... może się leczyć ("poprawiać")
		{
			Ag.Power+=(Ag.PowLimit-Ag.Power)*RECOVERY_POWER;
		}
	}

  // Losowa wymiana na przypadkowo narodzone (z domyślnej proporcji)
  // Czyli jakby wymiana agentów z dużo większym środowiskiem zewnętrznym o stabilnych proporcjach
  if(EXTERNAL_REPLACE>0)
  {
	unsigned WypadkiLosow=SIDE*EXTERNAL_REPLACE*SIDE;

	for(;WypadkiLosow>0;WypadkiLosow--)
	{
	  unsigned x=RANDOM(SIDE),y=RANDOM(SIDE);
	  HonorAgent& Ag=HonorAgent::World[y][x];	//Zapamiętanie referencji

	  Ag.RandomReset();  //Według inicjalnej dystrybucji więc może przywracać do istnienia już wymarłe strategie!
	}

  }
}

// OBSŁUGA CZYSZCZENIA POŁĄCZEŃ. Ważna zwłaszcza przy powtórzeniach.
//*/////////////////////////////////////////////////////////////////////////////////////////

/// Usuwanie połączeń z zarejestrowanymi sąsiadami
void DeleteAllConnections()
{
	for(int x=0;x<SIDE;x++)
		for(int y=0;y<SIDE;y++)
		{
		   HonorAgent& Ag=HonorAgent::World[y][x];	//Zapamiętanie referencji do agenta
		   Ag.forgetAllNeigh(); //Bezwarunkowe zapomnienie
		}
}


// Pomocnicze metody i akcesory agentów
//*//////////////////////////////////////////////////////////////////////////////

/// Jaka była ostatnia decyzja z kroku MC.
/// Do celów wizualizacyjnych i statystycznych
HonorAgent::Decision HonorAgent::LastDecision(bool clean)
{
	Decision Tmp=MemOfLastDecision;
	if(clean) MemOfLastDecision=NOTHING;
	return Tmp;
}

/// Czyszczenie pamięci zachowań.
void Reset_action_memories()
{
    for(unsigned v=0;v<SIDE;v++)
    {
        for(unsigned h=0;h<SIDE;h++)
        {
            HonorAgent::World[v][h].LastDecision(true);	//Zapamiętanie referencji do agenta
        }
    }
}

/// Dodaje sąsiada o określonych współrzędnych w świecie
bool HonorAgent::addNeigh(unsigned x,unsigned y)
{
    if(HowManyNeigh<Neighbourhood.get_size()) //Czy jest jeszcze miejsce wokół?
    {
        Neighbourhood[HowManyNeigh].X=x;
        Neighbourhood[HowManyNeigh].Y=y;
        HowManyNeigh++;
        return true;
    }
    else return false;
}

/// Ile ma zarejestrowanych sąsiadów
unsigned HonorAgent::NeighSize()const
{
	return HowManyNeigh;
}

/// Zapomina wszystkich dodanych sąsiadów, co nie znaczy, że oni zapominają jego
void HonorAgent::forgetAllNeigh()
{
   HowManyNeigh=0;
}

/// Czyta współrzędne sąsiada, o ile jest jakiś pod 'i'
/// \return czy był tam sąsiad.
bool HonorAgent::getNeigh(unsigned i,unsigned& x,unsigned& y) const
{
  if(i<HowManyNeigh) //Jest taki
   {
	x=Neighbourhood[i].X;
	y=Neighbourhood[i].Y;
	return true;
   }
   else return false;
}

/// Kolejny link do i-tego sąsiada
const HonorAgent::LinkTo* HonorAgent::Neigh(unsigned i)
{
  if(i<HowManyNeigh)
		return &Neighbourhood[i];
		else return NULL;
}

/// Ustala czy pierwszy czy drugi agent zwycięży w konfrontacji
/// WAŻNE UPROSZCZENIE: Zawsze któryś musi zwyciężyć, co nie jest realistyczne -
bool      HonorAgent::firstWin(HonorAgent& In,HonorAgent& Ho)
{
    if( In.Power*(1+DRAND())  >  Ho.Power*(1+DRAND()) ) //DRAND() --> random number from 0 to 1
        return true;
    else
        return false;
}

/// Spadek siły z zabezpieczeniem zakresu
void      HonorAgent::lost_power(double delta)
{                                  					                                                   assert(Power<=1);
    delta=fabs(delta);
    Power*=(1-delta);
    if(Power<0)
        Power=0;     //To be sure...
}

/// Wzrost lub spadek reputacji z zabezpieczeniem zakresu.
/// Jak MAFIAHONOR i gość jest honorowy to zmienia się tak samo całej rodzinie!
void    HonorAgent::change_reputation(double Delta,HonorAgent& Powod,int level)//level==0
{                                                                                        //Sam siebie nie może atakować!
                                                                                        assert(this!=&Powod);
                                                                                        assert(0<=HonorFeiRep);
                                                                                        assert(HonorFeiRep<=1);
#ifdef HONOR_WITHOUT_REPUTATION //For "null hipothesis" tests
    if(this->Honor==1) //Honor agent
	 {
		return; //nothing to do
	 }
#endif
    HonorAgent* Cappo=NULL; //Przy okazji sprawdzenia, czy ktoś jest w tej rodzinie, ustalamy "Ojca chrzestnego".
    if(&Powod!=NULL && MAFIAHONOR && !IsMyFamilyMember(Powod,Cappo) )//Jeżeli działa honor rodzinny to ...
    {
        if(Cappo==NULL) //Nie ma żyjącego ojca. Tu ślad się urywa.
            Cappo=this;
        Cappo->change_reputation_thru_family(Delta); //Ale może mieć dzieci...
    }
    else
    if(Delta>0) //Wzrost
    {
        HonorFeiRep+=(1-HonorFeiRep)*Delta;       	                                             assert(HonorFeiRep<=1);
    }
    else       //Spadek
    {
        HonorFeiRep-=HonorFeiRep*fabs(Delta);                                                 assert(0<=HonorFeiRep);
                                                                                                 assert(HonorFeiRep<=1);
    }
}

// FAMILY RELATIONS (not used in paper from 2015)
//*/////////////////////////////////////////////////////////////////////////////////////////

/// Czy ten "Inny" należy do "rodziny" danego agenta.
/// Przy okazji sprawdzenia ustalamy "Ojca chrzestnego" (Cappo) na później
bool    HonorAgent::IsMyFamilyMember(HonorAgent& Inny,HonorAgent*& Cappo,int MaxLevel) //=2
{
    if(MaxLevel>0) //Jak jeszcze nie szczyt zadanej hierarchii, to szuka ojca
        for(unsigned i=0;i<NeighSize();i++)
            if(Neighbourhood[i].Parent==1) //Jeżeli ma ojca to idziemy w górę
            {
                HonorAgent& Rodzic=World[Neighbourhood[i].Y][Neighbourhood[i].X];
                Cappo=&Rodzic; //Może "Rodzic" to zmieni, ale jak nie to już znaleziony!
                return  Rodzic.IsMyFamilyMember(Inny,Cappo,MaxLevel-1);  //REKURENCJA!
            }

    // Jak sprawdzenie nie poszło, z tego czy innego powodu, w górę to może nadal szukać:
    if(this==&Inny) //On sam okazał się szukanym!
        return true;

    //Jest najwyższym ojcem, teraz może szukać potomków...
    for(unsigned j=0;j<NeighSize();j++)
        if(Neighbourhood[j].Child==1) //Jest dzieckiem
        {
            HonorAgent& Dziecko=World[Neighbourhood[j].Y][Neighbourhood[j].X];
            bool res=Dziecko.IsMyFamilyMember(Inny,Cappo,-1);  //-1 zabezpiecza przed dalszą REKURENCJĄ
            if(res)
                return true;
        }

    // Nigdzie nie znalazł. To znaczy, że w jego pod-drzewie nie ma.
    return false;
}

/// Rodzinna zmiana reputacji. Dla siebie, oraz dla dzieci rekurencyjnie
void    HonorAgent::change_reputation_thru_family(double Delta)
{
    if(Delta>0) //Wzrost
    {
        HonorFeiRep+=(1-HonorFeiRep)*Delta;       	                                             assert(HonorFeiRep<=1);
    }
    else       //Spadek
    {
        HonorFeiRep-=HonorFeiRep*fabs(Delta);                                                 assert(0<=HonorFeiRep);
        assert(HonorFeiRep<=1);
    }
    // A teraz dla dzieci
    for(unsigned j=0;j<NeighSize();j++) //Czy jest może ojcem? Trzeba szukać potomków
        if(Neighbourhood[j].Child==1)   //Ten jest dzieckiem
        {
            HonorAgent& Dziecko=World[Neighbourhood[j].Y][Neighbourhood[j].X];
            Dziecko.change_reputation_thru_family(Delta); //REKURENCJA W DLA RODZINY
        }
}


//*//////////////////////////////////////////////////////////////////////////////
// Culture of honor evolution
//
// "The Evolutionary Basis of Honor Cultures"
//
// Contributors:
//    Andrzej Nowak Michele Gelfand Wojciech Borkowski
//*//////////////////////////////////////////////////////////////////////////////
