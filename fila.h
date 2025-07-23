#ifndef FILA_H
#define FILA_H

#include <stdbool.h>

#ifdef _WIN32
#define DLL_EXPORT __declspec(dllimport)
#else
#define DLL_EXPORT
#endif

typedef struct{
    char *buffer;
    char *first;
    char *last;
    int size;
    int sizeElement;
    int maxElement;
}TFila;

typedef struct{
    char nome[50];
    char telefone[20];
}Chamada;

// Funções exportadas
DLL_EXPORT bool Fila_create(TFila *fila, int sizeElement, int max);
DLL_EXPORT void Fila_destroy(TFila *fila);
DLL_EXPORT bool Fila_put(TFila *fila, char *data);
DLL_EXPORT bool Fila_get(TFila *fila, char *data);
DLL_EXPORT bool Fila_isEmpty(TFila *fila);
DLL_EXPORT bool Fila_isFull(TFila *fila);
DLL_EXPORT int  Fila_size(TFila *fila);
DLL_EXPORT void Fila_peek(TFila *fila, char *data);

#endif