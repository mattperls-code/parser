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
    });

    ParserCombinator number = sequence("NUMBER", {
        repetition("INT", satisfy("CHAR", isNumeric), 1),
        optional("DEC", sequence({
            satisfy(is('.')),
            repetition(satisfy("CHAR", isNumeric), 1)
        }))
    });

    ParserCombinator expression;

    ParserCombinator group = sequence("GROUP", {
        satisfy(is('(')),
        optional(proxyParserCombinator(&expression)),
        satisfy(is(')'))
    });

    ParserCombinator expressionTerm = sequence("EXPRESSION_TERM", {
        repetition("PREFIX_OPERATORS", satisfy("CHAR", anyOf({ is('+'), is('-') }))),
        choice({
            variable,
            number,
            group
        })
    });

    expression = sequence("EXPRESSION", {
        whitespace(),
        expressionTerm,
        repetition(sequence({
            whitespace(),
            satisfy("BINARY_OPERATOR", anyOf({ is('+'), is('-'), is('*'), is('/') })),
            whitespace(),
            expressionTerm
        }))
    });

    ParserCombinator ending = satisfy(anyOf({ is(';'), is('\n') }));

    ParserCombinator blocks = repetition("BLOCKS", choice({
        sequence({
            whitespace(),
            ending
        }),
        sequence("EVALUATE", {
            whitespace(),
            string("eval "),
            whitespace(),
            expression,
            whitespace(),
            ending
        }),
        sequence("ASSIGNMENT", {
            whitespace(),
            string("let "),
            whitespace(),
            variable,
            whitespace(),
            satisfy("EQUALS", is('=')),
            whitespace(),
            expression,
            whitespace(),
            ending
        })
    }));

    std::string testString = readFile("./tests/test.eval");

    ParserCombinatorResult result = parse(testString, blocks);
    
    if (result.has_value()) {
        Token resultToken = result.value();

        std::cout << resultToken.toString() << std::endl;
        
        // std::cout << resultToken.contentString() << std::endl;
    }
};

void xmlTest()
{
    ParserCombinator tagName = sequence("TAG_NAME", {
        satisfy("CHAR", isAlphabetical),
        repetition(satisfy("CHAR", anyOf({ isAlphabetical, isNumeric })))
    });

    ParserCombinator tagContent = sequence({
        whitespace(),
        tagName,
        repetition("ATTRIBUTES", sequence({
            whitespace(),
            sequence("KEY", {
                satisfy("CHAR", isAlphabetical),
                repetition(satisfy("CHAR", anyOf({ isAlphabetical, isNumeric })))
            }),
            whitespace(),
            satisfy(is('=')),
            whitespace(),
            satisfy(is('\"')),
            repetition("VALUE", satisfy("CHAR", isAlphabetical)),
            satisfy(is('\"')),
        })),
        whitespace()
    });

    ParserCombinator openingTag = sequence("OPENING_TAG", {
        whitespace(),
        satisfy(is('<')),
        tagContent,
        satisfy(is('>'))
    });

    ParserCombinator closingTag = sequence("CLOSING_TAG", {
        whitespace(),
        string("</"),
        whitespace(),
        tagName,
        whitespace(),
        satisfy(is('>'))
    });

    ParserCombinator selfClosingTag = sequence("SELF_CLOSING_TAG", {
        whitespace(),
        satisfy(is('<')),
        whitespace(),
        tagContent,
        whitespace(),
        string("/>")
    });

    ParserCombinator nestingTag;

    nestingTag = sequence("NESTING_TAG", {
        openingTag,
        repetition("CHILDREN", choice({
            repetition("TEXT", satisfy("CHAR", negate(anyOf({ is('<'), is('>') }))), 1),
            selfClosingTag,
            proxyParserCombinator(&nestingTag)
        })),
        closingTag
    });

    std::string testString = readFile("./tests/test.xml");

    ParserCombinatorResult result = parse(testString, nestingTag);

    if (result.has_value()) {
        Token resultToken = result.value();

        std::cout << resultToken.toString() << std::endl;

        // std::cout << resultToken.contentString() << std::endl;
    }
};

int main()
{
    simpleLanguageTest();

    // xmlTest();

    return 0;
};