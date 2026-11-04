# Catálogo de aparatos de protección

Funciona como la gestión de artículos de EPLAN: se elige una referencia y se rellenan los datos del aparato del circuito. Esos datos son intensidad, curva, poder de corte, norma, polos, sensibilidad, I2, disparo instantáneo e I²t.

**El programa no trae ningún catálogo.** Los valores de un fabricante los teclea el usuario a partir del catálogo de ese fabricante, indicando de dónde salen. El programa no comprueba que un valor sea el del fabricante: solo comprueba que el fichero esté bien formado.

## Uso

1. En el panel *Circuitos eléctricos*, pulse **Catálogo de aparatos...** y elija el fichero JSON. La ruta se guarda en la configuración, con la clave `electrical/device_catalogue_file`. Si el fichero se modifica en disco, se vuelve a leer.
2. Elija **Del catálogo...** en el diálogo del circuito o en la pestaña *Eléctrico* de un símbolo. Puede buscar por fabricante, referencia o fuente; el aparato se encuentra si contiene todas las palabras buscadas.
3. Al aceptar, **se sustituyen todos los datos del aparato** del circuito. Así no queda nada del aparato anterior, como la I²t de otro interruptor. Se conservan el símbolo enlazado y el diferencial indicado.

Los valores de I2, disparo instantáneo e I²t se guardan con la justificación «Catálogo: \<fuente\>».

En la pestaña *Eléctrico* no se muestran los campos I2, disparo instantáneo ni I²t. Esos valores se aplican solo si los campos visibles conservan los valores del catálogo. Si se cambia a mano un dato visible, por ejemplo la intensidad, ya no es el aparato del catálogo. En ese caso los datos ocultos no se toman del catálogo y se quedan como estaban. Para revisarlos, abra el diálogo del circuito.

## Formato (versión 1)

```json
{ "format": "qet-devices", "version": 1,
  "devices": [
    { "manufacturer": "...", "partNumber": "...", "source": "catálogo X, página Y",
      "type": "circuit-breaker", "standard": "EN60898",
      "ratedCurrent": 16, "curve": "C", "breakingCapacity": 6, "poles": 2,
      "conventionalCurrent": 23.2, "tripCurrent": 160,
      "letThrough": { "energy": 20000, "atCurrent": 6 } } ] }
```

| Campo | Obligatorio | Unidad / valores |
|---|---|---|
| `manufacturer`, `partNumber` | sí | texto; la pareja no puede repetirse (no distingue mayúsculas ni espacios) |
| `source` | sí | de dónde salen los valores |
| `type` | sí | `circuit-breaker`, `residual-current`, `rcbo`, `fuse`, `switch` |
| `ratedCurrent` | sí | A, > 0 |
| `standard` | no | `EN60898`, `EN61009`, `EN60947-2`, `gG` |
| `curve` | no | texto (B, C, D, gG…) |
| `breakingCapacity` | no | kA, > 0 |
| `sensitivity` | no | mA, > 0 |
| `poles` | no | 1 a 4 |
| `conventionalCurrent` | no | I2 en A, > 0 |
| `tripCurrent` | no | disparo instantáneo en A, > 0 |
| `letThrough` | no | `{"energy": A²s, "atCurrent": kA}`, los dos > 0 |

Si un aparato tiene un error, se descarta completo y el error se muestra al cargar el fichero y en el selector. Los demás aparatos se cargan.

## La I²t y su intensidad de ensayo

La I²t que deja pasar un aparato crece con la intensidad de cortocircuito, y un catálogo la da para una intensidad presunta. Por eso el formato exige `atCurrent`, y el diálogo del circuito tiene el campo **«I²t dada a (kA)»**.

En la comprobación térmica (REBT-ICC-005, ITC-BT-22 1.1 b, UNE-HD 60364-4-43 434.5.2):

- la I²t **se usa** si `atCurrent` es mayor o igual que la Icc máxima calculada en el circuito;
- **no se usa** si `atCurrent` es menor: aparece «I²t dada a una intensidad inferior a la Icc máxima» y la comprobación queda *no realizada*;
- tampoco se usa si la Icc máxima no se conoce, porque no puede compararse;
- si `atCurrent` no se indica (dato tecleado a mano), la I²t se toma como dada a la Icc máxima, igual que antes de esta versión. La justificación debe decir para qué intensidad es.

Limitación: el programa no interpola curvas I²t. Con una I²t dada para una intensidad mayor que la Icc máxima, el resultado queda del lado de la seguridad, porque la I²t real a menor intensidad no es mayor. Esto vale para aparatos limitadores en su zona de limitación, y lo debe confirmar la curva del fabricante.

## Ejemplo ficticio, solo para pruebas

**No son valores de ningún fabricante.** Sirve únicamente para probar el programa. Es el mismo ejemplo que usan las pruebas `tst_devicecatalogue` y `tst_electricalpanel`.

```json
{ "format": "qet-devices", "version": 1, "devices": [
  { "manufacturer": "TEST", "partNumber": "MCB-C16-2P", "source": "test data",
    "type": "circuit-breaker", "standard": "EN60898", "ratedCurrent": 16,
    "curve": "C", "breakingCapacity": 6, "poles": 2,
    "letThrough": {"energy": 20000, "atCurrent": 6} },
  { "manufacturer": "TEST", "partNumber": "RCD-40-30", "source": "test data p. 2",
    "type": "residual-current", "ratedCurrent": 40, "sensitivity": 30, "poles": 2 } ] }
```

## Pruebas

- `tst_devicecatalogue`: lectura válida; búsqueda sin distinguir mayúsculas ni espacios; rechazo de 21 tipos de error, en los que se descarta el aparato erróneo y se conserva el correcto; `applyDeviceEntry` borra la I²t, el disparo y el I2 del aparato anterior; el fichero se lee de nuevo cuando cambia en disco.
- `tst_rebtshortcircuit::letThroughAtTestCurrent`: con la Icc máxima del caso G, 2835,916084 A según `refs_h7.py`:
  - una I²t dada a 3000 A se usa;
  - dada a la propia Icc máxima, también se usa;
  - dada a 2800 A no se usa;
  - con 0 A es un dato no válido;
  - sin Icc máxima no se usa.
- `tst_electricalpanel`:
  - una I²t dada a 3 kA en un cuadro con 6 kA queda *no realizada*, y dada a 6 kA *pasa*;
  - el selector: búsqueda, aparato oculto que deja de estar elegido, errores del fichero;
  - el diálogo del circuito;
  - la pestaña *Eléctrico*, con datos ocultos y edición en directo;
  - la acción del panel.
