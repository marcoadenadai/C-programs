#include <stdio.h>
#include <stdlib.h>

#define INFINITO 1073741823
//------------------------------------------------------

typedef struct adjacencia {
	int vertice;
	int peso;
	int rua;
	struct adjacencia * prox;
} ADJACENCIA;

typedef struct VERTICE {
	//dados individuais de cada nó
	int dist_orig; //distancia da origem
	int antecessor; //antecessor no menor caminho
	int status; // 0=aberto, 1=fechado
	ADJACENCIA * cab; //cabeca fila de adjacencia
} VERTICE;

typedef struct grafo {
	int vertices;
	int arestas;
	VERTICE * v;
} GRAFO;
//------------------------------------------------------
GRAFO * cria_Grafo(int n);
ADJACENCIA * cria_Adj(int n, int peso);
int cria_Aresta(GRAFO * g, int vi, int vf, int peso);
void imprime_Grafo(GRAFO *g);
//------------------------------------------------------
void inicializa_Dijkstra(GRAFO * G, int s);
int existe_no_aberto(GRAFO * G);
int menor_aberto(GRAFO * G);
void relaxa_arestas_adj(GRAFO * G, int n);
int * dijkstra(GRAFO * G, int s, int e);
//------------------------------------------------------
