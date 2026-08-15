#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <switch.h>
#include "http_client.h"

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
    // Nueva forma de iniciar la consola de texto en libnx
    consoleInit(NULL);

    // Nueva forma de inicializar los controles en libnx
    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    Result romfs_res = romfsInit();

    char query_buffer[256] = "";
    char status_message[MAX_RESPONSE_BUFFER] = "Presiona [X] para buscar.\nPresiona [+] para salir.";
    
    while (appletMainLoop()) {
        // Nueva forma de leer los controles
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        // Los botones ahora usan el prefijo HidNpadButton_
        if (kDown & HidNpadButton_Plus) break;

        if (kDown & HidNpadButton_X) {
            if (abrir_teclado_virtual(query_buffer, sizeof(query_buffer), "")) {
                if (strlen(query_buffer) > 0) {
                    snprintf(status_message, sizeof(status_message), "Buscando: '%s'...", query_buffer);
                    
                    char url[MAX_URL_LENGTH];
                    snprintf(url, sizeof(url), "https://httpbin.org/get?busqueda=%s", query_buffer);

                    char response_buffer[2048] = {0}; 
                    
                    if (http_get_string(url, response_buffer, sizeof(response_buffer)) == 0) {
                        snprintf(status_message, sizeof(status_message), "Exito! Respuesta JSON obtenida:\n%.500s...", response_buffer);
                    } else {
                        snprintf(status_message, sizeof(status_message), "Error: Fallo la peticion HTTPS.");
                    }
                }
            }
        }

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

        // Nueva forma de refrescar la pantalla en la API reciente
        consoleUpdate(NULL);
    }

    if (R_SUCCEEDED(romfs_res)) romfsExit();
    consoleExit(NULL);
    return 0;
}
