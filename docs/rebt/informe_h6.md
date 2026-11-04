# Informe de cálculo en PDF (hito 6)

## Qué genera

Un **PDF vectorial** con, en este orden:

1. **Los folios del proyecto**, exportados por la rutina de PDF que ya tenía
   QElectroTech, con sus símbolos, textos, cajetines configurables y enlaces de
   referencias cruzadas. Esta parte es opcional.
2. **El anexo de cálculo**, en A4 apaisado:
   - **Portada:** título del documento, proyecto, empresa, nº de plano, revisión,
     fecha y autor; resumen de las comprobaciones (superadas, no superadas, no
     realizadas); tablas de intensidades utilizadas, con su fuente; y el aviso
     de alcance enmarcado.
   - **Características de los cuadros:** una tabla por cuadro, indicando de dónde
     se alimenta. Columnas: circuito, uso, sistema, U, P, Ib, protección (tipo,
     curva, In, sensibilidad, poder de corte, norma), cable (material,
     aislamiento, secciones de fase, N y PE, método), L, Iz, ΔU, ΔU total desde
     el origen, y las comprobaciones de ΔU, Ib ≤ Iz y protección ↔ cable, en color.
     Si una tabla no cabe, sigue en la página siguiente con la cabecera repetida
     y «(continuación)».
   - **Resultados y advertencias, circuito a circuito:** cada comprobación con
     sus datos, lo que falta, las hipótesis, los factores con su justificación,
     la sección propuesta y las notas del circuito. Los circuitos con algo no
     superado aparecen en rojo.
   - **Reglas aplicadas:** cada regla usada, una sola vez, con su fuente,
     apartado, texto de referencia y limitaciones.
   - **Cajetín en todas las páginas:** empresa, proyecto, documento, nº de plano,
     revisión, fecha, autor y página n / total.

**Todo sale del mismo modelo y de la misma evaluación que el panel.** El PDF no
calcula nada por su cuenta, así que no puede contradecir lo que se ve en
pantalla. Las pruebas lo comprueban leyendo el texto del PDF generado.

**Aviso de alcance**, en la portada y al final: el anexo no afirma que la
instalación cumpla el REBT en su conjunto. Se nombra lo que no se comprueba
(cortocircuito, contactos directos e indirectos, puesta a tierra, prescripciones
particulares de cada ITC) y se explica que «no realizada» significa que faltan datos.

## Cómo se genera

- **Desde el panel:** botón «Informe de cálculo (PDF)...». El diálogo propone los
  datos del cajetín: los ya guardados en el proyecto o, si no hay, el autor y la
  revisión del cajetín de los folios y la fecha de hoy. Se pueden cambiar. Se
  **guardan con el proyecto** (un paso de deshacer) y la casilla «Primero los
  folios del proyecto y después el anexo» decide si se incluyen los folios.
- **Desde la línea de órdenes**, para automatizar:

  ```sh
  qelectrotech --export-pdf-calculations proyecto.qet salida.pdf [--ampacity-tables=tablas.json]
  qelectrotech --export-calculations     proyecto.qet anexo.pdf  [--ampacity-tables=tablas.json]
  ```

  Sin `--ampacity-tables`, se usan las tablas de la configuración. Los números
  se escriben en el idioma del sistema.

## Formato de los datos del cajetín

En el `.qet`, dentro de `<electrical version="1">`:

```xml
<report company="..." documentTitle="..." planNumber="E-01" revision="B"
        date="03/11/2026" author="..."/>
```

Un campo vacío no se escribe y en el PDF queda en blanco: el programa no lo
inventa. Si no hay título de documento se usa «Anexo de cálculo eléctrico».

## Corrección encontrada al revisar el PDF

Al revisar las páginas generadas apareció un error de los hitos 4 y 5: la
**sección propuesta** se calculaba con la caída del circuito por sí solo. En un
circuito con alimentaciones aguas arriba podía proponer una sección cuya caída
**desde el origen** superaba el límite (el ejemplo C1: proponía 2,5 mm² y su caída
total es del 3,39 %). Ahora la propuesta usa el margen que dejan las
alimentaciones (3 % − 0,49 % → **4 mm²**, que da 1,81 + 0,49 = 2,30 %). Si la
caída de alguna alimentación no se conoce, la propuesta se hace solo por
intensidad y el texto lo dice. Prueba: `suggestionCountsTheFeeders`.

## Pruebas

| Prueba | Qué comprueba |
|---|---|
| `tst_electricalreport` (9 + ejemplo) | Ida y vuelta del cajetín en el XML; el constructor toma los valores del modelo (Ib, Iz, ΔU, ΔU total, estados, recuentos, reglas sin repetir, tablas usadas, notas sin repetir las reglas); sin tablas; **el PDF**: A4 apaisado, ≥ 4 páginas, título en los metadatos, texto extraído con `pdftotext` con los valores del modelo, cajetín y «n / total» en **cada** página; 60 circuitos se reparten en varias páginas con «(continuación)» sin perder ninguno; **la línea de órdenes**: folios + anexo = páginas de los folios + páginas del anexo, y el anexo después de los folios; el panel guarda los datos del cajetín en un paso de deshacer y escribe el PDF |
| `tst_electricalmodel` (+1) | La sección propuesta descuenta las alimentaciones |

Se introdujeron 5 errores a propósito y las pruebas detectaron los 5: columna
total con la caída propia, sin salto de página en las tablas, total de páginas
erróneo en el cajetín, propuesta sin descontar las alimentaciones y la línea de
órdenes sin añadir el anexo.

Las pruebas del PDF necesitan `pdftotext` y `pdfinfo` (poppler‑utils). Si no
están instalados, esas pruebas se saltan indicando el motivo.

## Logotipo de la empresa

En el diálogo del informe, **Logotipo → Elegir...** admite PNG, JPEG, SVG o BMP.
El logotipo:

- **se guarda dentro del proyecto** (`<logo format="png">` en base64 dentro de
  `<report>`), no como una ruta, así que el `.qet` se puede enviar a otra persona;
- se guarda como PNG de 1200 px como máximo en el lado mayor (unos 430 ppp a
  70 mm). Un SVG se dibuja directamente a ese tamaño;
- se dibuja **sin deformarlo**: arriba a la derecha de la portada (80 × 32 mm
  como máximo) y en una celda propia de 28 mm a la izquierda del cajetín de
  **cada** página. Sin logotipo, el cajetín conserva sus anchos originales;
- si el logotipo guardado en un proyecto está dañado, se avisa en
  `badValues` («report/logo») y no se usa.

Pruebas: `logoIsKeptInTheProject`, `logoFromAFile` (PNG grande reducido, JPEG,
archivo que no es imagen, SVG), `logoOnEveryPage` (con `pdfimages`: 2 imágenes en
la portada y 1 en cada una de las demás páginas; ninguna sin logotipo) y
`dialogTakesTheLogo`.

## Pendiente

- Listados de circuitos, protecciones y material en CSV y hoja de cálculo.
- Opción para que el anexo use el mismo tamaño de hoja que los folios.
