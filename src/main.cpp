#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>

enum class TokenType
{
    _return,
    int_lit,
    semi
};

struct Token
{
    TokenType type;
    std::optional<std::string> value;
};

std::vector<Token> tokenize(const std::string &str)
{
    std::vector<Token> tokens;
    std::string buff;
    for (int i = 0; i < str.length(); i++)
    {
        char c = str[i];
        if (std::isalpha(c))
        {
            buff.push_back(c);
            i++;
            while (std::isalnum(str[i]))
            {
                buff.push_back(str[i]);
                i++;
            }
            i--;

            if (buff == "return")
            {
                tokens.push_back({.type = TokenType::_return});
                buff.clear();
                continue;
            }
            else
            {
                std::cerr << "You messed \n";
                exit(EXIT_FAILURE);
            }
        }
        else if (std::isdigit(c))
        {
            buff.push_back(c);
            i++;
            while (std::isdigit(str[i]))
            {
                buff.push_back(str[i]);
                i++;
            }
            i--;

            tokens.push_back({.type = TokenType::int_lit, .value = buff});
            buff.clear();
        }
        else if (c == ';')
        {
            tokens.push_back({.type = TokenType::semi});
        }
        else if (std::isspace(c))
        {
            continue;
        }
        else
        {
            std::cerr << "Invalid Token : " << c << " \n";
            exit(EXIT_FAILURE);
        }
    }
    return tokens;
}

std::string tokensToASM(std::vector<Token> &tokens)
{
    std::stringstream output;
    output << "global _start\n_start:\n";
    for (int i = 0; i < tokens.size(); i++)
    {
        const Token &token = tokens[i];
        if (token.type == TokenType::_return)
        {
            if (i + 1 < tokens.size() && tokens[i + 1].type == TokenType::int_lit)
            {
                if (i + 2 < tokens.size() && tokens[i + 2].type == TokenType::semi)
                {
                    output << "    mov rax, 60\n";
                    output << "    mov rdi, " << tokens[i + 1].value.value() << "\n";
                    output << "    syscall\n";
                }
            }
        }
    }
    return output.str();
}

int main(int argc, char *argv[])
{
    // Arguments to get the hydrogen file
    if (argc != 2)
    {
        std::cerr << "Incorrect Usage" << std::endl;
        std::cerr << "Try using : hydro <input.hy>" << std::endl;
        return EXIT_FAILURE;
    }

    // Reading the hydrogen file
    std::string contents;
    {
        std::stringstream contents_stream;
        std::fstream input(argv[1], std::ios::in);
        contents_stream << input.rdbuf();
        contents = contents_stream.str();
    }
    std::vector<Token> tokens = tokenize(contents);

    // Creating asm file
    {
        std::fstream file("out.asm", std::ios::out);
        file << tokensToASM(tokens);
    }

    // System call to NASM to assemble assemby code and ld command for linked
    system("nasm -felf64 out.asm");
    system("ld -o out out.o");

    return EXIT_SUCCESS;
}