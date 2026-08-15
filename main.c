#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <switch.h>
#include "http_client.h"

// Función para invocar el teclado nativo
bool abrir_teclado_virtual(char *out_str, size_t max_len, const char *initial_text) {
    SwkbdConfig kbd;
    Result rc = swkbdCreate(&kbd, 0);
    if (R_FAILED(rc)) return false;

    swkbdConfigMakePresetDefault(&kbd);
    swkbdConfigSetGuideText(&kbd, "Escribe el nombre a buscar:");
    
    if (initial_text && initial_text[0] != '\0') {
        swkbdConfigSetInitialText(&kbd, initial_text);
    }

    rc = swkbdShow(&kbd, out_str, max_len);
    swkbdClose(&kbd);

    return R_SUCCEEDED(rc);
}

int main(int argc, char **argv) {
    gfxInitDefault();
    consoleInit(NULL);

    // Cargar romfs para el cacert.pem
    Result romfs_res = romfsInit();

    char query_buffer[256] = "";
    char status_message[MAX_RESPONSE_BUFFER] = "Presiona [X] para buscar.\nPresiona [A] para prueba de descarga.";
    
    while (appletMainLoop()) {
        hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break;

        // Buscar con teclado virtual
        if (kDown & KEY_X) {
            if (abrir_teclado_virtual(query_buffer, sizeof(query_buffer), "")) {
                if (strlen(query_buffer) > 0) {
                    snprintf(status_message, sizeof(status_message), "Buscando: '%s'...", query_buffer);
                    
                    // URL de ejemplo. Aquí conectas tu API real luego.
                    char url[MAX_URL_LENGTH];
                    snprintf(url, sizeof(url), "https://httpbin.org/get?busqueda=%s", query_buffer);

                    char response_buffer[2048]; // Usamos un buffer local para la respuesta HTTP
                    
                    if (http_get_string(url, response_buffer, sizeof(response_buffer)) == 0) {
                        snprintf(status_message, sizeof(status_message), "Exito! Respuesta JSON obtenida:\n%.500s...", response_buffer);
                    } else {
                        snprintf(status_message, sizeof(status_message), "Error: Fallo la peticion HTTPS.");
                    }
                }
            }
        }

        // Dibujar interfaz
        consoleClear();
        printf("===================================================\n");
        printf("       CLIENTE HOMEBREW - BUSCADOR INTERACTIVO     \n");
        printf("===================================================\n\n");

        if (R_FAILED(romfs_res)) {
            printf("[AVISO] Romfs no montado. El HTTPS fallara.\n\n");
        } else {
            printf("[ESTADO] RomFS activo. Certificados cargados.\n\n");
        }

        printf("Termino buscado : %s\n", query_buffer[0] != '\0' ? query_buffer : "(Vacio)");
        printf("---------------------------------------------------\n");
        printf("Consola de salida:\n\n%s\n", status_message);
        printf("\n---------------------------------------------------\n");
        printf(" Controles:\n");
        printf(" [X] Buscar con teclado\n");
        printf(" [+] Salir de la app\n");
        printf("===================================================\n");

        gfxFlushBuffers();
        gfxSwapBuffers();
        gputickWaitForVsync();
    }

    if (R_SUCCEEDED(romfs_res)) romfsExit();
    gfxExit();
    return 0;
}
