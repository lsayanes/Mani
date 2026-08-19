# Mani

Aplicación de finanzas personales en **Qt 6 + C++ (Widgets)** para registrar saldos en **USD** y **ARS**. Los importes se guardan en centavos (enteros) en **SQLite** para evitar errores de redondeo.

Especificación completa del producto: [`mani.md`](mani.md).

## Estado actual

**Fase 1** implementada:

- Alta, edición y borrado de cuentas (billetera, banco, efectivo, tarjeta).
- Pantalla principal con cuentas agrupadas por moneda (USD / ARS).
- Saldos inicial, actual y gastado por cuenta.
- Mes activo automático (`YYYY-MM`) con rollover al cambiar de mes.
- Persistencia local; la base se crea en el primer arranque.
- Pantalla de bienvenida cuando no hay cuentas cargadas.

**Fase 2** implementada:

- Tipo de cambio USD → ARS editable por mes (`1 USD = $ …`).
- Totales al pie: saldo **actual** y **gastado** en USD/ARS, más consolidado en pesos.
- El consolidado se muestra solo cuando hay tasa cargada para el mes activo.

**Fase 3** implementada:

- Selector de mes (combo, anterior/siguiente, botón **Hoy**).
- Vista de meses anteriores con sus saldos y totales.
- Diálogo **Historial** con comparativa de gastos por mes (doble clic para abrir un mes).
- Rollover automático al abrir el mes calendario actual.

**Fase 4** implementada:

- Registro de **movimientos** (ingreso/egreso) por cuenta: fecha, monto y concepto.
- El saldo actual se actualiza automáticamente al crear o eliminar un movimiento.
- Ya no se edita el saldo actual a mano; solo el saldo inicial al editar una cuenta.
- Botón **Movimientos** en cada cuenta y acción **Nuevo movimiento** en la barra.

**Fase 5** implementada:

- **Categoría opcional** en cada movimiento (combo editable con categorías usadas antes).
- Acción **Reportes** con dos pestañas:
  - **Por categoría**: gasto de egresos del mes en USD y ARS (tabla + gráfico de barras).
  - **Por mes**: comparativa de gastos mensuales (tabla + gráfico consolidado cuando hay tasa).

**Fase 6** implementada:

- Menú **Datos**: exportar e importar CSV (cuentas, saldos, movimientos, tasas).
- **Copia de respaldo** y **Restaurar respaldo** (archivo `.db` en carpeta de backups).
- Menú **Seguridad**: contraseña de bloqueo (hash SHA256 + salt en QSettings).
- **Touch ID** en macOS para desbloquear (si está disponible).
- Pantalla de bloqueo al iniciar y acción **Bloquear ahora**.

Próximas fases (ver `mani.md`): refinamientos y plataformas móviles.

## Requisitos

- **macOS** 13+ (desarrollo actual)
- **CMake** 3.16+
- **Qt 6** con módulos Widgets y Sql

En macOS con Homebrew:

```bash
brew install qt cmake
```

## Compilar y ejecutar

Desde la raíz del proyecto:

| Script | Descripción |
|---|---|
| `./build.sh` | Compila en `build/` (modo Debug) |
| `./run.sh` | Ejecuta la app compilada |
| `./brun.sh` | Compila y ejecuta |
| `./package.sh` | Empaqueta Release e instala en `/Applications` |
| `./clean.sh` | Limpia artefactos de build |

Compilación manual:

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=$(brew --prefix qt)
make -j$(sysctl -n hw.ncpu)
open Mani.app
```

## Instalar en Aplicaciones (macOS)

Para generar un `.app` autocontenido (con Qt y el driver SQLite incluidos) e instalarlo en el Launchpad:

```bash
./package.sh
```

Eso compila en **Release**, empaqueta con `macdeployqt`, deja una copia en `dist/Mani-1.2.0.app` e instala en `/Applications/Mani.app`.

Solo empaquetar sin instalar:

```bash
./package.sh --local
```

Luego podés arrastrar `dist/Mani-1.2.0.app` a Aplicaciones a mano.

**Nota:** la firma es ad-hoc (uso en tu Mac). La primera vez macOS puede pedirte confirmar en *Ajustes del sistema → Privacidad y seguridad*.

## Datos de la aplicación

La base SQLite se crea automáticamente en:

```text
~/Library/Application Support/Mani/Mani/mani.db
```

Las copias de respaldo se guardan por defecto en:

```text
~/Library/Application Support/Mani/Mani/backups/
```

No hace falta crear archivos ni carpetas a mano.

## Estructura del proyecto

```text
Mani/
├── CMakeLists.txt
├── mani.md              # Especificación funcional
├── build.sh / run.sh / brun.sh / clean.sh
└── src/
    ├── main.cpp
    ├── MainWindow.*     # Pantalla principal
    ├── database/        # SQLite (cuenta, saldo_mes, tasa_cambio)
    ├── model/           # Moneda, Cuenta
    ├── ui/              # Diálogos, tarjetas, reportes, bloqueo
    ├── platform/        # Touch ID (macOS)
    └── util/            # Montos, mes activo, rutas, CSV, bloqueo
```

## Convenciones

- **Monedas:** USD (1) y ARS (2). Una cuenta no cambia de moneda después de creada.
- **Montos:** almacenados en centavos; visibles como `$ 1.234,56` (locale `es_AR`).
- **Gastado:** suma de egresos del mes (los ingresos no lo modifican).
- **Colores en UI:** valores positivos en verde, negativos en rojo.

## Capturas

Pantallas principales de la Fase 1:

| Pantalla | Descripción |
|---|---|
| Bienvenida | Primer arranque sin cuentas; botón *Crear primera cuenta* |
| Principal | Cuentas agrupadas en columnas USD y ARS con inicial, actual y gastado |
| Nueva cuenta | Formulario: nombre, moneda y saldo inicial |
| Editar cuenta | Nombre, saldo inicial y saldo actual (moneda fija) |

Las capturas van en `docs/screenshots/`. Nombres sugeridos:

```text
docs/screenshots/
├── welcome.png
├── main.png
├── nueva-cuenta.png
└── editar-cuenta.png
```

Para generarlas en macOS, ejecutá la app (`./run.sh`), recorré cada pantalla y guardá con `Cmd+Shift+4`. Luego referenciá las imágenes acá si querés mostrarlas en el README:

```markdown
![Pantalla principal](docs/screenshots/main.png)
```

## iOS y Android (planificado)

El objetivo del proyecto es un **único codebase** Qt Widgets que compile a `.app` (macOS), `.ipa` (iOS) y `.apk` (Android). Hoy solo está verificado en **macOS**; el soporte móvil se agregará cuando avance el desarrollo.

### Requisitos previstos

| Plataforma | Herramientas |
|---|---|
| **iOS** | Xcode, cuenta de desarrollador Apple, Qt 6 para iOS (instalador de Qt) |
| **Android** | Android SDK + NDK, JDK, Qt 6 para Android |

Versiones mínimas de OS (según `mani.md`): iOS 16, Android 8.

### Pasos generales (referencia)

Estos pasos se documentarán en detalle cuando el proyecto tenga targets móviles en CMake. Resumen:

**iOS**

```bash
# Requiere Qt for iOS instalado (Maintenance Tool → Qt 6.x → iOS)
mkdir -p build-ios && cd build-ios
cmake .. -GXcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=16.0 \
  -DCMAKE_PREFIX_PATH=$HOME/Qt/6.x.x/ios
cmake --build . --config Debug
# Abrir el .xcodeproj generado y ejecutar en simulador o dispositivo
```

**Android**

```bash
# Requiere ANDROID_SDK_ROOT, ANDROID_NDK_ROOT y Qt for Android
mkdir -p build-android && cd build-android
cmake .. -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DCMAKE_PREFIX_PATH=$HOME/Qt/6.x.x/android_arm64_v8a
cmake --build .
# Desplegar con androiddeployqt (según artefactos generados por qt_add_executable)
```

### Consideraciones al portar

- **SQLite:** el mismo esquema y `QSqlDatabase` funcionan en las tres plataformas; la ruta de datos usa `QStandardPaths::AppDataLocation` (distinta por OS).
- **UI Widgets:** revisar tamaños de ventana, botones y diálogos en pantallas chicas; puede hacer falta layout adaptativo o scroll adicional.
- **Plugins Qt:** empaquetar el driver `sqldrivers/libqsqlite` en el bundle (.app / .apk).
- **Scripts:** `build.sh` y `run.sh` actuales son solo para macOS; se agregarán `build-ios.sh` / `build-android.sh` cuando corresponda.

## Licencia

Por definir.
