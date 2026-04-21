# Next Steps

Actualizado: 2026-04-21

## Objetivo de este documento

Este archivo es el roadmap operativo del agente para continuar el trabajo en `TBO`.
Debe usarse para retomar contexto rapido, priorizar bien y evitar mezclar tareas.

## Estado actual

- Proyecto en C con GTK4 y Meson.
- `Comic`, `Page` y `Frame` ya son `GObject`.
- La refactorizacion grande de modelo y ownership ya quedo consolidada.
- `undo/redo` cubre altas, borrados y la mayoria de mutaciones principales.
- El boton principal del toolbar es `Save`.
- La barra inferior ya expresa mejor la jerarquia y es traducible.
- Las coordenadas del panel inferior se eliminaron por rendimiento.
- Suite actual: `38/38` tests OK.

## Trabajo ya cerrado

Ya no debe tratarse como pendiente en este roadmap:

- crash al clonar selecciones agrupadas,
- limpieza del `TboObjectGroup` temporal,
- zoom minimo valido,
- color base de frame sin estado global compartido,
- doble escalado en exportacion,
- conversiones de coordenadas de frame view sin depender de globals fragiles,
- `KEY_BINDER` por ventana,
- duplicacion de extensiones en exportacion,
- manejo seguro de errores al abrir directorios de assets.

## Principio de trabajo

Orden recomendado:
1. Priorizar primero mejoras con mucho valor de usuario y bajo riesgo.
2. Despues atacar claridad de flujo, feedback y consistencia UX.
3. Dejar para mas tarde refactors amplios, identidad visual grande y packaging.
4. Mantener cambios minimos, locales y faciles de verificar.
5. No mezclar en una misma sesion una feature de producto con una refactorizacion amplia si no es necesario.

## Prioridad actual

No hay bugs criticos abiertos confirmados en esta lista.
La prioridad pasa a ser un roadmap combinado de producto + UX, ordenado por importancia real.

## Tareas restantes ordenadas por importancia

1. Plantillas al crear comic.
- Mejor siguiente tarea: alto valor, alcance corto y punto de entrada claro.

2. Autosave + recientes + reabrir ultimo proyecto.
- Mucho impacto en seguridad percibida y continuidad de trabajo.

3. Mostrar estado `dirty` de forma visible en titulo o UI persistente.
- Quick win UX muy importante.

4. Hacer persistente y claro el modo actual: `Page` vs `Frame`.
- Mejora fuerte de claridad y evita errores de contexto.

5. Exportar pagina actual o seleccion.
- Mejora de uso muy practica y contenida.

6. Mejorar feedback de insercion de imagen y drag and drop cuando falla.
- El usuario debe saber por que una insercion no se realizo.

7. Guia integrada de atajos.
- Mucho retorno con poco riesgo.

8. Inspector contextual persistente con jerarquia `pagina -> frame -> objeto`.
- Mejora fuerte de flujo y discoverability.

9. Reordenacion de paginas por drag and drop + duplicar pagina.
- Muy util para trabajo real con documentos largos.

10. Busqueda instantanea y mejor clasificacion de assets.
- Sube mucho el valor del panel lateral cuando el catalogo crece.

11. Favoritos y recientes de assets.
- Complementa bien la tarea anterior.

12. Exportacion guiada con preview + rango de paginas.
- Valiosa, pero despues de cerrar lo basico de export.

13. Presets de bocadillos y estilos de texto.
- Util, pero menos prioritaria que navegacion, contexto y export.

14. Miniaturas de paginas.
- Interesante, aunque de mayor coste estructural.

15. Mejorar accesibilidad por teclado en boton de menu y assets.
- Importante, pero despues de las mejoras principales de flujo.

16. Dejar de forzar `Adwaita-dark`.
- Buena mejora de integracion con el sistema y consistencia visual.

17. Unificar labels, capitalizacion, copy y tono textual.
- Pulido valioso, no urgente.

18. Reforzar consistencia visual entre toolbar, sidebar, dialogs e iconografia.
- Trabajo de polish visual progresivo.

19. Revisar si conviene simplificar la doble barra superior.
- Mejor hacerlo cuando la UX este mas asentada.

20. Identidad visual y consistencia estetica.
- Linea importante, pero no antes de cerrar lo funcional y la claridad base.

21. Modo presentacion / visor.
- Interesante como feature, menos importante ahora.

22. Packaging/metainfo.
- Importante para distribucion, no para el nucleo del flujo de trabajo.

## Deuda tecnica restante

No es prioritaria frente al roadmap anterior, pero sigue pendiente:

- patron repetido entre `Comic`, `Page` y `Frame` para lista + elemento actual,
- undo repartido entre `src/tbo-undo.c` y logica adicional en `src/tbo-tool-selector.c`,
- inconsistencias menores de APIs publicas y contratos de indices,
- include duplicado de `<string.h>` en `src/comic.c`.

## Cobertura pendiente

Faltan tests especificos para:

- plantillas con layout inicial,
- autosave y recuperacion al arrancar,
- recientes + reabrir ultimo proyecto,
- exportacion de pagina actual o seleccion,
- feedback y rutas de error de insercion por DnD,
- accesibilidad basica de flujos principales por teclado.

## Siguiente tarea a retomar

### 1. Plantillas al crear comic

Objetivo:
- Anadir presets utiles en `New Comic`.
- Crear el documento inicial con tamano y layout base acordes a la plantilla.

Plantillas previstas:
- Storyboard `16:9`
- Tira comica
- `A4`
- Presentacion

Punto de entrada principal:
- `src/comic-new-dialog.c`

Archivos previsibles a tocar:
- `src/comic-new-dialog.c`
- `src/tbo-window.c`
- `src/tbo-window.h`
- Solo anadir un helper nuevo si compensa claramente.

Implementacion recomendada:
1. Anadir selector de plantilla en el dialogo `New Comic`.
2. Al cambiar plantilla, autocompletar `width` y `height`.
3. Al aceptar, crear el comic con el tamano elegido.
4. Aplicar layout inicial solo si la plantilla no es la opcion vacia.
5. Anadir al menos un test de regresion para una plantilla con layout inicial.

Notas de diseno:
- Mantener simple el dialogo.
- No crear aun un sistema grande de presets persistentes.
- Empezar con layouts estaticos y claros.
- No mezclar esta tarea con miniaturas de paginas ni con una reforma general de UI.

## Decisiones abiertas a vigilar

- Si una plantilla crea varios frames iniciales, decidir si eso forma parte del estado base del documento o si entra en el stack de `undo/redo`.
- No mezclar todavia `GtkNotebook` con la futura tarea de miniaturas.
- Si se mejora exportacion, separar claramente correcciones de bugs de mejoras de UX.
- Si se toca multiventana, evitar introducir mas estado global compartido.
- Si se implementa autosave, definir antes el formato, frecuencia y politica de recuperacion.

## Comprobacion rapida al retomar

Ejecutar:
```bash
meson compile -C build && meson test -C build --no-rebuild --print-errorlogs --num-processes 1
```

## Regla practica para el agente

Antes de empezar una tarea:
1. Confirmar si toca producto, UX o deuda tecnica.
2. Leer los archivos exactos implicados.
3. Hacer el cambio minimo correcto.
4. Anadir o ajustar tests si el cambio altera comportamiento.
5. Verificar compile + tests.
6. Actualizar este documento si cambia la prioridad real del roadmap.
