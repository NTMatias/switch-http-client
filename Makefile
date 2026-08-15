name: Build Switch Homebrew (.nro)

on:
  push:
    branches: [ "main" ]
  pull_request:
    branches: [ "main" ]
  workflow_dispatch:

jobs:
  build:
    runs-on: ubuntu-latest

    # Usamos la imagen Docker oficial de devkitPro para Switch (devkitA64).
    # Esta imagen ya trae DEVKITPRO, DEVKITA64, DEVKITARM y DEVKITPPC
    # definidas como variables de entorno reales del contenedor, por lo
    # que cada "step" las hereda automáticamente sin necesidad de exportarlas
    # ni de hacer "source" de ningún script.
    container:
      image: devkitpro/devkita64:latest

    steps:
      - name: Checkout del repositorio
        uses: actions/checkout@v4

      - name: Mostrar variables de entorno de devkitPro (diagnóstico)
        run: |
          echo "DEVKITPRO=$DEVKITPRO"
          echo "DEVKITA64=$DEVKITA64"
          echo "PATH=$PATH"

      - name: Instalar dependencias de Switch (libcurl, mbedtls, zlib)
        # switch-curl, switch-mbedtls y switch-zlib son "portlibs":
        # no vienen preinstalados en la imagen base, hay que traerlos
        # explícitamente con dkp-pacman antes de compilar.
        run: |
          dkp-pacman -Sy --noconfirm
          dkp-pacman -S --noconfirm switch-curl switch-mbedtls switch-zlib

      - name: Preparar carpeta romfs (evita fallo si está vacía o no existe)
        run: mkdir -p romfs

      - name: Descargar cacert.pem si no está versionado en el repo
        # Así el .nro generado ya trae verificación TLS funcional
        # aunque no se haya comprometido el cacert.pem al repositorio.
        run: |
          if [ ! -f romfs/cacert.pem ]; then
            curl -fsSL -o romfs/cacert.pem https://curl.se/ca/cacert.pem
          fi

      - name: Compilar con make
        run: make -j$(nproc)

      - name: Verificar que el .nro se generó correctamente
        run: |
          ls -la
          test -f switch-http-client.nro

      - name: Subir el .nro como artefacto descargable
        uses: actions/upload-artifact@v4
        with:
          name: switch-http-client-nro
          path: switch-http-client.nro
          if-no-files-found: error
