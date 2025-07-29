#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <microhttpd.h>
#include <jansson.h>
#include <pthread.h>

// === CONFIGURAÇÕES ===
#define MAX_ULTIMOS 3

// === ESTRUTURAS ===
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
    char ficha[20];
} Chamada;

typedef struct {
    char buffer[4096];
    size_t offset;
} RequestData;

// === VARIÁVEIS GLOBAIS ===
TFila fila;
int capacidadeMaxima = 10;

Chamada ultimosChamados[MAX_ULTIMOS];
int totalChamados = 0; // contador de quantos já foram chamados

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

// Converte a fila inteira para JSON
json_t *Fila_to_json(TFila *fila) {
    json_t *array = json_array();
    if (!fila || fila->size == 0) return array;
    char *ptr = fila->first;
    for (int i = 0; i < fila->size; i++) {
        Chamada *c = (Chamada *)ptr;
        json_t *obj = json_object();
        json_object_set_new(obj, "nome", json_string(c->nome));
        json_object_set_new(obj, "ficha", json_string(c->ficha));
        json_array_append_new(array, obj);
        ptr += fila->sizeElement;
        if (ptr >= fila->buffer + fila->maxElement * fila->sizeElement)
            ptr = fila->buffer;
    }
    return array;
}

// === API HANDLER ===
static enum MHD_Result api_handler(void *cls, struct MHD_Connection *conn,
                                   const char *url, const char *method,
                                   const char *ver, const char *upload_data,
                                   size_t *upload_size, void **con_cls)
{
    struct MHD_Response *response;
    enum MHD_Result ret;

    // CORS preflight
    if (strcmp(method, "OPTIONS") == 0) {
        response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Access-Control-Allow-Methods", "POST, GET, OPTIONS, DELETE");
        MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type");
        ret = MHD_queue_response(conn, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    // Inicializa RequestData na primeira chamada
    if (*con_cls == NULL) {
        RequestData *rd = calloc(1, sizeof(RequestData));
        *con_cls = rd;
        return MHD_YES;
    }
    RequestData *reqData = *con_cls;

    // === POST /chamadas ===
    if (strcmp(method, "POST") == 0 && strcmp(url, "/chamadas") == 0) {
        if (*upload_size > 0) {
            // Acumula corpo
            if (reqData->offset + *upload_size >= sizeof(reqData->buffer))
                return MHD_NO;
            memcpy(reqData->buffer + reqData->offset, upload_data, *upload_size);
            reqData->offset += *upload_size;
            *upload_size = 0;
            return MHD_YES;
        }
        // Parse JSON
        reqData->buffer[reqData->offset] = '\0';
        json_error_t jerr;
        json_t *root = json_loads(reqData->buffer, 0, &jerr);
        if (!root) {
            const char *err = "{\"error\":\"JSON inválido\"}";
            response = MHD_create_response_from_buffer(strlen(err),(void*)err,MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            ret = MHD_queue_response(conn, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            free(reqData); *con_cls = NULL;
            return ret;
        }
        const char *nome;
        if (json_unpack(root, "{s:s}", "nome", &nome) != 0) {
            json_decref(root);
            const char *err = "{\"error\":\"Campo 'nome' obrigatório\"}";
            response = MHD_create_response_from_buffer(strlen(err),(void*)err,MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            ret = MHD_queue_response(conn, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            free(reqData); *con_cls = NULL;
            return ret;
        }

        // Cria nova chamada e insere na fila
        Chamada nova;
        strncpy(nova.nome, nome, sizeof(nova.nome)-1);
        pthread_mutex_lock(&mutex);
        snprintf(nova.ficha, sizeof(nova.ficha), "%d", totalChamados + fila.size + 1);
        bool ok = Fila_put(&fila, (char*)&nova);
        pthread_mutex_unlock(&mutex);
        json_decref(root);

        if (!ok) {
            const char *err = "{\"error\":\"Fila cheia\"}";
            response = MHD_create_response_from_buffer(strlen(err),(void*)err,MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            ret = MHD_queue_response(conn, MHD_HTTP_CONFLICT, response);
        } else {
            const char *okmsg = "{\"status\":\"Chamada adicionada\"}";
            response = MHD_create_response_from_buffer(strlen(okmsg),(void*)okmsg,MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            ret = MHD_queue_response(conn, MHD_HTTP_CREATED, response);
        }
        MHD_destroy_response(response);
        free(reqData); *con_cls = NULL;
        return ret;
    }

    // === DELETE /chamadas/proxima ===
    if (strcmp(method, "DELETE") == 0 && strcmp(url, "/chamadas/proxima") == 0) {
        Chamada atendida;
        bool ok;
        pthread_mutex_lock(&mutex);
        ok = Fila_get(&fila, (char*)&atendida);
        if (ok) {
            // atualiza buffer de últimos
            ultimosChamados[ totalChamados % MAX_ULTIMOS ] = atendida;
            totalChamados++;
        }
        pthread_mutex_unlock(&mutex);

        if (!ok) {
            const char *err = "{\"error\":\"Fila vazia\"}";
            response = MHD_create_response_from_buffer(strlen(err),(void*)err,MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
            ret = MHD_queue_response(conn, MHD_HTTP_NO_CONTENT, response);
            MHD_destroy_response(response);
            return ret;
        }

        // retorna chamada atendida
        json_t *res = json_object();
        json_object_set_new(res, "nome", json_string(atendida.nome));
        json_object_set_new(res, "ficha", json_string(atendida.ficha));
        char *str = json_dumps(res, 0);
        json_decref(res);

        response = MHD_create_response_from_buffer(strlen(str), str, MHD_RESPMEM_MUST_FREE);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        ret = MHD_queue_response(conn, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    // === GET /chamadas/mostrar_fila ===
    if (strcmp(method, "GET")==0 && strcmp(url,"/chamadas/mostrar_fila")==0) {
        pthread_mutex_lock(&mutex);
        json_t *arr = Fila_to_json(&fila);
        pthread_mutex_unlock(&mutex);

        char *str = json_dumps(arr, 0);
        json_decref(arr);
        response = MHD_create_response_from_buffer(strlen(str), str, MHD_RESPMEM_MUST_FREE);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        ret = MHD_queue_response(conn, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    // === NOVO: GET /chamadas/ultimos ===
    if (strcmp(method,"GET")==0 && strcmp(url,"/chamadas/ultimos")==0) {
        json_t *arr = json_array();
        pthread_mutex_lock(&mutex);
        int cnt = totalChamados < MAX_ULTIMOS ? totalChamados : MAX_ULTIMOS;
        for (int i = 0; i < cnt; i++) {
            int idx = (totalChamados - 1 - i + MAX_ULTIMOS) % MAX_ULTIMOS;
            Chamada *c = &ultimosChamados[idx];
            json_t *obj = json_object();
            json_object_set_new(obj, "nome", json_string(c->nome));
            json_object_set_new(obj, "ficha", json_string(c->ficha));
            json_array_append_new(arr, obj);
        }
        pthread_mutex_unlock(&mutex);

        char *str = json_dumps(arr, 0);
        json_decref(arr);
        response = MHD_create_response_from_buffer(strlen(str), str, MHD_RESPMEM_MUST_FREE);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        ret = MHD_queue_response(conn, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    // === ROTA PADRÃO 400 ===
    {
        const char *err = "{\"error\":\"Bad request\"}";
        response = MHD_create_response_from_buffer(strlen(err),(void*)err,MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        ret = MHD_queue_response(conn, MHD_HTTP_BAD_REQUEST, response);
        MHD_destroy_response(response);
        return ret;
    }
}

// === MAIN ===
int main() {
    Fila_create(&fila, sizeof(Chamada), capacidadeMaxima);
    pthread_mutex_init(&mutex, NULL);

    struct MHD_Daemon *daemon = MHD_start_daemon(
        MHD_USE_THREAD_PER_CONNECTION, 80,
        NULL, NULL, &api_handler, NULL,
        MHD_OPTION_END
    );
    if (!daemon) return 1;

    printf("API rodando na porta 8080\n");
    getchar();  // mantém vivo

    MHD_stop_daemon(daemon);
    Fila_destroy(&fila);
    pthread_mutex_destroy(&mutex);
    return 0;
}
