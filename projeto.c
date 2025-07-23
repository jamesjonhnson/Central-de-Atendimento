#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// CÓDIGO DO PROFESSOR
typedef struct tagFila {
    char *buffer;
    char *first;
    char *last;
    int size;
    int sizeElement;
    int maxElement;
} TFila;

// Cria a fila com o tamanho do elemento e o número máximo de elementos
bool Fila_create(TFila *fila, int sizeElement, int max) {
    if (fila == NULL || sizeElement == 0 || max == 0) return false;

    fila->buffer = malloc(sizeElement * max);
    if (fila->buffer == NULL) return false;

    fila->size = 0;
    fila->sizeElement = sizeElement;
    fila->maxElement = max;
    fila->first = fila->buffer;
    fila->last = fila->buffer;
    return true;
}

// Libera a momória alocada para a fila
void Fila_destroy(TFila *fila) {
    free(fila->buffer);
    fila->size = 0;
    fila->sizeElement = 0;
    fila->maxElement = 0;
    fila->first = NULL;
    fila->last = NULL;
}

// Adiciona um elemento na fila
bool Fila_put(TFila *fila, char *data) {
    if (fila == NULL || data == NULL || fila->size >= fila->maxElement) return false;
    memcpy(fila->last, data, fila->sizeElement);
    fila->last += fila->sizeElement;
    if (fila->last >= fila->buffer + fila->maxElement * fila->sizeElement)
        fila->last = fila->buffer;
    fila->size++;
    return true;
}

// Remove um elemento da fila e coloca na variavel data
bool Fila_get(TFila *fila, char *data) {
    if (fila == NULL || data == NULL || fila->size == 0) return false;
    memcpy(data, fila->first, fila->sizeElement);
    fila->first += fila->sizeElement;
    if (fila->first >= fila->buffer + fila->maxElement * fila->sizeElement)
        fila->first = fila->buffer;
    fila->size--;
    return true;
}

// Verifica se a fila está vazia
bool Fila_isEmpty(TFila *fila) {
    return fila->size == 0;
}

// Verifica se a fila está cheia
bool Fila_isFull(TFila *fila) {
    return fila->size == fila->maxElement;
}

// Retorna o número de elementos na fila
int Fila_size(TFila *fila) {
    return fila->size;
}

// ESTRUTURA DE CHAMADA
typedef struct{
    char nome[50];
    char telefone[20];
}Chamada;

// MENU PRINCIPAL
void menu(){
    printf("\n=== Central de Atendimento ===\n");
    printf("1. Adicionar chamada\n");
    printf("2. Atender proxima chamada\n");
    printf("3. Ver proxima chamada\n");
    printf("4. Ver quantidade de chamadas\n");
    printf("5. Verificar se a fila esta vazia\n");
    printf("6. Verificar se a fila esta cheia\n");
    printf("7. Encerrar\n");
    printf("Escolha: ");
}

void lerString(char *str, int tam){
    fgets(str, tam, stdin);
    str[strcspn(str, "\n")] = '\0'; // remove '\n'
}

// ADICIONA UMA CHAMADA Á FILA
void adicionarChamada(TFila *fila){
    if(Fila_isFull(fila)){
        printf("A fila esta cheia!\n");
    return;
    }
    Chamada c;
    printf("Nome: ");
    lerString(c.nome, 50);
    printf("Telefone: ");
    lerString(c.telefone, 20);
    if(Fila_put(fila, (char*)&c))
        printf("Chamada adicionada!\n");
    else
        printf("Erro ao adicionar chamada.\n");
}

// ATENDE(REMOVE) A PRIMEIRA CHAMADA DA FILA
void atenderChamada(TFila *fila) {
    if (Fila_isEmpty(fila)) {
        printf("Nenhuma chamada para atender.\n");
    return;
    }
    Chamada c;
    if(Fila_get(fila, (char*)&c))
        printf("Atendendo chamada de %s (%s)\n", c.nome, c.telefone);
    else
        printf("Erro ao atender chamada.\n");
}

// MOSTRA QUEM SERÁ O PRÓXIMO A SER ATENDIDO
void verProxima(TFila *fila){
    if(Fila_isEmpty(fila)){
        printf("Fila vazia.\n");
    return;
    }
    Chamada c;
    memcpy(&c, fila->first, sizeof(Chamada));
    printf("Proxima chamada: %s (%s)\n", c.nome, c.telefone);
}

// FUNÇÃO PRINCIPAL
int main(){
    TFila fila;
    int capacidadeMaxima = 10;

    if(!Fila_create(&fila, sizeof(Chamada), capacidadeMaxima)){
        printf("Erro ao criar fila.\n");
        return 1;
    }
    int opcao;
    do{
        menu();
        scanf("%d", &opcao);
        getchar(); // consumir o \n deixado pelo sacnf
        switch(opcao){
            case 1:
                adicionarChamada(&fila);
                break;
            case 2: 
                atenderChamada(&fila);
                break;
            case 3:
                verProxima(&fila);
                break;
            case 4:
                printf("Total de chamadas: %d\n", Fila_size(&fila));
                break;
            case 5: 
            printf(Fila_isEmpty(&fila) ? "Fila vazia.\n" : "Fila com chamadas.\n");
            break;
            case 6: 
            printf(Fila_isFull(&fila) ? "Fila cheia.\n" : "Ainda ha espaco na fila.\n"); 
            break;
            case 7: printf("Encerrando...\n"); 
            break;
            default: printf("Opção inválida.\n");
        }
    }while(opcao != 7);

    Fila_destroy(&fila);
return 0;
}