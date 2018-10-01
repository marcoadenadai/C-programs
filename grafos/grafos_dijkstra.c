#include "grafos_dijkstra.h"

int existe_no_aberto(GRAFO * G){
	int i;
	for(i=0;i<(G->vertices);i++){
		if(G->v[i].status==0)
			return 1;
	}
	return 0;
}

void inicializa_Dijkstra(GRAFO * G, int s){
	int i;
	for(i=0;i<(G->vertices);i++){
		G->v[i].dist_orig = INFINITO;
		G->v[i].antecessor = -1;//NULL;
		G->v[i].status = 0;
	}
	G->v[s].dist_orig = 0;
}

int menor_aberto(GRAFO * G){
	int i,menor;
	for(i=0;i<(G->vertices);i++){
		if(G->v[i].status == 0){
			break;
		}
	}
	if(i == G-> vertices){
		return -1;
	}
	menor = i;
	for(i=menor+1;i<(G->vertices);i++){
		if(G->v[i].status == 0 && (G->v[i].dist_orig <= G->v[menor].dist_orig)){
			menor = i;
		}
	}
	return menor;
}

void relaxa_arestas_adj(GRAFO * G, int n){
	//relaxo todas as arestas vizinhas do nó n
	ADJACENCIA * ad = G->v[n].cab;
	while(ad){
		//if(ad->vertice != n){
			if(G->v[n].dist_orig + ad->peso < G->v[(ad->vertice)].dist_orig){
				//se a distancia conhecida + peso da aresta < dist estimada para o alvo
				G->v[(ad->vertice)].dist_orig = G->v[n].dist_orig + ad->peso;
				G->v[(ad->vertice)].antecessor = n;
			}
		//}
		ad = ad->prox;
	}
}

int * dijkstra(GRAFO * G, int s, int e){
	inicializa_Dijkstra(G,s);
	int n;
	while(existe_no_aberto(G) == 1){
		n=menor_aberto(G);
		G->v[n].status = 1; //fecho o menor nó aberto
		relaxa_arestas_adj(G,n);
	}
	
	int * caminho = (int *) malloc(sizeof(int));
	int x,tam=1,tmp;
	for(x=e;x!=-1;x=(G->v[x].antecessor), tam++){
		caminho = (int *) realloc(caminho,tam*sizeof(int));
		caminho[tam-1]=x;
	}
	tam--;
	for(x=0;x<tam/2;x++){
		tmp=caminho[x];
		caminho[x]=caminho[tam-1-x];
		caminho[tam-1-x]=tmp;	
	}

	tam++;
	caminho = (int *) realloc(caminho,(tam)*sizeof(int));
	caminho[tam-1]=-1;
	return caminho;
	//retorna array de inteiros para o menor caminho com ultimo valor do vetor = -1 para indicar fim
}
//------------------------------------------------------
GRAFO * cria_Grafo(int n){
	GRAFO * g = (GRAFO *) malloc(sizeof(GRAFO));
	g->vertices = n;
	g->arestas = 0;
	g->v = (VERTICE *) malloc(n*sizeof(VERTICE));
	int i;
	for(i=0;i<n;i++){
		g->v[i].cab = NULL;
	}
	return(g);
}

ADJACENCIA * cria_Adj(int n, int peso){
	ADJACENCIA * temp = (ADJACENCIA *) malloc(sizeof(ADJACENCIA));
	temp->vertice = n;
	temp->peso = peso;
	temp->prox = NULL;
	return (temp);
}

int cria_Aresta(GRAFO * g, int vi, int vf, int peso){
	if (g==NULL) return 0;
	if (vf<0 || vf >= (g->vertices)) return 0;
	if (vi<0 || vi >= (g->vertices)) return 0;
	ADJACENCIA * novo = cria_Adj(vf,peso);
	novo->prox = g->v[vi].cab;
	g->v[vi].cab = novo;
	g->arestas++;
	return 1;
}

void imprime_Grafo(GRAFO *g){
	printf("Existem %d vertice(s), %d aresta(s) no grafo\n",g->vertices,g->arestas);
	int i;
	ADJACENCIA * ad;
	for(i=0;i<(g->vertices);i++){
		printf("v%d: ",i);
		ad = g->v[i].cab;
		while(ad){
			printf("v%d(%d) ",ad->vertice,ad->peso);
			ad=ad->prox;
		}
		printf("\n");
	}
}