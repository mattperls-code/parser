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
    ParserCombinator variable = sequence("VARIABLE", {
        satisfy("CHAR", anyOf({ isAlphabetical, is('_') })),
        repetition(satisfy("CHAR", anyOf({ isAlphabetical, isNumeric, is('_') })))
    }).named("variable");

    ParserCombinator number = sequence("NUMBER", {
        repetition("INT", satisfy("CHAR", isNumeric), 1),
        optional("DEC", sequence({
            satisfy(is('.')),
            repetition(satisfy("CHAR", isNumeric), 1)
        }))
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

    expression = sequence("EXPRESSION", {
        whitespace(),
        expressionTerm,
        repetition(sequence({
            whitespace(),
            satisfy("BINARY_OPERATOR", anyOf({ is('+'), is('-'), is('*'), is('/') })).named("binary operator"),
            whitespace(),
            expressionTerm
        }))
    }).named("expression");

    ParserCombinator ending = satisfy(anyOf({ is(';'), is('\n') })).named("ending delimiter");

    ParserCombinator blocks = strictlyRepetition("BLOCKS", choice({
        sequence({
            whitespace(),
            ending
        }),
        sequence("EVALUATE", {
            whitespace(),
            string("eval ").named("\"eval \""),
            whitespace(),
            expression,
            whitespace(),
            ending
        }),
        sequence("ASSIGNMENT", {
            whitespace(),
            string("let ").named("\"let \""),
            whitespace(),
            variable,
            whitespace(),
            satisfy("EQUALS", is('=')).named("\"=\""),
            whitespace(),
            expression,
            whitespace(),
            ending
        })
    }));

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
    ParserCombinator tagName = sequence("TAG_NAME", {
        satisfy("CHAR", isAlphabetical),
        repetition(satisfy("CHAR", anyOf({ isAlphabetical, isNumeric })))
    }).named("tag name");

    ParserCombinator tagAttributes = repetition("ATTRIBUTES", sequence({
        whitespace(),
        sequence("KEY", {
            satisfy("CHAR", isAlphabetical),
            repetition(satisfy("CHAR", anyOf({ isAlphabetical, isNumeric })))
        }).named("key"),
        whitespace(),
        satisfy(is('=')).named("\"=\""),
        whitespace(),
        satisfy(is('\"')).named("\""),
        repetition("VALUE", satisfy("CHAR", negate(is('\"')))).named("value"),
        satisfy(is('\"')).named("\""),
    }).named("attribute"));

    ParserCombinator tagContent = sequence({
        whitespace(),
        tagName,
        tagAttributes,
        whitespace()
    }).named("tag content");

    ParserCombinator openingTag = sequence("OPENING_TAG", {
        whitespace(),
        satisfy(is('<')).named("<"),
        tagContent,
        satisfy(is('>')).named(">")
    }).named("opening tag");

    ParserCombinator closingTag = sequence("CLOSING_TAG", {
        whitespace(),
        string("</").named("</"),
        whitespace(),
        tagName,
        whitespace(),
        satisfy(is('>')).named(">")
    }).named("closing tag");

    ParserCombinator selfClosingTag = sequence("SELF_CLOSING_TAG", {
        whitespace(),
        satisfy(is('<')).named("<"),
        whitespace(),
        tagContent,
        whitespace(),
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