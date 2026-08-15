// ============================================================================
// Cliente HTTP homebrew para Nintendo Switch (libnx + devkitPro)
// Descarga archivos por streaming/fragmentos desde un servidor propio y
// los guarda en la tarjeta microSD, con UI de consola de texto.
// ============================================================================

#include <switch.h>
#include <stdio.h>
#include <sys/stat.h>
#include "http_client.h"

// Ajusta estos valores a tu servidor/API propia
#define DOWNLOAD_URL   "https://tu-servidor-privado.local/archivo.bin"
#define DOWNLOAD_DIR   "sdmc:/switch/http-client-downloads"
#define DOWNLOAD_PATH  DOWNLOAD_DIR "/archivo.bin"

// Crea la carpeta de destino en la SD si no existe todavía.
static void ensure_download_dir(void)
{
    mkdir(DOWNLOAD_DIR, 0777); // si ya existe, mkdir falla silenciosamente (ignorado)
}

int main(int argc, char* argv[])
{
    // --- Inicialización de subsistemas ---
    consoleInit(NULL);

    // romfs contiene el cacert.pem que usa http_client.c para validar TLS
    Result romfsRc = romfsInit();

    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    int netOk = http_client_init();
    ensure_download_dir();

    // --- Estado de UI ---
    int lastResultShown = 0;
    HttpDownloadResult lastResult = HTTP_DOWNLOAD_OK;

    printf("\x1b[2J"); // limpia pantalla
    printf("=================================================\n");
    printf(" Cliente HTTP - Homebrew NRO (libnx + libcurl)\n");
    printf("=================================================\n\n");

    if (!netOk) {
        printf("ERROR: no se pudo inicializar la red/curl.\n");
    }
    if (R_FAILED(romfsRc)) {
        printf("AVISO: romfs no cargado (falta cacert.pem para HTTPS).\n");
    }

    printf("Destino de descargas: %s\n\n", DOWNLOAD_DIR);
    printf("Controles:\n");
    printf("  A  -> Descargar archivo de ejemplo\n");
    printf("  +  -> Salir\n\n");

    // --- Bucle principal ---
    while (appletMainLoop())
    {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        // Salida limpia con el botón +
        if (kDown & HidNpadButton_Plus) {
            break;
        }

        // Botón A: dispara la descarga
        if ((kDown & HidNpadButton_A) && netOk) {
            printf("\nIniciando descarga desde:\n  %s\n\n", DOWNLOAD_URL);
            consoleUpdate(NULL);

            lastResult = http_download_file(DOWNLOAD_URL, DOWNLOAD_PATH);
            lastResultShown = 1;

            switch (lastResult) {
                case HTTP_DOWNLOAD_OK:
                    printf("Descarga completada y guardada en la SD.\n\n");
                    break;
                case HTTP_DOWNLOAD_ERR_FOPEN:
                    printf("Error: no se pudo crear el archivo en la SD.\n\n");
                    break;
                case HTTP_DOWNLOAD_ERR_CURL_INIT:
                    printf("Error: no se pudo inicializar curl.\n\n");
                    break;
                case HTTP_DOWNLOAD_ERR_REQUEST:
                    printf("Error: fallo en la petición HTTP (revisa conexión/URL).\n\n");
                    break;
                case HTTP_DOWNLOAD_ERR_HTTP_STATUS:
                    printf("Error: el servidor respondió con un código HTTP de error.\n\n");
                    break;
                case HTTP_DOWNLOAD_ERR_WRITE_SD:
                    printf("Error: fallo al escribir en la microSD (¿espacio libre?).\n\n");
                    break;
            }
        } else if ((kDown & HidNpadButton_A) && !netOk) {
            printf("\nNo hay red disponible, no se puede descargar.\n\n");
        }

        consoleUpdate(NULL);
    }

    // --- Limpieza ordenada ---
    http_client_exit();
    if (R_SUCCEEDED(romfsRc)) romfsExit();
    consoleExit(NULL);

    return 0;
}
