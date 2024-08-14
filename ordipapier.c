// *******************************************************
// Nom ......... : ordipapier
// Rôle ........ : un émulateur pour "l'ordinateur en papier"
// Auteur ...... : Rosalie Duteuil
// Version ..... : V1.4 du 05/08/2024
// Licence ..... : réalisé dans le cadre du cours de d'Architecture des ordinateurs
// Compilation . : gcc -Wall -o ordipapier ordipapier.c
// Usage ....... : Pour exécuter : ./ordipapier <fichier>
// ********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>		// pour strchr

#define taille_memoire 256		// taille de la mémoire en octets 
#define NB_OPS 21				// nombre d'opérations gérées (type ADD # etc)
#define MAX_MC 11				// nombre max de microcodes pour une opération
#define iter_max 5000 			// nb max d'itérations ; -1 pour boucle infinie

typedef char * str ;			// type string
typedef unsigned char adr ; 	// type adresse (0 à 255)

enum erreur {			// codes d'erreur (cf fonction usage())
	INV_FILE_ERR = 1,
	FILE_LEN_ERR,
	ARG_ERR,
	INV_OP_ERR,
	EXEC_ERR } ;

enum operation {ADD, SUB, NAND} ;						// pour l'UAL
enum micro_op_status {OK, INV_CODE, ABORT, SKIP_P3} ;	// voir fonction microp()

struct assoc {adr codeop ; int ops_tab[MAX_MC] ; } ;	// associations code opération - liste de microcodes

adr memoire[taille_memoire] ;	// mémoire = tableau d'entiers non signés sur 8 bits
adr PC, A, OP, AD, RS, RM ;		// les registres
adr entree ;					// dernière saisie utilisateur
enum operation UAL;				// type d'opération à effectuer dans l'UAL

struct assoc table_corres[NB_OPS] = {
	{ 0, {1, 13, 3, 0} },
	{ 16, {1, 13, 2, 0} },	
	{ 17, {18, 1, 13, 2, 0} },		// avec microcode supplémentaire pour le test
	{ 18, {19, 1, 13, 2, 0} }, 		// idem
	{ 32, {10, 1, 13, 12, 0} },
	{ 33, {11, 1, 13, 12, 0} },
	{ 34, {17, 1, 13, 12, 0} },
	{ 64, {1, 13, 6, 7, 13, 3, 0} },
	{ 65, {1, 13, 6, 7, 13, 9, 0} },
	{ 72, {1, 13, 6, 7, 4, 14, 0} },
	{ 73, {1, 13, 6, 7, 16, 8, 14, 0} },
	{ 96, {10, 1, 13, 6, 7, 13, 12, 0} },
	{ 97, {11, 1, 13, 6, 7, 13, 12, 0} },
	{ 98, {17, 1, 13, 6, 7, 13, 12, 0} },
	{ 192, {1, 13, 6, 7, 13, 6, 7, 13, 3, 0} },
	{ 193, {1, 13, 6, 7, 13, 6, 7, 13, 9, 0} },
	{ 200, {1, 13, 6, 7, 13, 6, 7, 4, 14, 0} },
	{ 201, {1, 13, 6, 7, 13, 6, 7, 16, 8, 14, 0} },
	{ 224, {10, 1, 13, 6, 7, 13, 6, 7, 13, 12, 0} },
	{ 225, {11, 1, 13, 6, 7, 13, 6, 7, 13, 12, 0} },
	{ 226, {17, 1, 13, 6, 7, 13, 6, 7, 13, 12, 0} } };

int get_prgm(str) ; 			
int execute() ;
int get_ops_tab(int *);
int exec_ops_tab(int *);
int microp(int);
adr lire();
int calculer();
void usage(int) ;


int main(int argc, str argv[]) {
	if (argc < 2) usage(ARG_ERR) ;		// manque le nom de fichier
	get_prgm(argv[1]) ;					// récupération du programme
	if (execute()) usage(EXEC_ERR);		// exécution
	return 0 ; 
}

/* Récupère un programme depuis le fichier spécifié */
int get_prgm(str fichier) {	

	/* ouvrir le programme */
	FILE * flux = fopen(fichier, "r") ;		// ouvre le texte
	if (! flux) {							// impossible de lire le fichier
		perror("Erreur d'ouverture du fichier ") ;	
		exit(-1) ; }		

	/* récupérer l'adresse de chargement */
	printf("Adresse de chargement du programme [en hexadécimal] : ");
	adr debut_ram = lire() ;
	
	/* lecture du code (octet par octet) */
	int i = (int) debut_ram ;	// index (prochain emplacement disponible en mémoire)
	int lu; 
	unsigned int code ;			// opcode (bigramme hexa) lu dans le fichier
	while (i < taille_memoire + 1){	
		// caractère non conforme
		if (!(lu = fscanf(flux, "%2X", &code)))	usage(INV_FILE_ERR) ; 
		else if (lu == -1) break ;			// fin du fichier
		else memoire[i++] = (adr)code; }	// sinon : inscription en mémoire
	fclose(flux) ;

	/* si i > mémoire dispo : le programme est trop long */
	if (i == taille_memoire + 1) usage(FILE_LEN_ERR) ;
	
	PC = debut_ram ;		// initialisation du PC
	return 0 ;
}				

/* Boucle principale d'éxecution */
int execute(){	

	int phase1[] = {1, 13, 5, 15, 0} ;				// microcodes de la phase 1
	int *phase2 = malloc (sizeof(int) * MAX_MC);	// microcodes de la phase 2
	
	for(int k = 0 ; k != iter_max ; k++) {		// boucle finie ou non (cf iter_max)
		/* phase 1 */
		exec_ops_tab(phase1);	

		/* phase 2 */
		if (get_ops_tab(phase2) == -1) break ;	// récupère liste de microcodes
		if (exec_ops_tab(phase2) != SKIP_P3) 		// exécute les microcodes
			/* phase 3 (si besoin, ie pas un saut) */
			microp(15) ;		// incrémenter PC

		if (PC == 0) break ;	// on a atteint la fin du programme
	}
	free(phase2);
	return 0 ; 
}

/* Cherche l'opération OP dans la table de correspondances et la copie dans le tableau */
int get_ops_tab(int * tab){
	int i = 0 ;

	/* Chercher l'opération dans la table de correspondance */
	while (i < NB_OPS){
		if (table_corres[i].codeop == OP){
			memcpy(tab, table_corres[i].ops_tab, sizeof(int) * MAX_MC) ; // copie dans le vecteur
			return 0; }
		i++ ; }

	usage(INV_OP_ERR);		// opération non trouvée
	return -1 ;
}

/* Exécute une série de microcodes, fournis dans un tableau */
int exec_ops_tab(int * tab){
	int i = 0, status ;
	
	/* exécuter chaque microcode de la liste (terminée par 0) */
	while (i < MAX_MC && tab[i]) {
		status = microp(tab[i++]);					// exécuter le code
		if (status == INV_CODE) usage(EXEC_ERR) ;	// erreur (microcode erroné)
		else if (status == ABORT) break;	// arrêter l'exécution (condition non remplie, pour un saut)
	}
	return status ;		// OK ou SKIP_P3
}

/* Exécute un microcode */
int microp(int microcode){
	switch (microcode)
	{
		case 1 :  RS = PC ; break ;
		case 2 :  PC = RM ; return SKIP_P3 ;	// ne pas faire la phase 3
		case 3 :  A = RM ; break ;
		case 4 :  RM = A ; break ;
		case 5 :  OP = RM ; break ;
		case 6 :  AD = RM ; break ;
		case 7 :  RS = AD ; break ;
		case 8 :  RM = entree ; break ;
		case 9 :  printf("x%02X\t[%i]\n", RM, RM) ; break ;
		case 10 : UAL = ADD ; break ;
		case 11 : UAL = SUB ; break ;
		case 17 : UAL = NAND ; break ;
		case 12 : calculer() ; break ;
		case 13 : RM = memoire[RS] ; break ;
		case 14 : memoire[RS] = RM ; break ;
		case 15 : PC++ ; break ;
		case 16 : entree = lire() ; break ;
		case 18 : if (A < 128) return ABORT ; break ;
		case 19 : if (A != 0) return ABORT ; break ;
		default : return INV_CODE ;
	}
	return OK;
}

/* Récupère une saisie utilisateur (entier entre 0 et 255 en codage hexadécimal) */
adr lire(){	
	int lu, n;
	char buffer[3] ;
	
	while (1) {			// attente d'une saisie correcte
		/* récupérer un bigramme */
		fgets(buffer, 3, stdin) ;				// récupérer jusqu'à 3 caractères
		n = sscanf(buffer, "%2X", &lu) ;		// en extraire un nombre	
		
		/* nettoyer le tampon */
		if (strchr(buffer, '\n') == NULL) 		// s'il reste des caractères non lus sur la ligne ...
			while (getchar() != '\n') continue;	// ... vider la ligne
		
		if (n == 1) break ;				// la saisie est correcte --> fini
        else printf("Saisir un nombre entre 00 et FF\n");	// saisie incorrecte
	}
	return (adr) lu ; 
}		

/* Effectue le calcul dans l'UAL */
int calculer(){
	switch(UAL){
		case ADD : A += RM ; break ;
		case SUB : A -= RM ; break ;
		case NAND : A =  ~(A & RM) ; break ;	
		default : return -1; }
	return 0; 
}

/* Affichage des messages d'erreur */
void usage(int erreur) {	
	switch(erreur){	
		case INV_FILE_ERR : fprintf(stderr,"Erreur : le code contient des caractères non conformes.\nArrêt du programme...\n") ; break ;
		case FILE_LEN_ERR : fprintf(stderr,"Erreur : le code est trop long pour la mémoire (256 o).\nArrêt du programme...\n") ; break ; 
		case ARG_ERR : fprintf(stderr,"Usage : ordipapier <fichier>\n") ; break ;
		case INV_OP_ERR : fprintf(stderr,"Erreur : opération inconnue (x%02X)\nArrêt du programme...\n", OP) ; break ;
		case EXEC_ERR : fprintf(stderr,"Erreur : échec de l'exécution de l'opération x%02X\nArrêt du programme...\n", OP) ;} 
	exit(-1) ;
}
