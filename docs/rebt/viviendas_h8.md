# Viviendas (ITC‑BT‑25) y transformador propio (hito 8)

Motor: `sources/rebt/rebtdwelling.h/.cpp` y dos límites nuevos en
`rebtrules`. Integración: campos del circuito y del cuadro, `evaluate()`,
columna «ITC‑BT‑25» del panel y del informe.

## Circuitos de la tabla 1 de la ITC‑BT‑25

En el diálogo del circuito, «Circuito de vivienda» indica cuál de la tabla 1 es
y «Puntos de utilización», cuántos puntos de luz o tomas tiene. Si no se indica,
el circuito no es de vivienda y no se comprueba nada de esto.

Texto: ITC‑BT‑25 (RD 842/2002), con la modificación del RD 1053/2014 (C13).

| Circuito | Interruptor ≤ (A) | Sección Cu ≥ (mm²) | Puntos ≤ | Potencia ≤ (W) | Tubo ≥ (mm) |
|---|---|---|---|---|---|
| C1 Iluminación · C6 | 10 | 1,5 | 30 | — | 16 |
| C2 Tomas de uso general · C7 | 16 | 2,5 | 20 | — | 20 |
| C3 Cocina y horno · C12 (C3) | 25 | 6 | 2 | — | 25 |
| C4 Lavadora, lavavajillas, termo · C12 (C4) | 20 | 4 | 3 | — | 20 |
| C4.x Un aparato del C4, circuito propio (nota 8) | 16 | no lo fija | 1 | — | no lo fija |
| C5 Baño y cuarto de cocina · C12 (C5) | 16 | 2,5 | 6 | — | 20 |
| C8 Calefacción · C9 Aire acondicionado | 25 | 6 | — | 5750 (nota 2) | 25 |
| C10 Secadora | 16 | 2,5 | 1 | — | 20 |
| C11 Automatización | 10 | 1,5 | — | 2300 (nota 4) | 16 |
| C13 Recarga de vehículo eléctrico | ITC‑BT‑52 | 2,5 | 3 | ITC‑BT‑52 | 20 |

- **Cobre obligatorio** («Los conductores serán de cobre»): en aluminio no se supera.
- **Interruptor:** el valor de la tabla se toma como el **máximo**. Uno menor no
  se señala; que cubra la intensidad del circuito ya lo comprueba IB ≤ IN.
- **Secciones:** valen para dos conductores y tierra, PVC, bajo tubo empotrado
  (nota 5). Con otras condiciones puede hacer falta más; eso lo comprueba IB ≤ IZ.
- **C4 desdoblado (nota 8):** un circuito de 16 A por aparato. La norma no fija
  su sección, así que el programa no la comprueba.
- **C13:** el interruptor, la potencia y las tomas son de la ITC‑BT‑52. No se
  comprueban.
- **Tubo:** diámetro exterior (nota 3, ITC‑BT‑19). C6, C7 y C12 llevan los
  valores de su tipo («circuito adicional del tipo C1»…).
- Una casilla vacía de la tabla no se comprueba. Si falta un dato que sí hace
  falta, la comprobación queda «no realizada».

## Caída de tensión con la intensidad del interruptor (ITC‑BT‑25 3)

La caída se calcula «para una intensidad de funcionamiento del circuito igual a
la intensidad nominal del interruptor automático», hasta el punto más alejado y
desde el origen de la instalación interior. Por eso se suman las alimentaciones
interiores, como en el hito 5. El límite es «como máximo el 3 %»: la igualdad
supera, al contrario que el «menor del 3 %» de la ITC‑BT‑19.

- cos φ: el de la carga. Si no se da, 1, como la tabla B de la Guía‑BT‑25, y
  queda anotado.
- La compensación con la DI que permite la ITC‑BT‑19 2.2.2 no se aplica.

## Electrificación básica: C1 a C5 (ITC‑BT‑25 2.3.1)

Para cada cuadro de origen con circuitos de vivienda se comprueba que existan
C1, C2, C3, C4 (o sus desdoblamientos) y C5. Se cuentan también los circuitos de
los subcuadros que cuelgan de él. Un C6 no sustituye al C1, ni un C12 al C3. Se
ve en la fila del cuadro y en el subtítulo del informe.

## La vivienda en conjunto

En el cuadro donde empieza la vivienda (sin alimentación, o alimentado por su
DI) se indican, en el grupo «Vivienda (ITC‑BT‑25)»:
- la superficie útil;
- si están previstas la calefacción eléctrica, el aire acondicionado, la
  automatización y la recarga del vehículo eléctrico (cada una sí, no o
  desconocido);
- si es vivienda unifamiliar;
- la intensidad del interruptor general (IGA).

Los diferenciales se dan por cuadro (nombre, sensibilidad, intensidad
asignada), y cada circuito nombra el que lo protege. El nombre se busca primero
en su cuadro y luego en los cuadros de aguas arriba, siguiendo las
alimentaciones: un diferencial que protege la línea de un subcuadro protege
también sus circuitos. Un magnetotérmico diferencial se protege a sí mismo. Si
el nombre no se encuentra, el circuito cuenta como sin diferencial conocido y
las comprobaciones de diferenciales quedan «no realizadas».

**Grado de electrificación (2.3.2).** Es elevado en cuanto se sabe que se
cumple una de estas condiciones:
- más de 160 m² (estricto: «superior a»);
- calefacción, aire acondicionado o automatización previstos;
- recarga en vivienda unifamiliar (también ITC‑BT‑10 5.1);
- un circuito además de los básicos (C6, C7, C8, C9, C11 o C12).

La secadora no es condición. Si no se cumple ninguna y falta algún dato, el
grado queda **sin determinar**, y se dice qué falta.

| Comprobación | Regla |
|---|---|
| C1 a C5 presentes | REBT-VIV-003 |
| Circuitos de lo previsto: C7 si > 160 m², C8 calefacción, C9 aire, C11 automatización, C13 recarga (solo unifamiliar: en aparcamientos colectivos lo sustituye la ITC‑BT‑52) | REBT-VIV-004 |
| IGA ≥ 25 A; con electrificación elevada ≥ 40 A (ITC‑BT‑10 2.2: no menos de 9200 W, potencia que fija el IGA; 40 A según la tabla A de la Guía‑BT‑25) | REBT-VIV-005 |
| IZ de la DI ≥ IGA (capacidad de la DI, apartado 1) | REBT-VIV-005 |
| Un diferencial como mínimo por cada cinco circuitos | REBT-VIV-006 |
| Todos los circuitos a 30 mA como máximo | REBT-VIV-006 |
| Intensidad asignada de cada diferencial ≥ IGA (no se pide a un magnetotérmico diferencial de un solo circuito) | REBT-VIV-006 |
| C13 con diferencial exclusivo (ITC‑BT‑52) | REBT-VIV-006 |

Con el grado sin determinar, un IGA de 25 A a 39 A queda «no realizada»: basta
para la básica, no para la elevada. Uno de 40 A o más supera, y uno de menos de
25 A no supera.

El recuento de diferenciales es el del texto (uno por cada cinco circuitos
instalados, en la vivienda). No se comprueba cómo se reparten los circuitos
entre ellos.

## Estancias: puntos mínimos (tabla 2)

Desde el cuadro de origen, el botón «Estancias» abre una tabla con una fila por
estancia: nombre, tipo, superficie, longitud y accesos (pasillos), y cuántos
tiene de cada elemento. Una casilla vacía no se conoce.

| Estancia | Exigido |
|---|---|
| Acceso | 1 pulsador de timbre (no cuenta como punto de utilización) |
| Vestíbulo | 1 punto de luz, 1 interruptor, 1 base C2 |
| Salón | luz: 1 hasta 10 m², 2 si S > 10 m²; un interruptor por punto de luz; bases C2: una por cada 6 m² redondeando hacia arriba, 3 como mínimo; calefacción y aire, si están previstos: 1 hasta 10 m², 2 por encima |
| Dormitorio | luz y bases C2 como el salón; calefacción y aire, si están previstos: 1 |
| Baño | 1 punto de luz, 1 interruptor, 1 base C5; calefacción, si está prevista: 1 |
| Pasillo o distribuidor | un punto de luz cada 5 m (redondeando hacia arriba: interpretación, el texto no lo dice); un interruptor en cada acceso; bases C2: 1 hasta 5 m, 2 por encima; calefacción, si está prevista: 1 |
| Cocina | luz por superficie, un interruptor por punto de luz; 2 bases C2 (extractor y frigorífico); 1 base C3 de 25 A; 3 bases C4; 3 bases C5 sobre el plano de trabajo; calefacción y secadora (C10), si están previstas: 1 |
| Terraza o vestidor | luz por superficie, un interruptor por punto de luz |
| Garaje u otras | luz y bases C2 por superficie, un interruptor por punto de luz; toma C13, si está prevista la recarga: 1 |

Los mínimos son eso: más supera. Interruptor, conmutador y similares cuentan
igual (Guía‑BT‑25). No se comprueban las ubicaciones (nota 2: fuera de 0,5 m del
fregadero y de la cocina) ni las bases múltiples de TV (nota 1). Sin estancias
indicadas, la comprobación de la vivienda queda «no realizada» (REBT-VIV-007).

**Secadora:** si está prevista una secadora independiente, se exige el C10 y su
base en la cocina. No es condición de la electrificación elevada.

## Compensación de la caída con la DI (ITC‑BT‑19 2.2.2)

En un cuadro alimentado por una DI se puede marcar «Compensar la caída de
tensión con la DI». Entonces la caída desde el origen más la caída de la DI debe
ser **inferior** a la suma de los dos límites (REBT-CDT-007). Por ejemplo, para
una vivienda con DI de un único usuario: 3 % + 1,5 % = 4,5 %. Se aplica también
a la caída con la IN de los circuitos de vivienda. La DI ya no se comprueba
sola con su límite: lo hacen sus circuitos. Si la caída de la DI no se conoce,
la comprobación queda «no realizada», nunca «no superada» por el límite que ya
no se aplica.

| Caso (modelo de prueba) | Cálculo | Resultado |
|---|---|---|
| C1 de vivienda, sin compensar | 3,385690 % | no supera el 3 % |
| El mismo, compensado con una DI de 40 A, 12 m, 10 mm², γ 48 | 3,385690 + 2·12·40/(48·10)/230 % = 3,385690 + 0,869565 = 4,255255 % | supera (< 4,5 %) |

No se comprueban:
- C6 y C12 por número: ya los obliga el máximo de puntos de la tabla 1;
- los C12 de tipo C3 o C4: un C3 o C4 adicional es decisión de quien proyecta;
- la capacidad de la instalación interior en su conjunto;
- el ICP;
- la protección contra contactos indirectos por otros medios (ITC‑BT‑24 4.1);
- los factores Fs y Fu.

## Transformador propio (ITC‑BT‑19 2.2.2)

En el cuadro de origen, «Instalación industrial alimentada en alta tensión con
transformador propio». La instalación interior empieza a la salida del
transformador, y las «caídas de tensión máximas admisibles» pasan a:

| Id | Uso | Límite | Igualdad |
|---|---|---|---|
| REBT-CDT-005 | Alumbrado | 4,5 % | supera |
| REBT-CDT-006 | Demás usos | 6,5 % | supera |

Los circuitos de vivienda conservan su 3 %. Solo se aplica en el cuadro
**alimentado por nada**: un cuadro que cuelga de una LGA o una DI no tiene
transformador propio. No se incluye la caída del propio transformador.

## Corrección: el origen de la instalación interior

El cuadro donde empieza la instalación interior se deducía en el mismo bucle
que suma las caídas de las alimentaciones. Si una alimentación intermedia no
tenía los datos para calcular su caída, el bucle se paraba antes y tomaba un
subcuadro por origen. Eso afectaba a los circuitos mínimos de la vivienda y a
los límites del transformador propio. Ahora el origen sale solo de los cuadros
y de quién los alimenta (`interiorOrigin`). Si falta un dato de caída, la caída
desde el origen queda «no realizada», pero el origen sigue siendo el correcto.

## Reglas nuevas

| Id | Fuente | Contenido |
|---|---|---|
| REBT-VIV-001 | ITC‑BT‑25 3, tabla 1 | Interruptor, sección, cobre, puntos y potencia |
| REBT-VIV-002 | ITC‑BT‑25 3 | ΔU con IN desde el origen, como máximo el 3 % |
| REBT-VIV-003 | ITC‑BT‑25 2.3.1 | C1 a C5 como mínimo |
| REBT-VIV-004 | ITC‑BT‑25 2.3.2 | Grado de electrificación y circuitos de lo previsto |
| REBT-VIV-005 | ITC‑BT‑25 2.1 y 1; ITC‑BT‑10 2.2 | IGA y capacidad de la DI |
| REBT-VIV-006 | ITC‑BT‑25 2.1 y 2.3.2 | Interruptores diferenciales |
| REBT-VIV-007 | ITC‑BT‑25 4, tabla 2 | Puntos mínimos por estancia |
| REBT-CDT-007 | ITC‑BT‑19 2.2.2 | Caída compensada con la DI |
| REBT-CDT-005/006 | ITC‑BT‑19 2.2.2 | 4,5 % / 6,5 % con transformador propio |

## Pruebas

- `tst_rebtdwelling` (69): cada fila de la tabla copiada a mano del texto
  publicado, en el valor exacto (supera) y un paso más allá (no supera). Cada
  condición de la electrificación elevada con el circuito que exige, los
  límites del IGA y de la DI, y el recuento de diferenciales. Cada regla de la
  tabla 2 en su borde (10 m² frente a 10,5 m²; 18 m² frente a 18,1 m²; pasillos de
  5 m frente a 5,2 m) y la compensación en su igualdad.
- Más casos en `tst_electricalmodel`, `tst_electricalpanel` y
  `tst_electricalreport`.

Casos de caída con IN, con la alimentación del modelo de prueba de 0,487139 %:

| Circuito | Cálculo | Desde el origen | Resultado |
|---|---|---|---|
| C2, IN 16 A, 25 m, 2,5 mm², γ 48 | 2·25·16/(48·2,5) = 6,6667 V = 2,898551 % | 3,385690 % | no supera |
| El mismo con IN 10 A (IB 6 A) | 4,1667 V = 1,811594 % | 2,298733 % | supera |
| Otros usos, 45 m, transformador propio | 5,217391 % + 0,487139 % | 5,704530 % | supera el 6,5 %; no supera el 5 % ni el 4,5 % |

Mutaciones del motor: 18 de 18 en la tabla 1, 22 de 22 en la vivienda en
conjunto y 20 de 20 en estancias, secadora y compensación.
