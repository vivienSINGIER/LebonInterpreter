#ifndef VM_VM_H_DEFINED
#define VM_VM_H_DEFINED

#include <array>
#include <cstdint>
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

        // Statistiques d'exécution, désactivées par défaut : la boucle qui les compte est une version à part,
        // la boucle normale n'y perd rien. Elles sont remises à zéro au début de chaque Run
        void EnableStats(bool _enabled) { m_statsEnabled = _enabled; }
        uint64_t InstructionsExecuted() const { return m_executed; }
        uint64_t InstructionsExecuted(OpCode _op) const { return m_opCounts[static_cast<size_t>(_op)]; }

    private:
        // Une fonction en cours d'exécution. Son résultat sera écrit dans base[-1], la case où se trouvait la fonction appelée.
        // Les constantes sont copiées ici à l'appel : un retour recharge le contexte de l'appelant sans remonter closure->proto
        struct CallFrame
        {
            FunctionObj* closure = nullptr;
            Instruction const* ip = nullptr;       // prochaine instruction, à jour seulement quand la frame n'est pas la courante
            Value* base = nullptr;                 // R0 de la fonction
            Value const* constants = nullptr;
        };

        Heap& m_heap;
        std::ostream* m_out = &std::cout;

        std::vector<Value> m_stack;
        std::vector<CallFrame> m_frames;            // MaxFrames cases, jamais redimensionné : on y circule avec des pointeurs
        CallFrame* m_top = nullptr;                 // frame courante, nullptr hors exécution. À jour quand une erreur est signalée
        std::vector<Value> m_globals;
        UpvalueObj* m_openUpvalues = nullptr;

        bool m_statsEnabled = false;
        uint64_t m_executed = 0;
        std::array<uint64_t, static_cast<size_t>(OpCode::Count)> m_opCounts{};

        template <bool CountInstructions>
        bool Execute();
        bool RuntimeError(std::string const& _message);

        UpvalueObj* CaptureUpvalue(Value* _local);
        void CloseUpvalues(Value* _last);

        void DefineNative(CompiledProgram const& _program, std::string const& _name, int _arity, NativeFn _fn);
    };
}

#endif
