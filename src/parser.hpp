#pragma once
#include "tokenization.hpp"
#include <variant>

// Expressions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// @brief int_lit Expression
struct NodeExprIntLit
{
    Token int_lit;
};

/// @brief Identifier Expression
struct NodeExprIdent
{
    Token ident;
};

/// @brief Node Expression of parse tree.
struct NodeExpr
{
    std::variant<NodeExprIntLit, NodeExprIdent> var;
};

// Statements ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// @brief 'exit' Statement
struct NodeStmtExit
{
    NodeExpr expr;
};

/// @brief 'let' Statement
struct NodeStmtLet
{
    Token ident;
    NodeExpr expr;
};

/// @brief Node Statement for parse tree.
struct NodeStmt
{
    std::variant<NodeStmtExit, NodeStmtLet> var;
};

// Program Parse Tree ~~~~~~~~~~~~~~~~~~~~~~~~~~

struct NodeProg
{
    std::vector<NodeStmt> stmts;
};

class Parser
{
public:
    inline explicit Parser(std::vector<Token> tokens) : m_tokens(std::move(tokens))
    {
    }

    /// @brief  Parse the token into Node expression.
    /// @return
    std::optional<NodeExpr> parse_expr()
    {
        if (peek().has_value() && peek().value().type == TokenType::int_lit)
        {
            return NodeExpr{.var = NodeExprIntLit{.int_lit = consume()}};
        }
        else if (peek().has_value() && peek().value().type == TokenType::ident)
        {
            return NodeExpr{.var = NodeExprIdent{.ident = consume()}};
        }
        else
        {
            return {};
        }
    }

    /// @brief Parse the token into Node Statement.
    /// @return 
    std::optional<NodeStmt> parse_stmt()
    {
        // Parse 'exit' statement
        if (peek().value().type == TokenType::exit && peek(1).has_value() && peek(1).value().type == TokenType::open_paren)
        {
            consume(); // Consume 'exit' token
            consume(); // Consume '(' token
            NodeStmtExit stmt_exit;

            // Parse Expression for 'exit' statement.
            if (auto node_expr = parse_expr())
            {
                stmt_exit = {.expr = node_expr.value()};
            }
            else
            {
                std::cerr << "Invalid expression" << std::endl;
                exit(EXIT_FAILURE);
            }

            // Check ')' and consume.
            if (peek().has_value() && peek().value().type == TokenType::close_paren)
            {
                consume();
            }
            else
            {
                std::cerr << "Expected `)`" << std::endl;
                exit(EXIT_FAILURE);
            }

            // Check ';' and consume.
            if (peek().has_value() && peek().value().type == TokenType::semi)
            {
                consume();
            }
            else
            {
                std::cerr << "Expected `;`" << std::endl;
                exit(EXIT_FAILURE);
            }
            return NodeStmt{.var = stmt_exit};
        }
        // Parse 'let' statement
        else if (
            peek().has_value() && peek().value().type == TokenType::let && peek(1).has_value() && peek(1).value().type == TokenType::ident && peek(2).has_value() && peek(2).value().type == TokenType::eq)
        {
            consume(); // Consume 'let' token.
            auto stmt_let = NodeStmtLet{.ident = consume()}; // Store the identifier (variable name) of the 'let' token.
            consume(); // Consume '=' token.

            // Parse Expression for 'let' statement.
            if (auto expr = parse_expr())
            {
                stmt_let.expr = expr.value();
            }
            else
            {
                std::cerr << "Invalid expression" << std::endl;
                exit(EXIT_FAILURE);
            }

            // Check ';' and consume.
            if (peek().has_value() && peek().value().type == TokenType::semi)
            {
                consume();
            }
            else
            {
                std::cerr << "Expected `;`" << std::endl;
                exit(EXIT_FAILURE);
            }
            return NodeStmt{.var = stmt_let};
        }
        else
        {
            return {};
        }
    }

    /// @brief Parse the given list of tokens and create a parse tree (Node Program) of Node Statements.
    /// @return
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
    /// @brief  Peek at current m_index in list of tokens.
    /// @param ahead
    /// @return
    std::optional<Token> peek(const int offset = 0) const
    {
        if (m_index + offset >= m_tokens.size())
        {
            return {};
        }
        return m_tokens.at(m_index + offset);
    }

    /// @brief Peek at current m_index in list of tokens and increment the m_index.
    /// @return
    inline Token consume()
    {
        return m_tokens[m_index++];
    }

    const std::vector<Token> m_tokens;
    size_t m_index = 0;
};