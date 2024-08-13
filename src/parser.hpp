#pragma once
#include "tokenization.hpp"
#include "arena.hpp"
#include <variant>
#include <vector>
#include <cassert>

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
    NodeExpr *lhs; // Left-hand side of the multiplication expression.
    NodeExpr *rhs; // Right-hand side of the multiplication expression.
};

/// @brief Represents a subtraction binary expression in the parse tree.
struct NodeBinExprSub
{
    NodeExpr *lhs; // Left-hand side of the subtraction expression.
    NodeExpr *rhs; // Right-hand side of the subtraction expression.
};

/// @brief Represents a division binary expression in the parse tree.
struct NodeBinExprDiv
{
    NodeExpr *lhs; // Left-hand side of the division expression.
    NodeExpr *rhs; // Right-hand side of the division expression.
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
    Token ident;      // The identifier token.
    NodeExpr *expr{}; // The expression associated with the 'let' statement.
};

struct NodeStmt; // Forward declaration of NodeStmt

struct NodeIfPred; // Forward declaration of NodeIfPred

/// @brief Represents a 'scope' in the parse tree. Scope contains a list of statements inside.
struct NodeScope
{
    std::vector<NodeStmt *> stmts; // List of statements in the scope.
};

/// @brief Represents an 'if' statement in the parse tree.
struct NodeStmtIf
{
    NodeExpr *expr;   // Condition expression of the 'if' statement.
    NodeScope *scope; // Scope of statements executed if the condition is true.
    std::optional<NodeIfPred *> pred;
};

/// @brief Represents a Else If Predicate in the parse tree.
struct NodeIfPredElif
{
    NodeExpr *expr{};
    NodeScope *scope{};
    std::optional<NodeIfPred *> pred;
};

/// @brief Represents a Else Predicate in the parse tree.
struct NodeIfPredElse
{
    NodeScope *scope;
};

/// @brief Represents a If Predicate in the parse tree.
struct NodeIfPred
{
    std::variant<NodeIfPredElif *, NodeIfPredElse *> var;
};

struct NodeStmtAssign
{
    Token ident;      // The identifier token.
    NodeExpr *expr{}; // The expression associated with the reassignment.
};

/// @brief Represents a statement in the parse tree.
struct NodeStmt
{
    std::variant<NodeStmtExit *, NodeStmtLet *, NodeScope *, NodeStmtIf *, NodeStmtAssign *> var; // Variant holding the statement type.
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
    explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)), m_allocator(1024 * 1024 * 4) // 4 mb
    {
    }

    void error_expected(const std::string &msg) const
    {
        std::cerr << "[Parse Error] Expected " << msg << " on line " << peek(-1).value().line << std::endl;
        exit(EXIT_FAILURE);
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
            auto term_int_lit = m_allocator.emplace<NodeTermIntLit>(int_lit.value());
            auto term = m_allocator.emplace<NodeTerm>(term_int_lit);
            return term;
        }
        if (auto ident = try_consume(TokenType::ident))
        {
            auto expr_ident = m_allocator.emplace<NodeTermIdent>(ident.value());
            auto term = m_allocator.emplace<NodeTerm>(expr_ident);
            return term;
        }
        if (const auto open_paren = try_consume(TokenType::open_paren))
        {
            auto expr = parse_expr();
            if (!expr.has_value())
            {
                error_expected("expression");
            }
            try_consume_err(TokenType::close_paren);
            auto term_paren = m_allocator.emplace<NodeTermParen>(expr.value());
            auto term = m_allocator.emplace<NodeTerm>(term_paren);
            return term;
        }
        return {};
    }

    /**
     * @brief Parses an expression from the tokens.
     * @param min_prec Minimum precedence to check for operator precedence climbing for Binary Expressions.
     * @return An optional NodeExpr pointer if an expression is parsed successfully.
     */
    std::optional<NodeExpr *> parse_expr(const int min_prec = 0)
    {
        std::optional<NodeTerm *> term = parse_term();
        if (!term.has_value())
        {
            return {};
        }
        auto expr = m_allocator.emplace<NodeExpr>(term.value());

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
            const auto [type, line, value] = consume();
            const int next_min_prec = prec.value() + 1;
            auto expr_rhs = parse_expr(next_min_prec);
            if (!expr_rhs.has_value())
            {
                error_expected("RHS of Binary Expression");
            }
            auto bin_expr = m_allocator.emplace<NodeBinExpr>();
            auto expr_lhs = m_allocator.emplace<NodeExpr>();
            if (type == TokenType::plus)
            {
                expr_lhs->var = expr->var;
                auto add = m_allocator.emplace<NodeBinExprAdd>(expr_lhs, expr_rhs.value());
                bin_expr->var = add;
            }
            else if (type == TokenType::star)
            {
                expr_lhs->var = expr->var;
                auto multi = m_allocator.emplace<NodeBinExprMulti>(expr_lhs, expr_rhs.value());
                bin_expr->var = multi;
            }
            else if (type == TokenType::minus)
            {
                expr_lhs->var = expr->var;
                auto sub = m_allocator.emplace<NodeBinExprSub>(expr_lhs, expr_rhs.value());
                bin_expr->var = sub;
            }
            else if (type == TokenType::fslash)
            {
                expr_lhs->var = expr->var;
                auto div = m_allocator.emplace<NodeBinExprDiv>(expr_lhs, expr_rhs.value());
                bin_expr->var = div;
            }
            else
            {
                assert(false); // Unreachable;
            }
            expr->var = bin_expr;
        }
        return expr;
    }

    /**
     * @brief Parses a 'scope'.
     * @return An optional NodeScope pointer if a scope is parsed successfully.
     */
    std::optional<NodeScope *> parse_scope()
    {
        if (!try_consume(TokenType::open_curly).has_value())
        {
            return {};
        }
        auto scope = m_allocator.emplace<NodeScope>();
        while (auto stmt = parse_stmt())
        {
            scope->stmts.push_back(stmt.value());
        }
        try_consume_err(TokenType::close_curly);
        return scope;
    }

    /// @brief Parses additional predicates after the if statements.
    /// @return
    std::optional<NodeIfPred *> parse_if_pred()
    {
        if (try_consume(TokenType::elif_).has_value())
        {
            try_consume_err(TokenType::open_paren);
            auto elif_pred = m_allocator.alloc<NodeIfPredElif>();
            if (const auto expr = parse_expr())
            {
                elif_pred->expr = expr.value();
            }
            else
            {
                std::cerr << "Exprected an expression.";
                exit(EXIT_FAILURE);
            }
            try_consume_err(TokenType::close_paren);
            if (const auto scope = parse_scope())
            {
                elif_pred->scope = scope.value();
            }
            else
            {
                std::cerr << "Exprected a scope.";
                exit(EXIT_FAILURE);
            }
            elif_pred->pred = parse_if_pred();

            auto if_pred = m_allocator.emplace<NodeIfPred>(elif_pred);
            return if_pred;
        }
        if (try_consume(TokenType::else_).has_value())
        {
            auto else_pred = m_allocator.alloc<NodeIfPredElse>();

            if (const auto scope = parse_scope())
            {
                else_pred->scope = scope.value();
            }
            else
            {
                std::cerr << "Exprected a scope.";
                exit(EXIT_FAILURE);
            }
            auto if_pred = m_allocator.emplace<NodeIfPred>(else_pred);
            return if_pred;
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
            consume();
            consume();

            auto stmt_exit = m_allocator.emplace<NodeStmtExit>();

            if (const auto node_expr = parse_expr())
            {
                stmt_exit->expr = node_expr.value();
            }
            else
            {
                std::cerr << "Invalid expression" << std::endl;
                exit(EXIT_FAILURE);
            }

            try_consume_err(TokenType::close_paren);
            try_consume_err(TokenType::semi);

            auto stmt = m_allocator.emplace<NodeStmt>();
            stmt->var = stmt_exit;
            return stmt;
        }
        // Parse 'let' statement
        if (peek().has_value() && peek().value().type == TokenType::let &&
            peek(1).has_value() && peek(1).value().type == TokenType::ident &&
            peek(2).has_value() && peek(2).value().type == TokenType::eq)
        {
            consume();
            auto stmt_let = m_allocator.emplace<NodeStmtLet>();
            stmt_let->ident = consume();
            consume();
            if (const auto expr = parse_expr())
            {
                stmt_let->expr = expr.value();
            }
            else
            {
                std::cerr << "Invalid expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume_err(TokenType::semi);
            auto stmt = m_allocator.emplace<NodeStmt>();
            stmt->var = stmt_let;
            return stmt;
        }
        // Parse variable reassignment.
        if (peek().has_value() && peek().value().type == TokenType::ident && peek(1).has_value() && peek(1).value().type == TokenType::eq)
        {
            const auto assign = m_allocator.alloc<NodeStmtAssign>();
            assign->ident = consume();
            consume();
            if (const auto expr = parse_expr())
            {
                assign->expr = expr.value();
            }
            else
            {
                std::cerr << "Expected expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume_err(TokenType::semi);
            auto stmt = m_allocator.emplace<NodeStmt>(assign);
            return stmt;
        }
        // Parse Scopes.
        if (peek().has_value() && peek().value().type == TokenType::open_curly)
        {
            if (const auto scope = parse_scope())
            {
                auto stmt = m_allocator.emplace<NodeStmt>(scope.value());
                return stmt;
            }
            std::cerr << "Invalid scope" << std::endl;
            exit(EXIT_FAILURE);
        }
        // Parse 'if' statement
        if (auto if_ = try_consume(TokenType::if_))
        {
            try_consume_err(TokenType::open_paren);
            auto stmt_if = m_allocator.emplace<NodeStmtIf>();
            if (const auto expr = parse_expr())
            {
                stmt_if->expr = expr.value();
            }
            else
            {
                std::cerr << "Invalid expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume_err(TokenType::close_paren);
            if (const auto scope = parse_scope())
            {
                stmt_if->scope = scope.value();
            }
            else
            {
                std::cerr << "Invalid scope" << std::endl;
                exit(EXIT_FAILURE);
            }
            stmt_if->pred = parse_if_pred();
            auto stmt = m_allocator.emplace<NodeStmt>(stmt_if);
            return stmt;
        }
        return {};
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
                std::cerr << "Invalid statement" << std::endl;
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
    Token consume()
    {
        return m_tokens.at(m_index++);
    }

    /**
     * @brief Tries to consume a token of a specific type, with an error message if it fails.
     *
     * @param type The expected token type.
     * @return The consumed token.
     */
    Token try_consume_err(const TokenType type)
    {
        if (peek().has_value() && peek().value().type == type)
        {
            return consume();
        }
        error_expected(to_string(type));
        return {};
    }

    /**
     * @brief Tries to consume a token of a specific type.
     *
     * @param type The expected token type.
     * @return An optional token if the token type matches.
     */
    std::optional<Token> try_consume(const TokenType type)
    {
        if (peek().has_value() && peek().value().type == type)
        {
            return consume();
        }
        return {};
    }

    const std::vector<Token> m_tokens;
    size_t m_index = 0;
    ArenaAllocator m_allocator;
};