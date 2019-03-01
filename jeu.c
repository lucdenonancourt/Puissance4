/*
	Canvas pour algorithmes de jeux à 2 joueurs

	joueur 0 : humain
	joueur 1 : ordinateur

*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Paramètres du jeu
#define LARGEUR_MAX 7 		// nb max de fils pour un noeud (= nb max de coups possibles)

#define TEMPS 3	// temps de calcul pour un coup avec MCTS (en secondes)

// macros
#define AUTRE_JOUEUR(i) (1-(i))
#define min(a, b)       ((a) < (b) ? (a) : (b))
#define max(a, b)       ((a) < (b) ? (b) : (a))

// Critères de fin de partie
typedef enum {NON, MATCHNUL, ORDI_GAGNE, HUMAIN_GAGNE } FinDePartie;

// Definition du type Etat (état/position du jeu)
typedef struct EtatSt {

	int joueur; // à qui de jouer ?

	char plateau[6][7];

} Etat;

// Definition du type Coup
typedef struct {

	int colonne;

} Coup;

// Copier un état
Etat * copieEtat( Etat * src ) {
	Etat * etat = (Etat *)malloc(sizeof(Etat));

	etat->joueur = src->joueur;

	int i,j;
	for (i=0; i < 6; i++)
		for ( j=0; j < 7; j++)
			etat->plateau[i][j] = src->plateau[i][j];



	return etat;
}

// Etat initial
Etat * etat_initial( void ) {
	Etat * etat = (Etat *)malloc(sizeof(Etat));

	int i,j;
	for (i=0; i< 6; i++)
		for ( j=0; j<7; j++)
			etat->plateau[i][j] = ' ';

	return etat;
}


void afficheJeu(Etat * etat) {

	int i,j;
	printf("   |");
	for ( j = 0; j < 7; j++)
		printf(" %d |", j);
	printf("\n");
	printf("--------------------------------");
	printf("\n");

	for(i=0; i < 6; i++) {
		printf(" %d |", i);
		for ( j = 0; j < 7; j++)
			printf(" %c |", etat->plateau[i][j]);
		printf("\n");
		printf("--------------------------------");
		printf("\n");
	}
}


// Nouveau coup
Coup * nouveauCoup(int j ) {
	Coup * coup = (Coup *)malloc(sizeof(Coup));

	coup->colonne = j;

	return coup;
}

// Demander à l'humain quel coup jouer
Coup * demanderCoup () {

	int j;
	printf(" quelle colonne ? ") ;
	scanf("%d",&j);

	return nouveauCoup(j);
}

// Modifier l'état en jouant un coup
// retourne 0 si le coup n'est pas possible
int jouerCoup( Etat * etat, Coup * coup ) {

	/* On peut jouer le coup si il reste de la place sur la colonne */
	for(int i = 6; i >= 0; i--){
		if(etat->plateau[i][coup->colonne] == ' '){
			etat->plateau[i][coup->colonne] = etat->joueur ? 'O' : 'X';
			// à l'autre joueur de jouer
			etat->joueur = AUTRE_JOUEUR(etat->joueur);
			return 1;
		}
	}

	return 0;
}

// Retourne une liste de coups possibles à partir d'un etat
// (tableau de pointeurs de coups se terminant par NULL)
Coup ** coups_possibles( Etat * etat ) {

	Coup ** coups = (Coup **) malloc((1+LARGEUR_MAX) * sizeof(Coup *) );

	int k = 0;

	int j;
		for (j=0; j < 7; j++) {
			if ( etat->plateau[0][j] == ' ' ) {
				coups[k] = nouveauCoup(j);
				k++;
			}
	}
	/* fin de l'exemple */
	coups[k] = NULL;
	return coups;
}

// Retourne le nombre de coups possible depuis un etat
int nb_coups_possibles( Etat * etat ) {

	int k = 0;
	int j;
		for (j=0; j < 7; j++) {
			if ( etat->plateau[0][j] == ' ' ) {
				k++;
			}
	}

	return k;
}


// Definition du type Noeud
typedef struct NoeudSt {

	int joueur; // joueur qui a joué pour arriver ici
	Coup * coup;   // coup joué par ce joueur pour arriver ici

	Etat * etat; // etat du jeu

	struct NoeudSt * parent;
	struct NoeudSt * enfants[LARGEUR_MAX]; // liste d'enfants : chaque enfant correspond à un coup possible
	int nb_enfants;	// nb d'enfants présents dans la liste

	// POUR MCTS:
	double nb_victoires;
	double nb_simus;

} Noeud;


// Créer un nouveau noeud en jouant un coup à partir d'un parent
// utiliser nouveauNoeud(NULL, NULL) pour créer la racine
Noeud * nouveauNoeud (Noeud * parent, Coup * coup ) {
	Noeud * noeud = (Noeud *)malloc(sizeof(Noeud));

	if ( parent != NULL && coup != NULL ) {
		noeud->etat = copieEtat ( parent->etat );
		jouerCoup ( noeud->etat, coup );
		noeud->coup = coup;
		noeud->joueur = AUTRE_JOUEUR(parent->joueur);
	}
	else {
		noeud->etat = NULL;
		noeud->coup = NULL;
		noeud->joueur = 0;
	}
	noeud->parent = parent;
	noeud->nb_enfants = 0;

	// POUR MCTS:
	noeud->nb_victoires = 0;
	noeud->nb_simus = 0;


	return noeud;
}

// Ajouter un enfant à un parent en jouant un coup
// retourne le pointeur sur l'enfant ajouté
Noeud * ajouterEnfant(Noeud * parent, Coup * coup) {
	Noeud * enfant = nouveauNoeud (parent, coup ) ;
	parent->enfants[parent->nb_enfants] = enfant;
	parent->nb_enfants++;
	return enfant;
}

void freeNoeud ( Noeud * noeud) {
	if ( noeud->etat != NULL)
		free (noeud->etat);

	while ( noeud->nb_enfants > 0 ) {
		freeNoeud(noeud->enfants[noeud->nb_enfants-1]);
		noeud->nb_enfants --;
	}
	if ( noeud->coup != NULL)
		free(noeud->coup);

	free(noeud);
}


// Test si l'état est un état terminal
// et retourne NON, MATCHNUL, ORDI_GAGNE ou HUMAIN_GAGNE
FinDePartie testFin( Etat * etat ) {

	// tester si un joueur a gagné
	int i,j,k,n = 0;
	for ( i=0;i < 6; i++) {
		for(j=0; j < 7; j++) {
			if ( etat->plateau[i][j] != ' ') {
				n++;	// nb coups joués

				// lignes
				k=0;
				while ( k < 4 && i+k <= 7 && etat->plateau[i+k][j] == etat->plateau[i][j] )
					k++;
				if ( k == 4 )
					return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

				// colonnes
				k=0;
				while ( k < 4 && j+k <= 6 && etat->plateau[i][j+k] == etat->plateau[i][j] )
					k++;
				if ( k == 4 )
					return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

				// diagonales
				k=0;
				while ( k < 4 && i+k < 7 && j+k < 6 && etat->plateau[i+k][j+k] == etat->plateau[i][j] )
					k++;
				if ( k == 4 )
					return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

				k=0;
				while ( k < 4 && i+k < 7 && j-k >= 0 && etat->plateau[i+k][j-k] == etat->plateau[i][j] )
					k++;
				if ( k == 4 )
					return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;
			}
		}
	}

	// et sinon tester le match nul
	if ( n == 6*7 )
		return MATCHNUL;

	return NON;
}


double calculer_B_Valeur(Noeud * noeud){
	if(noeud->nb_simus == 0)
		return 999999999;

	double ui = noeud->nb_victoires / noeud->nb_simus;
	if(noeud->parent->joueur == 1)
			ui *= -1;
	//On fixe la constante c a Racine de 2
	double c = sqrt(2);

	double res = ui + c * sqrt(log(noeud->parent->nb_simus)/noeud->nb_simus);
	return res;
}


// Calcule et joue un coup de l'ordinateur avec MCTS-UCT
// en tempsmax secondes
void ordijoue_mcts(Etat * etat, int tempsmax) {

	clock_t tic, toc;
	tic = clock();
	int temps;

	Coup ** coups;
	Coup * meilleur_coup ;

	// Créer l'arbre de recherche
	Noeud * racine = nouveauNoeud(NULL, NULL);
	racine->etat = copieEtat(etat);
	Etat * etat_simulation = copieEtat(etat);
 //meilleur_coup = coups[ rand()%k ]; // choix aléatoire

	int iter = 0;

	//Noeud courant
	Noeud * courant = racine;

	//Simulation
	int simu = 0;

	//Etat de simulation
	do {
		if(simu == 0){
			//Si le noeud est completement développé, on passe au suivant
			if(courant->nb_enfants == nb_coups_possibles(courant->etat) && courant->nb_enfants > 0){
				//On prend le meilleur noeud suivant
				Noeud * prochain = courant->enfants[0];
				double score = calculer_B_Valeur(courant->enfants[0]);
				for(int i = 1; i < courant->nb_enfants; i++){
					if(calculer_B_Valeur(courant->enfants[i]) > score){
						//On ne parcourt jamais un noeud final
						if(testFin(courant->enfants[i]->etat) == NON){
							prochain = courant->enfants[i];
							score = calculer_B_Valeur(courant->enfants[i]);
						}
					}
				}
				courant = prochain;
			}
			else{
				//SI aucun coup ne ne peut etre joué, on remonte l'arbre en update le nb visite
				if(nb_coups_possibles(courant->etat) == 0){
					courant->nb_simus = courant->nb_simus + 1;
					while(courant->parent != NULL){
						courant = courant->parent;
						courant->nb_simus = courant->nb_simus + 1;
					}
					continue;
				}

				//On développe un noeud qui n'a pas été développé
				int possible[7] = {0};

				//Coup deja joués
				for(int i = 0; i < courant->nb_enfants; i++){
						possible[courant->enfants[i]->coup->colonne] = 1;
				}

				//Coups possibles
				Coup ** coups = coups_possibles(courant->etat);
				int iterator = 0;
				for(iterator; iterator < nb_coups_possibles(courant->etat); iterator++){
					if(possible[coups[iterator]->colonne] == 0){
						break;
					}
				}

				//On peut donc jouer le coup iterator
				Coup * c = nouveauCoup(coups[iterator]->colonne);

				//On creer un enfant avec ce coup
				courant = ajouterEnfant(courant, c);
				etat_simulation = copieEtat(courant->etat);
				//On passe au mode simulation
				simu = 1;

			}
		}else{
			//Mode Simulation
			//On regarde si on est arrivé a une feuille
			FinDePartie fin = testFin(etat_simulation);
			if(fin != NON){
				if ( fin == ORDI_GAGNE ){
						//Si on a gagné, on remonte l'arbre en ajustant les valeurs des noeuds
						courant->nb_victoires += 1;
						courant->nb_simus += 1;
						while(courant->parent != NULL){
							courant = courant->parent;
							courant->nb_simus += 1;
							courant->nb_victoires += 1;
						}
						simu = 0;
				}
				if ( fin == HUMAIN_GAGNE || fin == MATCHNUL ){
						//Si on a perdu, on met juste a jour le nombre de visite.
						courant->nb_simus = courant->nb_simus + 1;
						while(courant->parent != NULL){
							courant = courant->parent;
							courant->nb_simus = courant->nb_simus + 1;
						}
						simu = 0;
				}
			}else{
				//Sinon on joue un coup aléatoire
				coups = coups_possibles(etat_simulation);
				Coup * c = coups[rand()%nb_coups_possibles(etat_simulation)];
				//On joue ce coup sur l'état de simulation
				jouerCoup(etat_simulation, c);
			}
		}
		toc = clock();
		temps = (int)( ((double) (toc - tic)) / CLOCKS_PER_SEC );
		iter ++;
	} while ( temps < tempsmax );


	Noeud * prochain = NULL;
	double score = -1;

	//On parcourt les différents coups possibles et on prend le meilleur
	for(int i = 0; i < racine->nb_enfants; i++){
		//Premiere optimisation hors-MCTS : On choisit toujours le choix gagnant
		if(testFin(racine->enfants[i]->etat) == ORDI_GAGNE){
			prochain = racine->enfants[i];
			break;
		}

		if(calculer_B_Valeur(racine->enfants[i]) > score){
			//Si on ne peut pas jouer ce coup, on ne le sauvegarde pas
			if(etat->plateau[0][racine->enfants[i]->coup->colonne] == ' '){
					prochain = racine->enfants[i];
				score = calculer_B_Valeur(racine->enfants[i]);
			}
		}
	}

		/**  MAX **/
		/*
		for(int i = 0; i < racine->nb_enfants; i++){
			double ui = racine->enfants[i]->nb_victoires / racine->enfants[i]->nb_simus;
			if(ui > score){
				//Si on ne peut pas jouer ce coup, on ne le sauvegarde pas
				if(etat->plateau[0][racine->enfants[i]->coup->colonne] == ' '){
					prochain = racine->enfants[i];
					score = ui;
				}
			}
		}
		*/

			/** ROBUSTE **/
			/*
			for(int i = 0; i < racine->nb_enfants; i++){
				if(racine->enfants[i]->nb_simus > score){
					//Si on ne peut pas jouer ce coup, on ne le sauvegarde pas
					if(etat->plateau[0][racine->enfants[i]->coup->colonne] == ' '){
						prochain = racine->enfants[i];
						score = racine->enfants[i]->nb_simus;
					}
				}
			}
			*/



	printf("Nombre de simulation pour calculer le coup = %f\n", racine->nb_simus);
	printf("Probabilité de victoire %f%% \n", (prochain->nb_victoires/prochain->nb_simus) * 100);
	// Jouer le meilleur premier coup
	jouerCoup(etat, prochain->coup);

	// Penser à libérer la mémoire :
	freeNoeud(racine);
	free (coups);
}

int main(void) {

	Coup * coup;
	FinDePartie fin;

	// initialisation
	Etat * etat = etat_initial();

	// Choisir qui commence :
	printf("Qui commence (0 : humain, 1 : ordinateur) ? ");
	scanf("%d", &(etat->joueur) );

	// boucle de jeu
	do {
		printf("\n");
		afficheJeu(etat);

		if ( etat->joueur == 0 ) {
			// tour de l'humain

			do {
				coup = demanderCoup();
			} while ( !jouerCoup(etat, coup) );

		}
		else {
			// tour de l'Ordinateur

			ordijoue_mcts( etat, TEMPS );

		}

		fin = testFin( etat );
	}	while ( fin == NON ) ;

	printf("\n");
	afficheJeu(etat);

	if ( fin == ORDI_GAGNE )
		printf( "** L'ordinateur a gagné **\n");
	else if ( fin == MATCHNUL )
		printf(" Match nul !  \n");
	else
		printf( "** BRAVO, l'ordinateur a perdu  **\n");
	return 0;
}
