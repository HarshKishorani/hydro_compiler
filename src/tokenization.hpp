#pragma once

#include <iostream>
#include <optional>
#include <vector>

enum class TokenType
{
    exit,
    int_lit,
    semi
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

        while (peak().has_value())
        {
            // Alphabetical characters
            if (std::isalpha(peak().value()))
            {
                buff.push_back(consume());
                while (peak().has_value() && std::isalnum(peak().value()))
                {
                    buff.push_back(consume());
                }
                if (buff == "exit")
                {
                    tokens.push_back({.type = TokenType::exit});
                    buff.clear();
                    continue;
                }
                else
                {
                    std::cerr << "You messed \n";
                    exit(EXIT_FAILURE);
                }
            }
            // Digits
            else if (std::isdigit(peak().value()))
            {
                buff.push_back(consume());
                while (peak().has_value() && std::isdigit(peak().value()))
                {
                    buff.push_back(consume());
                }

                tokens.push_back({.type = TokenType::int_lit, .value = buff});
                buff.clear();
            }
            // Semi colons
            else if (peak().value() == ';')
            {
                consume();
                tokens.push_back({.type = TokenType::semi});
                continue;
            }
            // Blank Spaces
            else if (std::isspace(peak().value()))
            {
                consume();
                continue;
            }
            else
            {
                std::cerr << "Invalid Token : " << peak().value() << " \n";
                exit(EXIT_FAILURE);
            }
        }
        m_index = 0;
        return tokens;
    }

private:
    /// Peak at current m_index in source code.
    inline std::optional<char> peak(int ahead = 1) const
    {
        if (m_index + ahead > m_src.length())
        {
            return {};
        }
        else
        {
            return m_src[m_index];
        }
    }

    /// @brief Peak at current m_index in source code and increment the index.
    /// @return
    inline char consume() { return m_src[m_index++]; }

    const std::string m_src;
    size_t m_index = 0;
};