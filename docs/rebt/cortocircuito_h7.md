# Cortocircuito y poder de corte (hito 7)

Motor: `sources/rebt/rebtshortcircuit.h/.cpp`. Integración: `evaluate()` con el
modelo, panel (columnas «Icc máx», «Icc mín» y «Cortocircuito») e informe PDF.
Valores de referencia: `docs/rebt/refs_h7.py`, calculados aparte con las
fórmulas de las fuentes y no con el programa.

## Datos de alimentación del cuadro de origen

El cortocircuito depende de la red, y el programa no la inventa. En el cuadro
que no se alimenta de ningún circuito (diálogo «Cuadro», grupo «Alimentación
(cortocircuito)») el usuario elige una de estas opciones:

| Red | Cuándo | Qué se calcula |
|---|---|---|
| Desconocida | Por defecto | Nada: las comprobaciones salen «no realizadas» |
| Red desconocida, este cuadro es la CGP | Centro de transformación fuera del edificio. La LGA y la DI deben ser circuitos del modelo | Guía‑BT Anexo 3: Icc = 0,8 · U / R |
| Icc conocida en este cuadro | Carta de la distribuidora, estudio del CT propio, medida | Cotas a partir de la Icc máxima y mínima indicadas (kA), **siempre con su fuente** |

Un cuadro alimentado desde otro circuito no tiene datos propios: su Icc sale de
la línea. Si la Icc conocida no lleva su fuente, no se usa y se indica que falta
la justificación.

## Recorrido del defecto

Desde el cuadro del circuito se sube por **todas** las alimentaciones, LGA y DI
incluidas, hasta el cuadro de origen. Así se diferencia de la caída de tensión,
que se detiene en la LGA o la DI. Si hay un bucle entre cuadros, o el circuito
no cuelga de ningún cuadro, no se calcula nada y se dice por qué.

## Icc máxima, en el cuadro del que sale el circuito

Conductores a 20 °C (máxima Icc; UNE‑EN 60909‑0, Guía‑BT Anexo 3).
ρ20 según IEC 60228: Cu 1/58, Al 0,028264 Ω·mm²/m.

- **Guía‑BT Anexo 3**: `Icc = 0,8 · U0 / R`, con R la suma de las resistencias
  de ida y vuelta (fase + neutro) de las líneas desde la CGP. Es el ejemplo de la
  guía: 2·L en las dos líneas. Se desprecian las reactancias, como en la guía.
  Si el circuito sale del propio cuadro de origen, R = 0 y no hay valor.
- **Icc conocida**: `Zs = U0 / Icc_max,origen` y
  `Icc = U0 / √(Zs² + R_fase²)`. El ángulo de Zs no se conoce. Tomarla como
  reactancia pura da la **cota superior** para cualquier ángulo entre 0° y 90°,
  y solo la fase da la cota superior para cualquier tipo de defecto.

U0 = U en monofásica y U/√3 en trifásica.

## Icc mínima, fase‑neutro al final del circuito

Resistencias a la temperatura al final del cortocircuito, según UNE‑EN 60909‑0:
`R = [1 + 0,004 (θe − 20)] · R20`. θe es 160 °C en PVC (140 °C por encima de
300 mm²) y 250 °C en XLPE/EPR, las de la tabla 43A.

- **Guía**: `0,8 · U0 / √(R² + X²)`. Es una **extensión**, no un valor de la guía:
  el mismo factor 0,8 con las resistencias en caliente.
- **Icc conocida**: `U0 / (U0/Icc_min,origen + |Z línea|)`, sumando módulos (cota
  inferior). Hace falta la Icc mínima en el origen. No se deduce de la máxima.

Neutro: si no se indica su sección, para la Icc máxima se toma igual a la fase
(cota superior). Para la mínima se toma igual a la fase solo en circuitos de
instalaciones interiores, porque la ITC‑BT‑19 2.2.2 lo exige «como mínimo
igual a la de las fases». En la LGA, la DI o un circuito de uso desconocido hay
que indicarla.

## Comprobaciones

| Id | Comprobación | Fuente |
|---|---|---|
| REBT-ICC-003 | Poder de corte (Icu/Icn) ≥ Icc máx en el cuadro | ITC‑BT‑22 1.1 b; Guía‑BT‑22 |
| REBT-ICC-004 | Icc mín ≥ disparo instantáneo: B 5, C 10, D 20 · IN (límite superior del margen) | UNE‑EN 60898‑1, UNE‑EN 61009‑1 |
| REBT-ICC-005 | I²t ≤ k²S² | UNE‑HD 60364‑4‑43 434.5.2, tabla 43A |

k: Cu PVC 115 (103 por encima de 300 mm²), Cu XLPE/EPR 143, Al PVC 76 (68), Al
XLPE/EPR 94.

**Térmica sin I²t del fabricante.** Se da por superada solo con un aparato
UNE‑EN 60898/61009 cuya Icc mínima esté en la zona instantánea (despeja en
menos de 0,1 s) y con (k·S/Icc máx)² ≥ 0,1 s. En cualquier otro caso hace falta
la I²t del fabricante a la Icc máxima, con su fuente. No se considera la
asimetría.

Si se indica la intensidad a la que se dio la I²t, por ejemplo la de un
catálogo, la I²t se usa solo cuando esa intensidad no es menor que la Icc
máxima. Ver [catalogo_aparatos.md](catalogo_aparatos.md).

Los interruptores industriales (UNE‑EN 60947‑2) y los fusibles no tienen margen
normalizado: hay que indicar su intensidad de disparo instantáneo, con su fuente.
Un diferencial solo o un interruptor no cortan cortocircuitos, y se indica.

Igualdades: «mayor o igual» en las tres comprobaciones. La igualdad supera.

## Lo que no hace

- No comprueba el conductor de protección, la selectividad, la protección
  contra contactos indirectos ni el mínimo de 4500 A del IGA (ITC‑BT‑17 1.3).
- No tiene en cuenta la filiación (protección en cascada).
- No calcula la contribución de motores, la corriente de cresta ni la asimetría.
- Circuitos trifásicos sin neutro: el defecto mínimo calculado es fase‑neutro.

## Casos de referencia (`refs_h7.py`)

| Caso | Datos | Icc máx (A) | Icc mín (A) |
|---|---|---|---|
| Ejemplo de la guía | LGA 95 mm² 25 m + DI 10 mm² 15 m, ρ 0,018 | 2898,84 (la guía: 2898) | — |
| G | LGA 95/50 mm² 25 m, DI 10 mm² 15 m XLPE; circuito 2,5 mm² 20 m PVC, C16 | 2835,916 | 331,580 |
| H | DI 10 mm² 120 m XLPE; circuito 2,5 mm² 10 m PVC, C16 | 444,667 | 182,240 |
| K | Icc origen 10/6 kA; 16 mm² 30 m + 4 mm² 25 m XLPE, X 0,08 Ω/km | 5797,162 | 399,073 |
| M (modelo) | CPM (guía) → DI 10 mm² 15 m → CGMP → C1 2,5 mm² 20 m PVC | 3557,333 | 347,396 |

Pruebas: `tst_rebtshortcircuit` (21), más casos en `tst_electricalmodel`,
`tst_electricalpanel` y `tst_electricalreport`. Mutaciones del motor: 18 de 18
detectadas. Las de la integración también, salvo una equivalente: el código que
la contenía se ha eliminado.
