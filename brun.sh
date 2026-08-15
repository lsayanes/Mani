#!/bin/bash

echo "=== Compilacion + ejecucion ==="

mkdir -p build
cd build

rm -rf *

CMAKE_EXTRA_ARGS=""
if command -v brew >/dev/null 2>&1; then
    if [ "${QT_MAJOR:-6}" = "6" ]; then
        QT6_PREFIX=$(brew --prefix qt 2>/dev/null || true)
        if [ -n "$QT6_PREFIX" ] && [ -d "$QT6_PREFIX/lib/cmake/Qt6" ]; then
            echo "Qt6 encontrado en: $QT6_PREFIX"
            CMAKE_EXTRA_ARGS="-DCMAKE_PREFIX_PATH=$QT6_PREFIX"
        fi
    fi
    if [ -z "$CMAKE_EXTRA_ARGS" ]; then
        QT5_PREFIX=$(brew --prefix qt@5 2>/dev/null || true)
        if [ -n "$QT5_PREFIX" ] && [ -d "$QT5_PREFIX/lib/cmake/Qt5" ]; then
            echo "Qt5 encontrado en: $QT5_PREFIX"
            CMAKE_EXTRA_ARGS="-DCMAKE_PREFIX_PATH=$QT5_PREFIX"
        fi
    fi
fi

if [ -n "$Qt5_DIR" ] && [ -d "$Qt5_DIR" ]; then
    echo "Usando Qt5_DIR: $Qt5_DIR"
    CMAKE_EXTRA_ARGS="-DQt5_DIR=$Qt5_DIR $CMAKE_EXTRA_ARGS"
fi
if [ -n "$Qt6_DIR" ] && [ -d "$Qt6_DIR" ]; then
    echo "Usando Qt6_DIR: $Qt6_DIR"
    CMAKE_EXTRA_ARGS="-DQt6_DIR=$Qt6_DIR $CMAKE_EXTRA_ARGS"
fi

echo "Configurando con CMake para macOS..."
cmake .. -DCMAKE_BUILD_TYPE=Debug $CMAKE_EXTRA_ARGS

if [ $? -eq 0 ]; then
    echo "Compilando..."
    CORES=$(sysctl -n hw.ncpu)
    make -j$CORES

    if [ $? -eq 0 ]; then
        cd ..
        ./run.sh
    else
        echo "Error en la compilacion"
        exit 1
    fi
else
    echo "Error en la configuracion de CMake"
    exit 1
fi
