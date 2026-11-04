# Pestaña «Eléctrico» de los símbolos

Al abrir las propiedades de un símbolo (doble clic, o el panel de propiedades),
aparece la pestaña **Eléctrico** si está activado el ajuste «Panel de circuitos
eléctricos». Con el ajuste desactivado, el diálogo queda como en QElectroTech.

Su planteamiento sigue la separación que hace EPLAN en las propiedades de un
dispositivo:
- **Datos que identifican:** qué es el símbolo en el modelo.
- **Datos que describen:** las características técnicas del aparato, con su
  fabricante y su referencia de artículo.
- **Resultados:** junto a los datos, para no tener que ir al panel.

## Qué contiene

**Qué es este símbolo**
- La lista de lo que representa: cuadro, protección de un circuito, origen o
  destino de circuitos.
- **Cuadro que representa este símbolo** y **Protección del circuito**, para
  vincularlo. Un cuadro o circuito que ya estaba vinculado a otro símbolo pasa a
  este, porque un cuadro y una protección tienen un solo símbolo. «Ninguno» lo
  desvincula. Los cuadros y circuitos se crean en el panel.

**Aparato de protección** (del circuito elegido)
- Fabricante y referencia: texto libre, que el anexo muestra en las notas del
  circuito.
- Tipo, intensidad nominal, curva, poder de corte, sensibilidad, polos y norma.
  Son los mismos datos del diálogo del circuito, y el resto de datos del circuito
  no se toca.

**Comprobaciones del circuito**
- El resultado de caída de tensión, IB ≤ IZ, protección ↔ cable, cortocircuito y
  ITC‑BT‑25, calculado **con los datos tal como están escritos** en la pestaña,
  antes de aplicarlos.
- El detalle de la coordinación y del cortocircuito, con lo que falta para
  completarlas.

## Cómo se aplican los cambios

- En el diálogo, al aceptar: un único paso de deshacer («Datos eléctricos de un
  símbolo»), junto con los cambios de las demás pestañas.
- En el panel de propiedades (edición en vivo): al elegir en una lista o al
  terminar de escribir en un campo.
- Un campo numérico que no es un número se marca en rojo y no se aplica nada.
- Si no cambia nada, no se crea un paso de deshacer vacío.
- Los cambios hechos en el panel o al deshacer se ven en la pestaña al momento.
- Con un modelo de una versión más reciente (solo lectura), la pestaña solo
  muestra.

## Pruebas

`tst_electricalpanel::electricalTab`:
- la pestaña solo aparece con el ajuste activado;
- vincular, mover el vínculo desde otro símbolo y desvincular;
- los datos del aparato, con el resto del circuito intacto;
- deshacer y rehacer, también en la pestaña;
- un campo no numérico, que no aplica nada;
- sin cambios, sin paso de deshacer;
- la edición en vivo.

Mutaciones de la lógica de vínculos y aplicación: 7 de 8 detectadas. La que
sobrevive es equivalente: la comprobación de campos válidos ya la hace el modelo
editado.

## Siguiente paso posible

Un **catálogo de aparatos** al estilo de la gestión de artículos de EPLAN:
- un fichero propio, como el de las tablas de intensidades;
- al elegir una referencia, rellena intensidad, curva, poder de corte, norma,
  I²t y disparo instantáneo, con su fuente.
