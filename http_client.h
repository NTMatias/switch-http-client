#pragma once
#include <stddef.h>

// Tamaño del buffer de fragmento en memoria (RAM) antes de volcar a la SD.
// 64 KB es un valor seguro: suficientemente grande para rendimiento,
// suficientemente pequeño para no impactar la RAM de la consola.
#define HTTP_CHUNK_BUFFER_SIZE (64 * 1024)

// Códigos de resultado de la descarga
typedef enum {
    HTTP_DOWNLOAD_OK = 0,
    HTTP_DOWNLOAD_ERR_CURL_INIT,
    HTTP_DOWNLOAD_ERR_FOPEN,
    HTTP_DOWNLOAD_ERR_REQUEST,
    HTTP_DOWNLOAD_ERR_HTTP_STATUS,
    HTTP_DOWNLOAD_ERR_WRITE_SD
} HttpDownloadResult;

// Inicializa sockets + curl global. Llamar una vez al arrancar la app.
int http_client_init(void);

// Libera curl global + sockets. Llamar al salir de la app.
void http_client_exit(void);

// Descarga 'url' a 'destPath' (ruta en sdmc:/...) usando streaming
// por fragmentos con buffer controlado en RAM.
// Devuelve HTTP_DOWNLOAD_OK si todo fue bien.
HttpDownloadResult http_download_file(const char* url, const char* destPath);
