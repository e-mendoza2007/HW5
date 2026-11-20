"""Generador de binarios de prueba (NO forma parte del simulador solicitado).
Genera archivos crudos RV32I little-endian, en origen 0x0.
"""
from pathlib import Path
import struct
HERE=Path(__file__).resolve().parent.parent
OUT=HERE/'ejemplos';OUT.mkdir(exist_ok=True)
ABI={'zero':0,'ra':1,'sp':2,'gp':3,'tp':4,'t0':5,'t1':6,'t2':7,'s0':8,'s1':9,
     'a0':10,'a1':11,'a2':12,'a3':13,'a4':14,'a5':15,'a6':16,'a7':17,
     's2':18,'s3':19,'s4':20,'s5':21,'s6':22,'s7':23,'s8':24,'s9':25,'s10':26,'s11':27,
     't3':28,'t4':29,'t5':30,'t6':31}

def reg(x):
    if isinstance(x,int): return x
    if x.startswith('x'):return int(x[1:])
    return ABI[x]

class Asm:
    def __init__(self):self.ops=[];self.labels={}
    def label(self, name):
        if name in self.labels: raise ValueError('label duplicada: '+name)
        self.labels[name]=4*len(self.ops);return self
    def ins(self, m, *args):self.ops.append((m,args));return self
    def a(self,m,*args):return self.ins(m,*args)
    def value(self,v):return self.labels[v] if isinstance(v,str) else v
    def signed(self,v,b):
        assert -(1<<(b-1))<=v<(1<<(b-1)),(v,b)
        return v&((1<<b)-1)
    def encode(self,m,args,pc):
        r=reg
        defsR={'add':(0,0),'sub':(0,0x20),'sll':(1,0),'slt':(2,0),'sltu':(3,0),
          'xor':(4,0),'srl':(5,0),'sra':(5,0x20),'or':(6,0),'and':(7,0)}
        defsI={'addi':0,'slti':2,'sltiu':3,'xori':4,'ori':6,'andi':7}
        defsSh={'slli':(1,0),'srli':(5,0),'srai':(5,0x20)}
        defsL={'lb':0,'lh':1,'lw':2,'lbu':4,'lhu':5}
        defsS={'sb':0,'sh':1,'sw':2}
        defsB={'beq':0,'bne':1,'blt':4,'bge':5,'bltu':6,'bgeu':7}
        if m in defsR:
            rd,rs1,rs2=map(r,args)
            f3,f7=defsR[m]
            return f7<<25|rs2<<20|rs1<<15|f3<<12|rd<<7|0x33
        if m in defsI:
            rd,rs1,imm=args;imm=self.signed(self.value(imm),12)
            return imm<<20|r(rs1)<<15|defsI[m]<<12|r(rd)<<7|0x13
        if m in defsSh:
            rd,rs1,imm=args;assert 0<=imm<32
            f3,f7=defsSh[m]
            return f7<<25|imm<<20|r(rs1)<<15|f3<<12|r(rd)<<7|0x13
        if m in defsL:
            rd,rs1,imm=args;imm=self.signed(self.value(imm),12)
            return imm<<20|r(rs1)<<15|defsL[m]<<12|r(rd)<<7|0x03
        if m=='jalr':
            rd,rs1,imm=args;imm=self.signed(self.value(imm),12)
            return imm<<20|r(rs1)<<15|r(rd)<<7|0x67
        if m in defsS:
            rs2,rs1,imm=args;imm=self.signed(self.value(imm),12)
            return (imm>>5)<<25|r(rs2)<<20|r(rs1)<<15|defsS[m]<<12|(imm&31)<<7|0x23
        if m in defsB:
            rs1,rs2,target=args;off=(self.value(target)-pc) if isinstance(target,str) else target
            assert off%2==0;imm=self.signed(off,13)
            return ((imm>>12)&1)<<31|((imm>>5)&63)<<25|r(rs2)<<20|r(rs1)<<15|defsB[m]<<12|((imm>>1)&15)<<8|((imm>>11)&1)<<7|0x63
        if m=='jal':
            rd,target=args;off=(self.value(target)-pc) if isinstance(target,str) else target
            assert off%2==0;imm=self.signed(off,21)
            return ((imm>>20)&1)<<31|((imm>>1)&1023)<<21|((imm>>11)&1)<<20|((imm>>12)&255)<<12|r(rd)<<7|0x6F
        if m in ('lui','auipc'):
            rd,imm=args;assert 0<=imm<1<<20
            return imm<<12|r(rd)<<7|({'lui':0x37,'auipc':0x17}[m])
        raise ValueError('no soportado: '+m)
    def write(self,name):
        code=bytearray(); lines=[]
        reverse={v:k for k,v in self.labels.items()}
        for index,(m,args) in enumerate(self.ops):
            pc=4*index;w=self.encode(m,args,pc)
            code+=struct.pack('<I',w)
            if pc in reverse:lines.append(reverse[pc]+':')
            lines.append(f'{pc:08X}  {w:08X}  {m:6} '+', '.join(map(str,args)))
        (OUT/(name+'.bin')).write_bytes(code)
        (OUT/(name+'.lst')).write_text('\n'.join(lines)+'\n',encoding='utf-8')
        return len(self.ops)

# 01 Aritmetica y comparaciones
p=Asm()
for i in [('addi','x5','x0',10),('addi','x6','x0',7),('add','x7','x5','x6'),
 ('sub','x8','x5','x6'),('xor','x9','x5','x6'),('addi','x11','x0',2),
 ('sll','x10','x6','x11'),('srai','x12','x9',1),('slt','x13','x8','x7'),
 ('or','x14','x5','x6'),('sw','x7','x0',256)]:p.ins(*i)
p.write('01_aritmetica')

# 02 Little-endian, extensiones de signo, cargas y stores
p=Asm()
for i in [('addi','x1','x0',256),('addi','x2','x0',-1),('sw','x2','x1',0),
 ('lb','x3','x1',0),('lbu','x4','x1',0),('lh','x5','x1',0),('lhu','x6','x1',0),
 ('lw','x7','x1',0),('addi','x8','x0',128),('sb','x8','x1',4),('lb','x9','x1',4),
 ('lbu','x10','x1',4),('addi','x11','x0',291),('sh','x11','x1',6),
 ('lh','x12','x1',6),('lhu','x13','x1',6)]:p.ins(*i)
p.write('02_memoria')

# 03 Branches, jal, jalr (bit cero borrado)
p=Asm()
p.ins('addi','x5','x0',0).ins('addi','x6','x0',5)
p.label('loop').ins('addi','x5','x5',1).ins('bne','x5','x6','loop')
p.ins('jal','x7','destino').ins('addi','x8','x0',99)
p.label('destino').ins('addi','x12','x0',85).ins('addi','x13','x0','final+1') # resolver referencia tras labels
p.ins('jalr','x14','x13',0).ins('addi','x15','x0',99).ins('addi','x16','x0',99)
p.label('final').ins('addi','x17','x0',1).ins('sw','x17','x0',292)
p.labels['final+1']=p.labels['final']+1
p.write('03_saltos')

# 04 Integracion: 25 en M[100] y ramificaciones y x0 inmutable
p=Asm()
for i in [('addi','x1','x0',10),('addi','x2','x0',15),('add','x3','x1','x2'),
 ('sub','x4','x2','x1'),('and','x5','x3','x1'),('or','x6','x2','x1'),
 ('slt','x7','x1','x2'),('slti','x8','x2',16),('xor','x9','x1','x2'),
 ('addi','x0','x0',33),('sw','x3','x0',100),('lw','x10','x0',100)]:p.ins(*i)
p.ins('beq','x10','x3','good').ins('addi','x11','x0',99)
p.label('good').ins('jal','x12','end').ins('addi','x11','x0',99)
p.label('end').ins('addi','x13','x0',1)
p.write('04_integral_25')

# 05 Quicksort Lomuto recursivo sobre [6,4,3,2,1,8,9] en 0x1000
p=Asm().ins('lui','sp',8).ins('lui','a0',1)
for j,v in enumerate([6,4,3,2,1,8,9]):
    p.ins('addi','t0','zero',v).ins('sw','t0','a0',4*j)
p.ins('addi','a1','zero',0).ins('addi','a2','zero',6).ins('jal','ra','qsort')
p.label('end').ins('jal','zero','end')
p.label('qsort').ins('bge','a1','a2','return')
p.ins('addi','sp','sp',-32)
for r,o in [('ra',28),('s0',24),('s1',20),('s2',16),('s3',12),('s4',8),('s5',4),('s6',0)]:p.ins('sw',r,'sp',o)
p.ins('add','s0','a0','zero').ins('add','s1','a1','zero').ins('add','s2','a2','zero')
p.ins('slli','t0','s2',2).ins('add','t0','s0','t0').ins('lw','s3','t0',0)
p.ins('add','s4','s1','zero').ins('add','s5','s1','zero')
p.label('part_loop').ins('bge','s5','s2','part_done')
p.ins('slli','t0','s5',2).ins('add','t0','s0','t0').ins('lw','t1','t0',0)
p.ins('bge','t1','s3','no_swap')
p.ins('slli','t2','s4',2).ins('add','t2','s0','t2').ins('lw','t3','t2',0)
p.ins('sw','t3','t0',0).ins('sw','t1','t2',0).ins('addi','s4','s4',1)
p.label('no_swap').ins('addi','s5','s5',1).ins('jal','zero','part_loop')
p.label('part_done').ins('slli','t0','s4',2).ins('add','t0','s0','t0').ins('lw','t1','t0',0)
p.ins('slli','t2','s2',2).ins('add','t2','s0','t2').ins('sw','t1','t2',0).ins('sw','s3','t0',0)
p.ins('add','a0','s0','zero').ins('add','a1','s1','zero').ins('addi','a2','s4',-1).ins('jal','ra','qsort')
p.ins('add','a0','s0','zero').ins('addi','a1','s4',1).ins('add','a2','s2','zero').ins('jal','ra','qsort')
for r,o in [('s6',0),('s5',4),('s4',8),('s3',12),('s2',16),('s1',20),('s0',24),('ra',28)]:p.ins('lw',r,'sp',o)
p.ins('addi','sp','sp',32)
p.label('return').ins('jalr','zero','ra',0)
p.write('05_quicksort')

# 06 Arbol simetrico recursivo almacenado en 0x1000
p=Asm().ins('lui','sp',8).ins('lui','s0',1)
for j,v in enumerate([6,4,4,1,2,2,1]):
    p.ins('addi','t0','zero',v).ins('sw','t0','s0',4*j)
p.ins('addi','a0','zero',1).ins('addi','a1','zero',2).ins('jal','ra','mirror')
p.label('end').ins('jal','zero','end')
p.label('mirror')
p.ins('addi','t0','zero',7).ins('bge','a0','t0','left_null').ins('bge','a1','t0','not_equal')
p.ins('slli','t1','a0',2).ins('add','t1','s0','t1').ins('lw','t2','t1',0)
p.ins('slli','t1','a1',2).ins('add','t1','s0','t1').ins('lw','t3','t1',0)
p.ins('bne','t2','t3','not_equal')
p.ins('addi','sp','sp',-16).ins('sw','ra','sp',12).ins('sw','a0','sp',8).ins('sw','a1','sp',4)
# mirror(2*i+1,2*j+2)
p.ins('slli','a0','a0',1).ins('addi','a0','a0',1).ins('slli','a1','a1',1).ins('addi','a1','a1',2)
p.ins('jal','ra','mirror').ins('beq','a0','zero','branch_false')
p.ins('lw','a0','sp',8).ins('lw','a1','sp',4)
# mirror(2*i+2,2*j+1)
p.ins('slli','a0','a0',1).ins('addi','a0','a0',2).ins('slli','a1','a1',1).ins('addi','a1','a1',1)
p.ins('jal','ra','mirror').ins('jal','zero','branch_end')
p.label('branch_false').ins('addi','a0','zero',0)
p.label('branch_end').ins('lw','ra','sp',12).ins('addi','sp','sp',16).ins('jalr','zero','ra',0)
p.label('left_null').ins('bge','a1','t0','both_null').ins('jal','zero','not_equal')
p.label('both_null').ins('addi','a0','zero',1).ins('jalr','zero','ra',0)
p.label('not_equal').ins('addi','a0','zero',0).ins('jalr','zero','ra',0)
p.write('06_arbol_simetrico')
print('Generados:', [(x.name, x.stat().st_size) for x in sorted(OUT.glob('*.bin'))])
