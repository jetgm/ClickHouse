#pragma once

#include <Parsers/IParserBase.h>
#include <Parsers/IAST_fwd.h>


namespace DB
{
/// Parser for dictionary attribute declaration, similar with parser for table
/// column, but attributes has less parameters. Produces
/// ASTDictionaryAttributeDeclaration.
class ParserDictionaryAttributeDeclaration : public IParserBase
{
protected:
    const char * getName() const override { return "attribute declaration"; }

    bool parseImpl(Pos & pos, ASTPtr & node, Expected & expected) override;
};


/// Creates ASTExpressionList consists of dictionary attributes declaration.
class ParserDictionaryAttributeDeclarationList : public IParserBase
{
protected:
    const char * getName() const  override{ return "attribute declaration list"; }
    bool parseImpl(Pos & pos, ASTPtr & node, Expected & expected) override;
};

}
