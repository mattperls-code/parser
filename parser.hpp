#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <optional>

class Token
{
    private:
        std::string toString(int indent) const;

    public:
        std::string id;

        enum TokenType {
            STRING_LITERAL,
            NESTING
        } type;

        std::variant<std::string, std::vector<Token>> content;

        int start;
        int width;
    
        Token(std::string id, std::string stringLiteral, int start, int width);
        Token(std::string id, std::vector<Token> nesting, int start, int width);

        std::string toString() const;
        std::string contentString() const;
};

typedef std::function<bool(const char)> Predicate;

Predicate is(const char c);
Predicate anyOf(const std::vector<Predicate> predicates);
Predicate negate(const Predicate predicate);

const extern Predicate isAlphabetical;
const extern Predicate isNumeric;

typedef std::optional<Token> ParserCombinatorResult;

typedef std::function<ParserCombinatorResult(const std::string&, const int, const int)> ParserCombinator;

ParserCombinator satisfy(const Predicate predicate);
ParserCombinator satisfy(const std::string tokenId, const Predicate predicate);

ParserCombinator repetition(const ParserCombinator nestedTokenGenerator);
ParserCombinator repetition(const ParserCombinator nestedTokenGenerator, const int minCount);
ParserCombinator repetition(const ParserCombinator nestedTokenGenerator, const int minCount, const int maxCount);
ParserCombinator repetition(const std::string tokenId, const ParserCombinator nestedTokenGenerator);
ParserCombinator repetition(const std::string tokenId, const ParserCombinator nestedTokenGenerator, const int minCount);
ParserCombinator repetition(const std::string tokenId, const ParserCombinator nestedTokenGenerator, const int minCount, const int maxCount);

ParserCombinator optional(const ParserCombinator tokenGenerator);
ParserCombinator optional(const std::string tokenId, const ParserCombinator tokenGenerator);

ParserCombinator sequence(const std::vector<ParserCombinator> tokenGeneratorSequence);
ParserCombinator sequence(const std::string tokenId, const std::vector<ParserCombinator> tokenGeneratorSequence);

ParserCombinator string(const std::string str);
ParserCombinator string(const std::string tokenId, const std::string str);

ParserCombinator whitespace();

ParserCombinator choice(const std::vector<ParserCombinator> tokenGeneratorChoices);

ParserCombinator proxyParserCombinator(const ParserCombinator* parserCombinatorPointer);

void inlineAnonymousNests(Token& token);

ParserCombinatorResult parse(const std::string& str, const ParserCombinator parserCombinator);

#endif