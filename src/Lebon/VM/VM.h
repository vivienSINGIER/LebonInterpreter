#ifndef VM_VM_H_DEFINED
#define VM_VM_H_DEFINED

#include <iostream>
#include <string>
#include <vector>

#include "../Bytecode/Heap.hpp"
#include "../Bytecode/Prototype.hpp"

namespace Bytecode
{
    // Machine virtuelle à registres : exécute le bytecode produit par le Compiler
    class VM
    {
    public:
        static constexpr size_t StackSize = 1 << 17;
        static constexpr size_t MaxFrames = 1024;

        explicit VM(Heap& _heap);

        // Où afise écrit (std::cout par défaut), les tests y mettent un flux pour récupérer la sortie
        void SetOutput(std::ostream& _out) { m_out = &_out; }
        std::ostream& Out() { return *m_out; }

        Heap& GetHeap() { return m_heap; }

        // Exécute main. Renvoie false si une erreur d'exécution s'est produite (elle est loggée comme Error::Execution).
        // Le programme et le Heap doivent rester vivants pendant l'appel
        bool Run(CompiledProgram const& _program);

    private:
        // Une fonction en cours d'exécution. Son résultat sera écrit dans base[-1], la case où se trouvait la fonction appelée
        struct CallFrame
        {
            FunctionObj* closure = nullptr;
            Instruction const* ip = nullptr;   
            Value* base = nullptr;             
        };

        Heap& m_heap;
        std::ostream* m_out = &std::cout;

        std::vector<Value> m_stack;
        std::vector<CallFrame> m_frames;
        std::vector<Value> m_globals;               
        UpvalueObj* m_openUpvalues = nullptr;       

        bool Execute();
        bool PushFrame(FunctionObj* _closure, Value* _base);
        bool RuntimeError(std::string const& _message);

        UpvalueObj* CaptureUpvalue(Value* _local);
        void CloseUpvalues(Value* _last);

        void DefineNative(CompiledProgram const& _program, std::string const& _name, int _arity, NativeFn _fn);
    };
}

#endif
