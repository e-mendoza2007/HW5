"""Pruebas de regresion ejecutando binarios RV32I en el simulador.
Requiere compilar primero el ejecutable: g++ -std=c++17 -O2 src/riscv_sim.cpp -o riscv_sim
"""
from pathlib import Path
import subprocess, sys, re
ROOT=Path(__file__).resolve().parent.parent
EXE=ROOT/('riscv_sim.exe' if sys.platform=='win32' else 'riscv_sim')

def run(name,commands):
    proc=subprocess.run([str(EXE),str(ROOT/'ejemplos'/(name+'.bin'))],
      input=commands+'\nexit\n',text=True,capture_output=True,timeout=15)
    if proc.returncode: raise AssertionError(f'{name}: codigo de salida={proc.returncode}\n{proc.stderr}')
    return proc.stdout

def includes(text,*pieces):
    for piece in pieces:
        assert piece in text, f'Falta {piece!r} en salida:\n{text}'

a=run('01_aritmetica','step 11\nregs x5 x6 x7 x8 x9 x10 x13\nword 0x100')
includes(a,'x5=0x0000000A','x6=0x00000007','x7=0x00000011','x8=0x00000003',
    'x9=0x0000000D','x10=0x0000001C','x13=0x00000001','Mem[0x00000100]=0x00000011')
print('[PASS] 01 aritmetica: add, sub, xor, shifts, slt, or, sw')

b=run('02_memoria','step 16\nregs x3 x4 x5 x6 x7 x9 x10 x12 x13\nmem 0x100 0x107')
includes(b,'x3=0xFFFFFFFF','x4=0x000000FF','x5=0xFFFFFFFF','x6=0x0000FFFF',
    'x7=0xFFFFFFFF','x9=0xFFFFFF80','x10=0x00000080','x12=0x00000123',
    '0x00000100: FF FF FF FF 80 00 23 01')
print('[PASS] 02 memoria: byte/half/word, little-endian, signo')

c=run('03_saltos','step 18\nregs x5 x7 x8 x12 x13 x14 x17\nword 0x124')
includes(c,'x5=0x00000005','x8=0x00000000','x12=0x00000055',
   'x13=0x0000002D','x14=0x00000024','x17=0x00000001','Mem[0x00000124]=0x00000001')
print('[PASS] 03 saltos: bne, jal, jalr, enlace, bit 0 borrado')

d=run('04_integral_25','step 15\nregs x0 x3 x10 x11 x13\nword 0x64')
includes(d,'x0=0x00000000','x3=0x00000019','x10=0x00000019',
   'x11=0x00000000','x13=0x00000001','Mem[0x00000064]=0x00000019')
print('[PASS] 04 integral: 25 en M[100], x0 inmutable, beq, jal')

cmd='until 0x4c 10000\n'+'\n'.join('word 0x%X'%a for a in range(0x1000,0x101c,4))
e=run('05_quicksort',cmd)
includes(e,'[OBJETIVO]')
for i,v in enumerate([1,2,3,4,6,8,9]):
    includes(e,f'Mem[0x{0x1000+4*i:08X}]=0x{v:08X}')
print('[PASS] 05 quicksort recursivo: orden de 7 palabras')

f=run('06_arbol_simetrico','until 0x4c 10000\nregs x10')
includes(f,'[OBJETIVO]','x10=0x00000001')
print('[PASS] 06 arbol simetrico: a0 = 1')
print('RESULTADO: 6/6 casos correctos')
