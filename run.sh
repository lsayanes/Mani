#!/bin/bash

echo "=== RUN ==="

APP_BIN="build/Mani.app/Contents/MacOS/Mani"

if [ ! -x "$APP_BIN" ]; then
    echo "No se encontro el ejecutable. Compila primero con ./build.sh"
    exit 1
fi

if command -v brew >/dev/null 2>&1; then
    QT6_PREFIX=$(brew --prefix qt 2>/dev/null || true)
    if [ -n "$QT6_PREFIX" ] && [ -d "$QT6_PREFIX/plugins" ]; then
        export QT_PLUGIN_PATH="$QT6_PREFIX/plugins"
    fi
fi

"$APP_BIN"
