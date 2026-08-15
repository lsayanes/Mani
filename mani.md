# Mani

**Mani** es una aplicacion de informacion financiera personal. Cada registro es una **cuenta** (billetera, banco, efectivo, tarjeta)

Monedas soportadas:

1. **USD** – Dolares estadounidenses
2. **ARS** – Pesos argentinos

Una cuenta pertenece a una sola moneda. El saldo negativo esta permitido (p. ej. tarjeta de credito).

## Tecnologia (DECIDIDA)

- **Qt 6 + Widgets + C++**: un unico codebase que se compila a artefactos nativos (`.app` / `.ipa` / `.apk`) para macOS, iOS y Android. No hay separacion frontend/backend.
- **Build**: CMake.
- **Persistencia**: SQLite (una sola libreria, funciona en las 3 plataformas). La base se crea al primer arranque.
- **UI**: Qt Widgets. Locale de la app: `es_AR`. Montos visibles como `$ 1.234,56`.
- **Minimos de OS** (ajustables al armar el proyecto): macOS 13, iOS 16, Android 8.

## Conceptos

| Concepto | Significado |
|---|---|
| **Cuenta** | Billetera o cuenta con nombre, moneda y saldos. La moneda no se cambia despues de creada. |
| **Mes activo** | `YYYY-MM` del reloj del dispositivo. En Fase 1 no hay selector de mes. |
| **Saldo inicial** | Saldo de referencia del mes (lo que habia al empezar). |
| **Saldo actual** | Saldo del mes en curso. |
| **Gastado** | `saldo_inicial - saldo_actual` (puede ser negativo si el saldo subio). |
| **Tipo de cambio** | Una sola tasa editable por mes: cuantos ARS vale 1 USD (oficial, MEP o blue: la que el usuario quiera usar). |

**Rollover (regla de negocio, desde Fase 1):** al abrir un mes nuevo, para cada cuenta `saldo_inicial` del mes = `saldo_actual` del mes anterior, y `saldo_actual` arranca igual. Si no hay mes previo, el alta usa el saldo inicial que cargo el usuario.

## Modelo de datos

Los importes se guardan en **centavos** (`INTEGER`) para evitar errores de redondeo de `double`. La moneda es un enum, no un entero magico.

```cpp
enum class Moneda : uint8_t
{
    USD = 1,
    ARS = 2
};
```

```sql
CREATE TABLE cuenta (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    nombre     TEXT NOT NULL,
    moneda     INTEGER NOT NULL CHECK (moneda IN (1, 2)),
    creado_en  TEXT NOT NULL                  -- ISO-8601
);

CREATE TABLE saldo_mes (
    cuenta_id      INTEGER NOT NULL,
    mes            TEXT NOT NULL,             -- 'YYYY-MM'
    saldo_inicial  INTEGER NOT NULL,          -- centavos
    saldo_actual   INTEGER NOT NULL,          -- centavos
    PRIMARY KEY (cuenta_id, mes),
    FOREIGN KEY (cuenta_id) REFERENCES cuenta(id) ON DELETE CASCADE
);

CREATE TABLE tasa_cambio (
    mes       TEXT PRIMARY KEY,               -- 'YYYY-MM'
    usd_a_ars INTEGER NOT NULL                -- centavos de ARS que vale 1 USD
);
```

Ejemplo de tasa: 1 USD = $1500 → `usd_a_ars = 150000`.

La conversion consolidada usa solo enteros. La division trunca hacia cero; se redondea al **centavo mas cercano** con:

```text
total_consolidado_ars = total_ars + (total_usd * usd_a_ars + 50) / 100
```

(`+ 50` antes de dividir; si el gastado USD es negativo, usar el equivalente con signo: `+ 50` o `- 50` segun el signo del producto.)

## Fase 1 — Cuentas y pantalla principal

**Fuera de alcance:** historial de meses, movimientos, categorias, sync, multi-usuario, selector de mes, tipo de cambio (Fase 2).

**Criterio de listo:** crear cuentas, verlas agrupadas por moneda, editar nombre / saldo inicial / saldo actual, borrar con confirmacion, persistir al cerrar y reabrir. El mes activo se deriva del dispositivo y los saldos viven en `saldo_mes`.

### Alta de cuenta

Formulario: nombre, moneda, saldo inicial.

- Nombre no vacio. Se permiten nombres duplicados.
- Al crearse, en el mes activo `saldo_inicial` y `saldo_actual` quedan con el mismo valor.
- La cuenta aparece de inmediato en la pantalla principal.

### Pantalla principal

Cuentas del mes activo, agrupadas por moneda (primero USD, luego ARS). Cada cuenta se muestra **una sola vez**, con inicial, actual y gastado juntos (el gastado puede quedar en 0 hasta Fase 2 si se prefiere no calcularlo aun; el layout ya lo reserva).

```
USD                              ARS
Cuenta sueldo                    Efectivo
Inicial   1.000,00               Inicial  50.000,00
Actual      800,00               Actual   42.000,00
Gastado     200,00               Gastado   8.000,00
```

Orden: orden de creacion.

Acciones:

- **Editar saldo actual** (inline o dialogo). Provisorio: en Fase 4 lo reemplazan los movimientos.
- **Editar** nombre y/o saldo inicial (dialogo). Editar el inicial a mitad de mes esta permitido y recalcula el gastado.
- **Borrar** cuenta, con confirmacion. Borra tambien sus `saldo_mes`.
- La moneda no se edita.

Primera ejecucion: pantalla vacia + boton para crear la primera cuenta. La app es usable sin cuentas.

Navegacion Fase 1:

```
Principal  →  Alta de cuenta
           →  Editar cuenta (nombre / inicial / actual)
           →  Confirmar borrado
```

## Fase 2 — Totales y conversion

### Gasto por cuenta

`gastado = saldo_inicial - saldo_actual` (centavos de la moneda de la cuenta). Visible junto al saldo actual.

### Tipo de cambio USD → ARS

Campo editable por mes en la pantalla principal, formato visible: `1 USD = $ 1.500,00`. Se guarda en `tasa_cambio`.

- Si no hay tasa para el mes activo, se pueden ver las cuentas, pero el consolidado no se muestra hasta que el usuario la cargue.
- Una sola tasa por mes (la que el usuario decida usar).

### Totales al pie, separados por moneda

- **Total gastado USD** = Σ (inicial − actual) de cuentas USD
- **Total gastado ARS** = Σ de cuentas ARS
- **Total consolidado en $** = `total_ars + (total_usd * usd_a_ars ± 50) / 100`

Ejemplo: gastado USD $100,00 (10000 centavos) + gastado ARS $5.000,00 con tasa 150000:

`5.000.000 + 10.000 * 150000 / 100 = 20.000.000` centavos = **$ 200.000,00**.

## Fase 3 — Historial mensual

- Vista de meses anteriores y comparativa de totales gastados.
- El mes activo deja de ser solo el del reloj: selector de mes.
- Los snapshots ya existen (`saldo_mes` por `YYYY-MM`); esta fase es UI + el rollover automatico al detectar un mes nuevo.

## Fase 4 — Movimientos

- Registrar ingresos/egresos individuales (**cuenta**, fecha, monto, moneda, concepto) que actualizan `saldo_actual` del mes correspondiente.
- Reemplazan la edicion directa del saldo actual.
- Un movimiento pertenece siempre a una cuenta. La moneda del movimiento es la de la cuenta.

## Fase 5 — Categorias y reportes

- Categoria opcional en el **movimiento** (no en la cuenta).
- Reportes de gasto por categoria y por mes, con graficos simples.

## Fase 6 — Seguridad y datos

- Exportar/importar datos (CSV), copia de respaldo de la base.
- Bloqueo de la app (contrasena o biometria).
