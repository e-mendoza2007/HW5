# HW#5 - Simulador RV32I (proyecto de referencia)

Solucion educativa del enunciado `HW_Arch-9.pdf` (documento internamente rotulado como HW#5). Implementacion en **C++17**, comandos de consola, soporte obligatorio de las 37 instrucciones indicadas en el anexo A, memoria plana byte-addressable **little-endian** y carga de archivos binarios crudos a direccion `0x00000000`.

## Preparacion

Desde el directorio que contiene este README:

**Linux/macOS**

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic src/riscv_sim.cpp -o riscv_sim
./riscv_sim ejemplos/04_integral_25.bin
```

**Windows PowerShell (MinGW o g++ instalado):**

```powershell
g++ -std=c++17 -O2 -Wall -Wextra src/riscv_sim.cpp -o riscv_sim.exe
.\riscv_sim.exe ejemplos\04_integral_25.bin
```

Para usar otro programa: ejecutar el simulador con la ruta del binario, por ejemplo `./riscv_sim programa.bin`. Se espera un binario de instrucciones RV32I de 32 bits, no un archivo ELF.

## Interfaz

- `pc`: inspeccionar el PC y la instruccion de 32 bits en esa direccion.
- `step [n]`: ejecutar una o varias instrucciones de manera controlada.
- `run [max]`: ejecutar hasta un limite o un error (si hay un bucle sin fin, parara en el limite).
- `until direccion [max]`: ejecutar hasta que el PC llegue a una direccion sin ejecutarla; util para pruebas con bucle final.
- `regs` o `regs x1 x2`: mostrar todos o algunos registros.
- `mem inicio fin`: mostrar los bytes de memoria (rango inclusivo, maximo 4096 bytes).
- `word direccion`: mostrar una palabra de 32 bits en little-endian.
- `reset`: restaurar imagen, PC y registros.
- `exit`: salir.

## Pruebas y evidencias

Generar de nuevo los binarios de prueba (requiere Python 3):

```bash
python3 pruebas/generar_ejemplos.py
```

Ejecutar la regresion automatica (requiere que `riscv_sim` este compilado):

```bash
python3 pruebas/test_ejemplos.py
```

Se incluyen seis ejemplos con archivos `.bin` y `.lst`:

1. `01_aritmetica`: operaciones aritmeticas, desplazamientos y comparaciones.
2. `02_memoria`: little-endian, cargas de byte/half/word y signo.
3. `03_saltos`: branches, JAL, JALR y actualizacion del PC.
4. `04_integral_25`: resultado esperado `M[100]=25` y `x0=0`.
5. `05_quicksort`: quicksort **recursivo propio** de `{6,4,3,2,1,8,9}` a `{1,2,3,4,6,8,9}` en `0x1000`.
6. `06_arbol_simetrico`: recursion sobre `{6,4,4,1,2,2,1}` en `0x1000`; resultado `a0=1`.

Las implementaciones de quicksort y arbol son **ejemplos originales equivalentes en objetivo**, no copias textuales de los enlaces de Pastebin del enunciado.

## Decisiones y limites

- La memoria utiliza paginas dispersas de 4 KiB, habilitando el espacio logico de 32 bits sin reservar fisicamente 4 GiB.
- Registros y memoria se inicializan en cero; solo los bytes del programa se cargan en `0x0`.
- `x0` siempre es cero; los resultados de aritmetica son modulo `2^32`.
- Operaciones desalineadas `lh/lhu/sh` y `lw/sw`, PC desalineado e instrucciones invalidas emiten `TRAP` con detalle.
- `ecall`, desensamblador completo, archivos ELF y depuracion GDB son extras **no implementados**. `pc` muestra la instruccion hexadecimal y una nemotecnia basica, que no equivale a un desensamblador completo.
- No se ha creado repositorio publico ni video en Google Drive: esos enlaces los debe completar quien entregue el proyecto.

## Entrega real segun el enunciado

El profesor solicita un PDF con enlaces a: repositorio Git, carpeta de Google Drive con video y codigo fuente, y un informe breve. El video tiene que demostrar al menos cuatro ejecuciones. El archivo PDF de esta solucion incluye un guion propuesto.
