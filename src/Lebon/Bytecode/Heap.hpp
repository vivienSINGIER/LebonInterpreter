#ifndef BYTECODE_HEAP_HPP_DEFINED
#define BYTECODE_HEAP_HPP_DEFINED

#include <memory>
#include <string>
#include <unordered_map>

#include "Object.hpp"

namespace Bytecode
{
    // Owns the objects shared by the compiler and the VM.
    // No garbage collector yet : everything lives until the Heap dies
    class Heap
    {
    public:
        // The same text always gives the same StringObj
        StringObj* Intern(std::string const& _text)
        {
            auto it = m_strings.find(_text);
            if (it != m_strings.end())
                return it->second.get();

            auto inserted = m_strings.emplace(_text, std::make_unique<StringObj>(_text));
            return inserted.first->second.get();
        }

        size_t StringCount() const { return m_strings.size(); }

    private:
        std::unordered_map<std::string, std::unique_ptr<StringObj>> m_strings;
    };
}

#endif
