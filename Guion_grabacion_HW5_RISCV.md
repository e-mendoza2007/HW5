# Guía exacta para grabar el video de HW#5 (simulador RISC-V)

**Este video debe mostrar tu simulador propio**, no una simulación en Vivado ni únicamente CPUlator. CPUlator sirve de referencia y para obtener `.bin`; el profesor pide que tu programa *cargue y ejecute* binarios RV32I. En el video es suficiente grabar la terminal con resultados verificables.

## A. Preparación en Windows (PowerShell)

1. Descomprime `RISCV_HW5_Codigo_y_Pruebas(1).zip`.
2. Abre el directorio `RISCV_HW5_solucion` en VS Code, o una terminal PowerShell en esa carpeta. Confirma que existen `src/riscv_sim.cpp`, `ejemplos/` y `pruebas/`.
3. Necesitas un compilador `g++` compatible con C++17 (por ejemplo, MinGW-w64). Ejecuta:

```powershell
g++ --version
g++ -std=c++17 -O2 -Wall -Wextra src/riscv_sim.cpp -o riscv_sim.exe
```

Si `g++` no se reconoce, debes instalarlo o añadirlo al PATH; también puedes ejecutar la tarea desde WSL con g++ instalado. No necesitas Vivado.

**Importante:** `main (2).cpp` es un *ejemplo ajeno* de simulador ficticio de 16 bits; no corresponde al procesador RV32I exigido. El archivo que sí debes explicar y subir es `src/riscv_sim.cpp`.

## B. Estructura y tiempos orientativos del video (aprox. 6–9 min)

**0:00–0:45 — Presentación.** Muestra la carpeta del proyecto, `src/riscv_sim.cpp` y la terminal. Di: «Implementé un simulador funcional RV32I en C++17. El simulador carga un archivo binario crudo en la dirección cero, mantiene el PC, 32 registros y memoria little-endian, e interpreta una instrucción por paso. Para probarlo mostraré cuatro programas distintos».

**0:45–1:30 — Compilación e interfaz.** En PowerShell compila con el comando anterior. Explica `pc`, `step`, `regs`, `mem`, `word` y `until`. Muestra `help` cuando abras el primer programa.

### EJEMPLO 1 — Prueba integral: 25 en dirección 0x64 (aprox. 1 min)

En una terminal normal (fuera del prompt `rv32i>`):

```powershell
.\riscv_sim.exe ejemplos\04_integral_25.bin
```

Dentro del simulador, teclea:

```text
pc
step
regs x1
step 14
regs x0 x10
word 0x64
exit
```

**Qué se ve:** después del primer `step`, `x1 = 0x0000000A` (10). Después de los 15 pasos totales, `x10 = 0x00000019` (25); `x0 = 0`; y `Mem[0x00000064] = 0x00000019`.

**Qué dices:** «Este programa combina aritmética, comparaciones, cargas, almacenes y saltos. El resultado esperado es 25 en la dirección decimal 100, equivalente a 0x64. El comando `word` muestra ese resultado».

### EJEMPLO 2 — Little-endian y cargas con signo (aprox. 1–1.5 min)

Abre una nueva ejecución:

```powershell
.\riscv_sim.exe ejemplos\02_memoria.bin
```

Dentro:

```text
pc
step 16
mem 0x100 0x107
regs x3 x4 x5 x6 x7 x9 x10 x12 x13
exit
```

**Qué se ve:** los bytes en memoria `FF FF FF FF 80 00 23 01` desde `0x100` a `0x107`. Los registros `x9 = 0xFFFFFF80` (-128), `x10 = 0x00000080` (+128), `x12 = 0x00000123`, `x13 = 0x00000123`.

**Qué dices:** «Compruebo que la memoria es direccionable por bytes y almacena palabras en formato little-endian. También muestro la diferencia entre `lb`, que extiende signo, y `lbu`, que rellena con ceros».

### EJEMPLO 3 — Quicksort recursivo (aprox. 1.5 min)

```powershell
.\riscv_sim.exe ejemplos\05_quicksort.bin
```

Dentro:

```text
until 0x40
mem 0x1000 0x101b
until 0x4C
mem 0x1000 0x101b
exit
```

**Por qué hay dos `until`:** el primero detiene la ejecución cuando ya se cargó el arreglo inicial, pero antes de empezar a ordenarlo. El segundo espera a que el programa termine de ordenar, en su etiqueta de finalización.

**Qué se ve:** inicialmente los siete enteros de 32 bits son `{6,4,3,2,1,8,9}` y después quedan `{1,2,3,4,6,8,9}`, desde `0x1000`. Los cuatro bytes de cada entero aparecen en little-endian (por ejemplo, `06 00 00 00`). El segundo `until` llega a `PC=0x0000004C [OBJETIVO]`.

**Qué dices:** «El programa implementa Quicksort recursivo. Detengo el simulador después de inicializar los datos para mostrar el arreglo original; después lo dejo ejecutar hasta la etiqueta de finalización y verifico que el contenido de la misma zona de memoria quedó ordenado».

### EJEMPLO 4 — Árbol simétrico recursivo (aprox. 1 min)

```powershell
.\riscv_sim.exe ejemplos\06_arbol_simetrico.bin
```

Dentro:

```text
until 0x40
mem 0x1000 0x101b
until 0x4C
regs x10
exit
```

**Qué se ve:** arreglo que representa el árbol `{6,4,4,1,2,2,1}` y `x10=0x00000001 (1)`. En RISC-V, el registro `a0` es `x10`.

**Qué dices:** «El programa compara recursivamente los subárboles izquierdo y derecho. Como el árbol es simétrico, retorna verdadero, representado por `a0 = 1`».

**6:00–7:00 — Cierre y entrega.** Muestra unos segundos el código fuente y el informe. Explica que las 37 instrucciones del Anexo A están contempladas, pero las seis pruebas automáticas no son una demostración exhaustiva de todas ellas. Recuerda mencionar que `ecall`, GDB y ELF son funciones opcionales no implementadas.

## C. Errores que conviene evitar en el video

- **No uses `run` sin límite** en los ejemplos: algunos terminan en un bucle o se sale del binario y muestran `TRAP`. Es preferible usar exactamente `step 15`, `step 16` o `until 0x4C`, según el caso.
- **No muestres `mem 0x1000 ...` al iniciar Quicksort como «arreglo original»**: primero debes ejecutar `until 0x40`; es el programa quien inicializa los datos.
- El comando `word` lee 4 bytes como un entero de 32 bits; `mem` muestra bytes individuales.
- Las instrucciones que aparecen en el archivo `.lst` son el listado explicativo de las pruebas; los archivos `.bin` contienen el código máquina que realmente interpreta el simulador.
- La grabación debe tener legibles la terminal y los resultados (usa fuente de 16–20 puntos y resolución suficiente). No es necesario grabar toda la duración de Quicksort instrucción por instrucción: `until` ejecuta hasta el PC objetivo y permite verificar el resultado.

## D. Pruebas adicionales antes de enviar

Desde PowerShell, con el simulador compilado:

```powershell
python pruebas\test_ejemplos.py
```

Si todo funciona deberías ver seis líneas `[PASS]` y `RESULTADO: 6/6 casos correctos`.

**Recomendación importante:** exporta además un pequeño programa RV32I desde CPUlator con el formato **Raw**, dirección inicial `0000`, y pruébalo en tu simulador. Esto contrasta el lector de binarios con una fuente independiente. Todavía no se verificó aquí directamente un binario exportado desde CPUlator; las seis pruebas anteriores utilizan archivos RV32I generados por la herramienta auxiliar incluida.

### Ejemplo RV32I básico para CPUlator (verificación adicional)

```asm
.text
.globl _start
.option norvc
_start:
    addi x5, x0, 10
    addi x6, x0, 15
    add  x7, x5, x6
    sw   x7, 100(x0)
fin:
    jal  x0, fin
```

En CPUlator, selecciona sistema RV32 y compila (F5). En Memory, guarda como `Raw` desde `0000` hasta la dirección exclusiva `0014` **si el código se ensambló en 0 y ocupa cinco instrucciones de 4 bytes**. Confirma en Disassembly las direcciones reales; de otro modo ajusta el rango. Luego ejecuta el archivo en tu simulador y usa `step 4`, `word 0x64`: debes obtener 25.

## E. Lo que debes subir

El enunciado obliga a subir **un PDF a Canvas** que contenga: (1) enlace real al repositorio Git del simulador, (2) enlace real a una carpeta de Drive con el video y código fuente, (3) un informe breve (lenguaje y funcionalidades). `Informe_HW5_Simulador_RISCV.tex` es la fuente editable de ese PDF; compílala con Overleaf o `pdflatex`. Sustituye los marcadores `NOMBRE Y APELLIDOS`, `PEGAR AQUI EL ENLACE REAL ...`, y `FECHA DE ENTREGA`.

El video y el código van en Google Drive; **el PDF de Canvas es obligatorio** aunque no se pida un ejercicio teórico escrito largo.
