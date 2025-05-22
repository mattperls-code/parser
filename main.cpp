#include <iostream>
#include <fstream>

#include "parser.hpp"

std::string readFile(std::string path)
{
    std::string content;
    std::string line;

    std::ifstream file(path);

    if (!file.good()) return "";

    while(getline(file, line)) content += line + "\n";

    return content;
};

void simpleLanguageTest()
{
    Predicate isAlphabetical = [] (const char& c) {
        return std::isalpha(c);
    };

    Predicate isNumeric = [] (const char& c) {
        return std::isdigit(c);
    };

    ParserCombinator whitespace = satisfy(anyOf({ is(' '), is('\t') })).repeatedly();

    ParserCombinator variable = sequence("VARIABLE", {
        satisfy("CHAR", anyOf({ isAlphabetical, is('_') })),
        satisfy("CHAR", anyOf({ isAlphabetical, isNumeric, is('_') })).repeatedly()
    }).named("variable");

    ParserCombinator number = sequence("NUMBER", {
        repetition("INT", satisfy("CHAR", isNumeric), 1),
        optional("DEC", satisfy("CHAR", isNumeric).repeatedly(1).precededBy(satisfy(is('.'))))
    }).named("number");

    ParserCombinator expression;

    ParserCombinator group = sequence("GROUP", {
        satisfy(is('(')),
        optional(proxyParserCombinator(&expression)),
        satisfy(is(')'))
    }).named("group");

    ParserCombinator expressionTerm = sequence("EXPRESSION_TERM", {
        repetition("PREFIX_OPERATORS", satisfy("CHAR", anyOf({ is('+'), is('-') }))),
        choice({
            variable,
            number,
            group
        })
    }).named("expression term");

    ParserCombinator binaryOperator = satisfy("BINARY_OPERATOR", anyOf({ is('+'), is('-'), is('*'), is('/') })).named("binary operator");

    expression = expressionTerm.surroundedBy(whitespace).repeatedlyWithDelimeter(binaryOperator).named("expression");

    ParserCombinator evaluateBlock = string("eval ").named("\"eval \"").followedBy("EVALUATE", expression);

    ParserCombinator assignmentBlock = sequence("ASSIGNMENT", {
        string("let ").named("\"let \""),
        variable.surroundedBy(whitespace),
        satisfy(is('=')).named("="),
        expression
    });

    ParserCombinator ending = satisfy(anyOf({ is(';'), is('\n') })).named("ending");

    ParserCombinator blocks = strictlySequence("BLOCKS", {
        choice({
            whitespace,
            evaluateBlock,
            assignmentBlock
        }).surroundedBy(whitespace).repeatedlyWithDelimeter(ending),
        ending.optionally()
    }).named("blocks");

    std::string testString = readFile("./tests/test.eval");

    ParserCombinatorResult result = parse(testString, blocks);

    if (getResultType(result) == ParserCombinatorResultType::TOKEN) {
        Token token = getTokenFromResult(result);

        std::cout << token.toString() << std::endl;
    } else {
        ParserFailure parserFailure = getParserFailureFromResult(result);

        std::cout << parserFailure.toString() << std::endl;
    }
};

void xmlTest()
{
    Predicate isAlphabetical = [] (const char& c) {
        return std::isalpha(c);
    };

    Predicate isNumeric = [] (const char& c) {
        return std::isdigit(c);
    };

    ParserCombinator whitespace = satisfy(anyOf({ is(' '), is('\t') })).repeatedly();

    ParserCombinator tagName = sequence("TAG_NAME", {
        satisfy("CHAR", isAlphabetical),
        repetition(satisfy("CHAR", anyOf({ isAlphabetical, isNumeric })))
    }).named("tag name");

    ParserCombinator tagAttributes = repetition("ATTRIBUTES", sequence({
        whitespace,
        sequence("KEY", {
            satisfy("CHAR", isAlphabetical),
            repetition(satisfy("CHAR", anyOf({ isAlphabetical, isNumeric })))
        }).named("key"),
        whitespace,
        satisfy(is('=')).named("\"=\""),
        whitespace,
        satisfy(is('\"')).named("\""),
        repetition("VALUE", satisfy("CHAR", negate(is('\"')))).named("value"),
        satisfy(is('\"')).named("\""),
    }).named("attribute"));

    ParserCombinator tagContent = sequence({
        whitespace,
        tagName,
        tagAttributes,
        whitespace
    }).named("tag content");

    ParserCombinator openingTag = sequence("OPENING_TAG", {
        whitespace,
        satisfy(is('<')).named("<"),
        tagContent,
        satisfy(is('>')).named(">")
    }).named("opening tag");

    ParserCombinator closingTag = sequence("CLOSING_TAG", {
        whitespace,
        string("</").named("</"),
        whitespace,
        tagName,
        whitespace,
        satisfy(is('>')).named(">")
    }).named("closing tag");

    ParserCombinator selfClosingTag = sequence("SELF_CLOSING_TAG", {
        whitespace,
        satisfy(is('<')).named("<"),
        whitespace,
        tagContent,
        whitespace,
        string("/>").named("/>")
    }).named("self closing tag");

    ParserCombinator nestingTag;

    nestingTag = sequence("NESTING_TAG", {
        openingTag,
        repetition("CHILDREN", choice({
            repetition("TEXT", satisfy("CHAR", negate(anyOf({ is('<'), is('>') }))), 1).named("text"),
            selfClosingTag,
            proxyParserCombinator(&nestingTag)
        })),
        closingTag
    }).named("nesting tag");

    ParserCombinator document = strictlyRepetition(choice({
        nestingTag,
        satisfy(anyOf({ is(' '), is('\t'), is('\n') }))
    }));

    std::string testString = readFile("./tests/test.xml");

    ParserCombinatorResult result = parse(testString, document);
    
    if (getResultType(result) == ParserCombinatorResultType::TOKEN) {
        Token token = getTokenFromResult(result);

        std::cout << token.toString() << std::endl;
    } else {
        ParserFailure parserFailure = getParserFailureFromResult(result);

        std::cout << parserFailure.toString() << std::endl;
    }
};

int main()
{
    simpleLanguageTest();

    // xmlTest();

    return 0;
};