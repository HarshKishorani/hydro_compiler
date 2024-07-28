#pragma once
#include <sstream>
#include "parser.hpp"

class Generator
{
public:
    inline Generator(NodeExit root) : m_root(std::move(root))
    {
    }

    /// @brief Generate the asm code using the provided Parse tree.
    /// @return
    std::string generate() const
    {
        std::stringstream output;
        output << "global _start\n_start:\n";
        output << "    mov rax, 60\n";
        output << "    mov rdi, " << m_root.expr.int_lit.value.value() << "\n";
        output << "    syscall\n";
        return output.str();
    }

private:
    const NodeExit m_root;
};