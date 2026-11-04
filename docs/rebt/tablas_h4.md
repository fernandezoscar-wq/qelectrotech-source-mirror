# Intensidad admisible y sección (hito 4)

## Qué se comprueba

**IB ≤ IZ** (ITC‑BT‑19, apartado 2.2.3, que remite a la UNE 20460‑5‑523, hoy
UNE‑HD 60364‑5‑52):

    IZ = I tabla(material, aislamiento, conductores cargados, método, sección)
         × factor de temperatura × factor de agrupamiento [× otro factor]

y se **propone la sección** más pequeña de la tabla que cumple a la vez
IB ≤ IZ y el límite de caída de tensión del circuito. Si la caída no se puede
calcular (por ejemplo, falta la longitud), la propuesta se hace solo por
intensidad y el programa lo indica.

Lo que **no** se comprueba todavía: la coordinación con la protección
(IN, I2 ≤ 1,45·IZ) y la caída acumulada, que llegan en el hito 5, ni el
cortocircuito, en el hito 7.

## Por qué el programa no incluye tablas

Las tablas de la UNE‑HD 60364‑5‑52 tienen derechos de autor de UNE. Incluirlas
en un programa que se distribuye requiere licencia. Por eso:

- el programa **no incluye ninguna tabla**;
- se cargan desde **un fichero tuyo** (botón «Tablas de intensidades...» del
  panel), por ejemplo con los valores de tu copia de la norma o del catálogo
  de tu fabricante de cable;
- cada resultado indica **qué tabla y qué fuente** se han usado.

Sin tablas, la columna IZ muestra «—» y la comprobación sale «No realizada».

## Formato del fichero (`.json`)

```json
{
  "format": "qet-ampacity",
  "version": 1,
  "tables": [
    {
      "id": "PVC2-B1-Cu",
      "source": "<de dónde salen los valores: norma, tabla y columna, o catálogo>",
      "material": "Cu",
      "insulation": "PVC",
      "loadedConductors": 2,
      "method": "B1",
      "referenceAmbient": 30,
      "entries": [[1.5, <A>], [2.5, <A>], [4, <A>]]
    }
  ]
}
```

| Campo | Valores |
|---|---|
| `id` | Nombre libre, aparece en los resultados |
| `source` | **Obligatorio**: de dónde proceden los valores |
| `material` | `"Cu"` o `"Al"` |
| `insulation` | `"PVC"` (70 °C), `"XLPE"` o `"EPR"` (90 °C) |
| `loadedConductors` | `2` o `3` |
| `method` | Método de referencia, igual que en el circuito (`"B1"`, `"E"`…) |
| `referenceAmbient` | °C a la que valen las intensidades (habitualmente 30 en aire, 20 en terreno) |
| `entries` | Pares `[sección mm², intensidad A]` |

**Validación:** una tabla con cualquier error se **descarta entera** y el error
se muestra. Nunca se usa media tabla. Se rechazan: falta de fuente,
valores no positivos, secciones repetidas, intensidades que no crecen con la
sección (probable errata) y dos tablas para las mismas condiciones. El fichero
se vuelve a leer cuando cambia.

## Factores de corrección

| Factor | Cómo se obtiene |
|---|---|
| Temperatura | Si se da la temperatura ambiente, **se calcula**: k = √((θmáx − θa)/(θmáx − θref)), con θmáx = 70 °C (PVC) o 90 °C (XLPE, EPR) y θref la de la tabla. Coincide con los factores publicados una vez redondeados (PVC a 40 °C: 0,866 → 0,87; XLPE a 40 °C: 0,913 → 0,91; PVC a 25 °C: 1,061 → 1,06). Si prefiere el valor de la tabla, escríbalo con su justificación |
| Agrupamiento | **Siempre obligatorio, con justificación.** Para un circuito solo, 1 («circuito único») |
| Otro | Opcional, con justificación (aislamiento térmico, radiación solar…) |

Un factor sin justificación hace que la comprobación quede **«No realizada»**,
indicando el motivo.

Si no se indican los conductores cargados, se toman del sistema (2 en
monofásica, 3 en trifásica) y queda anotado como hipótesis.

## Casos de referencia

Las pruebas usan una **tabla sintética** con valores redondos (10, 20, 30, 40 y 60 A
para 1,5, 2,5, 4, 6 y 10 mm²), que no corresponde a ninguna norma. Los valores
esperados se calcularon aparte ([`refs_h4.py`](refs_h4.py)):

| Caso | Datos | Resultado |
|---|---|---|
| A | 2,5 mm², PVC, 40 °C, Fg = 1 | IZ = 20 × 0,866025 = 17,3205 A → 16 A **superada** |
| C | 2,5 mm², Ft = 0,87 dado, Fg = 0,8 | IZ = 13,92 A → 16 A **no superada** |
| D | 4 mm², XLPE, 3 cargados, método E, 40 °C | IZ = 35 × 0,912871 = 31,9505 A |
| E | 2,5 mm², 30 °C, Fg = 0,8, otro 0,9 | IZ = 14,4 A |
| Igualdad | 2,5 mm², 30 °C, Fg = 0,8 → IZ = 16 A, IB = 16 A | **superada** (IB ≤ IZ) |
| Propuesta 25 m | 16 A, γ = 48, límite 3 % | 2,5 mm² (IZ 17,32 A; ΔU 2,90 %) |
| Propuesta 40 m | Igual, 40 m | 2,5 mm² da 4,64 % → **4 mm²** (IZ 25,98 A; ΔU 2,90 %) |
| Ninguna basta | 100 A | «ninguna sección de la tabla es suficiente» |

## Pruebas

| Prueba | Qué comprueba |
|---|---|
| `tst_rebtampacity` (16) | Fórmula del factor de temperatura; búsqueda en la biblioteca; IZ con factores calculados y dados; trifásico; datos que faltan; factores sin justificación; datos no válidos; sección fuera de la tabla o sin tabla; IB ≤ IZ, incluida la igualdad; propuesta solo por intensidad, con caída, sin longitud y cuando ninguna basta |
| `tst_electricalmodel` (+16) | Ida y vuelta de los campos nuevos; paso al motor; lectura de un fichero válido; 12 tipos de error, descartando solo la tabla errónea; recarga del fichero al cambiar en disco o en la configuración; evaluación con y sin tablas |
| `tst_electricalpanel` (+3) | Columnas IZ e IB ≤ IZ en castellano; resumen; justificación en el detalle; fichero con errores; el diálogo propone la sección y «Usar la sección propuesta» la aplica |

Se introdujeron 5 errores a propósito (fórmula de temperatura sin la raíz, IB = IZ
como no superada, no exigir justificación, aceptar intensidades decrecientes y
proponer sin tener en cuenta la caída) y las pruebas detectaron los 5.
