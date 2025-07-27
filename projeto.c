#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <microhttpd.h>
#include <jansson.h>
#include <pthread.h>

// === ESTRUTURA DE FILA E CHAMADA ===
typedef struct tagFila {
    char *buffer;
    char *first;
    char *last;
    int size;
    int sizeElement;
    int maxElement;
} TFila;

typedef struct {
    char nome[50];
    char telefone[20];
} Chamada;

typedef struct {
    char buffer[4096];
    size_t offset;
} RequestData;

TFila fila;
int capacidadeMaxima = 10;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// === FUNÇÕES DA FILA ===
bool Fila_create(TFila *fila, int sizeElement, int max) {
    if (!fila || sizeElement == 0 || max == 0) return false;
    fila->buffer = malloc(sizeElement * max);
    if (!fila->buffer) return false;
    fila->size = 0;
    fila->sizeElement = sizeElement;
    fila->maxElement = max;
    fila->first = fila->buffer;
    fila->last = fila->buffer;
    return true;
}

void Fila_destroy(TFila *fila) {
    free(fila->buffer);
    fila->buffer = NULL;
    fila->size = 0;
    fila->sizeElement = 0;
    fila->maxElement = 0;
    fila->first = NULL;
    fila->last = NULL;
}

bool Fila_put(TFila *fila, char *data) {
    if (!fila || !data || fila->size >= fila->maxElement) return false;
    memcpy(fila->last, data, fila->sizeElement);
    fila->last += fila->sizeElement;
    if (fila->last >= fila->buffer + fila->maxElement * fila->sizeElement)
        fila->last = fila->buffer;
    fila->size++;
    return true;
}

bool Fila_get(TFila *fila, char *data) {
    if (!fila || !data || fila->size == 0) return false;
    memcpy(data, fila->first, fila->sizeElement);
    fila->first += fila->sizeElement;
    if (fila->first >= fila->buffer + fila->maxElement * fila->sizeElement)
        fila->first = fila->buffer;
    fila->size--;
    return true;
}

bool Fila_isEmpty(TFila *fila) {
    return fila->size == 0;
}

bool Fila_isFull(TFila *fila) {
    return fila->size == fila->maxElement;
}

int Fila_size(TFila *fila) {
    return fila->size;
}

// === API HANDLER CORRIGIDO ===
static enum MHD_Result api_handler(void *cls, struct MHD_Connection *connection,
                       const char *url, const char *method,
                       const char *version, const char *upload_data,
                       size_t *upload_data_size, void **con_cls)
{
    struct MHD_Response *response;
    enum MHD_Result ret;

    // Preflight CORS
    if (strcmp(method, "OPTIONS") == 0) {
        response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Access-Control-Allow-Methods", "POST, GET, OPTIONS, DELETE");
        MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type");
        ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    // Aloca memória para a requisição
    if (*con_cls == NULL) {
        RequestData *data = calloc(1, sizeof(RequestData));
        *con_cls = data;
        return MHD_YES;
    }

    RequestData *reqData = *con_cls;

    // POST /chamadas
    if (strcmp(method, "POST") == 0 && strcmp(url, "/chamadas") == 0) {
        if (*upload_data_size > 0) {
            // Copia dados recebidos
            if (reqData->offset + *upload_data_size >= sizeof(reqData->buffer)) {
                *upload_data_size = 0;
                return MHD_NO;
            }
            memcpy(reqData->buffer + reqData->offset, upload_data, *upload_data_size);
            reqData->offset += *upload_data_size;
            *upload_data_size = 0;
            return MHD_YES;
        }

        // Fim do upload, processar JSON
        reqData->buffer[reqData->offset] = '\0';
        json_error_t error;
        json_t *root = json_loads(reqData->buffer, 0, &error);
        if (!root) {
            printf("Erro JSON: %s (linha %d, coluna %d)\n", error.text, error.line, error.column);
            const char *err = "{\"error\":\"JSON inválido\"}";
            response = MHD_create_response_from_buffer(strlen(err), (void *)err, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            free(reqData);
            *con_cls = NULL;
            return ret;
        }

        const char *nome, *telefone;
        if (json_unpack(root, "{s:s, s:s}", "nome", &nome, "telefone", &telefone) != 0) {
            json_decref(root);
            const char *err = "{\"error\":\"Campos inválidos\"}";
            response = MHD_create_response_from_buffer(strlen(err), (void *)err, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            free(reqData);
            *con_cls = NULL;
            return ret;
        }

        Chamada nova;
        strncpy(nova.nome, nome, sizeof(nova.nome) - 1);
        strncpy(nova.telefone, telefone, sizeof(nova.telefone) - 1);

        bool success;
        pthread_mutex_lock(&mutex);
        success = Fila_put(&fila, (char *)&nova);
        pthread_mutex_unlock(&mutex);

        json_decref(root);

        if (!success) {
            const char *err = "{\"error\":\"Fila cheia\"}";
            response = MHD_create_response_from_buffer(strlen(err), (void *)err, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            ret = MHD_queue_response(connection, MHD_HTTP_CONFLICT, response);
            MHD_destroy_response(response);
        } else {
            const char *ok = "{\"status\":\"Chamada adicionada\"}";
            response = MHD_create_response_from_buffer(strlen(ok), (void *)ok, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            ret = MHD_queue_response(connection, MHD_HTTP_CREATED, response);
            MHD_destroy_response(response);
        }

        free(reqData);
        *con_cls = NULL;
        return ret;
    }

    // DELETE /chamadas/proxima
    if (strcmp(method, "DELETE") == 0 && strcmp(url, "/chamadas/proxima") == 0) {
        Chamada atendida;
        bool success;

        pthread_mutex_lock(&mutex);
        success = Fila_get(&fila, (char *)&atendida);
        pthread_mutex_unlock(&mutex);

        if (success) {
            json_t *res = json_object();
            json_object_set_new(res, "nome", json_string(atendida.nome));
            json_object_set_new(res, "telefone", json_string(atendida.telefone));
            char *json_str = json_dumps(res, 0);
            json_decref(res);

            response = MHD_create_response_from_buffer(strlen(json_str), json_str, MHD_RESPMEM_MUST_FREE);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            return ret;
        } else {
            const char *msg = "{\"error\":\"Fila vazia\"}";
            response = MHD_create_response_from_buffer(strlen(msg), (void *)msg, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            MHD_add_response_header(response, "Content-Type", "application/json");
            ret = MHD_queue_response(connection, MHD_HTTP_NO_CONTENT, response);
            MHD_destroy_response(response);
            return ret;
        }
    }

    // Rota desconhecida
    const char *msg = "{\"error\":\"Bad request\"}";
    response = MHD_create_response_from_buffer(strlen(msg), (void *)msg, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Content-Type", "application/json");
    ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
    MHD_destroy_response(response);
    return ret;
}

// === MAIN ===
int main() {
    Fila_create(&fila, sizeof(Chamada), capacidadeMaxima);
    pthread_mutex_init(&mutex, NULL);

    struct MHD_Daemon *daemon;
    daemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, 8080,
                              NULL, NULL, &api_handler, NULL,
                              MHD_OPTION_END);
    if (!daemon) return 1;

    printf("API rodando na porta 8080\n");
    getchar(); // Mantém servidor ativo

    MHD_stop_daemon(daemon);
    Fila_destroy(&fila);
    pthread_mutex_destroy(&mutex);
    return 0;
}
