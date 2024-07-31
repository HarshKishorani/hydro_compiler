#pragma once
#include <sstream>
#include <unordered_map>
#include "parser.hpp"
#include <cassert>

/// @brief Class to generate assembly code from the parse tree.
class Generator
{
public:
    /**
     * @brief Constructs the generator with a given parse tree root.
     *
     * @param root The root of the parse tree.
     */
    inline Generator(NodeProg root) : m_prog(std::move(root))
    {
    }

    /**
     * @brief Generates assembly code for a term node.
     *
     * @param term The term node to generate code for.
     */
    void generate_term(const NodeTerm *term)
    {
        struct TermVisitor
        {
            Generator *gen;

            void operator()(const NodeTermIntLit *term_int_lit) const
            {
                gen->m_output << "    mov rax, " << term_int_lit->int_lit.value.value() << "\n";
                gen->push("rax");
            }

            void operator()(const NodeTermIdent *term_ident) const
            {
                if (!gen->m_vars.contains(term_ident->ident.value.value()))
                {
                    std::cerr << "Undeclared Identifier: " << term_ident->ident.value.value() << "\n";
                    exit(EXIT_FAILURE);
                }
                const auto var = gen->m_vars[term_ident->ident.value.value()];
                std::stringstream offset;
                // Make a copy of the value from the position in stack again on stack (Multiply by 8 for bytes).
                offset << "QWORD [rsp + " << (gen->m_stack_size - var.stack_loc - 1) * 8 << "]\n";
                gen->push(offset.str());
            }
        };

        TermVisitor visitor{.gen = this};
        std::visit(visitor, term->var);
    }

    /**
     * @brief Generates assembly code for an expression node.
     *
     * @param expr The expression node to generate code for.
     */
    void generate_expression(const NodeExpr *expr)
    {
        struct ExprVisitor
        {
            Generator *gen;

            void operator()(const NodeTerm *node_term) const
            {
                gen->generate_term(node_term);
            }

            void operator()(const NodeBinExpr *bin_expr) const
            {
                // Pushing both lhs and rhs on the stack.
                gen->generate_expression(bin_expr->add->lhs);
                gen->generate_expression(bin_expr->add->rhs);

                gen->pop("rax");
                gen->pop("rbx");

                gen->m_output << "    add rax, rbx\n";

                // Putting result back on stack.
                gen->push("rax");
            }
        };

        ExprVisitor visitor{.gen = this};
        std::visit(visitor, expr->var);
    }

    /**
     * @brief Generates assembly code for a statement node.
     *
     * @param stmt The statement node to generate code for.
     */
    void generate_statement(const NodeStmt *stmt)
    {
        struct StmtVisitor
        {
            Generator *gen;

            void operator()(const NodeStmtExit *stmt_exit) const
            {
                gen->generate_expression(stmt_exit->expr);
                gen->m_output << "    mov rax, 60\n";
                gen->pop("rdi");
                gen->m_output << "    syscall\n";
            }

            void operator()(const NodeStmtLet *stmt_let) const
            {
                if (gen->m_vars.contains(stmt_let->ident.value.value()))
                {
                    std::cerr << "Identifier already used: " << stmt_let->ident.value.value() << "\n";
                    exit(EXIT_FAILURE);
                }
                gen->m_vars.insert({stmt_let->ident.value.value(), Var{.stack_loc = gen->m_stack_size}});
                gen->generate_expression(stmt_let->expr);
            }
        };

        StmtVisitor visitor{.gen = this};
        std::visit(visitor, stmt->var);
    }

    /**
     * @brief Generates the assembly code for the entire program.
     *
     * @return The generated assembly code as a string.
     */
    std::string generate_program()
    {
        m_output << "global _start\n_start:\n";

        // Generate assembly for every statement in the parse tree.
        for (const NodeStmt *stmt : m_prog.stmts)
        {
            generate_statement(stmt);
        }

        m_output << "    mov rax, 60\n";
        m_output << "    mov rdi, 0\n";
        m_output << "    syscall\n";
        return m_output.str();
    }

private:
    /**
     * @brief Pushes a register onto the system stack in assembly.
     *
     * @param reg The register to push.
     */
    void push(const std::string &reg)
    {
        m_output << "    push " << reg << "\n";
        m_stack_size++;
    }

    /**
     * @brief Pops a register from the system stack in assembly.
     *
     * @param reg The register to pop into.
     */
    void pop(const std::string &reg)
    {
        m_output << "    pop " << reg << "\n";
        m_stack_size--;
    }

    const NodeProg m_prog;      // The root of the parse tree.
    std::stringstream m_output; // The output string stream for the generated assembly code.

    /// @brief Represents a variable in the stack with its location.
    struct Var
    {
        size_t stack_loc; // The location of the variable in the stack.
    };

    size_t m_stack_size = 0;                     // The current size of the stack.
    std::unordered_map<std::string, Var> m_vars; // Map of variable names to their stack locations.
};
