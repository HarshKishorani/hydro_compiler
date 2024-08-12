#pragma once
#include <sstream>
#include <map>
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
    Generator(NodeProg root) : m_prog(std::move(root))
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
            Generator &gen;

            void operator()(const NodeTermIntLit *term_int_lit) const
            {
                gen.m_output << "    mov rax, " << term_int_lit->int_lit.value.value() << "\n";
                gen.push("rax");
            }

            void operator()(const NodeTermIdent *term_ident) const
            {
                const auto it = std::find_if(gen.m_vars.cbegin(), gen.m_vars.cend(), [&](const Var &var)
                                             { return var.name == term_ident->ident.value.value(); });
                if (it == gen.m_vars.cend())
                {
                    std::cerr << "Undeclared Identifier: " << term_ident->ident.value.value() << "\n";
                    exit(EXIT_FAILURE);
                }
                std::stringstream offset;
                // Make a copy of the value from the position in stack again on stack (Multiply by 8 for bytes).
                offset << "QWORD [rsp + " << (gen.m_stack_size - it->stack_loc - 1) * 8 << "]";
                gen.push(offset.str());
            }

            void operator()(const NodeTermParen *term_paren) const
            {
                gen.generate_expression(term_paren->expr);
            }
        };

        TermVisitor visitor{.gen = *this};
        std::visit(visitor, term->var);
    }

    /// @brief Generates assembly code for a binary expression node.
    /// @param bin_expr The binary expression node to generate code for.
    void generate_binary_expression(const NodeBinExpr *bin_expr)
    {
        struct BinExprVisitor
        {
            Generator &gen;

            void operator()(const NodeBinExprAdd *bin_expr_add) const
            {
                // Pushing both lhs and rhs on the stack.
                gen.generate_expression(bin_expr_add->rhs);
                gen.generate_expression(bin_expr_add->lhs);

                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    add rax, rbx\n";

                // Putting result back on stack.
                gen.push("rax");
            }

            void operator()(const NodeBinExprMulti *bin_expr_mult) const
            {
                // Pushing both lhs and rhs on the stack.
                gen.generate_expression(bin_expr_mult->rhs);
                gen.generate_expression(bin_expr_mult->lhs);

                gen.pop("rax");
                gen.pop("rbx");

                gen.m_output << "    mul rbx\n";

                // Putting result back on stack.
                gen.push("rax");
            }

            void operator()(const NodeBinExprSub *bin_expr_sub) const
            {
                // Pushing both rhs and lhs on the stack.
                gen.generate_expression(bin_expr_sub->rhs);
                gen.generate_expression(bin_expr_sub->lhs);

                gen.pop("rax");
                gen.pop("rbx");

                gen.m_output << "    sub rax, rbx\n";

                // Putting result back on stack.
                gen.push("rax");
            }

            void operator()(const NodeBinExprDiv *bin_expr_div) const
            {
                // Pushing both lhs and rhs on the stack.
                gen.generate_expression(bin_expr_div->rhs);
                gen.generate_expression(bin_expr_div->lhs);

                gen.pop("rax");
                gen.pop("rbx");

                gen.m_output << "    div rbx\n";

                // Putting result back on stack.
                gen.push("rax");
            }
        };

        BinExprVisitor visitor{.gen = *this};
        std::visit(visitor, bin_expr->var);
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
            Generator &gen;

            void operator()(const NodeTerm *node_term) const
            {
                gen.generate_term(node_term);
            }

            void operator()(const NodeBinExpr *bin_expr) const
            {
                gen.generate_binary_expression(bin_expr);
            }
        };

        ExprVisitor visitor{.gen = *this};
        std::visit(visitor, expr->var);
    }

    /// @brief Generates assembly code for a scope node.
    /// @param scope NodeScope to generate code for.
    void generate_scope(const NodeScope *scope)
    {
        begin_scope();
        for (const NodeStmt *stmt : scope->stmts)
        {
            generate_statement(stmt);
        }
        end_scope();
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
            Generator &gen;

            void operator()(const NodeStmtExit *stmt_exit) const
            {
                gen.generate_expression(stmt_exit->expr);
                gen.m_output << "    mov rax, 60\n";
                gen.pop("rdi");
                gen.m_output << "    syscall\n";
            }

            void operator()(const NodeStmtLet *stmt_let) const
            {
                auto it = std::find_if(gen.m_vars.cbegin(), gen.m_vars.cend(), [&](const Var &var)
                                       { return var.name == stmt_let->ident.value.value(); });
                if (it != gen.m_vars.cend())
                {
                    std::cerr << "Identifier already used: " << stmt_let->ident.value.value() << "\n";
                    exit(EXIT_FAILURE);
                }
                gen.m_vars.push_back(Var{.name = stmt_let->ident.value.value(), .stack_loc = gen.m_stack_size});
                gen.generate_expression(stmt_let->expr);
            }

            void operator()(const NodeScope *stmt_scope)
            {
                gen.generate_scope(stmt_scope);
            }

            void operator()(const NodeStmtIf *stmt_if)
            {
                gen.generate_expression(stmt_if->expr);
                gen.pop("rax");

                const std::string label = gen.create_label();

                gen.m_output << "    test rax, rax\n";      // check condition in assembly.
                gen.m_output << "    jz " << label << "\n"; // jump to label if condition is false i.e 0.
                gen.generate_scope(stmt_if->scope);
                gen.m_output << label << ":\n";
            }
        };

        StmtVisitor visitor{.gen = *this};
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

    /// @brief Beginning the scope.
    void begin_scope()
    {
        m_scopes.push_back(m_vars.size());
    }

    /// @brief Ending the scope.
    void end_scope()
    {
        const size_t pop_count = m_vars.size() - m_scopes.back();
        m_output << "    add rsp, " << pop_count * 8 << "\n";
        m_stack_size -= pop_count;
        for (size_t i = 0; i < pop_count; i++)
        {
            m_vars.pop_back();
        }
        m_scopes.pop_back();
    }

    /// @brief Create 'label' in assembly to jump to when 'if' condition is not true.
    /// @return
    std::string create_label()
    {
        return "label" + std::to_string(m_label_count++);
    }

    const NodeProg m_prog;      // The root of the parse tree.
    std::stringstream m_output; // The output string stream for the generated assembly code.

    /// @brief Represents a variable in the stack with its location.
    struct Var
    {
        std::string name; // Name of the variable.
        size_t stack_loc; // The location of the variable in the stack.
    };

    size_t m_stack_size = 0;   // The current size of the stack.
    std::vector<Var> m_vars{}; // List of all the variables created.
    std::vector<size_t> m_scopes{};
    int m_label_count = 0; // Number of labels created.
};
