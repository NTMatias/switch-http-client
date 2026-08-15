# Cliente HTTP Homebrew para Nintendo Switch

Aplicación `.nro` de ejemplo que descarga un archivo desde un servidor propio
(HTTP/HTTPS) y lo guarda en la microSD por fragmentos, con un buffer de RAM
controlado (64 KB por defecto) para no saturar la memoria de la consola.

## Estructura del proyecto

```
switch-http-client/
├── Makefile
├── romfs/
│   └── cacert.pem       <- debes añadirlo tú (ver abajo)
└── source/
    ├── main.c           <- bucle principal, input, UI de consola
    ├── http_client.c     <- lógica de descarga con libcurl
    └── http_client.h
```

## 1. Dependencias (devkitPro pacman)

```bash
sudo dkp-pacman -S switch-dev switch-curl switch-mbedtls switch-zlib switch-libnx
```

Esto instala:
- `switch-libnx`: toolkit base de Horizon OS.
- `switch-curl`: libcurl compilado para Switch.
- `switch-mbedtls`: motor TLS que usa curl para HTTPS.
- `switch-zlib`: compresión, dependencia de curl.

## 2. Certificado CA para HTTPS

libnx/curl no incluyen un almacén de certificados del sistema. Para validar
el certificado TLS de tu servidor necesitas colocar un `cacert.pem` dentro
de `romfs/`. Opciones:

- **Servidor con certificado público (Let's Encrypt, etc.)**: descarga el
  bundle oficial de Mozilla:
  ```bash
  curl -o romfs/cacert.pem https://curl.se/ca/cacert.pem
  ```
- **Servidor con certificado autofirmado propio**: exporta tu propio
  certificado raíz/autofirmado en formato PEM y guárdalo como
  `romfs/cacert.pem`.

Si prefieres omitir la verificación TLS (solo recomendable en tu propia red
local de confianza, nunca en producción), puedes cambiar en
`http_client.c`:
```c
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
```

## 3. Compilar

Con el entorno de devkitPro cargado (`DEVKITPRO` exportado):

```bash
cd switch-http-client
make
```

Esto genera `switch-http-client.nro`.

## 4. Ejecutar

Copia el `.nro` a `sdmc:/switch/` en tu consola y lánzalo desde el
Homebrew Menu (hbmenu). La app descargará el archivo de ejemplo a
`sdmc:/switch/http-client-downloads/`.

## Configuración

Edita en `source/main.c`:
```c
#define DOWNLOAD_URL   "https://tu-servidor-privado.local/archivo.bin"
#define DOWNLOAD_DIR   "sdmc:/switch/http-client-downloads"
```

Ajusta el tamaño del buffer de fragmento en `source/http_client.h`:
```c
#define HTTP_CHUNK_BUFFER_SIZE (64 * 1024) // 64 KB
```
