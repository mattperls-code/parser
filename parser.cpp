#include <future>

#include "parser.hpp"

Predicate is(const char& c)
{
    return [c] (const char& testC) {
        return c == testC;
    };
};

Predicate negate(const Predicate predicate) {
    return [predicate] (const char& c) {
        return !predicate(c);
    };
};

Predicate anyOf(const std::vector<Predicate> predicates) {
    return [predicates] (const char& c) {
        for (Predicate predicate : predicates) {
            if (predicate(c)) return true;
        }

        return false;
    };
};

Predicate noneOf(const std::vector<Predicate> predicates) {
    return [predicates] (const char& c) {
        for (Predicate predicate : predicates) {
            if (predicate(c)) return false;
        }

        return true;
    };
}

Token::Token(std::string id, std::string stringLiteral, const int start, int width)
{
    this->id = id;
    this->type = Token::TokenType::STRING_LITERAL;
    this->content = stringLiteral;
    this->start = start;
    this->width = width;
};

Token::Token(std::string id, std::vector<Token> NEST, const int start, int width)
{
    this->id = id;
    this->type = Token::TokenType::NEST;
    this->content = NEST;
    this->start = start;
    this->width = width;
};

const std::string& Token::getStringLiteralContent() const
{
    return std::get<std::string>(this->content);
};

const std::vector<Token>& Token::getNestingContent() const
{
    return std::get<std::vector<Token>>(this->content);
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
        return indentStr + this->id + " \"" + this->getStringLiteralContent() + "\"";
    } else {
        const std::vector<Token>& children = this->getNestingContent();

        if (children.empty()) return indentStr + this->id;

        std::string childrenString = "";

        for (int i = 0;i<(int)children.size();i++) {
            if (i == 0) childrenString += children[0].toString(indent + 4);
            
            else childrenString += ",\n" + children[i].toString(indent + 4);
        }

        return indentStr + this->id + " {\n" + childrenString + "\n" + indentStr + "}";
    };
};

std::string Token::contentString() const
{
    if (this->type == Token::TokenType::STRING_LITERAL) return this->getStringLiteralContent();

    std::string childrenString = "";
    
    for (const Token& child : this->getNestingContent()) childrenString += child.contentString();

    return childrenString;
};

inline void addChildToken(std::vector<Token>& parent, const Token& token)
{
    if (!token.id.empty()) parent.push_back(token);

    else if(token.type == Token::TokenType::NEST) {
        const std::vector<Token>& tokenChildren = token.getNestingContent();

        parent.insert(parent.end(), tokenChildren.begin(), tokenChildren.end());
    }
};

ParserFailure::ParserFailure(int start)
{
    this->start = start;
    this->name = "";
};

ParserFailure::ParserFailure(int start, std::string name)
{
    this->start = start;
    this->name = "\033[34m" + name + "\033[0m";
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
    std::string locationString = "Error at char " + std::to_string(this->start + 1) + ". ";

    std::string expectedString = this->name.empty() ? "" : "Expected " + this->name;
    
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

ParserCombinator::ParserCombinator(std::function<ParserCombinatorResult(const std::string&, const int)> implementation)
{
    this->implementation = implementation;
};

ParserCombinatorResult ParserCombinator::operator()(const std::string& str, const int start) const
{
    return this->implementation(str, start);
};

ParserCombinator ParserCombinator::repeatedly() const
{
    return repetition(*this);
};

ParserCombinator ParserCombinator::repeatedly(const int minCount) const
{
    return repetition(*this, minCount);
};

ParserCombinator ParserCombinator::repeatedly(const int minCount, const int maxCount) const
{
    return repetition(*this, minCount, maxCount);
};

ParserCombinator ParserCombinator::strictlyRepeatedly() const
{
    return strictlyRepetition(*this);
};

ParserCombinator ParserCombinator::strictlyRepeatedly(const int minCount) const
{
    return strictlyRepetition(*this, minCount);
};

ParserCombinator ParserCombinator::strictlyRepeatedly(const int minCount, const int maxCount) const
{
    return strictlyRepetition(*this, minCount, maxCount);
};

ParserCombinator ParserCombinator::repeatedlyWithDelimeter(const ParserCombinator delimiter) const
{
    return sequence({
        *this,
        sequence({
            delimiter,
            *this
        }).repeatedly()
    });
};

ParserCombinator ParserCombinator::repeatedlyWithDelimeter(const std::string wrapperTokenId, const ParserCombinator delimiter) const
{
    return sequence(wrapperTokenId, {
        *this,
        sequence({
            delimiter,
            *this
        }).repeatedly()
    });
};

ParserCombinator ParserCombinator::strictlyRepeatedlyWithDelimeter(const ParserCombinator delimiter) const
{
    return sequence({
        *this,
        sequence({
            delimiter,
            *this
        }).strictlyRepeatedly()
    });
};

ParserCombinator ParserCombinator::strictlyRepeatedlyWithDelimeter(const std::string wrapperTokenId, const ParserCombinator delimiter) const
{
    return sequence(wrapperTokenId, {
        *this,
        sequence({
            delimiter,
            *this
        }).strictlyRepeatedly()
    });
};

ParserCombinator ParserCombinator::optionally() const
{
    return optional(*this);
};

ParserCombinator ParserCombinator::optionally(const std::string wrapperTokenId) const
{
    return optional(wrapperTokenId, *this);
};

ParserCombinator ParserCombinator::precededBy(const ParserCombinator predecessor) const
{
    return precededBy("", predecessor);
};

ParserCombinator ParserCombinator::precededBy(const std::string wrapperTokenId, const ParserCombinator predecessor) const
{
    return sequence(wrapperTokenId, {
        predecessor,
        *this
    });
};

ParserCombinator ParserCombinator::followedBy(const ParserCombinator predecessor) const
{
    return followedBy("", predecessor);
};

ParserCombinator ParserCombinator::followedBy(const std::string wrapperTokenId, const ParserCombinator predecessor) const
{
    return sequence(wrapperTokenId, {
        *this,
        predecessor
    });
};

ParserCombinator ParserCombinator::sorroundedBy(const ParserCombinator neighbor) const
{
    return sorroundedBy("", neighbor);
};

ParserCombinator ParserCombinator::sorroundedBy(const std::string wrapperTokenId, const ParserCombinator neighbor) const
{
    return sequence(wrapperTokenId, {
        neighbor,
        *this,
        neighbor
    });
};

ParserCombinator ParserCombinator::named(const std::string name) const
{
    return ParserCombinator([*this, name] (const std::string& str, const int start) -> ParserCombinatorResult {
        ParserCombinatorResult result = (*this)(str, start);
        
        if (getResultType(result) == ParserCombinatorResultType::TOKEN) return result;

        ParserFailure defaultParserFailure = getParserFailureFromResult(result);

        std::string bestName = defaultParserFailure.name.empty() ? name : defaultParserFailure.name;

        return ParserFailure(defaultParserFailure.start, bestName);
    });
};

ParserCombinator satisfy(const Predicate predicate)
{
    return satisfy("", predicate);
};

ParserCombinator satisfy(const std::string tokenId, const Predicate predicate)
{
    return ParserCombinator([tokenId, predicate] (const std::string& str, const int start) -> ParserCombinatorResult {
        const char& c = str[start];

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
    return ParserCombinator([tokenId, nestedTokenGenerator, minCount, maxCount] (const std::string& str, const int start) -> ParserCombinatorResult {
        std::vector<Token> nestedTokens;

        int tokensFound = 0;
    
        int scanStart = start;

        while (scanStart != (int) str.size()) {
            if (tokensFound == maxCount) break;

            ParserCombinatorResult result = nestedTokenGenerator(str, scanStart);

            if (getResultType(result) == ParserCombinatorResultType::PARSER_FAILURE) break;

            Token token = getTokenFromResult(result);

            if (token.width == 0) break;

            tokensFound++;

            addChildToken(nestedTokens, token);

            scanStart += token.width;
        }

        if (tokensFound < minCount) return ParserFailure(scanStart);

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
    return ParserCombinator([tokenId, nestedTokenGenerator, minCount, maxCount] (const std::string& str, const int start) -> ParserCombinatorResult {
        std::vector<Token> nestedTokens;

        int tokensFound = 0;
    
        int scanStart = start;

        while (scanStart != (int) str.size()) {
            if (tokensFound == maxCount) break;

            ParserCombinatorResult result = nestedTokenGenerator(str, scanStart);

            if (getResultType(result) == ParserCombinatorResultType::PARSER_FAILURE) return result;

            Token token = getTokenFromResult(result);

            if (token.width == 0) return ParserFailure(scanStart);

            tokensFound++;

            addChildToken(nestedTokens, token);

            scanStart += token.width;
        }
        
        if (scanStart != (int) str.size()) return nestedTokenGenerator(str, scanStart);

        else if (tokensFound < minCount) return ParserFailure(scanStart);

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
    return ParserCombinator([tokenId, tokenGeneratorSequence] (const std::string& str, const int start) -> ParserCombinatorResult {
        std::vector<Token> sequenceTokens;

        int scanOffset = 0;

        for (ParserCombinator tokenGenerator : tokenGeneratorSequence) {
            ParserCombinatorResult result = tokenGenerator(str, start + scanOffset);

            if (getResultType(result) == ParserCombinatorResultType::PARSER_FAILURE) return result;

            Token token = getTokenFromResult(result);

            addChildToken(sequenceTokens, token);

            scanOffset += token.width;
        }

        return Token(tokenId, sequenceTokens, start, scanOffset);
    });
};

ParserCombinator strictlySequence(const std::vector<ParserCombinator> tokenGeneratorSequence) {
    return ParserCombinator([tokenGeneratorSequence] (const std::string& str, const int start) -> ParserCombinatorResult {
        ParserCombinatorResult result = sequence(tokenGeneratorSequence)(str, start);

        if (getResultType(result) == ParserCombinatorResultType::PARSER_FAILURE) return result;

        Token token = getTokenFromResult(result);

        if (token.start + token.width == (int) str.size()) return token;

        else return ParserFailure(token.start + token.width, "end of input");
    });
};

ParserCombinator strictlySequence(const std::string tokenId, const std::vector<ParserCombinator> tokenGeneratorSequence) {
    return ParserCombinator([tokenId, tokenGeneratorSequence] (const std::string& str, const int start) -> ParserCombinatorResult {
        ParserCombinatorResult result = sequence(tokenId, tokenGeneratorSequence)(str, start);

        if (getResultType(result) == ParserCombinatorResultType::PARSER_FAILURE) return result;

        Token token = getTokenFromResult(result);

        if (token.start + token.width == (int) str.size()) return token;

        else return ParserFailure(token.start + token.width, "end of input");
    });
};

ParserCombinator string(const std::string stringLiteral)
{
    return string("", stringLiteral);
};

ParserCombinator string(const std::string tokenId, const std::string stringLiteral)
{
    return ParserCombinator([tokenId, stringLiteral] (const std::string& str, const int start) -> ParserCombinatorResult {
        if (stringLiteral != str.substr(start, stringLiteral.size())) return ParserFailure(start);
        
        else return Token(tokenId, stringLiteral, start, stringLiteral.size());
    });
};

ParserCombinator negate(const ParserCombinator tokenGenerator)
{
    return negate("", tokenGenerator);
};

ParserCombinator negate(const std::string tokenId, const ParserCombinator tokenGenerator)
{
    return ParserCombinator([tokenId, tokenGenerator] (const std::string& str, const int start) -> ParserCombinatorResult {
        ParserCombinatorResult result = tokenGenerator(str, start);

        if (getResultType(result) == ParserCombinatorResultType::TOKEN) return ParserFailure(start);

        else return Token(tokenId, std::vector<Token>(), start, 0);
    });
};

ParserCombinator choice(const std::vector<ParserCombinator> tokenGeneratorChoices)
{
    return ParserCombinator([tokenGeneratorChoices] (const std::string& str, const int start) -> ParserCombinatorResult {
        if (tokenGeneratorChoices.empty()) return ParserFailure(start);

        bool foundToken = false;
        std::vector<ParserFailure> parseFailures;
        Token bestToken;

        for (const ParserCombinator& tokenGenerator : tokenGeneratorChoices) {
            ParserCombinatorResult result = tokenGenerator(str, start);

            if (getResultType(result) == ParserCombinatorResultType::TOKEN) {
                Token token = getTokenFromResult(result);

                if (!foundToken || token.width > bestToken.width) {
                    foundToken = true;

                    bestToken = token;
                }
            }
            else if (!foundToken) {
                ParserFailure parseFailure = getParserFailureFromResult(result);

                if (parseFailures.empty() || parseFailure.start > parseFailures[0].start) parseFailures = { parseFailure };

                else if (parseFailure.start == parseFailures[0].start) parseFailures.push_back(parseFailure);
            }
        }

        if (foundToken) return bestToken;

        else return ParserFailure::composeFrom(parseFailures);
    });
};

ParserCombinator choiceConcurrent(const std::vector<ParserCombinator> tokenGeneratorChoices)
{
    return ParserCombinator([tokenGeneratorChoices] (const std::string& str, const int start) -> ParserCombinatorResult {
        if (tokenGeneratorChoices.empty()) return ParserFailure(start);
        
        std::vector<std::future<ParserCombinatorResult>> tokenGeneratorThreads;

        for (const ParserCombinator& tokenGenerator : tokenGeneratorChoices) tokenGeneratorThreads.push_back(std::async(std::launch::async, tokenGenerator, str, start));

        std::vector<ParserCombinatorResult> tokenGeneratorResults;

        bool foundToken = false;
        std::vector<ParserFailure> parseFailures;
        Token bestToken;

        for (auto& thread : tokenGeneratorThreads) {
            ParserCombinatorResult result = thread.get();

            if (getResultType(result) == ParserCombinatorResultType::TOKEN) {
                Token token = getTokenFromResult(result);

                if (!foundToken || token.width > bestToken.width) {
                    foundToken = true;

                    bestToken = token;
                }
            }
            else if (!foundToken) {
                ParserFailure parseFailure = getParserFailureFromResult(result);

                if (parseFailures.empty() || parseFailure.start > parseFailures[0].start) parseFailures = { parseFailure };

                else if (parseFailure.start == parseFailures[0].start) parseFailures.push_back(parseFailure);
            }
        }

        if (foundToken) return bestToken;

        else return ParserFailure::composeFrom(parseFailures);
    });
};

ParserCombinator allOf(const std::string tokenId, const std::vector<ParserCombinator> tokenGeneratorRequirements)
{
    return ParserCombinator([tokenId, tokenGeneratorRequirements] (const std::string& str, const int start) -> ParserCombinatorResult {
        std::vector<Token> tokens;
        int largestTokenWidth = 0;

        for (const ParserCombinator& tokenGeneratorRequirement : tokenGeneratorRequirements) {
            ParserCombinatorResult result = tokenGeneratorRequirement(str, start);

            if (getResultType(result) == ParserCombinatorResultType::PARSER_FAILURE) return result;
            
            Token token = getTokenFromResult(result);

            addChildToken(tokens, token);

            if (token.width > largestTokenWidth) largestTokenWidth = token.width;
        }

        return Token(tokenId, tokens, start, largestTokenWidth);
    });
};

ParserCombinator noneOf(const std::vector<ParserCombinator> tokenGeneratorRequirements)
{
    return ParserCombinator([tokenGeneratorRequirements] (const std::string& str, const int start) -> ParserCombinatorResult {
        for (const ParserCombinator& tokenGeneratorRequirement : tokenGeneratorRequirements) {
            ParserCombinatorResult result = tokenGeneratorRequirement(str, start);

            if (getResultType(result) == ParserCombinatorResultType::TOKEN) return ParserFailure(start);
        }

        return Token("", std::vector<Token>(), start, 0);
    });
};

ParserCombinator proxyParserCombinator(const ParserCombinator* parserCombinatorPointer)
{
    return ParserCombinator([parserCombinatorPointer] (const std::string& str, const int start) -> ParserCombinatorResult {
        ParserCombinator proxiedParserCombinator = *parserCombinatorPointer;

        return proxiedParserCombinator(str, start);
    });
};

ParserCombinatorResult parse(const std::string& str, const ParserCombinator parserCombinator)
{
    ParserCombinatorResult result = parserCombinator(str, 0);

    if (getResultType(result) == ParserCombinatorResultType::PARSER_FAILURE) return result;

    Token token = getTokenFromResult(result);

    return token;
};