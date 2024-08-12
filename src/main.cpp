#include "generation.hpp"
#include <fstream>

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

    // Tokenizing the contents
    Tokenizer tokenizer(std::move(contents));
    std::vector<Token> tokens = tokenizer.tokenize();

    // Creating parse tree
    Parser parser(std::move(tokens));
    std::optional<NodeProg> prog = parser.parse_prog();

    // Generating asm code
    if (!prog.has_value())
    {
        std::cerr << "No Exit Statement Found." << std::endl;
        return EXIT_FAILURE;
    }
    
    // Creating asm file
    {
        Generator codeGenerator(prog.value());
        std::fstream file("out.asm", std::ios::out);
        file << codeGenerator.generate_program();
    }

    // System call to NASM to assemble assemby code and ld command for GNU linker
    // to link libs.
    system("nasm -felf64 out.asm");
    system("ld -o out out.o");

    return EXIT_SUCCESS;
}