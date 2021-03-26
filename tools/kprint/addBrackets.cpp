#include "kllvm/ast/AST.h"

#include <memory>

using namespace kllvm;

enum Fixity {
  EMPTY = 0,
  BARE_LEFT = 1,
  BARE_RIGHT = 2,
  BARE_BOTH = 3
};

/**
 * Get the index of the Nth nonterminal within the list of all terminals and
 * nonterminals in a production.
 *
 * terminals: the `terminals` attribute of a symbol, representing the pattern
 * of which production items are terminals and which are nonterminals as a
 * pattern of 1s and 0s.
 *
 * position: the index of the nonterminal to look up within the list of all
 * nonterminals of the symbol
 *
 * returns: the index of the nonterminal within the list of all nonterminals
 * and terminals of the production the symbol corresponds to.
 */
size_t getNTPositionInProd(std::string terminals, size_t position) {
  size_t terminalPos = 0;
  size_t ntIdx = 0;
  for (; terminalPos < terminals.size(); terminalPos++) {
    if (terminals[terminalPos] == '0') {
      if (ntIdx == position) {
        break;
      }
      ntIdx++;
    }
  }
  return terminalPos;
}

/** 
 * Get whether a production has a terminal at a particular index.
 *
 * Out of bounds errors do not occur with this function: if the index is out of
 * range, `false` is returned, indicating there is not a terminal at that index
 */
bool hasTerminalAtIdx(std::string terminals, size_t position) {
  if (position < 0 || position >= terminals.length()) {
    return false;
  }
  return terminals[position] == '1';
}

/**
 * Compute whether there are terminals immediately to the left and right of a
 * nonterminal in a production.
 *
 * position: the index of the nonterminal to look up within the list of all
 * nonterminals in a symbol
 *
 * sym: the symbol to examine
 *
 * data: pretty printing metadata
 *
 * returns: EMPTY if there are terminals on both sides of this nonterminal
 *          BARE_LEFT if there is a terminal only on the right of this nonterminal
 *          BARE_RIGHT if there is a terminal only on the left of this nonterminal
 *          BARE_BOTH if there are not terminals on either side of this nonterminal
 */
Fixity getFixity(int position, KORESymbol *sym, PrettyPrintData const& data) {
  int result = EMPTY;
  std::string terminals = data.terminals.at(sym->getName());
  size_t terminalPos = getNTPositionInProd(terminals, position);
  if (!hasTerminalAtIdx(terminals, terminalPos+1)) {
    result |= BARE_RIGHT;
  }
  if (terminalPos == 0 || !hasTerminalAtIdx(terminals, terminalPos-1)) {
    result |= BARE_LEFT;
  }
  return (Fixity) result;
}

/**
 * Compute whether there are terminals on the far left and far right of a
 * production.
 *
 * sym: the symbol to examine
 *
 * data: pretty printing metadata
 *
 * returns: EMPTY if there are terminals on both edges of this production.
 *          BARE_LEFT if there is a terminal only on the right edge of this production
 *          BARE_RIGHT if there is a terminal only on the left edge of this production
 *          BARE_BOTH if there are not terminals on either edge of this production
 */
Fixity getFixity(KORESymbol *sym, PrettyPrintData const& data) {
  std::string name = sym->getName();
  int result = EMPTY;
  if (data.terminals.at(name)[0] == '0') {
    result = result | BARE_LEFT;
  }
  if (data.terminals.at(name)[data.terminals.at(name).size()-1] == '0') {
    result = result | BARE_RIGHT;
  }
  return (Fixity)result;
}

/**
 * Compute the left capture of a pattern.
 *
 * If you were to unparse the AST of a term without any parentheses as a
 * sequence of terminals, start from the first terminal that is part of a
 * particular pattern (either directly or nested) and proceed left until you
 * reach the first terminal that is part of a pattern (directly) that is a
 * parent of that pattern, the pattern whose terminal you reach would be the
 * left capture of that pattern. Not all terms have a left capture; for
 * example, the left capture of the top term in an AST does not exist.
 *
 * previousLeftCapture: the left capture of `outer`
 *
 * outer: the parent of the symbol you wish to compute the left capture of
 *
 * position: the position within the nonterminals of `outer` of the pattern
 * you wish to compute the left capture of.
 *
 * data: pretty printing metadata
 *
 * returns: the left capture of the term, or NULL if no such term exists
 */ 
KORECompositePattern *getLeftCapture(KORECompositePattern *previousLeftCapture, KORECompositePattern *outer, int position, PrettyPrintData const& data) {
  Fixity fixity = getFixity(outer->getConstructor(), data);
  if (position == 0 && (fixity & BARE_LEFT)) {
    return previousLeftCapture;
  } else {
    return outer;
  }
}

/**
 * Compute the right capture of a pattern.
 *
 * If you were to unparse the AST of a term without any parentheses as a
 * sequence of terminals, start from the last terminal that is part of a
 * particular pattern (either directly or nested) and proceed right until you
 * reach the first terminal that is part of a pattern (directly) that is a
 * parent of that pattern, the pattern whose terminal you reach would be the
 * right capture of that pattern. Not all terms have a right capture; for
 * example, the right capture of the top term in an AST does not exist.
 *
 * previousRightCapture: the right capture of `outer`
 *
 * outer: the parent of the symbol you wish to compute the right capture of
 *
 * position: the position within the nonterminals of `outer` of the pattern
 * you wish to compute the right capture of.
 *
 * data: pretty printing metadata
 *
 * returns: the right capture of the term, or NULL if no such term exists
 */ 

KORECompositePattern *getRightCapture(KORECompositePattern *previousRightCapture, KORECompositePattern *outer, int position, PrettyPrintData const& data) {
  Fixity fixity = getFixity(outer->getConstructor(), data);
  if (position == (int)outer->getArguments().size() - 1 && (fixity & BARE_RIGHT)) {
    return previousRightCapture;
  } else {
    return outer;
  }
}

/**
 * Computes whether one sort is less than or equal to the other according to
 * the subsort relation of the definition.
 *
 * data: pretty printing metadata
 *
 * s1: first sort
 *
 * s2: second sort
 *
 * returns: s1 <= s2
 */
bool lessThanEq(PrettyPrintData const& data, KORESort *s1, KORESort *s2) {
  return *s1 == *s2 || (data.subsorts.count(s1) && data.subsorts.at(s1).count(s2));
}

sptr<KORESort> getArgSort(KORESymbol *symbol, int position, sptr<KORESort> firstArgSort) {
  if (!symbol->isBuiltin()) {
    return symbol->getArguments()[position];
  } else if (symbol->getName() == "\\and" ||
      symbol->getName() == "\\not" ||
      symbol->getName() == "\\or" ||
      symbol->getName() == "\\implies" ||
      symbol->getName() == "\\iff" ||
      symbol->getName() == "\\ceil" ||
      symbol->getName() == "\\floor" ||
      symbol->getName() == "\\equals" ||
      symbol->getName() == "\\in" ||
      symbol->getName() == "\\next" ||
      symbol->getName() == "\\rewrites") {
    return symbol->getFormalArguments()[0];
  } else if (symbol->getName() == "\\forall" ||
      symbol->getName() == "\\exists") {
    if (position == 0) {
      assert(firstArgSort != nullptr);
      return firstArgSort;
    } else {
      return symbol->getFormalArguments()[0];
    }
  } else if (symbol->getName() == "\\mu" ||
      symbol->getName() == "\\nu") {
    assert(firstArgSort != nullptr);
    return firstArgSort;
  } else {
    abort();
  }
}

sptr<KORESort> getReturnSort(KOREPattern *pat) {
  if (auto composite = dynamic_cast<KORECompositePattern *>(pat)) {
    auto symbol = composite->getConstructor();
    if (!symbol->isBuiltin()) {
      return pat->getSort();
    } else if (symbol->getName() == "\\top" ||
        symbol->getName() == "\\bottom" ||
        symbol->getName() == "\\and" ||
        symbol->getName() == "\\not" ||
        symbol->getName() == "\\or" ||
        symbol->getName() == "\\implies" ||
        symbol->getName() == "\\iff" ||
        symbol->getName() == "\\exists" ||
        symbol->getName() == "\\forall" ||
        symbol->getName() == "\\next" ||
        symbol->getName() == "\\rewrites") {
      return symbol->getFormalArguments()[0];
    } else if (symbol->getName() == "\\ceil" ||
        symbol->getName() == "\\floor" ||
        symbol->getName() == "\\equals" ||
        symbol->getName() == "\\in") {
      return symbol->getFormalArguments()[1];
    } else if (symbol->getName() == "\\mu" ||
        symbol->getName() == "\\nu") {
      return composite->getArguments()[0]->getSort();
    } else {
      abort();
    }
  } else {
    return pat->getSort();
  }
}

/**
 * Computes whether `inner` can appear at position `position` inside `outer`
 * according to the rules of subsorting, priority, and associativity. The
 * pattern does not actually have to exist at that position; we ask only
 * whether it `can` appear there.
 *
 * outer: the outer pattern
 *
 * inner: the inner pattern
 *
 * position: a position within `outer` you wish to test
 *
 * data: pretty printing metadata
 *
 * returns: true if priority forbids `inner` to appear inside `outer` at that
 * position.
 */
bool isPriorityWrong(KORECompositePattern *outer, KORECompositePattern *inner, int position, PrettyPrintData const& data) {
  std::string outerName = outer->getConstructor()->getName();
  std::string innerName = inner->getConstructor()->getName();
  KORESort *innerSort = getReturnSort(inner).get();
  KORESort *outerSort = getArgSort(outer->getConstructor(), position, outer->getArguments()[0]->getSort()).get();
  if (!lessThanEq(data, innerSort, outerSort)) {
    return true;
  }
  if (data.priorities.count(outerName) && data.priorities.at(outerName).count(innerName)) {
    return true;
  }
  std::string terminals = data.terminals.at(outerName);
  size_t terminalPos = getNTPositionInProd(terminals, position);
  if (data.leftAssoc.count(outerName) && data.leftAssoc.at(outerName).count(innerName) && terminalPos + 1 == terminals.size()) {
    return true;
  }
  if (data.rightAssoc.count(outerName) && data.rightAssoc.at(outerName).count(innerName) && terminalPos == 0) {
    return true;
  }
  return false;
}

/**
 * Computes whether a bracket needs to appear around `inner`.
 *
 * outer: the parent of `inner`
 *
 * leftCapture: the left capture of `inner`
 *
 * rightCapture: the right capture of `inner`
 *
 * inner: the pattern to test
 *
 * position: the position of `inner` within the nonterminals of `outer`
 *
 * data: pretty printing metadata
 *
 * returns: true if a bracket is required according to this algorithm. Roughly
 * speaking, parentheses are not needed around associative symbols, constants,
 * or patterns that appear between two terminals in their parent. Otherwise,
 * parentheses are needed if priority or associativity forbids the term inside
 * its parent, or if the term begins or ends with a nonterminal and priority
 * and associativity do not forbid the term from appearing inside its left
 * or right capture, respectively, and its left or right capture does not
 * end or begin, respectively, with a terminal.
 *
 * Warning: this algorithm is *unsound*. There is no guarantee that after 
 * it completes, the resulting term can be unparsed into a string that will
 * parse unambiguously as the original AST. Indeed, the problem of inserting
 * brackets into an AST in order to ensure this property is NP-hard, and 
 * grammars can be constructed such that solving the unparsing problem is 
 * equivalent to boolean satisfiability. This algorithm is merely a linear-time
 * approximation of the general problem which performs very well in most
 * real-world grammars, especially the common expression-grammar category of
 * grammar where parentheses are most often required in practice.
 *
 * As an example, consider the following simple grammar:
 *
 * syntax Exp ::= Exp "++" | "++" Exp | Exp Exp | "(" Exp ")" [bracket]
 *
 * The terms `(1 ++) 1` and `1 (++ 1)` will not have brackets inserted
 * into them by this algorithm, even though they are required.
 */
bool requiresBracketWithSimpleAlgorithm(KORECompositePattern *outer, KORECompositePattern *leftCapture, KORECompositePattern *rightCapture, KOREPattern *inner, int position, PrettyPrintData const& data) {
  if (auto innerComposite = dynamic_cast<KORECompositePattern *>(inner)) {
    std::string innerName = innerComposite->getConstructor()->getName();
    if (innerName == outer->getConstructor()->getName()) {
      if (data.assoc.count(innerName)) {
        return false;
      }
    }
    if (innerName == "\\dv") {
      return false;
    }
    Fixity fixity = getFixity(position, outer->getConstructor(), data);
    if (fixity == EMPTY) {
      return false;
    }
    bool priority = isPriorityWrong(outer, innerComposite, position, data);
    if (priority) {
      return true;
    }
    if (data.terminals.at(innerName) == "0") {
      return false;
    }
    Fixity innerFixity = getFixity(innerComposite->getConstructor(), data);
    if ((innerFixity & BARE_RIGHT) && rightCapture != nullptr) {
      bool inversePriority = isPriorityWrong(innerComposite, rightCapture, innerComposite->getArguments().size() - 1, data);
      Fixity rightCaptureFixity = getFixity(rightCapture->getConstructor(), data);
      if (!inversePriority && (rightCaptureFixity & BARE_LEFT)) {
        return true;
      }
    }
    if ((innerFixity & BARE_LEFT) && leftCapture != nullptr) {
      bool inversePriority = isPriorityWrong(innerComposite, leftCapture, 0, data);
      Fixity leftCaptureFixity = getFixity(leftCapture->getConstructor(), data);
      if (!inversePriority && (leftCaptureFixity & BARE_RIGHT)) {
        return true;
      }
    }
    return false;
  } else {
    return false;
  }
}

sptr<KOREPattern> addBrackets(sptr<KOREPattern> inner, KORECompositePattern *outer, KORECompositePattern *leftCapture, KORECompositePattern *rightCapture, int position, PrettyPrintData const& data) {
  if (auto innerComposite = dynamic_cast<KORECompositePattern *>(inner.get())) {
    if (innerComposite->getConstructor()->getName() == "inj") {
      return addBrackets(innerComposite->getArguments()[0], outer, leftCapture, rightCapture, position, data);
    }
  }
  if (requiresBracketWithSimpleAlgorithm(outer, leftCapture, rightCapture, inner.get(), position, data)) {
    sptr<KORESort> outerSort = getArgSort(outer->getConstructor(), position, outer->getArguments()[0]->getSort());
    sptr<KORESort> innerSort = getReturnSort(inner.get());
    for (auto &entry : data.brackets) {
      bool isCorrectOuterSort = lessThanEq(data, entry.first, outerSort.get());
      if (isCorrectOuterSort) {
        for (KORESymbol *s : entry.second) {
          bool isCorrectInnerSort = lessThanEq(data, innerSort.get(), getArgSort(s, 0, nullptr).get());
          if (isCorrectInnerSort) {
            sptr<KORECompositePattern> result = KORECompositePattern::Create(s);
            result->addArgument(inner);
            return result;
          }
        }
      }
    }
    sptr<KORECompositePattern> result = KORECompositePattern::Create("bracket");
    result->addArgument(inner);
    result->getConstructor()->addSort(innerSort);
    return result;
  }
  return inner;
}

sptr<KOREPattern> addBrackets(sptr<KOREPattern> t, KORECompositePattern *previousLeftCapture, KORECompositePattern *previousRightCapture, PrettyPrintData const& data) {
  if (auto outer = dynamic_cast<KORECompositePattern *>(t.get())) {
    if (outer->getConstructor()->getName() == "\\dv") {
      return t;
    }
    std::vector<sptr<KOREPattern>> newItems;

    sptr<KORECompositePattern> result = KORECompositePattern::Create(outer->getConstructor());
    size_t position = 0;
    for (auto &inner : outer->getArguments()) {
      KORECompositePattern *leftCapture = getLeftCapture(previousLeftCapture, outer, position, data);
      KORECompositePattern *rightCapture = getRightCapture(previousRightCapture, outer, position, data);
      sptr<KOREPattern> newInner = addBrackets(inner, outer, leftCapture, rightCapture, position, data);
      newInner = addBrackets(newInner, leftCapture, rightCapture, data);
      result->addArgument(newInner);
      position++;
    }
    return result;
  } else {
    return t;
  }
}

sptr<KOREPattern> addBrackets(sptr<KOREPattern> t, PrettyPrintData const& data) {
  return addBrackets(t, nullptr, nullptr, data);
}
