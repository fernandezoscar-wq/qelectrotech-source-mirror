# Módulo REBT (`sources/rebt`)

Biblioteca estática `qetrebt`, en C++17 puro y sin Qt, con los cálculos de baja
tensión según el REBT (RD 842/2002). En el hito 1 **todavía no la usa la
aplicación**: se compila y se prueba sola.

## API

```cpp
#include "rebt/rebtcalc.h"
#include "rebt/rebtrules.h"

rebt::CircuitInput in;
in.system = rebt::System::SinglePhase;
in.voltage = 230; in.power = 3680; in.powerFactor = 1;
in.length = 25; in.section = 2.5;
in.material = rebt::Material::Copper; in.conductorTemperature = 70;

rebt::VoltageDropResult r = rebt::voltageDrop(in);
if (r.status == rebt::Status::Ok) {
	rebt::CheckResult c = rebt::checkVoltageDrop(r, rebt::rules::dwellingInterior());
	// c.outcome: Passed / NotPassed / NotPerformed, c.rule: fundamento normativo
} else {
	// r.diagnostics: lo que falta o es incorrecto. No hay valor numérico.
}
```

| Función | Qué hace |
|---|---|
| `designCurrent(in)` | Intensidad de cálculo, monofásica o trifásica, con rendimiento opcional |
| `resistivity(material, T)` | ρ del Cu o del Al a la temperatura T |
| `voltageDrop(in)` | Caída de un tramo con la carga al final, en V y en %. Usa `in.designCurrent` si se da; si no, la calcula |
| `checkVoltageDrop(r, limit)` | Superado, no superado o no realizado, con la regla aplicada |

## Principios

- **No se inventan datos.** Todos los campos son `std::optional`. Si falta algo, el resultado es
  `NotComputable` con la lista de lo que falta.
- **Trazabilidad.** Cada resultado lleva las reglas aplicadas (`NormRef`: id,
  fuente, apartado, versión y limitaciones) y las hipótesis tomadas
  (`Assumption`), por ejemplo «reactancia despreciada» o «carga al final del tramo».
- **Nunca se declara cumplimiento global.** Solo hay comprobaciones individuales.
- Los textos del módulo son códigos. La interfaz los traducirá con `tr()`.

## Reglas incluidas

| Id | Fuente | Contenido |
|---|---|---|
| REBT-I-001 | Teoría de circuitos; Guía‑BT Anexo 2 | Intensidad de cálculo |
| REBT-CDT-001 | Teoría de circuitos; Guía‑BT Anexo 2 | Caída de tensión de un tramo |
| REBT-RHO-001 | IEC 60028 / IEC 60889 | Resistividad y temperatura |
| REBT-CDT-002 | ITC‑BT‑19 2.2.2 | < 3 % en circuitos interiores de viviendas |
| REBT-CDT-003 | ITC‑BT‑19 2.2.2 | < 3 % en alumbrado de otras instalaciones interiores |
| REBT-CDT-004 | ITC‑BT‑19 2.2.2 | < 5 % en los demás usos de otras instalaciones interiores |
| REBT-CDT-005 | ITC‑BT‑19 2.2.2 | ≤ 4,5 % alumbrado, transformador propio |
| REBT-CDT-006 | ITC‑BT‑19 2.2.2 | ≤ 6,5 % demás usos, transformador propio |
| REBT-VIV-001 | ITC‑BT‑25 3, tabla 1 | Circuitos de vivienda |
| REBT-VIV-002 | ITC‑BT‑25 3 | ΔU con la IN del interruptor ≤ 3 % |
| REBT-VIV-003 | ITC‑BT‑25 2.3.1 | C1 a C5 como mínimo |
| REBT-VIV-004 | ITC‑BT‑25 2.3.2 | Grado de electrificación y circuitos de lo previsto |
| REBT-VIV-005 | ITC‑BT‑25 2.1, 1; ITC‑BT‑10 2.2 | IGA ≥ 25 A (40 A elevada), IZ de la DI ≥ IGA |
| REBT-VIV-006 | ITC‑BT‑25 2.1, 2.3.2 | Diferenciales: uno por cada cinco circuitos, 30 mA, ≥ IGA, C13 exclusivo |
| REBT-VIV-007 | ITC‑BT‑25 4, tabla 2 | Puntos mínimos por estancia |
| REBT-CDT-007 | ITC‑BT‑19 2.2.2 | Caída compensada entre la instalación interior y la DI |
| REBT-ICC-001 | Guía‑BT Anexo 3 | Icc = 0,8 U / R, red desconocida |
| REBT-ICC-002 | UNE‑EN 60909‑0 | Icc a partir de la del origen (cotas) |
| REBT-ICC-003 | ITC‑BT‑22 1.1 b | Poder de corte ≥ Icc máx |
| REBT-ICC-004 | UNE‑EN 60898‑1, 61009‑1 | Icc mín ≥ disparo instantáneo |
| REBT-ICC-005 | UNE‑HD 60364‑4‑43 434.5.2 | I²t ≤ k²S² |
| REBT-CIN-001 | ITC‑BT‑24 4.1, 4.1.2 | Contactos indirectos en TT: RA × Ia ≤ U |
| REBT-CIN-002 | ITC‑BT‑24 4.1.1, tabla 1 | Contactos indirectos en TN: Zs × Ia ≤ U0; sin diferencial en TN-C |
| REBT-CIN-003 | ITC‑BT‑24 4.1.3 | IT, primer defecto: RA × Id ≤ UL |
| REBT-CIN-004 | ITC‑BT‑24 4.1.3, tabla 2 | IT, segundo defecto: como TT, o 2 Zs × Ia ≤ U (U0) |
| REBT-FV-001…006 | UNE‑HD 60364‑7‑712; IDAE PCT 5.5.2 | Lado de continua FV: UOC MAX, MPPT, corrientes, fusibles, cable y caída |
| REBT-FV-007 | ITC‑BT‑40 5 | Línea del inversor: cable para el 125 % y caída ≤ 1,5 % |

Pendiente de revisión: antes de publicar una versión hay que contrastar cada regla con
el texto consolidado del BOE (BOE‑A‑2002‑18099).

## Modelo eléctrico del proyecto

Desde el hito 2, los datos de cuadros y circuitos se guardan en el proyecto
(`sources/electrical`). Ver [modelo_h2.md](modelo_h2.md).

Desde el hito 3, el panel «Circuitos eléctricos» permite editarlos y ver sus
cálculos. Ver [panel_h3.md](panel_h3.md).

Desde el hito 4 se comprueba IB ≤ IZ con tablas que carga el usuario y se
propone la sección. Ver [tablas_h4.md](tablas_h4.md).

Desde el hito 5 se comprueba la coordinación con la protección (IB ≤ IN ≤ IZ,
I2 ≤ 1,45 IZ) y la caída de tensión desde el origen, a través de los cuadros.
Ver [coordinacion_h5.md](coordinacion_h5.md).

Desde el hito 6, el informe de cálculo en PDF (folios + anexo con cajetín) se
genera desde el panel o la línea de órdenes. Ver [informe_h6.md](informe_h6.md).

Desde el hito 7 se calculan la Icc máxima y mínima de cada circuito, a partir de
los datos de alimentación del cuadro de origen, y se comprueban el poder de
corte, el disparo instantáneo y la solicitación térmica (I²t ≤ k²S²). Ver
[cortocircuito_h7.md](cortocircuito_h7.md).

Desde el hito 8 se comprueban los circuitos de vivienda de la tabla 1 de la
ITC‑BT‑25 (interruptor, sección, puntos, tubo y caída con la IN), y la vivienda
en conjunto: grado de electrificación, circuitos exigidos, interruptor general,
DI y diferenciales. Con transformador propio, los límites del 4,5 % y el 6,5 %.
Ver [viviendas_h8.md](viviendas_h8.md).

La pestaña «Eléctrico» de las propiedades de un símbolo lo vincula a su cuadro
o circuito, edita los datos de su aparato (con fabricante y referencia) y muestra
sus comprobaciones. Ver [pestana_electrico.md](pestana_electrico.md).

El catálogo de aparatos es un fichero JSON del usuario; el programa no trae
ninguno. Al elegir una referencia se rellenan los datos del aparato, cada uno
con su fuente. La I²t va con la intensidad a la que se dio, y no se usa si esa
intensidad es menor que la Icc máxima. Ver [catalogo_aparatos.md](catalogo_aparatos.md).

Contactos indirectos (ITC-BT-24 4.1) en TT, TN (TN-S, TN-C-S, TN-C) e IT, con la
RA y la Id que da el usuario, el PE de cada línea y el diferencial o el
interruptor de cada circuito. Ver [contactos_indirectos.md](contactos_indirectos.md).

## Pruebas

`tests/qttest/tst_rebtcalc.cpp`. Los casos y su cálculo independiente están en
[casos_h1.md](casos_h1.md).

```sh
cmake --build build --target tst_rebtcalc
ctest --test-dir build -R '^tst_rebtcalc$' --output-on-failure
```

Generador fotovoltaico: la línea del inversor según la ITC-BT-40 y el lado de
continua (strings, inversor, fusibles, cable). Ver [fotovoltaica.md](fotovoltaica.md).
