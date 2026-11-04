# Modelo eléctrico y persistencia (hito 2)

## Qué es

`sources/electrical` (biblioteca `qetelectrical`, QtCore + QtXml, sin interfaz)
guarda los **datos eléctricos** de un proyecto:

- **Cuadros** (`Board`): id, nombre y el símbolo que los representa en el esquema.
- **Circuitos** (`Circuit`): id, nombre, cuadro de origen, elemento de origen y
  de destino, sistema (1F/3F), tensión y uso (vivienda / alumbrado / otros
  usos). Cada circuito incluye:
  - **Carga** (`Load`): potencia, cos φ, rendimiento e intensidad, si se conoce.
  - **Conductor** (`ConductorData`): material, aislamiento, método de instalación,
    secciones de fase, neutro y PE, longitud, temperatura, conductividad,
    reactancia, y los conductores del esquema que lo dibujan.
  - **Protección** (`ProtectionData`): su símbolo, tipo, In, curva, poder de corte,
    sensibilidad y polos.
  - **Notas**: hipótesis y datos pendientes.

**Vínculo con el dibujo:** por uuid de elemento (`Element::uuid()`) y de
conductor (`Conductor::uuid()`). El modelo no copia nada del dibujo.
`danglingItems()` indica qué referencias apuntan a algo que ya no está dibujado.

**Datos desconocidos:** todos los valores son opcionales. Lo que no se conoce
no se escribe en el fichero y vuelve a leerse como desconocido.
`toCircuitInput()` pasa el circuito al motor de cálculo sin rellenar nada.

## Formato en el fichero

```xml
<project ...>
  ...
  <electrical version="1">
    <board id="{…}" name="CGMP" element="{…}"/>
    <circuit id="{…}" name="C1" board="{…}" origin="{…}" destination="{…}"
             system="single-phase" voltage="230" use="dwelling-interior">
      <load power="3680" powerFactor="1"/>
      <conductor material="Cu" insulation="XLPE" installationMethod="B1"
                 section="2.5" length="25" temperature="70">
        <wire uuid="{…}"/>
      </conductor>
      <protection element="{…}" type="circuit-breaker" ratedCurrent="16" curve="C"/>
      <notes>Longitud estimada</notes>
    </circuit>
  </electrical>
</project>
```

- Los números se escriben con punto decimal y en su forma exacta más corta,
  sea cual sea el idioma del sistema. Al leer, una coma decimal **no** se
  interpreta.
- Las palabras clave (`single-phase`, `Cu`, `circuit-breaker`…) no cambiarán. Un
  valor nuevo llevará una palabra nueva.
- Un valor ilegible se deja vacío y se anota en `ReadReport::badValues`
  (p. ej. `circuit[0]@voltage`). Nunca se adivina.

## Versiones y migraciones

| El fichero trae | Qué ocurre |
|---|---|
| Sin `<electrical>` | Modelo vacío. El proyecto se abre y se guarda exactamente igual que antes |
| `version="1"` | Se lee |
| Una versión mayor que la del programa | **No se interpreta.** Se conserva tal cual, el modelo queda en solo lectura y `setElectricalModel()` lo rechaza, para que nunca se pierdan datos de una versión más nueva |
| Sin versión o con una versión no válida | Igual que una versión nueva: se conserva y se anota en `badValues` |

Cuando haya una versión 2, `fromXml()` migrará la 1 al leerla.

## Cómo se guarda en `.qet`, `.qetz` y `.qetdb`

QET conserva en todos sus formatos cualquier sección del proyecto que no
conoce, los «foreign blocks» (`projectDataBase::foreignChildren()`). El modelo
eléctrico **viaja como uno de esos bloques**:

- al abrir, `QETProject::readElectricalXml()` lo interpreta;
- al cambiarlo, `QETProject::setElectricalModel()` sustituye el bloque por el
  nuevo (`storeElectricalBlock()`), y `toXml()` y el escritor de `.qetdb` lo
  guardan sin cambios propios.

Ventajas:

1. No se ha tocado la capa de base de datos ni `toXml()`.
2. **Un QElectroTech original que abra un proyecto con datos eléctricos los
   conserva al guardar**, porque para él es un bloque desconocido más.
3. Un proyecto que no se edita guarda el bloque byte a byte igual.

## Cambios en código existente

| Fichero | Cambio |
|---|---|
| `CMakeLists.txt` | Añade las bibliotecas `qetrebt` y `qetelectrical` y enlaza `qetelectrical` con la aplicación |
| `sources/qetproject.h` | `electricalModel()`, `setElectricalModel()`, `electricalReadReport()` y dos miembros |
| `sources/qetproject.cpp` | Una llamada en `readProjectXml()` y tres funciones nuevas |
| `tests/qttest/CMakeLists.txt` | Registra las pruebas nuevas |

Todavía no hay interfaz ni deshacer: llegan en el hito 3. Entonces cada edición
pasará por un `QUndoCommand`, como exige `AGENTS.md`. Ahora
`setElectricalModel()` marca el proyecto como modificado.

## Pruebas

| Prueba | Qué comprueba |
|---|---|
| `tst_electricalmodel` (13) | Ida y vuelta del XML, valores desconocidos, números exactos con independencia del idioma (probado en alemán), valores ilegibles, versión nueva o ausente en solo lectura, referencias colgantes, paso al motor de cálculo (caso CDT‑M1 del hito 1) |
| `tst_electricalproject` (12) | Con un proyecto real (`fixtures/every_feature.qet`): un proyecto sin modelo no cambia; marcar como modificado; un solo bloque; borrar el modelo quita el bloque; guardado y reapertura en `.qet`, `.qetz` y `.qetdb`; cadena `.qet` → `.qetdb` → `.qetz` → `.qet` con una edición por el camino; se conservan otros bloques desconocidos; una versión nueva nunca se sobrescribe; avisos de valores ilegibles; las referencias apuntan a símbolos reales tras reabrir |

Se comprobó que las pruebas detectan errores: 4 fallos introducidos a propósito
en la integración (no leer el modelo, no guardarlo, no quitar el bloque anterior
y no proteger la versión nueva) hacen fallar entre 1 y 7 pruebas cada uno.

## Pendiente

- **Compilación en Windows:** el motor `qetrebt` compila sin avisos con MinGW‑w64
  (compilación cruzada). La aplicación completa no se ha podido compilar para
  Windows desde este entorno porque no hay acceso a los paquetes de MSYS2. La
  forma más sencilla es la CI de Windows del repositorio (`windows-build.yml`),
  subiendo la rama a un fork en GitHub.
- **Referencias colgantes a nivel de proyecto:** con los folios «perezosos» de QET,
  no todos los elementos están cargados en memoria. La comprobación sobre todo
  el proyecto se hará en el hito 3 a partir de la base de datos del proyecto.
