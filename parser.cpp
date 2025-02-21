#include <iostream>
#include <limits>

#include "parser.hpp"

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

ParserCombinator satisfy(const Predicate predicate)
{
    return satisfy("", predicate);
};

ParserCombinator satisfy(const std::string tokenId, const Predicate predicate)
{
    return [tokenId, predicate] (const std::string& str, int start, int width) -> ParserCombinatorResult {
        (void) width;

        char c = str[start];
        
        return predicate(c) ? std::make_optional<Token>(tokenId, std::string(1, c), start, 1) : std::nullopt;
    };
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
    return [tokenId, nestedTokenGenerator, minCount, maxCount] (const std::string& str, int start, int width) -> ParserCombinatorResult {
        std::vector<Token> nestedTokens;
    
        int scanStart = start;

        while (scanStart != start + width) {
            bool foundToken = false;

            int maxScanWidth = start + width - scanStart;

            for (int i = 0;i<maxScanWidth;i++) {
                int scanWidth = maxScanWidth - i;

                if ((int) nestedTokens.size() == maxCount) continue;

                ParserCombinatorResult tokenTestResult = nestedTokenGenerator(str, scanStart, scanWidth);

                if (!tokenTestResult.has_value()) continue;

                Token token = tokenTestResult.value();

                nestedTokens.push_back(token);

                scanStart += token.width;
                
                foundToken = true;

                break;
            }

            if (!foundToken) break;
        }

        if ((int) nestedTokens.size() < minCount) return std::nullopt;

        return std::make_optional<Token>(tokenId, nestedTokens, start, scanStart - start);
    };
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
    return [tokenId, tokenGeneratorSequence] (const std::string& str, int start, int width) -> ParserCombinatorResult {
        std::vector<Token> sequenceTokens;

        int scanStart = 0;

        for (ParserCombinator tokenGenerator : tokenGeneratorSequence) {
            ParserCombinatorResult tokenTestResult = tokenGenerator(str, start + scanStart, width - scanStart);

            if (!tokenTestResult.has_value()) return std::nullopt;

            Token sequenceToken = tokenTestResult.value();

            sequenceTokens.push_back(sequenceToken);

            scanStart += sequenceToken.width;

            if (scanStart > width) return std::nullopt;
        }

        return std::make_optional<Token>(tokenId, sequenceTokens, start, scanStart);
    };
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
    return repetition(satisfy(anyOf({ is(' '), is('\t') })));
};

ParserCombinator choice(const std::vector<ParserCombinator> tokenGeneratorChoices)
{
    return [tokenGeneratorChoices] (const std::string& str, int start, int width) -> ParserCombinatorResult {
        ParserCombinatorResult firstChoiceFound = std::nullopt;
        
        for (ParserCombinator tokenGenerator : tokenGeneratorChoices) {
            ParserCombinatorResult tokenTestResult = tokenGenerator(str, start, width);

            if (!tokenTestResult.has_value()) continue;

            if (!firstChoiceFound.has_value() || firstChoiceFound.value().width > tokenTestResult.value().width) firstChoiceFound = tokenTestResult;
        }

        return firstChoiceFound;
    };
};

ParserCombinator proxyParserCombinator(const ParserCombinator* parserCombinatorPointer)
{
    return [parserCombinatorPointer] (const std::string& str, int start, int width) -> ParserCombinatorResult {
        ParserCombinator proxiedParserCombinator = *parserCombinatorPointer;

        return proxiedParserCombinator(str, start, width);
    };
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
    ParserCombinatorResult parserCombinatorResult = parserCombinator(str, 0, str.size());
    
    if (parserCombinatorResult.has_value()) inlineAnonymousNests(parserCombinatorResult.value());

    return parserCombinatorResult;
};