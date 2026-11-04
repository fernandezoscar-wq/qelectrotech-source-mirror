# Coordinación con la protección y caída acumulada (hito 5)

## Coordinación conductor ↔ protección

ITC‑BT‑22, apartado 1.1, que remite a la UNE 20460‑4‑43, apartado 433 (hoy
UNE‑HD 60364‑4‑43), con las condiciones de la Guía‑BT‑22:

    1)  IB ≤ IN ≤ IZ
    2)  I2 ≤ 1,45 · IZ

| Dispositivo (norma) | I2 |
|---|---|
| Magnetotérmico de uso doméstico (UNE‑EN 60898) | 1,45 · IN |
| Magnetotérmico diferencial (UNE‑EN 61009) | 1,45 · IN |
| Interruptor automático industrial (UNE‑EN 60947‑2) | 1,30 · IN |
| Fusible gG (UNE‑EN 60269‑1), IN ≥ 16 A | 1,6 · IN |
| Fusible gG de menos de 16 A, u otro dispositivo | **I2 del fabricante**, con su fuente: el programa no lo supone |

- **Cada condición se comprueba por separado** (IB ≤ IN, IN ≤ IZ, I2 ≤ 1,45 IZ) y
  se ve en el detalle. El resultado global es «no superada» en cuanto una falla, y
  «superada» solo si se cumplen las tres.
- Un **diferencial solo** o un **interruptor** no protegen el cable contra
  sobrecargas. La comprobación sale «no realizada» y se indica el motivo.
- IN es la intensidad nominal o, en aparatos regulables, la de regulación.
- No se comprueba aquí el cortocircuito (poder de corte, I²t), que llega en el hito 7.

## Caída de tensión desde el origen

Cada cuadro puede indicar **qué circuito lo alimenta**. La caída de un circuito
se suma a la de las alimentaciones de los cuadros por los que pasa, hasta el
**origen de la instalación interior**:

- un cuadro sin alimentación es el origen, y queda anotado como hipótesis;
- una alimentación con uso **LGA** o **DI** (instalación de enlace) también
  cierra la cadena: su caída **no** se suma a la interior, porque cada una tiene
  su propio límite.

Los circuitos interiores se comprueban **con la caída desde el origen**
(ITC‑BT‑19 2.2.2, «menor del» 3 % / 5 %). En el panel ya no es una comprobación
parcial. Si falta algún dato aguas arriba:

- si la caída del circuito por sí sola ya supera el límite → **no superada**;
- si no la supera → **no realizada**, indicando qué alimentación falla (uso
  desconocido, datos que faltan o un bucle entre cuadros).

Las caídas en % de tramos de distinta tensión (alimentación trifásica a 400 V,
circuito monofásico a 230 V) se suman directamente, que es la práctica habitual
para cargas equilibradas.

### Límites de la instalación de enlace

| Uso | Límite | Fuente |
|---|---|---|
| LGA, contadores totalmente centralizados | 0,5 % | ITC‑BT‑14 3 |
| LGA, centralizaciones parciales | 1 % | ITC‑BT‑14 3 |
| DI, contadores concentrados en más de un lugar | 0,5 % | ITC‑BT‑15 3 |
| DI, contadores totalmente concentrados | 1 % | ITC‑BT‑15 3 |
| DI de un único usuario, sin LGA | 1,5 % | ITC‑BT‑15 3 |

Estos textos dicen «caída de tensión **máxima**», así que **la igualdad se
admite**. En la ITC‑BT‑19 («menor del») no se admite.

No se incluye todavía el caso de instalaciones alimentadas por un transformador
propio (límites distintos en la ITC‑BT‑19). Queda para el hito 8.

## Casos de referencia

Calculados aparte en [`refs_h5.py`](refs_h5.py):

| Caso | Datos | Resultado |
|---|---|---|
| Alimentación del CS | 3~ 400 V, 30 A, 20 m, 10 mm², γ 48, cos φ 0,9 | 1,9486 V → **0,4871 %** |
| C1 en el CS | 1~ 230 V, 16 A, 25 m, 2,5 mm², γ 48 | 6,6667 V → 2,8986 % |
| C1 desde el origen | 2,8986 + 0,4871 | **3,3857 %**: por sí solo pasaría el 3 %; desde el origen, **no superada** |
| Dos niveles | Otra alimentación igual | 3,8728 % |
| C1 con 20 m | 2,3188 + 0,4871 | 2,8060 % |
| P1 | IB 16, IN 16, IZ 17,32, UNE‑EN 60898 | I2 23,2 ≤ 25,11 → **superada** |
| P2 | IN 20 sobre IZ 17,32 | **no superada** (IN > IZ) |
| P5 | Fusible gG 16 A sobre IZ 17 | IN ≤ IZ, pero I2 25,6 > 24,65 → **no superada** |
| P11 / P12 | gG 16 A sobre IZ 17,6 / 17,7 | 25,6 > 25,52 no superada / 25,6 ≤ 25,665 superada |
| P8 | IB = IN = IZ = 16 | Superada (las igualdades se admiten) |

## Pruebas

| Prueba | Qué comprueba |
|---|---|
| `tst_rebtprotection` (17) | Factores de I2; 9 casos con las tres condiciones; fusible pequeño sin I2, con I2 y sin justificación; diferencial solo; datos que faltan; datos no válidos; límites de LGA y DI con la igualdad |
| `tst_electricalmodel` (+6) | Todos los usos y normas en el XML; suma desde el origen; dos niveles; la DI cierra la cadena y se comprueba con su propio límite; uso desconocido, datos que faltan, propia caída ya excesiva y bucles; coordinación en la evaluación |
| `tst_electricalpanel` (+5) | Columnas ΔU total y Protección ↔ cable; aviso de alimentación en los cuadros; eliminar una alimentación libera su cuadro (y se deshace en un paso); diálogo de cuadro; diálogo de circuito con la caída desde el origen mientras se escribe |

Se introdujeron 6 errores a propósito. El primer intento de cambiar 1,45 por 1,5
**no lo detectaba ninguna prueba**: se añadieron los casos límite P11 y P12, y
ahora sí lo detectan. Los otros cinco se detectaron desde el principio: fusible
gG pequeño con 1,6, no sumar las alimentaciones, LGA o DI que no cortan la
cadena, eliminar una alimentación sin liberar su cuadro y quitar la detección de
bucles (que provoca un bucle infinito y se detecta por tiempo).
