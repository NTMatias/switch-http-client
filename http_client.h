#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stddef.h>
#include <stdbool.h>

// Definiciones de tamaños recomendados para evitar desbordamientos de memoria
#define MAX_URL_LENGTH 512
#define MAX_RESPONSE_BUFFER 4096

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Realiza una petición HTTP GET a la URL especificada.
 * Guarda la respuesta (texto/JSON) dentro del buffer proporcionado.
 * 
 * @param url La dirección web a consultar (HTTPS).
 * @param buffer El arreglo de caracteres donde se guardará la respuesta.
 * @param max_len El tamaño máximo del buffer.
 * @return 0 si la petición fue exitosa, -1 si ocurrió un error.
 */
int http_get_string(const char *url, char *buffer, size_t max_len);

/**
 * Descarga un archivo binario desde una URL y lo guarda en la SD.
 * 
 * @param url La dirección web del archivo a descargar.
 * @param filepath La ruta local donde se guardará (ej. "sdmc:/switch/archivo.zip").
 * @return 0 si la descarga fue exitosa, -1 si ocurrió un error.
 */
int http_download_file(const char *url, const char *filepath);

#ifdef __cplusplus
}
#endif

#endif // HTTP_CLIENT_H
