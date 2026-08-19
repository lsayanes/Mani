#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

INSTALL=1
if [ "${1:-}" = "--local" ] || [ "${1:-}" = "--no-install" ]; then
    INSTALL=0
fi

BUILD_DIR="build-release"
APP_NAME="Mani.app"
APP_PATH="$BUILD_DIR/$APP_NAME"
VERSION=$(grep -E '^project\(Mani VERSION ' CMakeLists.txt | sed -E 's/.*VERSION ([0-9.]+).*/\1/')

echo "=== Empaquetado Mani $VERSION ==="

QT_PREFIX=""
if command -v brew >/dev/null 2>&1; then
    QT_PREFIX=$(brew --prefix qt 2>/dev/null || true)
fi

MACDEPLOYQT=""
if [ -n "$QT_PREFIX" ] && [ -x "$QT_PREFIX/bin/macdeployqt" ]; then
    MACDEPLOYQT="$QT_PREFIX/bin/macdeployqt"
elif command -v macdeployqt >/dev/null 2>&1; then
    MACDEPLOYQT="$(command -v macdeployqt)"
fi

if [ -z "$MACDEPLOYQT" ]; then
    echo "Error: no se encontro macdeployqt. Instala Qt 6 con Homebrew: brew install qt"
    exit 1
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

CMAKE_ARGS=(-DCMAKE_BUILD_TYPE=Release)
if [ -n "$QT_PREFIX" ] && [ -d "$QT_PREFIX/lib/cmake/Qt6" ]; then
    echo "Qt6 encontrado en: $QT_PREFIX"
    CMAKE_ARGS+=("-DCMAKE_PREFIX_PATH=$QT_PREFIX")
fi

echo "Configurando Release..."
cmake .. "${CMAKE_ARGS[@]}"

echo "Compilando..."
make -j"$(sysctl -n hw.ncpu)"

if [ ! -d "$APP_NAME" ]; then
    echo "Error: no se genero $APP_NAME"
    exit 1
fi

echo "Empaquetando dependencias Qt (macdeployqt)..."
"$MACDEPLOYQT" "$APP_NAME" -always-overwrite -codesign=-

cd "$SCRIPT_DIR"

ARCHIVE_DIR="dist"
ARCHIVE_PATH="$ARCHIVE_DIR/Mani-$VERSION.app"
mkdir -p "$ARCHIVE_DIR"
rm -rf "$ARCHIVE_PATH"
ditto "$APP_PATH" "$ARCHIVE_PATH"

echo ""
echo "=== Empaquetado listo ==="
echo "  Carpeta local: $ARCHIVE_PATH"
echo "  Version:       $VERSION"

if [ "$INSTALL" -eq 1 ]; then
    TARGET="/Applications/Mani.app"
    echo ""
    echo "Instalando en $TARGET ..."
    rm -rf "$TARGET"
    ditto "$ARCHIVE_PATH" "$TARGET"
    echo ""
    echo "Mani $VERSION instalado en Aplicaciones."
    echo "Abri Launchpad o Spotlight (Cmd+Espacio) y busca \"Mani\"."
else
    echo ""
    echo "Para instalar en Aplicaciones:"
    echo "  ./package.sh"
    echo ""
    echo "O arrastra manualmente dist/Mani-$VERSION.app a /Applications"
fi
