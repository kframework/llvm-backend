#include "kllvm/ast/AST.h"
#include "kllvm/parser/KOREScanner.h"
#include "kllvm/parser/KOREParser.h"
#include <iostream>

namespace kllvm {
namespace parser {

void KOREParser::error(
      const location &loc, const std::string &err_message) {
  std::cerr << "Syntax error at " << loc << ": " << err_message << "\n";
  exit(-1);
}

static std::string str(token tok) {
  switch (tok) {
  case token::EMPTY: abort();
  case token::MODULE: return "module";
  case token::ENDMODULE: return "endmodule";
  case token::IMPORT: return "import";
  case token::SORT: return "sort";
  case token::SYMBOL: return "symbol";
  case token::WHERE: return "where";
  case token::ALIAS: return "alias";
  case token::AXIOM: return "axiom";
  case token::CLAIM: return "claim";
  case token::HOOKEDSORT: return "hooked-sort";
  case token::HOOKEDSYMBOL: return "hooked-symbol";
  case token::COLON: return ":";
  case token::LEFTBRACE: return "{";
  case token::RIGHTBRACE: return "}";
  case token::LEFTBRACKET: return "[";
  case token::RIGHTBRACKET: return "]";
  case token::LEFTPAREN: return "(";
  case token::RIGHTPAREN: return ")";
  case token::COMMA: return ",";
  case token::COLONEQUAL: return ":=";
  case token::ID: return "<identifier>";
  case token::STRING: return "<string>";
  case token::TOKEN_EOF: return "<EOF>";
  }
  abort();
}

std::string KOREParser::consume(token next) {
  std::string data;
  token actual;
  if (buffer.tok == token::EMPTY) {
    actual = scanner.yylex(&data, &loc);
  } else {
    actual = buffer.tok;
    data = buffer.data;
    buffer.tok = token::EMPTY;
  }
  if (actual == next) return data;
  error(loc, "Expected: " + str(next) + " Actual: " + str(actual));
}

token KOREParser::peek(void) {
  std::string data;
  if (buffer.tok == token::EMPTY) {
    buffer.tok = scanner.yylex(&data, &loc);
    buffer.data = data;
  }
  return buffer.tok;
}

ptr<KOREDefinition> KOREParser::definition() {
  consume(token::LEFTBRACKET);
  auto result = KOREDefinition::Create();
  attributes(result.get());
  consume(token::RIGHTBRACKET);
  modules(result.get());
  consume(token::TOKEN_EOF);
  return result;
}

ptr<KOREPattern> KOREParser::pattern() {
  auto result = _pattern();
  consume(token::TOKEN_EOF);
  return result;
}

template <typename Node>
void KOREParser::attributes(Node *node) {
  if (peek() == token::ID) {
    attributesNE(node);
  }
}

template <typename Node>
void KOREParser::attributesNE(Node *node) {
  auto pat = _applicationPattern();
  node->addAttribute(std::move(pat));
  while (peek() == token::COMMA) {
    consume(token::COMMA);
    pat = _applicationPattern();
    node->addAttribute(std::move(pat));
  }
}

void KOREParser::modules(KOREDefinition *node) {
  auto mod = module();
  node->addModule(std::move(mod));
  while (peek() == token::MODULE) {
    mod = module();
    node->addModule(std::move(mod));
  }
}

ptr<KOREModule> KOREParser::module() {
  consume(token::MODULE);
  std::string name = consume(token::ID);
  auto mod = KOREModule::Create(name);
  sentences(mod.get());
  consume(token::ENDMODULE);
  consume(token::LEFTBRACKET);
  attributes(mod.get());
  consume(token::RIGHTBRACKET);
  return mod;
}

void KOREParser::sentences(KOREModule *node) {
  while (peek() != token::ENDMODULE) {
    auto decl = sentence();
    node->addDeclaration(std::move(decl));
  }
}

std::vector<ptr<KOREDeclaration>> KOREParser::declarations(void) {
  std::vector<ptr<KOREDeclaration>> decls;
  while (peek() != token::TOKEN_EOF) {
    auto decl = sentence();
    decls.push_back(std::move(decl));
  }
  return decls;
}

ptr<KOREDeclaration> KOREParser::sentence() {
  std::string name;
  token current = peek();
  switch(current) {
  case token::IMPORT: {
    consume(token::IMPORT);
    name = consume(token::ID);
    auto import = KOREModuleImportDeclaration::Create(name);
    consume(token::LEFTBRACKET);
    attributes(import.get());
    consume(token::RIGHTBRACKET);
    return import;
  } case token::SORT:
  case token::HOOKEDSORT: {
    consume(current);
    name = consume(token::ID);
    consume(token::LEFTBRACE);
    auto sortDecl = KORECompositeSortDeclaration::Create(name, current == token::HOOKEDSORT);
    sortVariables(sortDecl.get());
    consume(token::RIGHTBRACE);
    consume(token::LEFTBRACKET);
    attributes(sortDecl.get());
    consume(token::RIGHTBRACKET);
    return sortDecl;
  } case token::SYMBOL:
  case token::HOOKEDSYMBOL: {
    consume(current);
    name = consume(token::ID);
    consume(token::LEFTBRACE);
    auto symbol = KORESymbolDeclaration::Create(name, current == token::HOOKEDSYMBOL);
    sortVariables(symbol.get());
    consume(token::RIGHTBRACE);
    consume(token::LEFTPAREN);
    sorts(symbol->getSymbol());
    consume(token::RIGHTPAREN);
    consume(token::COLON);
    auto returnSort = sort();
    symbol->getSymbol()->addSort(std::move(returnSort));
    consume(token::LEFTBRACKET);
    attributes(symbol.get());
    consume(token::RIGHTBRACKET);
    return symbol;
  } case token::ALIAS: {
    consume(token::ALIAS);
    name = consume(token::ID);
    consume(token::LEFTBRACE);
    auto alias = KOREAliasDeclaration::Create(name);
    sortVariables(alias.get());
    consume(token::RIGHTBRACE);
    consume(token::LEFTPAREN);
    sorts(alias->getSymbol());
    consume(token::RIGHTPAREN);
    consume(token::COLON);
    auto returnSort = sort();
    alias->getSymbol()->addSort(std::move(returnSort));
    consume(token::WHERE);
    auto variables = _applicationPattern();
    alias->addVariables(std::move(variables));
    consume(token::COLONEQUAL);
    auto pat = _pattern();
    alias->addPattern(std::move(pat));
    consume(token::LEFTBRACKET);
    attributes(alias.get());
    consume(token::RIGHTBRACKET);
    return alias;
  } case token::AXIOM:
  case token::CLAIM: {
    consume(current);
    consume(token::LEFTBRACE);
    auto axiom = KOREAxiomDeclaration::Create(current == token::CLAIM);
    sortVariables(axiom.get());
    consume(token::RIGHTBRACE);
    auto pat = _pattern();
    axiom->addPattern(std::move(pat));
    consume(token::LEFTBRACKET);
    attributes(axiom.get());
    consume(token::RIGHTBRACKET);
    return axiom;
  } default:
    error(loc, "Expected: [import, sort, hooked-sort, symbol, hooked-symbol, alias, axiom, claim] Actual: " + str(current));
  }
}

void KOREParser::sortVariables(KOREDeclaration *node) {
  if (peek() == token::ID) {
    sortVariablesNE(node);
  }
}

void KOREParser::sortVariablesNE(KOREDeclaration *node) {
  std::string name = consume(token::ID);
  auto var = KORESortVariable::Create(name);
  node->addObjectSortVariable(std::move(var));
  while (peek() == token::COMMA) {
    consume(token::COMMA);
    name = consume(token::ID);
    var = KORESortVariable::Create(name);
    node->addObjectSortVariable(std::move(var));
  }
}

template <typename Node>
void KOREParser::sorts(Node *node) {
  if (peek() == token::ID) {
    sortsNE(node);
  }
}

template <typename Node>
void KOREParser::sortsNE(Node *node) {
  auto _sort = sort();
  node->addArgument(std::move(_sort));
  while (peek() == token::COMMA) {
    consume(token::COMMA);
    _sort = sort();
    node->addArgument(std::move(_sort));
  }
}

sptr<KORESort> KOREParser::sort() {
  std::string name = consume(token::ID);
  if (peek() == token::LEFTBRACE) {
    consume(token::LEFTBRACE);
    auto sort = KORECompositeSort::Create(name); 
    sorts(sort.get());
    consume(token::RIGHTBRACE);
    return sort;
  } else {
    return KORESortVariable::Create(name);
  }
}

ptr<KOREPattern> KOREParser::_pattern() {
  token current = peek();
  switch(current) {
  case token::ID: {
    std::string name = consume(token::ID);
    current = peek();
    switch(current) {
    case token::COLON:
      consume(token::COLON);
      return KOREVariablePattern::Create(name, sort());
    case token::LEFTBRACE:
      return applicationPattern(name);
    default:
      error(loc, "Expected: [:, {] Actual: " + str(current));
    }
  } case token::STRING:
    return KOREStringPattern::Create(consume(token::STRING));
  default:
    error(loc, "Expected: [<id>, <string>] Actual: " + str(current));
  }
}

ptr<KOREPattern> KOREParser::applicationPattern() {
  return applicationPattern(consume(token::ID));
}
ptr<KORECompositePattern> KOREParser::_applicationPattern() {
  return _applicationPattern(consume(token::ID));
}

ptr<KOREPattern> KOREParser::applicationPattern(std::string name) {
  if (name == "\\left-assoc" || name == "\\right-assoc") {
    consume(token::LEFTBRACE);
    consume(token::RIGHTBRACE);
    consume(token::LEFTPAREN);
    std::string symbol = consume(token::ID);
    consume(token::LEFTBRACE);
    auto pat = KORECompositePattern::Create(symbol);
    sorts(pat->getConstructor());
    pat->getConstructor()->initPatternArguments();
    consume(token::RIGHTBRACE);
    consume(token::LEFTPAREN);
    std::vector<ptr<KOREPattern>> pats;
    patterns(pats);
    consume(token::RIGHTPAREN);
    consume(token::RIGHTPAREN);
    if (name == "\\left-assoc") {
      ptr<KOREPattern> accum = std::move(pats[0]);
      for (size_t i = 1; i < pats.size(); i++) {
        auto newAccum = KORECompositePattern::Create(pat->getConstructor());
        newAccum->addArgument(std::move(accum));
        newAccum->addArgument(std::move(pats[i]));
        accum = ptr<KOREPattern>(newAccum.release());
      }
      return accum;
    } else {
      ptr<KOREPattern> accum = std::move(pats[pats.size()-1]);
      for (size_t i = pats.size() - 2; i >= 0; i--) {
        auto newAccum = KORECompositePattern::Create(pat->getConstructor());
        newAccum->addArgument(std::move(pats[i]));
        newAccum->addArgument(std::move(accum));
        accum = ptr<KOREPattern>(newAccum.release());
      }
      return accum;
    }
  }
  return _applicationPattern(name);
}

ptr<KORECompositePattern> KOREParser::_applicationPattern(std::string name) {
  consume(token::LEFTBRACE);
  auto pat = KORECompositePattern::Create(name);
  sorts(pat->getConstructor());
  pat->getConstructor()->initPatternArguments();
  consume(token::RIGHTBRACE);
  consume(token::LEFTPAREN);
  patterns(pat.get());
  consume(token::RIGHTPAREN);
  return pat;
}

void KOREParser::patterns(KORECompositePattern *node) {
  if (peek() == token::RIGHTPAREN) return;
  patternsNE(node);
}

void KOREParser::patternsNE(KORECompositePattern *node) {
  auto pat = _pattern();
  node->addArgument(std::move(pat));
  while (peek() == token::COMMA) {
    consume(token::COMMA);
    pat = _pattern();
    node->addArgument(std::move(pat));
  }
}

void KOREParser::patterns(std::vector<ptr<KOREPattern>> &node) {
  if (peek() == token::RIGHTPAREN) return;
  patternsNE(node);
}

void KOREParser::patternsNE(std::vector<ptr<KOREPattern>> &node) {
  auto pat = _pattern();
  node.push_back(std::move(pat));
  while (peek() == token::COMMA) {
    consume(token::COMMA);
    pat = _pattern();
    node.push_back(std::move(pat));
  }
}

}
}
