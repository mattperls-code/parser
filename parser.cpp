#include <iostream>
#include <limits>

#include "parser.hpp"

Predicate is(const char c)
{
    return [c] (char testC) {
        return c == testC;
    };
};

Predicate anyOf(const std::vector<Predicate> predicates) {
    return [predicates] (char c) {
        for (Predicate predicate : predicates) {
            if (predicate(c)) return true;
        }

        return false;
    };
};

Predicate negate(const Predicate predicate) {
    return [predicate] (char c) {
        return !predicate(c);
    };
};

const Predicate isAlphabetical = anyOf({
    is('a'),
    is('b'),
    is('c'),
    is('d'),
    is('e'),
    is('f'),
    is('g'),
    is('h'),
    is('i'),
    is('j'),
    is('k'),
    is('l'),
    is('m'),
    is('n'),
    is('o'),
    is('p'),
    is('q'),
    is('r'),
    is('s'),
    is('t'),
    is('u'),
    is('v'),
    is('w'),
    is('x'),
    is('y'),
    is('z'),
    is('A'),
    is('B'),
    is('C'),
    is('D'),
    is('E'),
    is('F'),
    is('G'),
    is('H'),
    is('I'),
    is('J'),
    is('K'),
    is('L'),
    is('M'),
    is('N'),
    is('O'),
    is('P'),
    is('Q'),
    is('R'),
    is('S'),
    is('T'),
    is('U'),
    is('V'),
    is('W'),
    is('X'),
    is('Y'),
    is('Z'),
});

const Predicate isNumeric = anyOf({
    is('0'),
    is('1'),
    is('2'),
    is('3'),
    is('4'),
    is('5'),
    is('6'),
    is('7'),
    is('8'),
    is('9')
});

Token::Token(std::string id, std::string stringLiteral, int start, int width)
{
    this->id = id;
    this->type = Token::TokenType::STRING_LITERAL;
    this->content = stringLiteral;
    this->start = start;
    this->width = width;
};

Token::Token(std::string id, std::vector<Token> nesting, int start, int width)
{
    this->id = id;
    this->type = Token::TokenType::NESTING;
    this->content = nesting;
    this->start = start;
    this->width = width;
};

std::string Token::toString() const
{
    return toString(0);
};

std::string Token::toString(int indent) const
{
    std::string indentStr;

    for (int i = 0;i<indent;i++) indentStr += ' ';

    if (this->type == Token::TokenType::STRING_LITERAL) {
        return indentStr + this->id + " \"" + std::get<std::string>(this->content) + "\"";
    } else {
        std::vector<Token> children = std::get<std::vector<Token>>(this->content);

        std::string childrenString = "";

        for (int i = 0;i<(int)children.size();i++) {
            if (i == 0) childrenString += children[0].toString(indent + 2);
            
            else childrenString += ",\n" + children[i].toString(indent + 2);
        }

        return indentStr + this->id + " {\n" + childrenString + "\n" + indentStr + "}";
    };
};

std::string Token::contentString() const
{
    if (this->type == Token::TokenType::STRING_LITERAL) return std::get<std::string>(this->content);

    std::string childrenString = "";
    
    for (const Token& child : std::get<std::vector<Token>>(this->content)) childrenString += child.contentString();

    return childrenString;
};

ParserFailure::ParserFailure(int start)
{
    this->start = start;
    this->name = "";
};

ParserFailure::ParserFailure(int start, std::string name)
{
    this->start = start;
    this->name = name;
};

ParserFailure ParserFailure::composeFrom(std::vector<ParserFailure> parserFailures)
{
    std::vector<std::string> parserFailureNames;

    for (const ParserFailure& parserFailure : parserFailures) if (parserFailure.name.size() != 0) parserFailureNames.push_back(parserFailure.name);

    std::string names;

    for (int i = 0;i<(int)parserFailureNames.size();i++) {
        if (i == 0) names += parserFailureNames[0];

        else names += " | " + parserFailureNames[i];
    }

    return ParserFailure(parserFailures[0].start, names);
};

std::string ParserFailure::toString() const
{
    std::string locationString = "Error at character " + std::to_string(this->start + 1) + ". ";

    std::string expectedString = this->name.size() == 0 ? "" : "Expected " + this->name + ". ";
    
    return locationString + expectedString;
};

ParserCombinatorResultType getResultType(ParserCombinatorResult result)
{
    return result.index() == 0 ? ParserCombinatorResultType::TOKEN : ParserCombinatorResultType::PARSER_FAILURE;
};

Token getTokenFromResult(ParserCombinatorResult result)
{
    return std::get<Token>(result);
};

ParserFailure getParserFailureFromResult(ParserCombinatorResult result)
{
    return std::get<ParserFailure>(result);
};

ParserCombinator::ParserCombinator(std::function<ParserCombinatorResult(const std::string&, const int, const int)> implementation)
{
    this->implementation = implementation;
};

ParserCombinatorResult ParserCombinator::operator()(const std::string& str, const int start, const int width) const
{
    return this->implementation(str, start, width);
};

ParserCombinator ParserCombinator::repeatedly()
{
    return repetition(*this);
};

ParserCombinator ParserCombinator::repeatedly(const int minCount)
{
    return repetition(*this, minCount);
};

ParserCombinator ParserCombinator::repeatedly(const int minCount, const int maxCount)
{
    return repetition(*this, minCount, maxCount);
};

ParserCombinator ParserCombinator::optionally()
{
    return optional(*this);
};

ParserCombinator ParserCombinator::named(const std::string name)
{
    return ParserCombinator([*this, name] (const std::string& str, const int start, const int width) -> ParserCombinatorResult {
        ParserCombinatorResult result = (*this)(str, start, width);
        
        if (getResultType(result) == ParserCombinatorResultType::TOKEN) return getTokenFromResult(result);

        ParserFailure defaultParserFailure = getParserFailureFromResult(result);

        std::string bestName = defaultParserFailure.name.size() == 0 ? name : defaultParserFailure.name;

        return ParserFailure(defaultParserFailure.start, bestName);
    });
};

ParserCombinator satisfy(const Predicate predicate)
{
    return satisfy("", predicate);
};

ParserCombinator satisfy(const std::string tokenId, const Predicate predicate)
{
    return ParserCombinator([tokenId, predicate] (const std::string& str, int start, int width) -> ParserCombinatorResult {
        (void) width;

        char c = str[start];

        if (predicate(c)) return Token(tokenId, std::string(1, c), start, 1);

        else return ParserFailure(start);
    });
};

ParserCombinator repetition(const ParserCombinator nestedTokenGenerator)
{
    return repetition("", nestedTokenGenerator);
};

ParserCombinator repetition(const ParserCombinator nestedTokenGenerator, const int minCount)
{
    return repetition("", nestedTokenGenerator, minCount);
};

ParserCombinator repetition(const ParserCombinator nestedTokenGenerator, const int minCount, const int maxCount)
{
    return repetition("", nestedTokenGenerator, minCount, maxCount);
};

ParserCombinator repetition(const std::string tokenId, const ParserCombinator nestedTokenGenerator)
{
    return repetition(tokenId, nestedTokenGenerator, 0, std::numeric_limits<int>::max());
};

ParserCombinator repetition(const std::string tokenId, const ParserCombinator nestedTokenGenerator, const int minCount)
{
    return repetition(tokenId, nestedTokenGenerator, minCount, std::numeric_limits<int>::max());
};

ParserCombinator repetition(const std::string tokenId, const ParserCombinator nestedTokenGenerator, const int minCount, const int maxCount)
{
    return ParserCombinator([tokenId, nestedTokenGenerator, minCount, maxCount] (const std::string& str, int start, int width) -> ParserCombinatorResult {
        std::vector<Token> nestedTokens;
    
        int scanStart = start;

        while (scanStart != start + width) {
            bool foundToken = false;

            int scanWidth = start + width - scanStart;

            if ((int) nestedTokens.size() == maxCount) break;

            ParserCombinatorResult result = nestedTokenGenerator(str, scanStart, scanWidth);

            if (getResultType(result) == ParserCombinatorResultType::PARSER_FAILURE) break;

            Token token = getTokenFromResult(result);

            nestedTokens.push_back(token);

            scanStart += token.width;
            
            foundToken = true;

            if (!foundToken) break;
        }

        if ((int) nestedTokens.size() < minCount) return ParserFailure(scanStart);

        else return Token(tokenId, nestedTokens, start, scanStart - start);
    });
};

ParserCombinator strictlyRepetition(const ParserCombinator nestedTokenGenerator)
{
    return strictlyRepetition("", nestedTokenGenerator);
};

ParserCombinator strictlyRepetition(const ParserCombinator nestedTokenGenerator, const int minCount)
{
    return strictlyRepetition("", nestedTokenGenerator, minCount);
};

ParserCombinator strictlyRepetition(const ParserCombinator nestedTokenGenerator, const int minCount, const int maxCount)
{
    return strictlyRepetition("", nestedTokenGenerator, minCount, maxCount);
};

ParserCombinator strictlyRepetition(const std::string tokenId, const ParserCombinator nestedTokenGenerator)
{
    return strictlyRepetition(tokenId, nestedTokenGenerator, 0, std::numeric_limits<int>::max());
};

ParserCombinator strictlyRepetition(const std::string tokenId, const ParserCombinator nestedTokenGenerator, const int minCount)
{
    return strictlyRepetition(tokenId, nestedTokenGenerator, minCount, std::numeric_limits<int>::max());
};

ParserCombinator strictlyRepetition(const std::string tokenId, const ParserCombinator nestedTokenGenerator, const int minCount, const int maxCount)
{
    return ParserCombinator([tokenId, nestedTokenGenerator, minCount, maxCount] (const std::string& str, int start, int width) -> ParserCombinatorResult {
        std::vector<Token> nestedTokens;
    
        int scanStart = start;

        while (scanStart != start + width) {
            bool foundToken = false;

            int scanWidth = start + width - scanStart;

            if ((int) nestedTokens.size() == maxCount) break;

            ParserCombinatorResult result = nestedTokenGenerator(str, scanStart, scanWidth);

            if (getResultType(result) == ParserCombinatorResultType::PARSER_FAILURE) break;

            Token token = getTokenFromResult(result);

            nestedTokens.push_back(token);

            scanStart += token.width;
            
            foundToken = true;

            if (!foundToken) break;
        }

        if ((int) nestedTokens.size() < minCount) return ParserFailure(scanStart);

        // failed to parse last repetition, so generator must return parse failure for unconsumed remainder
        else if (scanStart != start + width) return nestedTokenGenerator(str, scanStart, start + width - scanStart);

        else return Token(tokenId, nestedTokens, start, scanStart - start);
    });
};

ParserCombinator optional(const ParserCombinator tokenGenerator)
{
    return optional("", tokenGenerator);
};

ParserCombinator optional(const std::string tokenId, const ParserCombinator tokenGenerator)
{
    return repetition(tokenId, tokenGenerator, 0, 1);
};

ParserCombinator sequence(const std::vector<ParserCombinator> tokenGeneratorSequence)
{
    return sequence("", tokenGeneratorSequence);
};

ParserCombinator sequence(const std::string tokenId, const std::vector<ParserCombinator> tokenGeneratorSequence)
{
    return ParserCombinator([tokenId, tokenGeneratorSequence] (const std::string& str, int start, int width) -> ParserCombinatorResult {
        std::vector<Token> sequenceTokens;

        int scanOffset = 0;

        for (ParserCombinator tokenGenerator : tokenGeneratorSequence) {
            ParserCombinatorResult result = tokenGenerator(str, start + scanOffset, width - scanOffset);

            if (getResultType(result) == ParserCombinatorResultType::PARSER_FAILURE) return result;

            Token token = getTokenFromResult(result);

            sequenceTokens.push_back(token);

            scanOffset += token.width;

            if (scanOffset > width) return ParserFailure(start);
        }

        return Token(tokenId, sequenceTokens, start, scanOffset);
    });
};

ParserCombinator string(const std::string str)
{
    return string("", str);
};

ParserCombinator string(const std::string tokenId, const std::string str)
{
    std::vector<ParserCombinator> charParserCombinators;

    for (const char c : str) charParserCombinators.push_back(satisfy(is(c)));

    return sequence(tokenId, charParserCombinators);
};

ParserCombinator whitespace()
{
    return satisfy(anyOf({ is(' '), is('\t') })).repeatedly();
};

ParserCombinator choice(const std::vector<ParserCombinator> tokenGeneratorChoices)
{
    return ParserCombinator([tokenGeneratorChoices] (const std::string& str, int start, int width) -> ParserCombinatorResult {
        if (tokenGeneratorChoices.size() == 0) return ParserFailure(start);

        bool foundToken = false;
        std::vector<ParserFailure> parseFailures;
        Token bestToken;
        
        for (ParserCombinator tokenGenerator : tokenGeneratorChoices) {
            ParserCombinatorResult result = tokenGenerator(str, start, width);

            if (getResultType(result) == ParserCombinatorResultType::TOKEN) {
                Token token = getTokenFromResult(result);

                if (!foundToken || token.width < bestToken.width) {
                    foundToken = true;

                    bestToken = token;
                }
            }
            else if (!foundToken) {
                ParserFailure parseFailure = getParserFailureFromResult(result);

                if (parseFailures.size() == 0 || parseFailure.start > parseFailures[0].start) parseFailures = { parseFailure };

                else if (parseFailure.start == parseFailures[0].start) parseFailures.push_back(parseFailure);
            }
        }

        if (foundToken) return bestToken;

        else return ParserFailure::composeFrom(parseFailures);
    });
};

ParserCombinator proxyParserCombinator(const ParserCombinator* parserCombinatorPointer)
{
    return ParserCombinator([parserCombinatorPointer] (const std::string& str, int start, int width) -> ParserCombinatorResult {
        ParserCombinator proxiedParserCombinator = *parserCombinatorPointer;

        return proxiedParserCombinator(str, start, width);
    });
};

void inlineAnonymousNests(Token& token)
{
    if (token.type == Token::TokenType::STRING_LITERAL) return;

    std::vector<Token> children = std::get<std::vector<Token>>(token.content);

    for (Token& child : children) inlineAnonymousNests(child);

    // TODO: could be done in place
    std::vector<Token> inlinedChildren;

    for (Token child : children) {
        if (child.type == Token::TokenType::NESTING && child.id.size() == 0) {
            std::vector<Token> childrenOfChild = std::get<std::vector<Token>>(child.content);

            inlinedChildren.insert(inlinedChildren.end(), childrenOfChild.begin(), childrenOfChild.end());
        } else if(child.id.size() != 0) {
            inlinedChildren.push_back(child);
        }
    }

    token.content = inlinedChildren;
};

ParserCombinatorResult parse(const std::string& str, const ParserCombinator parserCombinator)
{
    ParserCombinatorResult result = parserCombinator(str, 0, (int) str.size());

    if (getResultType(result) == ParserCombinatorResultType::PARSER_FAILURE) return result;

    Token token = getTokenFromResult(result);

    inlineAnonymousNests(token);

    return token;
};