#!/bin/bash

# ¿Qué hace este script?
# Hace una copia de seguridad (backup) de una carpeta
# y la guarda en otra carpeta con la fecha en el nombre.

# -------------------------------------------------------
# PASO 1: Mostrar ayuda si el usuario pide "-help"
# -------------------------------------------------------

if [ "$1" = "-help" ]; then
    echo ""
    echo "Uso del script:"
    echo "  ./backup_full.sh /carpeta/a/copiar /donde/guardar"
    echo ""
    echo "Ejemplo:"
    echo "  ./backup_full.sh /var/log /backup_dir"
    echo ""
    echo "Esto va a crear un archivo llamado: log_bkp_20240302.tar.gz"
    echo ""
    exit 0
fi

# -------------------------------------------------------
# PASO 2: Guardar los argumentos en variables
# -------------------------------------------------------

# $1 es lo primero que escribió el usuario (origen)
# $2 es lo segundo que escribió el usuario (destino)

ORIGEN=$1
DESTINO=$2

# -------------------------------------------------------
# PASO 3: Verificar que el usuario pasó los dos argumentos
# -------------------------------------------------------

if [ -z "$ORIGEN" ]; then
    echo "Error: falta indicar la carpeta de origen."
    echo "Usá ./backup_full.sh -help para ver cómo usarlo."
    exit 1
fi

if [ -z "$DESTINO" ]; then
    echo "Error: falta indicar la carpeta de destino."
    echo "Usá ./backup_full.sh -help para ver cómo usarlo."
    exit 1
fi

# -------------------------------------------------------
# PASO 4: Verificar que la carpeta de origen existe
# -------------------------------------------------------

if [ ! -d "$ORIGEN" ]; then
    echo "Error: la carpeta de origen no existe: $ORIGEN"
    exit 1
fi

# -------------------------------------------------------
# PASO 5: Verificar que la carpeta de destino existe
# -------------------------------------------------------

if [ ! -d "$DESTINO" ]; then
    echo "Error: la carpeta de destino no existe: $DESTINO"
    exit 1
fi

# -------------------------------------------------------
# PASO 6: Armar el nombre del archivo de backup
# -------------------------------------------------------

# Obtenemos la fecha de hoy en formato YYYYMMDD
FECHA=$(date +%Y%m%d)

# Obtenemos solo el nombre de la carpeta (sin la ruta completa)
# Por ejemplo: de "/var/log" sacamos "log"
NOMBRE=$(basename $ORIGEN)

# Armamos el nombre del archivo final
# Por ejemplo: log_bkp_20240302.tar.gz
ARCHIVO="${NOMBRE}_bkp_${FECHA}.tar.gz"

# Armamos la ruta completa donde se va a guardar
RUTA_FINAL="${DESTINO}/${ARCHIVO}"

# -------------------------------------------------------
# PASO 7: Hacer el backup
# -------------------------------------------------------

echo "Iniciando backup..."
echo "Copiando: $ORIGEN"
echo "Guardando en: $RUTA_FINAL"

# tar: empaqueta y comprime la carpeta
# -c : crear archivo
# -z : comprimir con gzip
# -f : nombre del archivo de salida
tar -czf $RUTA_FINAL $ORIGEN

# -------------------------------------------------------
# PASO 8: Verificar si el backup salió bien
# -------------------------------------------------------

# $? guarda el resultado del último comando
# 0 significa que salió bien, cualquier otro número es error

if [ $? -eq 0 ]; then
    echo "Backup completado con éxito: $RUTA_FINAL"
else
    echo "Error: el backup falló."
    exit 1
fi