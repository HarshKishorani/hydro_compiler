#pragma once

#include <iostream>
#include <optional>
#include <vector>

enum class TokenType
{
    exit,
    int_lit,
    semi,
    open_paren,
    close_paren,
    ident,
    let,
    eq
};

struct Token
{
    TokenType type;
    std::optional<std::string> value;
};

class Tokenizer
{
public:
    inline explicit Tokenizer(std::string src)
        : m_src(std::move(src))
    {
    }

    /// @brief Convert the given souce code into the list of Tokens.
    /// @return
    inline std::vector<Token> tokenize()
    {
        std::vector<Token> tokens;
        std::string buff;
        while (peek().has_value())
        {
            // is Alpha
            if (std::isalpha(peek().value()))
            {
                buff.push_back(consume());
                while (peek().has_value() && std::isalnum(peek().value()))
                {
                    buff.push_back(consume());
                }
                if (buff == "exit")
                {
                    tokens.push_back({.type = TokenType::exit});
                    buff.clear();
                    continue;
                }
                else if (buff == "let")
                {
                    tokens.push_back({.type = TokenType::let});
                    buff.clear();
                    continue;
                }
                else
                {
                    tokens.push_back({.type = TokenType::ident, .value = buff});
                    buff.clear();
                    continue;
                }
            }
            // is digit
            else if (std::isdigit(peek().value()))
            {
                buff.push_back(consume());
                while (peek().has_value() && std::isdigit(peek().value()))
                {
                    buff.push_back(consume());
                }
                tokens.push_back({.type = TokenType::int_lit, .value = buff});
                buff.clear();
                continue;
            }
            // Parenthesis
            else if (peek().value() == '(')
            {
                consume();
                tokens.push_back({.type = TokenType::open_paren});
                continue;
            }
            else if (peek().value() == ')')
            {
                consume();
                tokens.push_back({.type = TokenType::close_paren});
                continue;
            }
            // Semi Colon
            else if (peek().value() == ';')
            {
                consume();
                tokens.push_back({.type = TokenType::semi});
                continue;
            }
            // Equals
            else if (peek().value() == '=')
            {
                consume();
                tokens.push_back({.type = TokenType::eq});
                continue;
            }
            // Blank Space
            else if (std::isspace(peek().value()))
            {
                consume();
                continue;
            }
            // Invalid Token
            else
            {
                std::cerr << "You messed up!" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        m_index = 0;
        return tokens;
    }

private:
    /// Peek at current m_index in source code.
    std::optional<char> peek(const size_t offset = 0) const
    {
        if (m_index + offset >= m_src.length())
        {
            return {};
        }
        return m_src.at(m_index + offset);
    }

    /// @brief Peek at current m_index in source code and increment the index.
    /// @return
    inline char consume() { return m_src[m_index++]; }

    const std::string m_src;
    size_t m_index = 0;
};