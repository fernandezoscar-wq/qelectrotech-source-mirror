# Generador fotovoltaico: línea del inversor y lado de continua

| Regla | Fuente | Qué se comprueba | Carácter |
|---|---|---|---|
| REBT-FV-001 | UNE-HD 60364-7-712:2017, 712.512.1.1.102.1 | UOC MAX ≤ tensión máxima del inversor y del módulo | Buena práctica |
| REBT-FV-002 | Hoja de datos del inversor | UMPP dentro del rango MPPT | Datos del fabricante |
| REBT-FV-003 | Hoja de datos del inversor | Corrientes de entrada por MPPT | Datos del fabricante |
| REBT-FV-004 | UNE-HD 60364-7-712, 712.430.3.103 y 712.433.1.101.2 | Si hacen falta fusibles de string, y su In | Buena práctica |
| REBT-FV-005 | UNE-HD 60364-7-712, tabla 712.2 | Corriente mínima del cable de string | Buena práctica |
| REBT-FV-006 | IDAE PCT-C-REV (julio 2011), 5.5.2 | Caída en continua «inferior del 1,5 %» | Recomendación |
| REBT-FV-007 | ITC-BT-40 5 (RD 842/2002) | Línea del inversor: cable para el 125 % y caída ≤ 1,5 % | Obligatorio |

La UNE-HD 60364-7-712 no figura en la ITC-BT-02. Sus requisitos son buena práctica, no obligatorios por el REBT, y el programa lo indica en cada regla. La numeración de los apartados es la de la IEC 60364-7-712:2017 y está pendiente de contrastar con el texto UNE.

## Uso

1. **Circuito de la línea del inversor.** En el diálogo del circuito elija el uso **«Línea del generador, del inversor al punto de conexión (ITC-BT-40)»**. La corriente de diseño es la corriente máxima de salida del inversor.
   - La caída se calcula solo para esta línea, con un límite del 1,5 % (no estricto).
   - La Iz se compara con 1,25 × IB.
   - La sección propuesta tiene en cuenta ese 1,25.
2. **Datos del generador.** En el mismo diálogo, pulse **Generador fotovoltaico...** y rellene:
   - **Módulo:** los valores STC de la hoja de datos (Voc, Vmpp, Isc, Impp), los coeficientes de Voc y Vmpp en %/°C, la tensión máxima del sistema, el fusible en serie máximo (IMOD_MAX_OCPR) y la fuente.
   - **Inversor:** tensión continua máxima, rango MPPT, corriente de entrada máxima y corriente de cortocircuito máxima por MPPT, y la fuente.
   - **Emplazamiento:** la temperatura de célula mínima y la máxima en funcionamiento, con su fuente.
   - **Strings:** una fila por entrada del inversor. Cada fila lleva:
     - módulos por string y strings en paralelo;
     - el fusible de string, si existe;
     - el cable más largo: longitud de ida, sección, material (Cu o Al) y temperatura del conductor;
     - la Iz del cable con su fuente.

Sin fuente, los datos del módulo, del inversor, las temperaturas y la Iz no se usan. Nada se supone: lo que falta sale como «no realizada».

## Cálculos

- **UOC MAX** = Nm · Voc · [1 + βVoc/100 · (Tmín − 25)].
- **UMPP** = Nm · Vmpp · [1 + βVmpp/100 · (T − 25)], a la temperatura máxima y a la mínima.
- **Corrientes de entrada:** Ns · Isc y Ns · Impp, en STC.
- **Fusibles de string:**
  - hacen falta si (Ns − 1) · 1,25 · Isc > IMOD_MAX_OCPR;
  - su intensidad debe cumplir 1,5 · Isc < In < 2,4 · Isc e In ≤ IMOD_MAX_OCPR.
- **Cable de string:**
  - si lleva fusible de string, debe soportar al menos la In del fusible;
  - si no lleva, al menos 1,25 · Isc · (Ns − 1). Con 1 o 2 strings, al menos 1,25 · Isc (es una interpretación).
- **Caída en continua** = 2 · L · Impp · ρ(θ) / S, dividido entre Nm · Vmpp.

## Limitaciones

- **Anexo B de la norma:** no se aplica. ISC MAX se toma como 1,25 · ISC STC, del lado de la seguridad.
- **Corrientes:** se usan en STC, sin corrección por temperatura ni por irradiancia.
- **Protecciones de continua:** no se comprueban las del subgenerador ni las del generador, ni la tensión asignada y el tipo gPV de los fusibles, ni el seccionador, ni el protector de sobretensiones.
- **Pérdidas:** no se consideran sombras, desajustes ni envejecimiento.
- **Baterías:** todavía no se tratan.

## Pruebas

**`tst_rebtpv`** usa el módulo del ejemplo de Prysmian Club (Voc 42,5 V, Vmpp 37,0 V, Isc 9,18 A, Impp 8,64 A, fusible máximo 15 A), con coeficientes de prueba, en 2 strings de 8 módulos y 30 m de cable de 4 mm². Resultados de referencia, calculados aparte:

| Magnitud | Valor |
|---|---|
| UOC MAX | 374,51 V |
| UMPP | 246,716 V a 334,332 V |
| Caída en continua | 0,962564 % |
| Fusibles de string, 2 strings | no hacen falta (11,475 A ≤ 15 A) |
| Fusibles de string, 3 strings | hacen falta (22,95 A > 15 A) |
| Caída con 2,5 mm² | 1,540102 %: no cumple |

También se prueban:
- los límites de tensión, del rango MPPT y de los fusibles;
- los datos sin fuente o no válidos.

**Otras pruebas:**
- `tst_electricalmodel`: los datos en el XML, la línea del generador (0,978261 %, el 125 %) y el generador a través del circuito.
- `tst_electricalpanel`: el diálogo y la columna «FV».
- `tst_electricalreport`: las notas y el recuento del informe.
