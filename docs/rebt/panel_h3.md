# Panel «Circuitos eléctricos» (hito 3)

## Activarlo

**Configuración → General → «Panel de circuitos eléctricos: cuadros, circuitos y
cálculos de baja tensión (REBT)».** Viene **desactivado**, como exige
`AGENTS.md`: así nada cambia para quien no lo use. Al activarlo aparece en
**Listas → Circuitos eléctricos**, abajo por defecto. Se puede mover, dejar
flotante o cerrar. Al desactivarlo, desaparece del editor y de los menús.

## Qué muestra

Una tabla por cuadro con sus circuitos (los circuitos sin cuadro aparecen en
«(sin cuadro)»):

| Columna | Contenido |
|---|---|
| Cuadro / circuito | Nombre. Si el circuito tiene notas, se ven al pasar el ratón |
| Sistema | 1~ / 3~ |
| U, P, L, S | Tensión, potencia, longitud, sección de fase |
| Protección | Curva e intensidad nominal (p. ej. «C 16 A») |
| Ib | Intensidad de cálculo, calculada o indicada |
| ΔU (%) | Caída de tensión del circuito; «—» si no se puede calcular |
| Comprobación ΔU | **Superada** (verde), **No superada** (rojo), **No realizada** (faltan datos) o **Sin límite** (no se ha indicado el uso) |

Al pasar el ratón por los resultados se ven los datos que faltan, el límite
aplicado y su regla (p. ej. «< 3 % (REBT‑CDT‑002 – ITC‑BT‑19 2.2.2)»), las
hipótesis («sin reactancia: se desprecia»…) y el aviso de que es una
**comprobación parcial**. Debajo de la tabla, un resumen indica cuántos circuitos están superados,
no superados y no calculables.

## Qué se puede hacer

| Acción | Detalle |
|---|---|
| Añadir cuadro / circuito | El circuito se crea en el cuadro de la fila actual |
| Editar (o doble clic) | Diálogo con circuito, carga, protección, conductor, notas y **cálculo en directo** |
| Eliminar | Al eliminar un cuadro se eliminan también sus circuitos, en **un solo paso** de deshacer |
| Vincular al esquema | Asigna lo seleccionado en el folio: símbolo de origen, de destino o de protección, conductores del circuito o símbolo del cuadro |
| Mostrar en el esquema | Selecciona en el folio mostrado los símbolos y conductores vinculados |

**Cada cambio es un paso de la lista de deshacer del proyecto**
(`ChangeElectricalModelCommand`): Ctrl+Z y Ctrl+Y funcionan igual que con el
resto del esquema.

**Diálogo:** un campo vacío significa **desconocido** y nunca se rellena. Los
números admiten coma o punto decimal; un texto que no es un número se marca en
rojo y bloquea «Aceptar». Los números se muestran según el idioma del sistema
(coma en castellano) y **sin separador de miles**, porque «3.680» se leería
como 3,68.

Si el proyecto trae datos eléctricos de una versión más nueva, el panel lo
indica y no permite editarlos.

## Idioma

Los textos están en inglés en el código (con `tr()`, como exige el proyecto) y
traducidos al castellano en `lang/qet_es.ts` (162 mensajes nuevos, sin
regenerar el fichero con `lupdate`).

## Pruebas

| Prueba | Qué comprueba |
|---|---|
| `tst_electricalmodel` (+15) | Lectura de números (coma, punto, ambos, unidades, inf/nan), formato sin separador de miles en castellano e inglés, límite según el uso, evaluación de un circuito |
| `tst_electricalpanel` (13 + captura) | En el editor real: desactivado por defecto; la opción lo muestra y lo oculta; deshacer y rehacer; sin cambios no se apila nada; la tabla sigue al deshacer; resultados en castellano («2,90», «3680»); superada, no superada, no realizada y sin límite; eliminar un cuadro es un solo paso; vincular símbolos y conductores de un proyecto real y mostrarlos; solo lectura; el diálogo lee lo escrito y rechaza lo que no es un número |

Se introdujeron 4 errores a propósito en la capa de interfaz (activado por
defecto, deshacer que no restaura, separador de miles y eliminar un cuadro
sin sus circuitos) y las pruebas detectaron los 4.

## Pendiente (siguientes hitos)

- Avisos dibujados en el propio esquema (marca en los conductores de un circuito
  no superado).
- Pestaña «Eléctrico» en las propiedades de cada símbolo, enlazada con esta tabla.
- Comprobar referencias colgantes en todo el proyecto (folios no cargados).
