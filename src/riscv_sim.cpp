#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace {
std::string hx(uint32_t v, int width=8) {
    std::ostringstream os;
    os << "0x" << std::hex << std::uppercase << std::setw(width)
       << std::setfill('0') << v;
    return os.str();
}
uint32_t parseNumber(const std::string& s) {
    std::size_t pos = 0;
    auto v = std::stoull(s, &pos, 0);
    if(pos != s.size() || v > 0xFFFFFFFFull) throw std::runtime_error("Numero fuera de rango: " + s);
    return static_cast<uint32_t>(v);
}
unsigned regIndex(const std::string& s) {
    if (s.size() < 2 || s[0] != 'x') throw std::runtime_error("Registro invalido: " + s);
    std::size_t pos = 0;
    const auto v = std::stoul(s.substr(1), &pos, 10);
    if (pos != s.size()-1 || v >= 32) throw std::runtime_error("Registro fuera de rango: " + s);
    return static_cast<unsigned>(v);
}
uint32_t bits(uint32_t w, unsigned hi, unsigned lo) {
    return (w >> lo) & ((uint32_t(1) << (hi - lo + 1)) - 1);
}
int32_t sext(uint32_t val, unsigned width) {
    const uint32_t sign = uint32_t(1) << (width - 1);
    return static_cast<int32_t>((val ^ sign) - sign);
}
uint32_t addSigned(uint32_t a, int32_t b) { return a + static_cast<uint32_t>(b); }
std::string opcodeName(uint32_t word) {
    const auto opcode = word & 0x7Fu, f3 = bits(word,14,12), f7=bits(word,31,25);
    if (opcode == 0x37) return "lui";
    if (opcode == 0x17) return "auipc";
    if (opcode == 0x6F) return "jal";
    if (opcode == 0x67) return "jalr";
    if (opcode == 0x03) { const char* s[]={"lb","lh","lw","?","lbu","lhu","?","?"}; return s[f3]; }
    if (opcode == 0x23) { const char* s[]={"sb","sh","sw","?","?","?","?","?"}; return s[f3]; }
    if (opcode == 0x63) { const char* s[]={"beq","bne","?","?","blt","bge","bltu","bgeu"}; return s[f3]; }
    if (opcode == 0x13) { const char* s[]={"addi","slli","slti","sltiu","xori","srli/srai","ori","andi"}; return s[f3]; }
    if (opcode == 0x33) { const char* s[]={"add/sub","sll","slt","sltu","xor","srl/sra","or","and"};
       if(f3==0) return f7==0x20 ? "sub" : "add";
       if(f3==5) return f7==0x20 ? "sra" : "srl";
       return s[f3]; }
    return "desconocida";
}
class SparseMemory {
    static constexpr uint32_t PAGE_SHIFT = 12, PAGE_SIZE = 1u<<PAGE_SHIFT;
    std::unordered_map<uint32_t,std::array<uint8_t,PAGE_SIZE>> pages_;
    static void valid(uint32_t addr, uint64_t len) {
        if(uint64_t(addr) + len > (1ull<<32)) throw std::runtime_error("Acceso fuera de direccion de 32 bits");
    }
public:
    void clear() { pages_.clear(); }
    uint8_t r8(uint32_t addr) const {
        const auto it = pages_.find(addr >> PAGE_SHIFT);
        return it == pages_.end() ? 0 : it->second[addr & (PAGE_SIZE-1)];
    }
    void w8(uint32_t addr, uint8_t v) {
        auto [it, inserted]=pages_.try_emplace(addr >> PAGE_SHIFT);
        if(inserted) it->second.fill(0);
        it->second[addr & (PAGE_SIZE-1)] = v;
    }
    uint32_t read(uint32_t addr, unsigned n) const {
        valid(addr,n);
        if((n==2 && (addr&1)) || (n==4 && (addr&3)))
            throw std::runtime_error("Acceso desalineado en " + hx(addr));
        uint32_t val=0;
        for(unsigned i=0;i<n;i++) val |= uint32_t(r8(addr+i)) << (8*i);
        return val;
    }
    void write(uint32_t addr, unsigned n, uint32_t val) {
        valid(addr,n);
        if((n==2 && (addr&1)) || (n==4 && (addr&3)))
            throw std::runtime_error("Acceso desalineado en " + hx(addr));
        for(unsigned i=0;i<n;i++) w8(addr+i, static_cast<uint8_t>(val >> (8*i)));
    }
};
class CPU {
    SparseMemory mem_;
    std::array<uint32_t,32> r_{};
    std::vector<uint8_t> image_;
    uint32_t pc_=0;
    uint64_t steps_=0;
    bool trapped_=false;
    std::string last_error_;
    void regwrite(unsigned rd,uint32_t val) { if(rd!=0) r_[rd]=val; }
    static void require(bool condition,const std::string& why) {
        if(!condition) throw std::runtime_error(why);
    }
public:
    void load(const std::string& file) {
        std::ifstream in(file,std::ios::binary);
        if(!in) throw std::runtime_error("No se puede abrir: " + file);
        image_.assign(std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>());
        if (image_.empty()) throw std::runtime_error("El archivo binario esta vacio");
        if (uint64_t(image_.size()) > (1ull<<32)) throw std::runtime_error("Imagen mayor que 4 GiB");
        if (image_.size()%4!=0) throw std::runtime_error("Binario no contiene un numero entero de instrucciones de 32 bits");
        reset();
    }
    void reset() {
        mem_.clear(); r_.fill(0); pc_=0; steps_=0; trapped_=false; last_error_.clear();
        for(std::size_t i=0;i<image_.size();i++) mem_.w8(static_cast<uint32_t>(i),image_[i]);
    }
    uint32_t pc() const {return pc_;}
    uint32_t reg(unsigned n) const {return r_.at(n);}
    uint32_t mem(uint32_t addr,unsigned n=4) const {return mem_.read(addr,n);}
    uint8_t byte(uint32_t a) const {return mem_.r8(a);}
    uint64_t count() const {return steps_;}
    bool trapped() const {return trapped_;}
    const std::string& error() const {return last_error_;}
    uint32_t inst() const {return mem_.read(pc_,4);}
    std::string current() const {auto w=inst();return hx(pc_)+": "+hx(w)+"  "+opcodeName(w);}

    bool step(bool verbose=true) {
        if(trapped_) { if(verbose) std::cout << "CPU detenida: " << last_error_ << '\n';return false; }
        const uint32_t old_pc=pc_;
        uint32_t w=0;
        try {
            require((pc_&3)==0,"PC desalineado: "+hx(pc_));
            w=mem_.read(pc_,4);
            require((w&3)==3,"Instruccion comprimida/no RV32I: "+hx(w));
            const unsigned op=w&0x7F,rd=bits(w,11,7),f3=bits(w,14,12),
                  rs1=bits(w,19,15),rs2=bits(w,24,20),f7=bits(w,31,25);
            const uint32_t a=r_[rs1],b=r_[rs2];
            const int32_t immI=sext(bits(w,31,20),12);
            const int32_t immS=sext((bits(w,31,25)<<5)|bits(w,11,7),12);
            const int32_t immB=sext((bits(w,31,31)<<12)|(bits(w,7,7)<<11)|
                      (bits(w,30,25)<<5)|(bits(w,11,8)<<1),13);
            const int32_t immJ=sext((bits(w,31,31)<<20)|(bits(w,19,12)<<12)|
                      (bits(w,20,20)<<11)|(bits(w,30,21)<<1),21);
            const uint32_t immU=w&0xFFFFF000u;
            uint32_t next=pc_+4;
            switch(op) {
                case 0x37: regwrite(rd,immU); break; // LUI
                case 0x17: regwrite(rd,pc_+immU); break; // AUIPC
                case 0x6F: regwrite(rd,pc_+4);next=addSigned(pc_,immJ); break; // JAL
                case 0x67: // JALR
                    require(f3==0,"funct3 invalido para JALR");
                    next=addSigned(a,immI)&~uint32_t(1);regwrite(rd,pc_+4);break;
                case 0x63: { // branches
                    bool take=false;
                    switch(f3) {
                        case 0:take=a==b;break; case 1:take=a!=b;break;
                        case 4:take=int32_t(a)<int32_t(b);break;
                        case 5:take=int32_t(a)>=int32_t(b);break;
                        case 6:take=a<b;break; case 7:take=a>=b;break;
                        default:throw std::runtime_error("funct3 de branch invalido");
                    }
                    if(take) next=addSigned(pc_,immB);
                    break;
                }
                case 0x03: { // load
                    unsigned n=0;bool is_signed=true;
                    switch(f3) {
                        case 0:n=1;break;case 1:n=2;break;case 2:n=4;break;
                        case 4:n=1;is_signed=false;break;case 5:n=2;is_signed=false;break;
                        default:throw std::runtime_error("funct3 de LOAD invalido");
                    }
                    uint32_t v=mem_.read(addSigned(a,immI),n);
                    if(is_signed&&n<4) v=uint32_t(sext(v,8*n));
                    regwrite(rd,v);break;
                }
                case 0x23: { // store
                    unsigned n=0;
                    switch(f3) {case 0:n=1;break;case 1:n=2;break;case 2:n=4;break;
                        default:throw std::runtime_error("funct3 de STORE invalido"); }
                    mem_.write(addSigned(a,immS),n,b);break;
                }
                case 0x13: { // OP-IMM
                    uint32_t res=0; const unsigned shamt=bits(w,24,20);
                    switch(f3) {
                        case 0:res=addSigned(a,immI);break;  // ADDI
                        case 2:res=int32_t(a)<immI;break;   // SLTI
                        case 3:res=a<uint32_t(immI);break;  // SLTIU
                        case 4:res=a^uint32_t(immI);break;  // XORI
                        case 6:res=a|uint32_t(immI);break;  // ORI
                        case 7:res=a&uint32_t(immI);break;  // ANDI
                        case 1:
                            require(f7==0,"SLLI con funct7 invalido");res=a<<shamt;break;
                        case 5:
                            require(f7==0||f7==0x20,"SRLI/SRAI con funct7 invalido");
                            res=(f7==0) ? a>>shamt : uint32_t(int32_t(a)>>shamt);break;
                        default:throw std::runtime_error("funct3 de OP-IMM invalido");
                    }
                    regwrite(rd,res);break;
                }
                case 0x33: { // OP
                    uint32_t res=0; const unsigned shamt=b&31;
                    if (f3==0 && f7==0x20) res=a-b;
                    else if(f3==5 && f7==0x20) res=uint32_t(int32_t(a)>>shamt);
                    else {
                        require(f7==0,"funct7 de OP invalido");
                        switch(f3) {
                            case 0:res=a+b;break;case 1:res=a<<shamt;break;
                            case 2:res=int32_t(a)<int32_t(b);break;
                            case 3:res=a<b;break;case 4:res=a^b;break;
                            case 5:res=a>>shamt;break;case 6:res=a|b;break;
                            case 7:res=a&b;break;
                            default:throw std::runtime_error("funct3 de OP invalido");
                        }
                    }
                    regwrite(rd,res);break;
                }
                default:throw std::runtime_error("Opcode no admitido: "+hx(op,2));
            }
            require((next&3)==0,"Salto a PC desalineado: "+hx(next));
            pc_=next;r_[0]=0;steps_++;
            if(verbose) std::cout << "[" << steps_ << "] PC=" << hx(old_pc) << " "
                          << opcodeName(w) << " " << hx(w) << " -> " << hx(pc_) << '\n';
            return true;
        } catch(const std::exception& e) {
            trapped_=true;
            last_error_="TRAP en PC="+hx(old_pc)+" instruccion="+hx(w)+": "+e.what();
            if(verbose) std::cerr << last_error_ << '\n';
            return false;
        }
    }
    void regsAll() const {
        for(unsigned i=0;i<32;i++) {
            std::cout << "x" << std::setw(2) << std::setfill('0') << i << "=" << hx(r_[i]);
            if(i%4==3) std::cout << '\n'; else std::cout << "  ";
        }
        std::cout << std::setfill(' ');
    }
    void dump(uint32_t from,uint32_t to) const {
        if(to<from) throw std::runtime_error("Rango invalido de memoria");
        if(uint64_t(to)-from>4095) throw std::runtime_error("Vista limitada a 4096 bytes por comando");
        for(uint64_t a=from;a<=to;a++) {
            if ((a-from)%16==0) std::cout << hx(static_cast<uint32_t>(a)) << ": ";
            std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                << unsigned(mem_.r8(static_cast<uint32_t>(a))) << ' ';
            if((a-from)%16==15 || a==to) std::cout << std::dec << std::setfill(' ') << '\n';
        }
    }
};
void help() {
    std::cout << "Comandos:\n"
       << "  help                  ayuda\n"
       << "  pc                    muestra PC e instruccion actual\n"
       << "  step [n]              ejecuta 1 o n instrucciones\n"
       << "  run [max]             ejecuta hasta max instrucciones (100000 por defecto) o error\n"
       << "  until pc [max]        ejecuta hasta que PC llegue a esa direccion (sin ejecutarla)\n"
       << "  regs [x1 x2 ...]      muestra todos o registros concretos\n"
       << "  mem inicio fin        muestra bytes de memoria (rango inclusivo; <=4096)\n"
       << "  word direccion        lee una palabra de 32 bits, little-endian\n"
       << "  reset                 reinicia PC, registros y memoria con el binario\n"
       << "  exit                  termina\n";
}
} // namespace
int main(int argc,char** argv) {
    if(argc!=2) {
        std::cerr << "Uso: " << argv[0] << " programa.bin\n";
        return 1;
    }
    CPU cpu;
    try {cpu.load(argv[1]);}
    catch(const std::exception& e) {std::cerr << "Carga fallida: " << e.what() << '\n';return 1;}
    std::cout << "RV32I simulador - imagen cargada en 0x00000000\n";
    help();
    std::string line;
    while(std::cout<<"rv32i> " && std::getline(std::cin,line)) {
        std::istringstream in(line);std::string cmd;in>>cmd;
        if(cmd.empty()) continue;
        try {
            if(cmd=="exit"||cmd=="quit") break;
            if(cmd=="help"||cmd=="?") help();
            else if(cmd=="pc") std::cout << cpu.current() << " pasos=" << cpu.count() << '\n';
            else if(cmd=="regs") {
                std::string r;bool any=false;
                while(in>>r) {any=true;auto n=regIndex(r);std::cout << "x"<<n<<"="<<hx(cpu.reg(n))<<" ("<<int32_t(cpu.reg(n))<<")\n";}
                if(!any)cpu.regsAll();
            }
            else if(cmd=="mem") {
                std::string a,b; if(!(in>>a>>b)) throw std::runtime_error("Uso: mem inicio fin");
                cpu.dump(parseNumber(a),parseNumber(b));
            }
            else if(cmd=="word") {
                std::string a;if(!(in>>a)) throw std::runtime_error("Uso: word direccion");
                uint32_t addr=parseNumber(a);
                std::cout << "Mem["<<hx(addr)<<"]="<<hx(cpu.mem(addr))<<"\n";
            }
            else if(cmd=="until") {
                std::string targetText, maxText;
                if(!(in>>targetText)) throw std::runtime_error("Uso: until 0xPC [max]");
                const uint32_t target=parseNumber(targetText);
                uint32_t max=100000; if(in>>maxText) max=parseNumber(maxText);
                if(max>1000000) throw std::runtime_error("Limite maximo por comando: 1000000");
                uint32_t n=0;
                while(cpu.pc()!=target && n<max && !cpu.trapped()) {
                    if(!cpu.step(false)) break;
                    ++n;
                }
                std::cout<<"until: ejecutadas="<<n<<" PC="<<hx(cpu.pc())
                         <<(cpu.pc()==target?" [OBJETIVO]":cpu.trapped()?" [TRAP]":" [LIMITE]")<<"\n";
            }
            else if(cmd=="step" || cmd=="run") {
                std::string n;uint32_t max=cmd=="step"?1u:100000u;
                if(in>>n) max=parseNumber(n);
                if(max>1000000) throw std::runtime_error("Limite maximo por comando: 1000000");
                uint32_t completed=0;
                for(;completed<max;completed++) if(!cpu.step(cmd=="step")) break;
                std::cout << "Ejecutadas en comando="<<completed << ", total="<<cpu.count()
                          <<", PC="<<hx(cpu.pc())<<(cpu.trapped()?" [TRAP]":"")<<"\n";
            }
            else if(cmd=="reset") {cpu.reset();std::cout<<"Reiniciado. PC="<<hx(cpu.pc())<<'\n';}
            else std::cout<<"Comando desconocido: "<<cmd<<"; escribe help\n";
        }catch(const std::exception& e){std::cerr<<"Error de comando: "<<e.what()<<'\n';}
    }
    std::cout << "Fin de simulacion.\n";
    return 0;
}
