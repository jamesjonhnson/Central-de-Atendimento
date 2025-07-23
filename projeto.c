#include<stdio.h>
#include<stdlib.h>
#include<string.h>

// Definindo o tamanho máximo para nome e telefone
#define TAM_NOME 50
#define TAM_TEL 20

// Estrutura de uma chamada
typedef struct Chamada{
    char nome[TAM_NOME];
    char telefone[TAM_TEL];
    struct Chamada* prox;
}Chamada;

// Estrutura da fila
typedef struct{
    Chamada* inicio;
    Chamada* fim;
}Fila;

// Inicializa a fila (vazia)
void inicializarFila(Fila* fila){
    fila->inicio = NULL;
    fila->fim = NULL;
}

// Verifica se a fila está vazia
int filaVazia(Fila* fila){
return fila->inicio == NULL;
}

// Adiciona uma nova chamada à fila
void adicionarChamada(Fila* fila, char* nome, char* telefone){
    Chamada* nova = (Chamada*) malloc(sizeof(Chamada));
    if(nova == NULL){
        printf("Erro de alocacao de memoria.\n");
    return;
    }

    strcpy(nova->nome, nome);
    strcpy(nova->telefone, telefone);
    nova->prox = NULL;

    if(filaVazia(fila)){
        fila->inicio = nova;
    }else{
        fila->fim->prox = nova;
    }
    fila->fim = nova;
    printf("Chamada de %s adicionada a fila!\n", nome);
}

// Atende (remove) a primeira chamada da fila
void atenderChamada(Fila* fila){
    if(filaVazia(fila)){
        printf("Nenhuma chamada na fila para atender.\n");
    return;
    }
    Chamada* atendida = fila->inicio;
    printf("Atendendo chamada de %s (%s)...\n", atendida->nome, atendida->telefone);

    fila->inicio = atendida->prox;
    if(fila->inicio == NULL){
        fila->fim = NULL;
    }
    free(atendida);
}

// Lista todas as chamadas na fila
void listarChamadas(Fila* fila){
    if(filaVazia(fila)){
        printf("A fila esta vazia.\n");
    return;
    }
    printf("Chamadas na fila:\n");
    Chamada* atual = fila->inicio;
    int pos = 1;
    while(atual != NULL){
        printf("%d. %s (%s)\n", pos, atual->nome, atual->telefone);
        atual = atual->prox;
        pos++;
    }
}

// Mostra quem será o próximo a ser atendido
void verProximaChamada(Fila* fila){
    if(filaVazia(fila)) {
        printf("A fila esta vazia.\n");
    return;
    }
    printf("Proxima chamada: %s (%s)\n", fila->inicio->nome, fila->inicio->telefone);
}

// Libera toda a memória alocada (quando encerrar o programa)
void liberarFila(Fila* fila){
    while(!filaVazia(fila)){
        atenderChamada(fila);
    }
}

// Menu principal
void menu(){
    printf("\n==== Central de Atendimento ====\n");
    printf("1. Adicionar chamada\n");
    printf("2. Atender proxima chamada\n");
    printf("3. Ver proxima chamada\n");
    printf("4. Listar todas as chamadas\n");
    printf("5. Verificar se a fila esta vazia\n");
    printf("6. Encerrar\n");
    printf("Escolha uma opcao: ");
}

// Função principal
int main() {
    Fila fila;
    inicializarFila(&fila);

    int opcao;
    char nome[TAM_NOME];
    char telefone[TAM_TEL];
    do{
        menu();
        scanf("%d", &opcao);
        getchar(); // Consumir o \n deixado pelo scanf
        switch(opcao){
            case 1:
                printf("Nome do cliente: ");
                fgets(nome, TAM_NOME, stdin);
                nome[strcspn(nome, "\n")] = '\0'; // Remover \n

                printf("Telefone: ");
                fgets(telefone, TAM_TEL, stdin);
                telefone[strcspn(telefone, "\n")] = '\0';
                adicionarChamada(&fila, nome, telefone);
                break;
            case 2:
                atenderChamada(&fila);
                break;
            case 3:
                verProximaChamada(&fila);
                break;
            case 4:
                listarChamadas(&fila);
                break;
            case 5:
                if(filaVazia(&fila))
                    printf("A fila esta vazia.\n");
                else
                    printf("A fila contem chamadas.\n");
                break;
            case 6:
                printf("Encerrando o sistema...\n");
                liberarFila(&fila);
                break;
            default:
                printf("Opcao invalida.\n");
        }

    }while(opcao != 6);
return 0;
}
