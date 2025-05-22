#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>
#include <variant>
#include <functional>

typedef std::function<bool(const char&)> Predicate;

Predicate is(const char& c);
Predicate negate(const Predicate predicate);
Predicate anyOf(const std::vector<Predicate> predicates);
Predicate noneOf(const std::vector<Predicate> predicates);

class Token
{
    private:
        std::string toString(int indent) const;

    public:
        std::string id;

        enum TokenType {
            STRING_LITERAL,
            NEST
        } type;

        std::variant<std::string, std::vector<Token>> content;

        int start;
        int width;

        Token() = default;
    
        Token(std::string id, std::string stringLiteral, int start, int width);
        Token(std::string id, std::vector<Token> nesting, int start, int width);

        const std::string& getStringLiteralContent() const;
        const std::vector<Token>& getNestingContent() const;

        std::string toString() const;
        std::string contentString() const;
};

class ParserFailure
{
    public:
        int start;

        std::string name;

        ParserFailure(int start);
        ParserFailure(int start, std::string name);

        static ParserFailure composeFrom(std::vector<ParserFailure> parserFailures);

        std::string toString() const;
};

enum ParserCombinatorResultType
{
    TOKEN,
    PARSER_FAILURE
};

typedef std::variant<Token, ParserFailure> ParserCombinatorResult;

ParserCombinatorResultType getResultType(ParserCombinatorResult result);
Token getTokenFromResult(ParserCombinatorResult result);
ParserFailure getParserFailureFromResult(ParserCombinatorResult result);

class ParserCombinator
{
    private:
        std::function<ParserCombinatorResult(const std::string&, const int)> implementation;

    public:
        ParserCombinator() = default;

        ParserCombinator(std::function<ParserCombinatorResult(const std::string&, const int)> implementation);

        ParserCombinatorResult operator()(const std::string&, const int) const;

        ParserCombinator repeatedly() const;
        ParserCombinator repeatedly(const int minCount) const;
        ParserCombinator repeatedly(const int minCount, const int maxCount) const;

        ParserCombinator strictlyRepeatedly() const;
        ParserCombinator strictlyRepeatedly(const int minCount) const;
        ParserCombinator strictlyRepeatedly(const int minCount, const int maxCount) const;

        ParserCombinator repeatedlyWithDelimeter(const ParserCombinator delimeter) const;
        ParserCombinator repeatedlyWithDelimeter(const std::string wrapperTokenId, const ParserCombinator delimeter) const;

        ParserCombinator strictlyRepeatedlyWithDelimeter(const ParserCombinator delimeter) const;
        ParserCombinator strictlyRepeatedlyWithDelimeter(const std::string wrapperTokenId, const ParserCombinator delimeter) const;

        ParserCombinator optionally() const;
        ParserCombinator optionally(const std::string wrapperTokenId) const;

        ParserCombinator precededBy(const ParserCombinator predecessor) const;
        ParserCombinator precededBy(const std::string wrapperTokenId, const ParserCombinator predecessor) const;

        ParserCombinator followedBy(const ParserCombinator successor) const;
        ParserCombinator followedBy(const std::string wrapperTokenId, const ParserCombinator successor) const;

        ParserCombinator surroundedBy(const ParserCombinator neighbor) const;
        ParserCombinator surroundedBy(const std::string wrapperTokenId, const ParserCombinator neighbor) const;

        ParserCombinator named(const std::string name) const;
};

ParserCombinator satisfy(const Predicate predicate);
ParserCombinator satisfy(const std::string tokenId, const Predicate predicate);

ParserCombinator repetition(const ParserCombinator nestedTokenGenerator);
ParserCombinator repetition(const ParserCombinator nestedTokenGenerator, const int minCount);
ParserCombinator repetition(const ParserCombinator nestedTokenGenerator, const int minCount, const int maxCount);
ParserCombinator repetition(const std::string tokenId, const ParserCombinator nestedTokenGenerator);
ParserCombinator repetition(const std::string tokenId, const ParserCombinator nestedTokenGenerator, const int minCount);
ParserCombinator repetition(const std::string tokenId, const ParserCombinator nestedTokenGenerator, const int minCount, const int maxCount);

ParserCombinator strictlyRepetition(const ParserCombinator nestedTokenGenerator);
ParserCombinator strictlyRepetition(const ParserCombinator nestedTokenGenerator, const int minCount);
ParserCombinator strictlyRepetition(const ParserCombinator nestedTokenGenerator, const int minCount, const int maxCount);
ParserCombinator strictlyRepetition(const std::string tokenId, const ParserCombinator nestedTokenGenerator);
ParserCombinator strictlyRepetition(const std::string tokenId, const ParserCombinator nestedTokenGenerator, const int minCount);
ParserCombinator strictlyRepetition(const std::string tokenId, const ParserCombinator nestedTokenGenerator, const int minCount, const int maxCount);

ParserCombinator optional(const ParserCombinator tokenGenerator);
ParserCombinator optional(const std::string tokenId, const ParserCombinator tokenGenerator);

ParserCombinator sequence(const std::vector<ParserCombinator> tokenGeneratorSequence);
ParserCombinator sequence(const std::string tokenId, const std::vector<ParserCombinator> tokenGeneratorSequence);

ParserCombinator strictlySequence(const std::vector<ParserCombinator> tokenGeneratorSequence);
ParserCombinator strictlySequence(const std::string tokenId, const std::vector<ParserCombinator> tokenGeneratorSequence);

ParserCombinator string(const std::string strLiteral);
ParserCombinator string(const std::string tokenId, const std::string strLiteral);

ParserCombinator negate(const ParserCombinator tokenGenerator);
ParserCombinator negate(const std::string tokenId, const ParserCombinator tokenGenerator);

ParserCombinator choice(const std::vector<ParserCombinator> tokenGeneratorChoices);
ParserCombinator choiceConcurrent(const std::vector<ParserCombinator> tokenGeneratorChoices);

ParserCombinator allOf(const std::string tokenId, const std::vector<ParserCombinator> tokenGeneratorRequirements);
ParserCombinator noneOf(const std::vector<ParserCombinator> tokenGeneratorRequirements);

ParserCombinator proxyParserCombinator(const ParserCombinator* parserCombinatorPointer);

ParserCombinatorResult parse(const std::string& str, const ParserCombinator parserCombinator);

#endif