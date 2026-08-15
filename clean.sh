#!/bin/bash

echo "=== clean ==="
echo "current directory: $(pwd)"
echo ""

clean_directory() {
    local dir="$1"
    if [ -d "$dir" ]; then
        echo "Limpiando directorio: $dir"
        rm -rf "$dir"/*
        echo "  ✅ $dir limpiado"
    else
        echo "  ⚠️  Directorio $dir no existe"
    fi
}

echo "Limpiando directorios de build..."
clean_directory "build"

echo ""
echo "Limpiando archivos generados por CMake..."
if [ -f "CMakeCache.txt" ]; then
    rm -f CMakeCache.txt
    echo "  ✅ CMakeCache.txt eliminado"
fi

if [ -d "CMakeFiles" ]; then
    rm -rf CMakeFiles
    echo "  ✅ CMakeFiles eliminado"
fi

if [ -f "cmake_install.cmake" ]; then
    rm -f cmake_install.cmake
    echo "  ✅ cmake_install.cmake eliminado"
fi

if [ -f "Makefile" ]; then
    rm -f Makefile
    echo "  ✅ Makefile eliminado"
fi

if [ -L "compile_commands.json" ] || [ -f "compile_commands.json" ]; then
    rm -f compile_commands.json
    echo "  ✅ compile_commands.json eliminado"
fi

echo ""
echo "Limpiando archivos generados por Qt..."
find . -name "*.moc" -delete 2>/dev/null && echo "  ✅ Archivos .moc eliminados"
find . -name "moc_*.cpp" -delete 2>/dev/null && echo "  ✅ Archivos moc_*.cpp eliminados"
find . -name "ui_*.h" -delete 2>/dev/null && echo "  ✅ Archivos ui_*.h eliminados"
find . -name "qrc_*.cpp" -delete 2>/dev/null && echo "  ✅ Archivos qrc_*.cpp eliminados"

echo ""
echo "Limpiando archivos de compilacion..."
find . -name "*.o" -delete 2>/dev/null && echo "  ✅ Archivos .o eliminados"
find . -name "*.obj" -delete 2>/dev/null && echo "  ✅ Archivos .obj eliminados"

echo ""
echo "Limpiando ejecutables..."
find . -path "./build/Mani.app" -prune -o -name "Mani" -type f -delete 2>/dev/null
if [ -d "build/Mani.app" ]; then
    rm -rf build/Mani.app
    echo "  ✅ Mani.app eliminado"
fi

echo ""
echo "=== LIMPIEZA COMPLETADA ==="
echo ""
echo "Para recompilar:"
echo "  ./build.sh    # Compilacion local (macOS)"
echo "  ./brun.sh     # Compilar y ejecutar"
echo ""
echo "O manualmente:"
echo "  mkdir build && cd build"
echo "  cmake .. -DCMAKE_PREFIX_PATH=\$(brew --prefix qt)"
echo "  make"
