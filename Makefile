#---------------------------------------------------------------------------------
# Verifica que DEVKITPRO esté definido en el entorno
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

#---------------------------------------------------------------------------------
# Opciones generales de la app
#---------------------------------------------------------------------------------
# Fijamos el nombre explícitamente en vez de derivarlo de $(notdir $(CURDIR)).
# Si se deriva del nombre de carpeta, el .nro cambia de nombre según cómo
# se llame el checkout (p. ej. en GitHub Actions, donde la carpeta toma el
# nombre exacto del repositorio), rompiendo cualquier paso de CI que
# espere "switch-http-client.nro".
TARGET      :=  switch-http-client
BUILD       :=  build
SOURCES     :=  source
DATA        :=  data
INCLUDES    :=  source
ROMFS       :=  romfs

APP_TITLE   :=  Cliente HTTP Homebrew
APP_AUTHOR  :=  TuNombre
APP_VERSION :=  1.0.0
# APP_ICON  :=  icon.jpg   # descomenta si añades un icono de 256x256

#---------------------------------------------------------------------------------
# Arquitectura (Nintendo Switch - Cortex-A57)
#---------------------------------------------------------------------------------
ARCH    :=  -march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE

#---------------------------------------------------------------------------------
# Flags de compilación
#---------------------------------------------------------------------------------
CFLAGS  :=  -g -Wall -O2 -ffunction-sections \
            $(ARCH) $(BUILD_CFLAGS) \
            -D__SWITCH__

CFLAGS  +=  $(INCLUDE)

CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++17

ASFLAGS :=  -g $(ARCH)
LDFLAGS  =  -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# Librerías: libcurl (HTTP/HTTPS), mbedtls (TLS), z (compresión), nx (libnx)
# El orden de enlazado importa: curl depende de mbedtls y de z.
#---------------------------------------------------------------------------------
LIBS    :=  -lcurl -lmbedtls -lmbedx509 -lmbedcrypto -lz -lnx

#---------------------------------------------------------------------------------
# Rutas de librerías: portlibs (curl/mbedtls/zlib) + libnx
#---------------------------------------------------------------------------------
LIBDIRS :=  $(PORTLIBS) $(LIBNX)

#---------------------------------------------------------------------------------
# No tocar a partir de aquí (reglas estándar de devkitPro)
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT   :=  $(CURDIR)/$(TARGET)
export TOPDIR   :=  $(CURDIR)

export VPATH    :=  $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                     $(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR  :=  $(CURDIR)/$(BUILD)

CFILES      :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES    :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES      :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES    :=  $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifeq ($(strip $(CPPFILES)),)
    export LD   :=  $(CC)
else
    export LD   :=  $(CXX)
endif

export OFILES_BIN   :=  $(addsuffix .o,$(BINFILES))
export OFILES_SRC    :=  $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES        :=  $(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN    :=  $(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE  :=  $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                     $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                     -I$(CURDIR)/$(BUILD)

export LIBPATHS :=  $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(ROMFS)),)
else
    export ROMFS_ARG := --romfsdir=$(CURDIR)/$(ROMFS)
endif

.PHONY: $(BUILD) clean all

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).pfs0 $(TARGET).nso $(TARGET).nro $(TARGET).nacp $(TARGET).elf

else

DEPENDS :=  $(OFILES:.o=.d)

all: $(OUTPUT).nro

$(OUTPUT).nro: $(OUTPUT).elf $(OUTPUT).nacp
$(OUTPUT).elf: $(OFILES)

$(OFILES_SRC): $(HFILES_BIN)

-include $(DEPENDS)

endif
