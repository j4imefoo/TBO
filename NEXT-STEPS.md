# Next Steps

Actualizado: 2026-04-20

## Estado actual

- Refactor grande de modelo/ownership: hecho
- Calidad: hecha
- Patrones detectados: hechos
- Cobertura final: hecha
- Bugs extra resueltos durante la sesión: hechos
- Suite actual: `29/29` tests OK

## Cambios importantes ya consolidados

- `Comic`, `Page` y `Frame` ya son `GObject`
- `undo/redo` cubre ya altas/borrados y la mayoría de mutaciones principales
- El botón principal del toolbar es `Save`
- La barra baja ahora expresa mejor la jerarquía y es traducible
- Las coordenadas del panel inferior se eliminaron por rendimiento

## Lista global pendiente, en orden recomendado

1. Plantillas al crear cómic
2. Autosave + recientes + reabrir último proyecto
3. Inspector contextual persistente + jerarquía `página -> frame -> objeto`
4. Miniaturas de páginas
5. Reordenación de páginas por drag&drop + duplicar página
6. Búsqueda instantánea y mejor clasificación de assets
7. Favoritos y recientes de assets
8. Presets de bocadillos y estilos de texto
9. Exportación guiada con preview + rango de páginas
10. Exportar página actual o selección
11. Guía integrada de atajos
12. Modo presentación / visor
13. Identidad visual y consistencia estética
14. Packaging/metainfo

## Siguiente tarea a retomar

### 1. Plantillas al crear cómic

Objetivo:

- Añadir presets útiles en `New Comic`
- Crear el documento inicial con tamaño y layout base acordes a la plantilla

Plantillas previstas:

- Storyboard `16:9`
- Tira cómica
- `A4`
- Presentación

Punto de entrada principal:

- `src/comic-new-dialog.c`

Archivos previsibles a tocar:

- `src/comic-new-dialog.c`
- `src/tbo-window.c`
- `src/tbo-window.h`
- quizá un helper nuevo pequeño si compensa, pero mejor cambio mínimo

Implementación recomendada:

1. Añadir selector de plantilla en el diálogo `New Comic`
2. Al cambiar plantilla, autocompletar `width/height`
3. Al aceptar, crear el cómic con el tamaño elegido
4. Aplicar layout inicial solo si la plantilla no es "vacía" implícita
5. Añadir test de regresión para al menos una plantilla con layout inicial

Notas de diseño:

- Mantener simple el diálogo
- No crear aún un sistema grande de presets persistentes
- Empezar con layouts estáticos y claros

## Riesgos / cosas a vigilar al retomar

- `undo/redo` ahora cubre mucho más, pero si una feature nueva crea contenido automáticamente debe decidir si entra o no en el stack
- Si una plantilla crea varios frames iniciales, conviene decidir si se considera estado base del documento o una acción deshacible
- La UI actual usa `GtkNotebook`; las miniaturas de páginas vendrán después, no mezclar ambas tareas

## Punto de comprobación rápido al volver

Ejecutar:

```bash
meson compile -C build && meson test -C build --no-rebuild --print-errorlogs --num-processes 1
```
