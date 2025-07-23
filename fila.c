#include "fila.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

DLL_EXPORT bool Fila_create(TFila *fila, int sizeElement, int max){
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

DLL_EXPORT void Fila_destroy(TFila *fila){
    free(fila->buffer);
    fila->size = 0;
    fila->sizeElement = 0;
    fila->maxElement = 0;
    fila->first = NULL;
    fila->last = NULL;
}

DLL_EXPORT bool Fila_put(TFila *fila, char *data){
    if (fila == NULL || data == NULL || fila->size >= fila->maxElement) return false;
    memcpy(fila->last, data, fila->sizeElement);
    fila->last += fila->sizeElement;
    if (fila->last >= fila->buffer + fila->maxElement * fila->sizeElement)
        fila->last = fila->buffer;
    fila->size++;
return true;
}

DLL_EXPORT bool Fila_get(TFila *fila, char *data){
    if (fila == NULL || data == NULL || fila->size == 0) return false;
    memcpy(data, fila->first, fila->sizeElement);
    fila->first += fila->sizeElement;
    if (fila->first >= fila->buffer + fila->maxElement * fila->sizeElement)
        fila->first = fila->buffer;
    fila->size--;
return true;
}

DLL_EXPORT bool Fila_isEmpty(TFila *fila){
return fila->size == 0;
}

DLL_EXPORT bool Fila_isFull(TFila *fila){
return fila->size == fila->maxElement;
}

DLL_EXPORT int Fila_size(TFila *fila){
return fila->size;
}

DLL_EXPORT void Fila_peek(TFila *fila, char *data){
    if(fila != NULL && data != NULL && fila->size > 0){
        memcpy(data, fila->first, fila->sizeElement);
    }
}