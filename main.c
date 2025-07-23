#include <stdio.h>
#include "fila.h"

int main(){
    TFila fila;
    Chamada chamada = {"Lucas", "12345-6789"};
    Chamada proxima;

    if(!Fila_create(&fila, sizeof(Chamada), 10)){
        printf("Erro ao criar fila.\n");
    return 1;
    }

    Fila_put(&fila, (char*)&chamada);

    Fila_peek(&fila, (char*)&proxima);
    printf("Proxima chamada: %s (%s)\n", proxima.nome, proxima.telefone);

    Fila_destroy(&fila);
return 0;
}
