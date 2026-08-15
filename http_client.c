#include "http_client.h"

#include <switch.h>
#include <curl/curl.h>
#include <string.h>
#include <stdio.h>

// Contexto que viaja junto al callback de escritura de curl.
// Guarda el buffer de fragmento y el FILE* de destino en la SD.
typedef struct {
    FILE*  file;
    char   buffer[HTTP_CHUNK_BUFFER_SIZE];
    size_t bufferUsed;
    long   totalBytesWritten;
    int    writeError; // se activa si falla un fwrite
} DownloadContext;

// ---------------------------------------------------------------------
// Vuelca a disco el contenido acumulado en el buffer de RAM y lo resetea.
// Se llama automáticamente cuando el buffer se llena, y al final de la
// transferencia para no perder el remanente.
// ---------------------------------------------------------------------
static void flush_buffer(DownloadContext* ctx)
{
    if (ctx->bufferUsed == 0 || ctx->writeError)
        return;

    size_t written = fwrite(ctx->buffer, 1, ctx->bufferUsed, ctx->file);
    if (written != ctx->bufferUsed) {
        ctx->writeError = 1;
    } else {
        ctx->totalBytesWritten += (long)written;
    }
    ctx->bufferUsed = 0;
}

// ---------------------------------------------------------------------
// Callback de escritura de curl (CURLOPT_WRITEFUNCTION).
// curl entrega los datos recibidos por red en trozos variables; aquí
// los copiamos a NUESTRO buffer de tamaño fijo y solo tocamos la SD
// cuando ese buffer se llena. Esto acota el uso de RAM sin importar
// el tamaño total del archivo descargado.
// ---------------------------------------------------------------------
static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp)
{
    DownloadContext* ctx = (DownloadContext*)userp;
    size_t totalSize = size * nmemb;
    size_t offset = 0;

    while (offset < totalSize) {
        size_t spaceLeft = HTTP_CHUNK_BUFFER_SIZE - ctx->bufferUsed;
        size_t toCopy = (totalSize - offset < spaceLeft) ? (totalSize - offset) : spaceLeft;

        memcpy(ctx->buffer + ctx->bufferUsed, (const char*)contents + offset, toCopy);
        ctx->bufferUsed += toCopy;
        offset += toCopy;

        if (ctx->bufferUsed == HTTP_CHUNK_BUFFER_SIZE) {
            flush_buffer(ctx);
            if (ctx->writeError) {
                // Devolver algo distinto a totalSize aborta la transferencia
                return 0;
            }
        }
    }

    return totalSize;
}

// ---------------------------------------------------------------------
// Callback de progreso (CURLOPT_XFERINFOFUNCTION). Solo informa en
// consola; no toca la SD ni asigna memoria extra.
// ---------------------------------------------------------------------
static int xferinfo_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                              curl_off_t ultotal, curl_off_t ulnow)
{
    (void)clientp; (void)ultotal; (void)ulnow;

    if (dltotal > 0) {
        double percent = ((double)dlnow / (double)dltotal) * 100.0;
        printf("\rProgreso: %6.1f%%  (%lld / %lld bytes)  ",
               percent, (long long)dlnow, (long long)dltotal);
    } else {
        // Servidor no envía Content-Length: mostramos bytes acumulados
        printf("\rDescargando... %lld bytes", (long long)dlnow);
    }
    consoleUpdate(NULL);
    return 0; // 0 = continuar, distinto de 0 abortaría la transferencia
}

int http_client_init(void)
{
    // Inicializa la pila de sockets de Horizon OS con un tamaño de
    // buffer de sockets razonable para descargas HTTP.
    Result rc = socketInitializeDefault();
    if (R_FAILED(rc)) {
        return 0;
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        socketExit();
        return 0;
    }

    return 1;
}

void http_client_exit(void)
{
    curl_global_cleanup();
    socketExit();
}

HttpDownloadResult http_download_file(const char* url, const char* destPath)
{
    // Abrimos el archivo destino en la SD en modo escritura binaria.
    // sdmc:/ debe existir la ruta padre (creación de carpetas: ver main.c)
    FILE* file = fopen(destPath, "wb");
    if (!file) {
        return HTTP_DOWNLOAD_ERR_FOPEN;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        fclose(file);
        return HTTP_DOWNLOAD_ERR_CURL_INIT;
    }

    DownloadContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.file = file;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);

    // Progreso
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo_callback);

    // Seguir redirecciones (útil para APIs propias detrás de un proxy)
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);

    // Timeouts razonables para evitar que la app se quede colgada
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "SwitchHomebrewHttpClient/1.0");

    // --- Verificación TLS ---
    // Empaquetamos un cacert.pem en romfs para validar el certificado
    // del servidor. Si tu servidor usa un certificado autofirmado propio,
    // sustituye este cacert.pem por el tuyo (ver README).
    curl_easy_setopt(curl, CURLOPT_CAINFO, "romfs:/cacert.pem");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);

    HttpDownloadResult result = HTTP_DOWNLOAD_OK;

    if (res != CURLE_OK) {
        result = HTTP_DOWNLOAD_ERR_REQUEST;
    } else {
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        if (httpCode < 200 || httpCode >= 300) {
            result = HTTP_DOWNLOAD_ERR_HTTP_STATUS;
        }
    }

    // Vaciamos cualquier resto que quedara en el buffer de RAM
    flush_buffer(&ctx);
    if (ctx.writeError && result == HTTP_DOWNLOAD_OK) {
        result = HTTP_DOWNLOAD_ERR_WRITE_SD;
    }

    curl_easy_cleanup(curl);
    fclose(file);

    printf("\n");
    return result;
}
