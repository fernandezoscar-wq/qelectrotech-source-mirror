# Contactos indirectos (ITC-BT-24 4.1)

Protección por corte automático de la alimentación en los tres esquemas de la ITC-BT-08. Fuente: ITC-BT-24, apartados 4.1, 4.1.1, 4.1.2 y 4.1.3 (RD 842/2002, BOE de 18-09-2002).

| Regla | Esquema | Condición |
|---|---|---|
| REBT-CIN-001 | TT (4.1.2) | RA × Ia ≤ U |
| REBT-CIN-002 | TN (4.1.1, tabla 1) | Zs × Ia ≤ U0; en TN-C no se admiten diferenciales |
| REBT-CIN-003 | IT, primer defecto (4.1.3) | RA × Id ≤ UL |
| REBT-CIN-004 | IT, segundo defecto (4.1.3, tabla 2) | Masas separadas: como TT. Masas interconectadas: 2 Zs × Ia ≤ U (neutro no distribuido) o 2 Z's × Ia ≤ U0 (neutro distribuido) |

En todas, la igualdad cumple: el texto dice «≤».

## Magnitudes

- **RA**: suma de la resistencia de la toma de tierra y de los conductores de protección de las masas, en Ω. La da el usuario con su fuente (medida, o estimada según ITC-BT-18). No se calcula.
- **U, UL**: tensión de contacto límite convencional. Es 50 V en condiciones normales, y se toma así si no se indica otra. Un valor menor se da con su motivo. No se admite un valor mayor de 50 V.
- **Ia**:
  - Con diferencial, Ia es su IΔn.
  - Sin diferencial, Ia es el disparo instantáneo del interruptor: el indicado con su fuente, o el límite de la curva B, C o D (5, 10 o 20 IN) de un aparato UNE-EN 60898 o 61009.
  - El disparo instantáneo actúa en menos de 0,1 s, dentro de todos los tiempos de las tablas 1 y 2. No se usa la intensidad correspondiente a un tiempo mayor de esas tablas.
- **Zs (TN)**: impedancia de la fuente más la de los conductores de fase y de protección de cada línea, desde el origen hasta el final del circuito, LGA y DI incluidas. Las resistencias se toman a la temperatura final del cortocircuito, como en la Icc mínima, y las reactancias se suman cuando se indican.
  - Si se conoce la Icc del origen, la fuente aporta U0 / Icc mín.
  - Si no se conoce y el origen es la CGP, Zs = R / 0,8, con el factor de la Guía-BT Anexo 3.
- **Zs (IT)**: conductores de fase y de protección de las líneas que salen del cuadro donde se indica el esquema IT, sin fuente.
  - **Z's**: conductores de neutro y de protección. Un neutro que falta se toma igual a la fase solo en la instalación interior (ITC-BT-19 2.2.2).
  - Se incluyen las derivaciones aguas abajo de ese cuadro. Así Zs sale mayor y el resultado queda del lado de la seguridad (es una interpretación).
  - Con neutro no distribuido, U = √3 U0.
- **Id**: corriente del primer defecto en IT. La da el usuario con su fuente, porque depende de las capacidades de fuga. No se calcula.

El conductor de protección de cada línea (sección del PE, o del PEN en TN-C) **es obligatorio** en TN y en IT con masas interconectadas. **Nunca se supone.**

## Datos

**Diálogo del cuadro**, grupo «Puesta a tierra y contactos indirectos»:

- Esquema: TT, TN-S o TN-C-S, TN-C o IT. Si un cuadro no lo indica, toma el del cuadro que lo alimenta, subiendo hasta el primero que lo indique.
- RA y su fuente, y U y su motivo. Se usan en TT e IT.
- Solo en IT:
  - Id y su fuente;
  - masas: interconectadas con una sola tierra, o puestas a tierra por separado;
  - neutro: distribuido o no.

**Diálogo del circuito**:

- Sección del conductor de protección. Ya existía; ahora la usan TN e IT.
- El diferencial que protege el circuito, buscado en su cuadro o en uno de aguas arriba. Un RCBO usa su propia sensibilidad.

En TN la fuente es la del cuadro de origen, con los datos de alimentación del cortocircuito: método y, si se conoce, Icc mínima con su fuente.

## Resultados

- **Panel**: columna «Contactos indirectos», con el detalle en la ayuda emergente:
  - en TT, RA máxima y RA × Ia;
  - en TN, Zs, Zs máxima, límite y Zs × Ia;
  - en IT, primer defecto (RA × Id) y segundo defecto, que puede ser 2 Zs × Ia;
  - en todos los casos, las hipótesis y las reglas.
- **Informe**: columna «ITC-BT-24», notas de cada circuito y fila del resumen, estas dos últimas cuando algún cuadro indica su esquema. La advertencia final dice qué se comprueba.
- **IT**: el resultado es «superada» solo si se superan el primer y el segundo defecto. Si uno de los dos no se supera, el resultado es «no superada».

## Limitaciones (no se comprueba)

- El cálculo de RA o de Id.
- Las medidas de la ITC-BT-24 distintas de las condiciones de corte automático: contactos directos, clase II, separación eléctrica, MBTS y conexiones equipotenciales suplementarias.
- Los tiempos mayores que la norma UNE 20460-4-41 permite en casos especiales.
- La puesta a tierra repartida del conductor de protección en TN.
- La disposición del PEN en TN-C-S: si hay algún PEN aguas abajo de un diferencial.
- El controlador permanente de aislamiento en IT y su señal.
- Que todas las masas protegidas por un mismo dispositivo compartan toma de tierra.
- El tiempo de los diferenciales selectivos (tipo S, como máximo 1 s).
- En TN con fuente conocida, se toma como impedancia de la fuente la que resulta de la Icc mínima fase-neutro dada en el origen.
- En IT, se supone que el neutro no está puesto a tierra.

## Pruebas

**`tst_rebtindirect`**, con valores calculados a mano. Constantes: ρ = 1/58; R(θ) = [1 + 0,004 (θ − 20)] R20, con 160 °C en PVC y 250 °C en XLPE.

- **TT**: 30 Ω y 30 mA dan 0,9 V; los límites de 50 V y 24 V; C16 y B16.
- **TN con Icc mínima de 5 kA y C16 de 25 m (2,5/2,5 PVC)**:
  - Zs = 0,046 + 0,537931 = 0,583931 Ω, y Zs × Ia = 93,43 V: cumple.
  - Con 70 m, 248,35 V: no cumple.
  - Con 30 mA, Zs máxima de 7666,67 Ω.
- **TN con factor de la guía**: derivación de 15 m 10/10 XLPE (0,099310 Ω) más el circuito; Zs = 0,637241 / 0,8 = 0,796552 Ω, y Zs × Ia = 127,45 V.
- **TN, casos sin datos o no válidos**: PE ausente o nulo, origen o tensión desconocidos, cadena no seguida.
- **TN-C**: con interruptor, se comprueba como TN; con diferencial, no cumple.
- **IT, primer defecto con RA de 20 Ω**: Id 0,5 A da 10 V; 3 A, 60 V; 2,5 A, 50 V en el límite. También Id sin fuente.
- **IT, segundo defecto**:
  - masas separadas: 0,6 V con diferencial y 3200 V con el interruptor;
  - masas interconectadas, neutro no distribuido: 2 × 0,537931 × 160 = 172,14 V frente a √3 × 230 = 398,37 V;
  - neutro distribuido: con 50 m, 344,28 V frente a 230 V;
  - fuera de la instalación interior, el neutro que falta no se supone.

**`tst_electricalmodel`**:

- **XML**: todos los datos nuevos, incluidos TN-C e IT.
- **TN a través de la cadena**: 230/5000 + 0,132414 + 0,537931 = 0,716345 Ω, y 114,62 V.
- **PE de la derivación**: se pide por su nombre.
- **IT indicado en CS**: solo cuenta el circuito, 0,537931 Ω.
- **IT indicado en el CGMP**: cuenta también la derivación, 0,670345 Ω.
- **TN-C**: con el diferencial de aguas arriba no cumple.

**`tst_electricalpanel`**:

- el diálogo del cuadro: RA y U en TT e IT, los datos propios de IT y TN-C en la lista;
- la columna del panel.

**`tst_electricalreport`**: la columna, las notas, el recuento y la advertencia final.
