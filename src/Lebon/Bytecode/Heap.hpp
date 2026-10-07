#ifndef BYTECODE_HEAP_HPP_DEFINED
#define BYTECODE_HEAP_HPP_DEFINED

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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

        // Creates an object that lives as long as the heap
        template <typename T, typename... Args>
        T* New(Args&&... _args)
        {
            auto object = std::make_unique<T>(std::forward<Args>(_args)...);
            T* raw = object.get();
            m_objects.push_back(std::move(object));
            return raw;
        }

        size_t StringCount() const { return m_strings.size(); }

    private:
        std::unordered_map<std::string, std::unique_ptr<StringObj>> m_strings;
        std::vector<std::unique_ptr<Obj>> m_objects;
    };
}

#endif