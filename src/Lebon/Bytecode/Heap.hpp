#ifndef BYTECODE_HEAP_HPP_DEFINED
#define BYTECODE_HEAP_HPP_DEFINED

#include <memory>
#include <string>
#include <unordered_map>

#include "Object.hpp"

namespace Bytecode
{
    // Manage les objects que partagent le compilo et la VM 
    // Pas de Garbage Collector donc tout vit avec le heap
    class Heap
    {
    public:
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
