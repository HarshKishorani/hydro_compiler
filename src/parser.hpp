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

/// @brief Represents a parenthesis term in the parse tree with an expression inside it.
struct NodeTermParen
{
    NodeExpr *expr; // An expression inside the parenthesis.
};

/// @brief Represents an addition binary expression in the parse tree.
struct NodeBinExprAdd
{
    NodeExpr *lhs; // Left-hand side of the addition expression.
    NodeExpr *rhs; // Right-hand side of the addition expression.
};

/// @brief Represents a multiplication binary expression in the parse tree.
struct NodeBinExprMulti
{
    NodeExpr *lhs; // Left-hand side of the addition expression.
    NodeExpr *rhs; // Right-hand side of the addition expression.
};

/// @brief Represents a Subtraction binary expression in the parse tree.
struct NodeBinExprSub
{
    NodeExpr *lhs; // Left-hand side of the addition expression.
    NodeExpr *rhs; // Right-hand side of the addition expression.
};

/// @brief Represents a Disivion binary expression in the parse tree.
struct NodeBinExprDiv
{
    NodeExpr *lhs; // Left-hand side of the addition expression.
    NodeExpr *rhs; // Right-hand side of the addition expression.
};

/// @brief Represents a binary expression in the parse tree.
struct NodeBinExpr
{
    std::variant<NodeBinExprAdd *, NodeBinExprMulti *, NodeBinExprSub *, NodeBinExprDiv *> var; // Variant holding the Binary Expression type.
};

/// @brief Represents a term in the parse tree.
struct NodeTerm
{
    std::variant<NodeTermIntLit *, NodeTermIdent *, NodeTermParen *> var; // Variant holding the term type.
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

struct NodeStmt; // Forward declaration of NodeStmt

/// @brief Represents a 'scope" in the parse tree. Scope contains list of statements inside.
struct NodeScope
{
    std::vector<NodeStmt *> stmts;
};

/// @brief Represents an 'if' statement in the parse tree.
struct NodeStmtIf
{
    NodeExpr *expr;
    NodeScope *scope;
};

/// @brief Represents a statement in the parse tree.
struct NodeStmt
{
    std::variant<NodeStmtExit *, NodeStmtLet *, NodeScope *, NodeStmtIf *> var; // Variant holding the statement type.
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
        else if (auto open_paren = try_consume(TokenType::open_paren))
        {
            auto expr = parse_expr();
            if (!expr.has_value())
            {
                std::cerr << "Expected Expression inside parenthesis.";
                exit(EXIT_FAILURE);
            }

            try_consume(TokenType::close_paren, "Invalid Token. Expected ')'");

            auto term_paren = m_allocator.alloc<NodeTermParen>();
            term_paren->expr = expr.value();

            auto term = m_allocator.alloc<NodeTerm>();
            term->var = term_paren;

            return term;
        }
        return {};
    }

    /**
     * @brief Parses an expression from the tokens.
     * @param min_prec Minimum precedence to check for operator precedence climbing for Binary Expressions.
     * @return An optional NodeExpr pointer if an expression is parsed successfully.
     */
    std::optional<NodeExpr *> parse_expr(int min_prec = 0)
    {
        std::optional<NodeTerm *> term = parse_term();
        if (!term.has_value())
        {
            return {};
        }
        NodeExpr *expr = m_allocator.alloc<NodeExpr>();
        expr->var = term.value();

        // Operator Precedence Climbing for Binary operations.
        //+ https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing
        while (true)
        {
            std::optional<Token> curr_tok = peek();
            std::optional<int> prec;

            // Break if not a Binary Expression.
            if (!curr_tok.has_value())
                break;
            prec = checkAndGetBinaryPrecedence(curr_tok.value().type);
            if (!prec.has_value() || prec < min_prec)
            {
                break;
            }

            // Get the operator's precedence and associativity, and compute a
            // minimal precedence for the recursive call
            Token op = consume();
            int next_min_prec = prec.value() + 1;

            std::optional<NodeExpr *> rhs_expr = parse_expr(next_min_prec); // Recursive call
            if (!rhs_expr.has_value())
            {
                std::cerr << "Unable to parse expression." << std::endl;
                exit(EXIT_FAILURE);
            }
            auto lhs_expr = m_allocator.alloc<NodeExpr>();
            auto bin_expr = m_allocator.alloc<NodeBinExpr>();

            if (op.type == TokenType::plus)
            {
                auto add = m_allocator.alloc<NodeBinExprAdd>();

                lhs_expr->var = expr->var;

                add->lhs = lhs_expr;
                add->rhs = rhs_expr.value();

                bin_expr->var = add;
            }
            else if (op.type == TokenType::star)
            {
                auto mult = m_allocator.alloc<NodeBinExprMulti>();

                lhs_expr->var = expr->var;

                mult->lhs = lhs_expr;
                mult->rhs = rhs_expr.value();

                bin_expr->var = mult;
            }
            else if (op.type == TokenType::minus)
            {
                auto minus = m_allocator.alloc<NodeBinExprSub>();

                lhs_expr->var = expr->var;

                minus->lhs = lhs_expr;
                minus->rhs = rhs_expr.value();

                bin_expr->var = minus;
            }
            else if (op.type == TokenType::fslash)
            {
                auto fslash = m_allocator.alloc<NodeBinExprDiv>();

                lhs_expr->var = expr->var;

                fslash->lhs = lhs_expr;
                fslash->rhs = rhs_expr.value();

                bin_expr->var = fslash;
            }
            expr->var = bin_expr;
        }
        return expr;
    }

    /// @brief Parses a 'scope'.
    /// @return
    std::optional<NodeScope *> parse_scope()
    {
        if (try_consume(TokenType::open_curly).has_value())
        {
            auto scope = m_allocator.alloc<NodeScope>();
            while (auto stmt = parse_stmt())
            {
                scope->stmts.push_back(stmt.value());
            }

            try_consume(TokenType::close_curly, "Expected a closed curly parenthesis. '}'");

            return scope;
        }
        return {};
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
        // Parse Scopes.
        else if (peek().has_value() && peek().value().type == TokenType::open_curly)
        {
            if (auto scope = parse_scope())
            {
                auto node_stmt = m_allocator.alloc<NodeStmt>();
                node_stmt->var = scope.value();
                return node_stmt;
            }
            else
            {
                std::cerr << "Expected a scope. \n";
                exit(EXIT_FAILURE);
            }
        }
        // Parse 'if' statement
        else if (auto if_ = try_consume(TokenType::if_))
        {
            try_consume(TokenType::open_paren, "Expected a '('");
            auto stmt_if = m_allocator.alloc<NodeStmtIf>();

            // Parse expression for 'if' statement
            if (auto node_expr = parse_expr())
            {
                stmt_if->expr = node_expr.value();
            }
            else
            {
                std::cerr << "Invalid expression" << std::endl;
                exit(EXIT_FAILURE);
            }

            try_consume(TokenType::close_paren, "Expected `)`");

            if (auto scope = parse_scope())
            {
                stmt_if->scope = scope.value();

                auto node_stmt = m_allocator.alloc<NodeStmt>();
                node_stmt->var = stmt_if;
                return node_stmt;
            }
            else
            {
                std::cerr << "Expected a scope. \n";
                exit(EXIT_FAILURE);
            }
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