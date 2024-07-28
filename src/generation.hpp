#pragma once
#include <sstream>
#include <unordered_map>
#include "parser.hpp"

class Generator
{
public:
    inline Generator(NodeProg root) : m_prog(std::move(root))
    {
    }

    void generate_expression(const NodeExpr &expr)
    {
        struct ExprVisitor
        {
            Generator *gen;

            void operator()(const NodeExprIntLit &expr_int_lit) const
            {
                gen->m_output << "    mov rax, " << expr_int_lit.int_lit.value.value() << "\n";
                gen->push("rax");
            }

            void operator()(const NodeExprIdent &expr_ident) const
            {
                if (!gen->m_vars.contains(expr_ident.ident.value.value()))
                {
                    std::cerr << "Undeclared Identifier: " << expr_ident.ident.value.value() << "\n";
                    exit(EXIT_FAILURE);
                }
                const auto var = gen->m_vars[expr_ident.ident.value.value()];
                std::stringstream offset;
                // Make a copy of the value from the position in stack again on stack. (Multiply by 8 for bytes)
                offset << "QWORD [rsp + " << (gen->m_stack_size - var.stack_loc - 1) * 8 << "]\n";
                gen->push(offset.str());
            }
        };

        ExprVisitor visitor{.gen = this};
        std::visit(visitor, expr.var);
    }

    void generate_statement(const NodeStmt &stmt)
    {
        struct StmtVisitor
        {
            Generator *gen;
            void operator()(const NodeStmtExit &stmt_exit) const
            {
                gen->generate_expression(stmt_exit.expr);
                gen->m_output << "    mov rax, 60\n";
                gen->pop("rdi");
                gen->m_output << "    syscall\n";
            }

            void operator()(const NodeStmtLet &stmt_let) const
            {
                if (gen->m_vars.contains(stmt_let.ident.value.value()))
                {
                    std::cerr << "Identifier already used : " << stmt_let.ident.value.value() << "\n";
                    exit(EXIT_FAILURE);
                }
                gen->m_vars.insert({stmt_let.ident.value.value(), Var{.stack_loc = gen->m_stack_size}});
                gen->generate_expression(stmt_let.expr);
            }
        };

        StmtVisitor visitor{.gen = this};
        std::visit(visitor, stmt.var);
    }

    /// @brief Generate the asm code using the provided Parse tree.
    /// @return
    std::string generate_program()
    {
        m_output << "global _start\n_start:\n";

        // Make Assembly for every statement in the parse tree.
        for (const NodeStmt &stmt : m_prog.stmts)
        {
            generate_statement(stmt);
        }

        m_output << "    mov rax, 60\n";
        m_output << "    mov rdi, 0\n";
        m_output << "    syscall\n";
        return m_output.str();
    }

private:
    /// @brief Push command for system stack in assembly.
    /// @param reg Register to push in.
    void push(const std::string &reg)
    {
        m_output << "    push " << reg << "\n";
        m_stack_size++;
    }

    /// @brief pop command for system stack in assembly.
    /// @param reg Register to pop into.
    void pop(const std::string &reg)
    {
        m_output << "    pop " << reg << "\n";
        m_stack_size--;
    }

    const NodeProg m_prog;
    std::stringstream m_output;

    // Stack Size and Stack Pointer
    struct Var
    {
        size_t stack_loc;
    };

    size_t m_stack_size = 0;
    std::unordered_map<std::string, Var> m_vars;
};