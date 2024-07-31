#pragma once
#include "tokenization.hpp"
#include "arena.hpp"
#include <variant>
#include <vector>

// Expressions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// @brief Represents an integer literal term in the parse tree.
struct NodeTermIntLit
{
    Token int_lit; // The token representing the integer literal.
};

/// @brief Represents an identifier term in the parse tree.
struct NodeTermIdent
{
    Token ident; // The token representing the identifier.
};

struct NodeExpr; // Forward declaration of NodeExpr

/// @brief Represents an addition binary expression in the parse tree.
struct NodeBinExprAdd
{
    NodeExpr *lhs; // Left-hand side of the addition expression.
    NodeExpr *rhs; // Right-hand side of the addition expression.
};

// Uncomment for multiplication binary expression
// struct NodeBinExprMulti
// {
//     NodeExpr *lhs;
//     NodeExpr *rhs;
// };

/// @brief Represents a binary expression in the parse tree.
struct NodeBinExpr
{
    NodeBinExprAdd *add; // The addition binary expression.
};

/// @brief Represents a term in the parse tree.
struct NodeTerm
{
    std::variant<NodeTermIntLit *, NodeTermIdent *> var; // Variant holding the term type.
};

/// @brief Represents an expression in the parse tree.
struct NodeExpr
{
    std::variant<NodeTerm *, NodeBinExpr *> var; // Variant holding the expression type.
};

// Statements ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// @brief Represents an 'exit' statement in the parse tree.
struct NodeStmtExit
{
    NodeExpr *expr; // Expression associated with the 'exit' statement.
};

/// @brief Represents a 'let' statement in the parse tree.
struct NodeStmtLet
{
    Token ident;    // The identifier token.
    NodeExpr *expr; // The expression associated with the 'let' statement.
};

/// @brief Represents a statement in the parse tree.
struct NodeStmt
{
    std::variant<NodeStmtExit *, NodeStmtLet *> var; // Variant holding the statement type.
};

// Program Parse Tree ~~~~~~~~~~~~~~~~~~~~~~~~~~

/// @brief Represents the root of the parse tree.
struct NodeProg
{
    std::vector<NodeStmt *> stmts; // List of statements in the program.
};

/// @brief Class to parse tokens into a parse tree.
class Parser
{
public:
    /**
     * @brief Constructs the parser with a given list of tokens.
     *
     * @param tokens The list of tokens to parse.
     */
    inline explicit Parser(std::vector<Token> tokens) : m_tokens(std::move(tokens)), m_allocator(1024 * 1024 * 4) // 4MB
    {
    }

    /**
     * @brief Parses a term from the tokens.
     *
     * @return An optional NodeTerm pointer if a term is parsed successfully.
     */
    std::optional<NodeTerm *> parse_term()
    {
        if (auto int_lit = try_consume(TokenType::int_lit))
        {
            auto term_int_lit = m_allocator.alloc<NodeTermIntLit>();
            term_int_lit->int_lit = int_lit.value();

            auto term = m_allocator.alloc<NodeTerm>();
            term->var = term_int_lit;

            return term;
        }
        else if (auto ident = try_consume(TokenType::ident))
        {
            auto term_ident = m_allocator.alloc<NodeTermIdent>();
            term_ident->ident = ident.value();

            auto term = m_allocator.alloc<NodeTerm>();
            term->var = term_ident;

            return term;
        }
        return {};
    }

    /**
     * @brief Parses an expression from the tokens.
     *
     * @return An optional NodeExpr pointer if an expression is parsed successfully.
     */
    std::optional<NodeExpr *> parse_expr()
    {
        if (auto term = parse_term())
        {
            if (try_consume(TokenType::plus).has_value())
            {
                auto bin_expr = m_allocator.alloc<NodeBinExpr>();
                auto bin_expr_add = m_allocator.alloc<NodeBinExprAdd>();
                auto lhs_expr = m_allocator.alloc<NodeExpr>();

                lhs_expr->var = term.value();
                bin_expr_add->lhs = lhs_expr;
                if (auto rhs = parse_expr())
                {
                    bin_expr_add->rhs = rhs.value();

                    bin_expr->add = bin_expr_add;

                    auto expr = m_allocator.alloc<NodeExpr>();
                    expr->var = bin_expr;
                    return expr;
                }
                else
                {
                    std::cerr << "Expected expression rhs." << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                auto expr = m_allocator.alloc<NodeExpr>();
                expr->var = term.value();
                return expr;
            }
        }
        else
        {
            return {};
        }
    }

    /**
     * @brief Parses a statement from the tokens.
     *
     * @return An optional NodeStmt pointer if a statement is parsed successfully.
     */
    std::optional<NodeStmt *> parse_stmt()
    {
        // Parse 'exit' statement
        if (peek().value().type == TokenType::exit && peek(1).has_value() && peek(1).value().type == TokenType::open_paren)
        {
            consume(); // Consume 'exit' token
            consume(); // Consume '(' token
            auto stmt_exit = m_allocator.alloc<NodeStmtExit>();

            // Parse expression for 'exit' statement
            if (auto node_expr = parse_expr())
            {
                stmt_exit->expr = node_expr.value();
            }
            else
            {
                std::cerr << "Invalid expression" << std::endl;
                exit(EXIT_FAILURE);
            }

            // Check ')' and consume
            try_consume(TokenType::close_paren, "Expected `)`");

            // Check ';' and consume
            try_consume(TokenType::semi, "Expected `;`");

            auto node_stmt = m_allocator.alloc<NodeStmt>();
            node_stmt->var = stmt_exit;
            return node_stmt;
        }
        // Parse 'let' statement
        else if (peek().has_value() && peek().value().type == TokenType::let &&
                 peek(1).has_value() && peek(1).value().type == TokenType::ident &&
                 peek(2).has_value() && peek(2).value().type == TokenType::eq)
        {
            consume(); // Consume 'let' token
            auto stmt_let = m_allocator.alloc<NodeStmtLet>();
            stmt_let->ident = consume(); // Store the identifier (variable name) of the 'let' token
            consume();                   // Consume '=' token

            // Parse expression for 'let' statement
            if (auto expr = parse_expr())
            {
                stmt_let->expr = expr.value();
            }
            else
            {
                std::cerr << "Invalid expression" << std::endl;
                exit(EXIT_FAILURE);
            }

            // Check ';' and consume
            try_consume(TokenType::semi, "Expected `;`");

            auto node_stmt = m_allocator.alloc<NodeStmt>();
            node_stmt->var = stmt_let;
            return node_stmt;
        }
        else
        {
            return {};
        }
    }

    /**
     * @brief Parses the list of tokens into a program parse tree.
     *
     * @return An optional NodeProg if the parsing is successful.
     */
    std::optional<NodeProg> parse_prog()
    {
        NodeProg prog;
        while (peek().has_value())
        {
            if (auto stmt = parse_stmt())
            {
                prog.stmts.push_back(stmt.value());
            }
            else
            {
                std::cerr << "Invalid statement." << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        return prog;
    }

private:
    /**
     * @brief Peeks at the current position in the list of tokens.
     *
     * @param offset The offset from the current position to peek at.
     * @return The token at the current position plus the offset, if valid; otherwise, an empty optional.
     */
    std::optional<Token> peek(const int offset = 0) const
    {
        if (m_index + offset >= m_tokens.size())
        {
            return {};
        }
        return m_tokens.at(m_index + offset);
    }

    /**
     * @brief Consumes the current token in the list of tokens and increments the index.
     *
     * @return The consumed token.
     */
    inline Token consume()
    {
        return m_tokens[m_index++];
    }

    /**
     * @brief Tries to consume a token of a specific type, with an error message if it fails.
     *
     * @param type The expected token type.
     * @param err_msg The error message to display if the token type does not match.
     * @return The consumed token.
     */
    inline Token try_consume(TokenType type, const std::string &err_msg)
    {
        if (peek().has_value() && peek().value().type == type)
        {
            return consume();
        }
        else
        {
            std::cerr << err_msg << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    /**
     * @brief Tries to consume a token of a specific type.
     *
     * @param type The expected token type.
     * @return An optional token if the token type matches.
     */
    inline std::optional<Token> try_consume(TokenType type)
    {
        if (peek().has_value() && peek().value().type == type)
        {
            return consume();
        }
        else
        {
            return {};
        }
    }

    const std::vector<Token> m_tokens;
    size_t m_index = 0;
    ArenaAllocator m_allocator;
};