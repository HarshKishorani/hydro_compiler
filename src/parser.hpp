#pragma once
#include "tokenization.hpp"

struct NodeExpr
{
    Token int_lit;
};

struct NodeExit
{
    NodeExpr expr;
};

class Parser
{
public:
    inline explicit Parser(std::vector<Token> tokens) : m_tokens(std::move(tokens))
    {
    }

    /// @brief  Parse the expression provided to the 'exit' literal.
    /// @return
    std::optional<NodeExpr> parseExpr()
    {
        if (peak().has_value() && peak().value().type == TokenType::int_lit)
        {
            return NodeExpr{.int_lit = consume()};
        }
        else
        {
            return {};
        }
    }

    /// @brief Parse the list of tokens provided.
    /// @return
    std::optional<NodeExit> parse()
    {
        std::optional<NodeExit> exit_node;
        while (peak().has_value())
        {
            if (peak().value().type == TokenType::exit)
            {
                consume(); // Consume exit
                if (auto exprNode = parseExpr())
                {
                    // Return the tree of NodeExpr inside NodeExit.
                    exit_node = NodeExit{.expr = exprNode.value()};
                }
                else
                {
                    std::cerr << "Invalid Expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
                if (peak().has_value() && peak().value().type == TokenType::semi)
                    consume(); // Consume Semi colon
                else
                {
                    std::cerr << "Invalid Expression. Missing semi colon." << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
        }
        m_index = 0;
        return exit_node;
    }

private:
    /// @brief  Peak at current m_index in source code.
    /// @param ahead
    /// @return
    inline std::optional<Token> peak(int ahead = 1) const
    {
        if (m_index + ahead > m_tokens.size())
        {
            return {};
        }
        else
        {
            return m_tokens[m_index];
        }
    }

    /// @brief Peak at current m_index in tokens and increment the m_index.
    /// @return
    inline Token consume()
    {
        return m_tokens[m_index++];
    }

    const std::vector<Token> m_tokens;
    size_t m_index = 0;
};