#pragma once

#include <iostream>
#include <optional>
#include <vector>

/// @brief Enumeration of token types that the tokenizer can recognize.
enum class TokenType
{
    exit,        // Represents the 'exit' keyword.
    int_lit,     // Represents an integer literal.
    if_,         // Represents the 'if' keyword.
    semi,        // Represents a semicolon.
    open_paren,  // Represents an opening parenthesis.
    close_paren, // Represents a closing parenthesis.
    ident,       // Represents an identifier.
    let,         // Represents the 'let' keyword.
    eq,          // Represents the equals sign.
    plus,        // Represents the plus sign.
    star,        // Represents the star sign.
    minus,       // Represents the subtraction sign.
    fslash,      // Represents the forward slash sign.
    open_curly,  // Represents open curly braces.
    close_curly  // Represents closed curly braces.
};

/// @brief Checks weather the given TokenType is a Binary Operator or not.
/// @param type The expected token type.
/// @return Precedence of the token.
std::optional<int> checkAndGetBinaryPrecedence(TokenType type)
{
    switch (type)
    {
    case TokenType::plus:
    case TokenType::minus:
        return 0;
    case TokenType::star:
    case TokenType::fslash:
        return 1;

    default:
        return {};
    }
}

/// @brief Structure to represent a token.
struct Token
{
    TokenType type;                   // The type of the token.
    std::optional<std::string> value; // The value of the token, if applicable.
};

/// @brief Class to convert source code into a list of tokens.
class Tokenizer
{
public:
    /**
     * @brief Constructs the tokenizer with a given source code string.
     *
     * @param src The source code to tokenize.
     */
    inline explicit Tokenizer(std::string src)
        : m_src(std::move(src))
    {
    }

    /**
     * @brief Tokenizes the source code into a list of tokens.
     *
     * @return A vector of tokens parsed from the source code.
     */
    inline std::vector<Token> tokenize()
    {
        std::vector<Token> tokens;
        std::string buff;
        while (peek().has_value())
        {
            // Check if the current character is alphabetic
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
                }
                else if (buff == "let")
                {
                    tokens.push_back({.type = TokenType::let});
                    buff.clear();
                }
                else if (buff == "if")
                {
                    tokens.push_back({.type = TokenType::if_});
                    buff.clear();
                }
                else
                {
                    tokens.push_back({.type = TokenType::ident, .value = buff});
                    buff.clear();
                }
            }
            // Check if the current character is a digit
            else if (std::isdigit(peek().value()))
            {
                buff.push_back(consume());
                while (peek().has_value() && std::isdigit(peek().value()))
                {
                    buff.push_back(consume());
                }
                tokens.push_back({.type = TokenType::int_lit, .value = buff});
                buff.clear();
            }
            // Check for specific symbols and operators
            else if (peek().value() == '(')
            {
                consume();
                tokens.push_back({.type = TokenType::open_paren});
            }
            else if (peek().value() == ')')
            {
                consume();
                tokens.push_back({.type = TokenType::close_paren});
            }
            else if (peek().value() == ';')
            {
                consume();
                tokens.push_back({.type = TokenType::semi});
            }
            else if (peek().value() == '=')
            {
                consume();
                tokens.push_back({.type = TokenType::eq});
            }
            else if (peek().value() == '+')
            {
                consume();
                tokens.push_back({.type = TokenType::plus});
            }
            else if (peek().value() == '*')
            {
                consume();
                tokens.push_back({.type = TokenType::star});
            }
            else if (peek().value() == '-')
            {
                consume();
                tokens.push_back({.type = TokenType::minus});
            }
            else if (peek().value() == '/')
            {
                consume();
                tokens.push_back({.type = TokenType::fslash});
            }
            else if (peek().value() == '{')
            {
                consume();
                tokens.push_back({.type = TokenType::open_curly});
            }
            else if (peek().value() == '}')
            {
                consume();
                tokens.push_back({.type = TokenType::close_curly});
            }
            // Skip whitespace characters
            else if (std::isspace(peek().value()))
            {
                consume();
            }
            // Handle invalid tokens
            else
            {
                std::cerr << "Invalid token encountered!" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        m_index = 0;
        return tokens;
    }

private:
    /**
     * @brief Peeks at the current position in the source code.
     *
     * @param offset The offset from the current position to peek at.
     * @return The character at the current position plus the offset, if valid; otherwise, an empty optional.
     */
    std::optional<char> peek(const size_t offset = 0) const
    {
        if (m_index + offset >= m_src.length())
        {
            return {};
        }
        return m_src.at(m_index + offset);
    }

    /**
     * @brief Consumes the current character in the source code and increments the index.
     *
     * @return The consumed character.
     */
    inline char consume() { return m_src[m_index++]; }

    const std::string m_src; // The source code to tokenize.
    size_t m_index = 0;      // The current index in the source code.
};