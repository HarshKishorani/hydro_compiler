#pragma once

#include <iostream>
#include <optional>
#include <vector>
#include <cassert>

/// @brief Enumeration of token types that the tokenizer can recognize.
enum class TokenType
{
    exit,        // Represents the 'exit' keyword.
    int_lit,     // Represents an integer literal.
    if_,         // Represents the 'if' keyword.
    else_,       // Represents the 'else' keyword.
    elif_,       // Represents the 'else if' keyword.
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

/// @brief Token types to string,
/// @param type
/// @return
std::string to_string(const TokenType type)
{
    switch (type)
    {
    case TokenType::exit:
        return "`exit`";
    case TokenType::int_lit:
        return "int literal";
    case TokenType::semi:
        return "`;`";
    case TokenType::open_paren:
        return "`(`";
    case TokenType::close_paren:
        return "`)`";
    case TokenType::ident:
        return "identifier";
    case TokenType::let:
        return "`let`";
    case TokenType::eq:
        return "`=`";
    case TokenType::plus:
        return "`+`";
    case TokenType::star:
        return "`*`";
    case TokenType::minus:
        return "`-`";
    case TokenType::fslash:
        return "`/`";
    case TokenType::open_curly:
        return "`{`";
    case TokenType::close_curly:
        return "`}`";
    case TokenType::if_:
        return "`if`";
    case TokenType::elif_:
        return "`elif`";
    case TokenType::else_:
        return "`else`";
    }
    assert(false);
}

/// @brief Checks weather the given TokenType is a Binary Operator or not.
/// @param type The expected token type.
/// @return Precedence of the token.
std::optional<int> checkAndGetBinaryPrecedence(const TokenType type)
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
    size_t line;                      // Line number of the tolen.
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
    explicit Tokenizer(std::string src)
        : m_src(std::move(src))
    {
    }

    /**
     * @brief Tokenizes the source code into a list of tokens.
     *
     * @return A vector of tokens parsed from the source code.
     */
    std::vector<Token> tokenize()
    {
        std::vector<Token> tokens;
        std::string buff;
        size_t line_count = 1;
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
                    tokens.push_back({.type = TokenType::exit, .line = line_count});
                    buff.clear();
                }
                else if (buff == "let")
                {
                    tokens.push_back({.type = TokenType::let, .line = line_count});
                    buff.clear();
                }
                else if (buff == "if")
                {
                    tokens.push_back({.type = TokenType::if_, .line = line_count});
                    buff.clear();
                }
                else if (buff == "else")
                {
                    tokens.push_back({.type = TokenType::else_, .line = line_count});
                    buff.clear();
                }
                else if (buff == "elif")
                {
                    tokens.push_back({.type = TokenType::elif_, .line = line_count});
                    buff.clear();
                }
                else
                {
                    tokens.push_back({
                        .type = TokenType::ident,
                        .line = line_count,
                        .value = buff,
                    });
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
                tokens.push_back({.type = TokenType::int_lit, .line = line_count, .value = buff});
                buff.clear();
            }
            // Check comments
            else if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '/')
            {
                consume();
                consume();
                while (peek().has_value() && peek().value() != '\n')
                {
                    consume();
                }
                line_count++;
            }
            else if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '*')
            {
                consume();
                consume();
                while (peek().has_value())
                {
                    if (peek().value() == '*' && peek(1).has_value() && peek(1).value() == '/')
                    {
                        break;
                    }
                    if (peek().value() == '\n')
                    {
                        line_count++;
                    }
                    consume();
                }
                if (peek().has_value())
                    consume();
                if (peek().has_value())
                    consume();
            }
            // Check for specific symbols and operators
            else if (peek().value() == '(')
            {
                consume();
                tokens.push_back({.type = TokenType::open_paren, .line = line_count});
            }
            else if (peek().value() == ')')
            {
                consume();
                tokens.push_back({.type = TokenType::close_paren, .line = line_count});
            }
            else if (peek().value() == ';')
            {
                consume();
                tokens.push_back({.type = TokenType::semi, .line = line_count});
            }
            else if (peek().value() == '=')
            {
                consume();
                tokens.push_back({.type = TokenType::eq, .line = line_count});
            }
            else if (peek().value() == '+')
            {
                consume();
                tokens.push_back({.type = TokenType::plus, .line = line_count});
            }
            else if (peek().value() == '*')
            {
                consume();
                tokens.push_back({.type = TokenType::star, .line = line_count});
            }
            else if (peek().value() == '-')
            {
                consume();
                tokens.push_back({.type = TokenType::minus, .line = line_count});
            }
            else if (peek().value() == '/')
            {
                consume();
                tokens.push_back({.type = TokenType::fslash, .line = line_count});
            }
            else if (peek().value() == '{')
            {
                consume();
                tokens.push_back({.type = TokenType::open_curly, .line = line_count});
            }
            else if (peek().value() == '}')
            {
                consume();
                tokens.push_back({.type = TokenType::close_curly, .line = line_count});
            }
            // Line Count
            else if (peek().value() == '\n')
            {
                consume();
                line_count++;
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
    char consume() { return m_src[m_index++]; }

    const std::string m_src; // The source code to tokenize.
    size_t m_index = 0;      // The current index in the source code.
};