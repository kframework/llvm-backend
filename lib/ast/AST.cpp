#include "kllvm/ast/AST.h"

#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <algorithm>

using namespace kllvm;

std::string kllvm::decodeKore(std::string kore) {
  static std::unordered_map<std::string, char> codes;
  static bool once = true;
  if (once) {
    codes["Spce"] = ' ';
    codes["Bang"] = '!';
    codes["Quot"] = '"';
    codes["Hash"] = '#';
    codes["Dolr"] = '$';
    codes["Perc"] = '%';
    codes["And-"] = '&';
    codes["Apos"] = '\'';
    codes["LPar"] = '(';
    codes["RPar"] = ')';
    codes["Star"] = '*';
    codes["Plus"] = '+';
    codes["Comm"] = ',';
    codes["Hyph"] = '-';
    codes["Stop"] = '.';
    codes["Slsh"] = '/';
    codes["Coln"] = ':';
    codes["SCln"] = 'l';
    codes["-LT-"] = '<';
    codes["Eqls"] = '=';
    codes["-GT-"] = '>';
    codes["Ques"] = '?';
    codes["-AT-"] = '@';
    codes["LSqB"] = '[';
    codes["RSqB"] = ']';
    codes["Bash"] = '\\';
    codes["Xor-"] = '^';
    codes["Unds"] = '_';
    codes["BQuo"] = '`';
    codes["LBra"] = '{';
    codes["Pipe"] = '|';
    codes["RBra"] = '}';
    codes["Tild"] = '~';
    once = false;
  }
  bool literal = true;
  std::string result;
  size_t i = 0;
  while(i < kore.length()) {
    if (kore[i] == '\'') {
      literal = !literal;
      i++;
    } else if (literal) {
      result.push_back(kore[i]);
      i++;
    } else {  
      auto code = kore.substr(i, 4);
      result.push_back(codes[code]);
      i += 4;
    }
  }
  return result;
}

size_t kllvm::hash_value(const kllvm::KORESort &s) {
  return HashSort{}(s);
}

bool KORESortVariable::operator==(const KORESort &other) const {
  if (auto var = dynamic_cast<const KORESortVariable *>(&other)) {
    return var->name == name;
  }
  return false;
}

void KORECompositeSort::addArgument(sptr<KORESort> Argument) {
  arguments.push_back(Argument);
}

bool KORECompositeSort::operator==(const KORESort &other) const {
  if (auto sort = dynamic_cast<const KORECompositeSort *>(&other)) {
    if (sort->name != name || sort->arguments.size() != arguments.size()) {
      return false;
    }
    for (size_t i = 0; i < arguments.size(); ++i) {
      if (*sort->arguments[i] != *arguments[i]) return false;
    }
    return true;
  }
  return false;
}

sptr<KORESort> KORECompositeSort::substitute(const substitution &subst) {
  bool dirty = false;
  std::vector<sptr<KORESort>> newArgs;
  for (auto &arg : arguments) {
    auto newArg = arg->substitute(subst);
    if (newArg != arg) {
      dirty = true;
    }
    newArgs.push_back(newArg);
  }
  if (dirty) {
    sptr<KORECompositeSort> retval = Create(name);
    retval->arguments = newArgs;
    return retval;
  }
  return shared_from_this();
}

ValueType KORECompositeSort::getCategory(KOREDefinition *definition) {
  if (category.cat != SortCategory::Uncomputed)
    return category;
  std::string name = getHook(definition);
  if (name == "MINT.MInt") {
    if (auto param = dynamic_cast<KORECompositeSort *>(arguments[0].get())) {
      auto &att = definition->getSortDeclarations().at(param->getName())->getAttributes();
      auto &natAtt = att.at("nat");
      assert(natAtt->getArguments().size() == 1);
      auto strPattern = dynamic_cast<KOREStringPattern *>(natAtt->getArguments()[0].get());
      name = name + " " + strPattern->getContents();
    } else {
      print(std::cerr);
      abort();
    }
  }
  category = getCategory(name);
  return category;
}

std::string KORECompositeSort::getHook(KOREDefinition *definition) {
  auto &att = definition->getSortDeclarations().at(this->getName())->getAttributes();
  if (!att.count("hook")) {
    return "STRING.String";
  }
  auto &hookAtt = att.at("hook");
  assert(hookAtt->getArguments().size() == 1);
  auto strPattern = dynamic_cast<KOREStringPattern *>(hookAtt->getArguments()[0].get());
  return strPattern->getContents();
}

ValueType KORECompositeSort::getCategory(std::string name) {
  SortCategory category;
  uint64_t bits = 0;
  if (name == "MAP.Map") category = SortCategory::Map;
  else if (name == "LIST.List") category = SortCategory::List;
  else if (name == "SET.Set") category = SortCategory::Set;
  else if (name == "ARRAY.Array") category = SortCategory::Symbol; // ARRAY is implemented in K
  else if (name == "INT.Int") category = SortCategory::Int;
  else if (name == "FLOAT.Float") category = SortCategory::Float;
  else if (name == "BUFFER.StringBuffer") category = SortCategory::StringBuffer;
  else if (name == "BOOL.Bool") category = SortCategory::Bool;
  else if (name == "KVAR.KVar") category = SortCategory::Variable;
  // we expect the "hook" of a MInt to be of the form "MINT.MInt N" for some bitwidth N
  else if (name.substr(0, 10) == "MINT.MInt ") {
    category = SortCategory::MInt;
    bits = std::stoi(name.substr(10));
  }
  else category = SortCategory::Symbol;
  return {category, bits};
}

void KORESymbol::addArgument(sptr<KORESort> Argument) {
  arguments.push_back(Argument);
}

void KORESymbol::addFormalArgument(sptr<KORESort> Argument) {
  formalArguments.push_back(Argument);
}

void KORESymbol::addSort(sptr<KORESort> Sort) {
  sort = Sort;
}

bool KORESymbol::operator==(const KORESymbol &other) const {
  if (name != other.name || arguments.size() != other.arguments.size()) {
    return false;
  }
  for (size_t i = 0; i < arguments.size(); ++i) {
    if (*arguments[i] != *other.arguments[i]) return false;
  }
  return true;
}

std::string KORESymbol::layoutString(KOREDefinition *definition) const {
  std::string result;
  for (auto arg : arguments) {
    auto sort = dynamic_cast<KORECompositeSort *>(arg.get());
    ValueType cat = sort->getCategory(definition);
    switch(cat.cat) {
    case SortCategory::Map:
      result.push_back('1');
      break;
    case SortCategory::List:
      result.push_back('2');
      break;
    case SortCategory::Set:
      result.push_back('3');
      break;
    case SortCategory::Int:
      result.push_back('4');
      break;
    case SortCategory::Float:
      result.push_back('5');
      break;
    case SortCategory::StringBuffer:
      result.push_back('6');
      break;
    case SortCategory::Bool:
      result.push_back('7');
      break;
    case SortCategory::Variable:
      result.push_back('8');
      break;
    case SortCategory::MInt:
      result.append("_" + std::to_string(cat.bits) + "_");
    case SortCategory::Symbol:
      result.push_back('0');
      break;
    case SortCategory::Uncomputed:
      abort();
    }
  }
  return result;
}

bool KORECompositeSort::isConcrete() const {
  for (auto sort : arguments) {
    if (!sort->isConcrete()) {
      return false;
    }
  }
  return true;
}

bool KORESymbol::isConcrete() const {
  for (auto sort : arguments) {
    if (!sort->isConcrete()) {
      return false;
    }
  }
  return true;
}

bool KORESymbol::isPolymorphic() const {
  for (auto sort : arguments) {
    if (sort->isConcrete()) {
      return false;
    }
  }
  return true;
}

static std::unordered_set<std::string> BUILTINS{
  "\\and", "\\not", "\\or", "\\implies", "\\iff", "\\forall", "\\exists",
  "\\ceil", "\\floor", "\\equals", "\\in", "\\top", "\\bottom", "\\dv",
  "\\rewrites", "\\next", "\\mu", "\\nu"};

bool KORESymbol::isBuiltin() const {
  return BUILTINS.count(name);
}

void KORESymbol::instantiateSymbol(KORESymbolDeclaration *decl) {
  std::vector<sptr<KORESort>> instantiated;
  int i = 0;
  KORESort::substitution vars;
  for (auto var : decl->getObjectSortVariables()) {
    vars.emplace(*var, formalArguments[i++]);
  }
  for (auto sort : decl->getSymbol()->getArguments()) {
    instantiated.push_back(sort->substitute(vars));
  }
  auto returnSort = decl->getSymbol()->sort;
  sort = returnSort->substitute(vars);

  arguments = instantiated;
}

std::string KOREVariable::getName() const {
  return name;
}

std::string KOREVariablePattern::getName() const {
  return name->getName();
}

void KORECompositePattern::addArgument(sptr<KOREPattern> Argument) {
  arguments.push_back(Argument);
}

void KORECompositePattern::markSymbols(std::map<std::string, std::vector<KORESymbol *>> &map) {
  if (!constructor->isBuiltin()) {
    if (!map.count(constructor->getName())) {
      map.emplace(constructor->getName(), std::vector<KORESymbol *>{});
    }
    map.at(constructor->getName()).push_back(constructor.get());
  }
  for (auto &arg : arguments) {
    arg->markSymbols(map);
  }
}

void KORECompositePattern::markVariables(std::map<std::string, KOREVariablePattern *> &map) {
  for (auto &arg : arguments) {
    arg->markVariables(map);
  }
}

sptr<KOREPattern> KORECompositePattern::substitute(const substitution &subst) {
  if (arguments.empty()) {
    return shared_from_this();
  }
  auto ptr = KORECompositePattern::Create(constructor.get());
  auto name = constructor->getName();
  if (name == "\\forall" || name == "\\exists") {
    ptr->addArgument(arguments[0]);
    auto newSubst = subst;
    newSubst.erase(dynamic_cast<KOREVariablePattern *>(arguments[0].get())->getName());
    ptr->addArgument(arguments[1]->substitute(newSubst));
    return ptr;
  }
  for (auto &arg : arguments) {
    ptr->addArgument(arg->substitute(subst));
  }
  return ptr;
}

sptr<KOREPattern> KORECompositePattern::expandAliases(KOREDefinition *def) {
  if (def->getAliasDeclarations().count(constructor->getName())) {
    auto alias = def->getAliasDeclarations().at(constructor->getName());
    auto subst = alias->getSubstitution(this);
    return alias->getPattern()->substitute(subst)->expandAliases(def);
  }
  if (arguments.empty()) {
    return shared_from_this();
  }
  auto ptr = KORECompositePattern::Create(constructor.get());
  for (auto &arg : arguments) {
    ptr->addArgument(arg->expandAliases(def));
  }
  return ptr;
}

static int indent = 0;
static bool atNewLine = true;

#define INDENT_SIZE 2

static void newline(std::ostream &out) {
  out << std::endl;
  atNewLine = true;
}

static void printIndent(std::ostream &out) {
  if (atNewLine) {
    for (int i = 0; i < INDENT_SIZE * indent; i++) {
      out << ' ';
    }
    atNewLine = false;
  }
}

static void append(std::ostream &out, char c) {
  printIndent(out);
  out << c;
}

static void append(std::ostream &out, std::string str) {
  printIndent(out);
  out << str;
}

static void color(std::ostream &out, std::string color, PrettyPrintData const& data) {
  if (data.hasColor) {
    static bool once = true;
    static std::map<std::string, std::string> colors;
    if (once) {
      colors["MediumOrchid"] = "\x1b[38;5;134m";
      colors["Black"] = "\x1b[38;5;16m";
      colors["DarkSeaGreen"] = "\x1b[38;5;108m";
      colors["Sienna"] = "\x1b[38;5;130m";
      colors["Gainsboro"] = "\x1b[38;5;188m";
      colors["LightCoral"] = "\x1b[38;5;210m";
      colors["DarkGrey"] = "\x1b[38;5;145m";
      colors["DodgerBlue"] = "\x1b[38;5;33m";
      colors["CarnationPink"] = "\x1b[38;5;211m";
      colors["Aquamarine"] = "\x1b[38;5;122m";
      colors["Beige"] = "\x1b[38;5;230m";
      colors["OliveDrab"] = "\x1b[38;5;64m";
      colors["YellowOrange"] = "\x1b[38;5;214m";
      colors["Mulberry"] = "\x1b[38;5;126m";
      colors["Violet"] = "\x1b[38;5;213m";
      colors["LimeGreen"] = "\x1b[38;5;40m";
      colors["PaleGoldenrod"] = "\x1b[38;5;187m";
      colors["Magenta"] = "\x1b[38;5;201m";
      colors["PowderBlue"] = "\x1b[38;5;152m";
      colors["DarkTurquoise"] = "\x1b[38;5;44m";
      colors["IndianRed"] = "\x1b[38;5;167m";
      colors["LightGray"] = "\x1b[38;5;188m";
      colors["PeachPuff"] = "\x1b[38;5;223m";
      colors["LightBlue"] = "\x1b[38;5;152m";
      colors["ProcessBlue"] = "\x1b[38;5;39m";
      colors["SpringGreen"] = "\x1b[38;5;48m";
      colors["Indigo"] = "\x1b[38;5;54m";
      colors["RedViolet"] = "\x1b[38;5;125m";
      colors["DarkRed"] = "\x1b[38;5;88m";
      colors["Wheat"] = "\x1b[38;5;223m";
      colors["DarkCyan"] = "\x1b[38;5;30m";
      colors["yellow"] = "\x1b[38;5;226m";
      colors["LawnGreen"] = "\x1b[38;5;118m";
      colors["gray"] = "\x1b[38;5;102m";
      colors["DarkOrange"] = "\x1b[38;5;208m";
      colors["Teal"] = "\x1b[38;5;30m";
      colors["Maroon"] = "\x1b[38;5;88m";
      colors["JungleGreen"] = "\x1b[38;5;37m";
      colors["Blue"] = "\x1b[38;5;21m";
      colors["Periwinkle"] = "\x1b[38;5;104m";
      colors["Moccasin"] = "\x1b[38;5;223m";
      colors["black"] = "\x1b[38;5;16m";
      colors["Chocolate"] = "\x1b[38;5;166m";
      colors["teal"] = "\x1b[38;5;30m";
      colors["SeaGreen"] = "\x1b[38;5;29m";
      colors["Thistle"] = "\x1b[38;5;182m";
      colors["Red"] = "\x1b[38;5;196m";
      colors["blue"] = "\x1b[38;5;21m";
      colors["MistyRose"] = "\x1b[38;5;224m";
      colors["purple"] = "\x1b[38;5;161m";
      colors["Crimson"] = "\x1b[38;5;197m";
      colors["DarkGray"] = "\x1b[38;5;145m";
      colors["Fuchsia"] = "\x1b[38;5;201m";
      colors["DarkBlue"] = "\x1b[38;5;18m";
      colors["red"] = "\x1b[38;5;196m";
      colors["Cornsilk"] = "\x1b[38;5;230m";
      colors["white"] = "\x1b[38;5;231m";
      colors["DarkSlateGrey"] = "\x1b[38;5;23m";
      colors["PaleGreen"] = "\x1b[38;5;120m";
      colors["DimGray"] = "\x1b[38;5;59m";
      colors["lightgray"] = "\x1b[38;5;145m";
      colors["Gold"] = "\x1b[38;5;220m";
      colors["Gray"] = "\x1b[38;5;102m";
      colors["DarkOliveGreen"] = "\x1b[38;5;58m";
      colors["LemonChiffon"] = "\x1b[38;5;230m";
      colors["brown"] = "\x1b[38;5;137m";
      colors["NavyBlue"] = "\x1b[38;5;18m";
      colors["FloralWhite"] = "\x1b[38;5;231m";
      colors["LightGoldenrod"] = "\x1b[38;5;186m";
      colors["GreenYellow"] = "\x1b[38;5;154m";
      colors["Silver"] = "\x1b[38;5;145m";
      colors["Khaki"] = "\x1b[38;5;186m";
      colors["Ivory"] = "\x1b[38;5;231m";
      colors["LightSkyBlue"] = "\x1b[38;5;117m";
      colors["DarkGreen"] = "\x1b[38;5;22m";
      colors["DarkSalmon"] = "\x1b[38;5;173m";
      colors["TealBlue"] = "\x1b[38;5;37m";
      colors["Linen"] = "\x1b[38;5;231m";
      colors["LightGoldenrodYellow"] = "\x1b[38;5;230m";
      colors["LightGreen"] = "\x1b[38;5;120m";
      colors["LightGrey"] = "\x1b[38;5;188m";
      colors["Brown"] = "\x1b[38;5;124m";
      colors["Aqua"] = "\x1b[38;5;51m";
      colors["Cerulean"] = "\x1b[38;5;39m";
      colors["Peach"] = "\x1b[38;5;209m";
      colors["Bisque"] = "\x1b[38;5;223m";
      colors["MediumBlue"] = "\x1b[38;5;20m";
      colors["BlueViolet"] = "\x1b[38;5;93m";
      colors["RubineRed"] = "\x1b[38;5;198m";
      colors["Lavender"] = "\x1b[38;5;189m";
      colors["CornflowerBlue"] = "\x1b[38;5;68m";
      colors["Goldenrod"] = "\x1b[38;5;178m";
      colors["Grey"] = "\x1b[38;5;102m";
      colors["MediumSpringGreen"] = "\x1b[38;5;49m";
      colors["DarkKhaki"] = "\x1b[38;5;143m";
      colors["green"] = "\x1b[38;5;46m";
      colors["ForestGreen"] = "\x1b[38;5;28m";
      colors["DarkOrchid"] = "\x1b[38;5;128m";
      colors["White"] = "\x1b[38;5;231m";
      colors["Purple"] = "\x1b[38;5;90m";
      colors["DarkMagenta"] = "\x1b[38;5;90m";
      colors["BlueGreen"] = "\x1b[38;5;37m";
      colors["Green"] = "\x1b[38;5;28m";
      colors["Melon"] = "\x1b[38;5;216m";
      colors["SkyBlue"] = "\x1b[38;5;117m";
      colors["Rhodamine"] = "\x1b[38;5;205m";
      colors["Apricot"] = "\x1b[38;5;216m";
      colors["RedOrange"] = "\x1b[38;5;202m";
      colors["LightSlateGray"] = "\x1b[38;5;102m";
      colors["cyan"] = "\x1b[38;5;51m";
      colors["Orange"] = "\x1b[38;5;214m";
      colors["DarkSlateGray"] = "\x1b[38;5;23m";
      colors["DimGrey"] = "\x1b[38;5;59m";
      colors["LightSeaGreen"] = "\x1b[38;5;37m";
      colors["RoyalBlue"] = "\x1b[38;5;62m";
      colors["darkgray"] = "\x1b[38;5;59m";
      colors["Sepia"] = "\x1b[38;5;52m";
      colors["DarkViolet"] = "\x1b[38;5;92m";
      colors["MediumAquamarine"] = "\x1b[38;5;79m";
      colors["MediumSlateBlue"] = "\x1b[38;5;99m";
      colors["Dandelion"] = "\x1b[38;5;214m";
      colors["MidnightBlue"] = "\x1b[38;5;18m";
      colors["SandyBrown"] = "\x1b[38;5;215m";
      colors["violet"] = "\x1b[38;5;90m";
      colors["DarkSlateBlue"] = "\x1b[38;5;61m";
      colors["DeepSkyBlue"] = "\x1b[38;5;74m";
      colors["Chartreuse"] = "\x1b[38;5;118m";
      colors["Olive"] = "\x1b[38;5;100m";
      colors["MediumPurple"] = "\x1b[38;5;98m";
      colors["Yellow"] = "\x1b[38;5;226m";
      colors["Peru"] = "\x1b[38;5;173m";
      colors["RosyBrown"] = "\x1b[38;5;138m";
      colors["pink"] = "\x1b[38;5;217m";
      colors["FireBrick"] = "\x1b[38;5;124m";
      colors["RawSienna"] = "\x1b[38;5;124m";
      colors["VioletRed"] = "\x1b[38;5;162m";
      colors["OrangeRed"] = "\x1b[38;5;202m";
      colors["Bittersweet"] = "\x1b[38;5;130m";
      colors["Turquoise"] = "\x1b[38;5;80m";
      colors["Cyan"] = "\x1b[38;5;51m";
      colors["WhiteSmoke"] = "\x1b[38;5;231m";
      colors["MediumSeaGreen"] = "\x1b[38;5;35m";
      colors["LavenderBlush"] = "\x1b[38;5;231m";
      colors["LightCyan"] = "\x1b[38;5;195m";
      colors["PineGreen"] = "\x1b[38;5;29m";
      colors["OliveGreen"] = "\x1b[38;5;28m";
      colors["SlateGray"] = "\x1b[38;5;102m";
      colors["LightSlateBlue"] = "\x1b[38;5;99m";
      colors["NavajoWhite"] = "\x1b[38;5;223m";
      colors["SlateBlue"] = "\x1b[38;5;62m";
      colors["Orchid"] = "\x1b[38;5;170m";
      colors["Tan"] = "\x1b[38;5;180m";
      colors["LightSalmon"] = "\x1b[38;5;216m";
      colors["Seashell"] = "\x1b[38;5;231m";
      colors["Snow"] = "\x1b[38;5;231m";
      colors["WildStrawberry"] = "\x1b[38;5;197m";
      colors["Tomato"] = "\x1b[38;5;203m";
      colors["RoyalPurple"] = "\x1b[38;5;61m";
      colors["LightSlateGrey"] = "\x1b[38;5;102m";
      colors["Plum"] = "\x1b[38;5;182m";
      colors["YellowGreen"] = "\x1b[38;5;112m";
      colors["olive"] = "\x1b[38;5;100m";
      colors["MintCream"] = "\x1b[38;5;231m";
      colors["PaleVioletRed"] = "\x1b[38;5;168m";
      colors["Azure"] = "\x1b[38;5;231m";
      colors["BurntOrange"] = "\x1b[38;5;208m";
      colors["Salmon"] = "\x1b[38;5;210m";
      colors["BlanchedAlmond"] = "\x1b[38;5;223m";
      colors["Pink"] = "\x1b[38;5;217m";
      colors["AliceBlue"] = "\x1b[38;5;231m";
      colors["PapayaWhip"] = "\x1b[38;5;230m";
      colors["Honeydew"] = "\x1b[38;5;231m";
      colors["MediumTurquoise"] = "\x1b[38;5;44m";
      colors["AntiqueWhite"] = "\x1b[38;5;231m";
      colors["magenta"] = "\x1b[38;5;201m";
      colors["LightPink"] = "\x1b[38;5;217m";
      colors["OldLace"] = "\x1b[38;5;231m";
      colors["CadetBlue"] = "\x1b[38;5;73m";
      colors["BurlyWood"] = "\x1b[38;5;180m";
      colors["Lime"] = "\x1b[38;5;46m";
      colors["BrickRed"] = "\x1b[38;5;124m";
      colors["Emerald"] = "\x1b[38;5;37m";
      colors["LightSteelBlue"] = "\x1b[38;5;153m";
      colors["Mahogany"] = "\x1b[38;5;124m";
      colors["GhostWhite"] = "\x1b[38;5;231m";
      colors["SteelBlue"] = "\x1b[38;5;67m";
      colors["PaleTurquoise"] = "\x1b[38;5;159m";
      colors["DarkGoldenrod"] = "\x1b[38;5;136m";
      colors["lime"] = "\x1b[38;5;154m";
      colors["DeepPink"] = "\x1b[38;5;198m";
      colors["MediumVioletRed"] = "\x1b[38;5;162m";
      colors["orange"] = "\x1b[38;5;220m";
      colors["HotPink"] = "\x1b[38;5;205m";
      colors["LightYellow"] = "\x1b[38;5;230m";
      colors["Navy"] = "\x1b[38;5;18m";
      colors["SaddleBrown"] = "\x1b[38;5;94m";
      colors["Coral"] = "\x1b[38;5;209m";
      colors["SlateGrey"] = "\x1b[38;5;102m";
      once = false;
    }
    append(out, colors[color]);
  }
}

#define RESET_COLOR "\x1b[0m"

std::string enquote(std::string str) {
  std::string result;
  result.push_back('"');
  for (size_t i = 0; i < str.length(); ++i) {
    char c = str[i];
    switch(c) {
    case '\\':
      result.append("\\\\");
      break;
    case '"':
      result.append("\\\"");
      break;
    case '\n':
      result.append("\\n");
      break;
    case '\t':
      result.append("\\t");
      break;
    case '\r':
      result.append("\\r");
      break;
    case '\f':
      result.append("\\f");
      break;
    default:
      if ((unsigned char)c >= 32 && (unsigned char)c < 127) {
        result.push_back(c);
      } else {
        char buf[3];
        buf[2] = 0;
        snprintf(buf, 3, "%02x", (unsigned char)c);
        result.append("\\x");
        result.append(buf);
      }
      break;
    }
  }
  result.push_back('"');
  return result;
}

void KORESortVariable::prettyPrint(std::ostream &out) const {
  append(out, name);
}

void KORECompositeSort::prettyPrint(std::ostream &out) const {
  append(out, name.substr(4));
  if (!arguments.empty()) {
    append(out, '{');
    std::string conn = "";
    for (auto &sort : arguments) {
      append(out, conn);
      sort->prettyPrint(out);
      conn = ",";
    }
    append(out, '}');
  }
}

void KOREVariablePattern::prettyPrint(std::ostream &out, PrettyPrintData const& data) const {
  append(out, decodeKore(getName().substr(3)));
  append(out, ':');
  sort->prettyPrint(out);
}

void KORECompositePattern::prettyPrint(std::ostream &out, PrettyPrintData const& data) const {
  std::string name = getConstructor()->getName();
  if (name == "\\dv") {
    KORECompositeSort *s = dynamic_cast<KORECompositeSort *>(getConstructor()->getFormalArguments()[0].get()); 
    bool hasHook = data.hook.count(s->getName());
    auto str = dynamic_cast<KOREStringPattern *>(arguments[0].get());
    if (hasHook) {
      auto hook = data.hook.at(s->getName());
      if (hook == "STRING.String") {
        append(out, enquote(str->getContents()));
      } else if (hook == "BYTES.Bytes") {
        append(out, 'b');
        append(out, enquote(str->getContents()));
      } else {
        append(out, str->getContents());
      }
    } else {
      append(out, str->getContents());
    }
    return;
  }
  if (data.format.count(name)) {
    auto format = data.format.at(name);
    int localIndent = 0;
    int localColor = 0;
    for (size_t i = 0; i < format.length(); ++i) {
      char c = format[i];
      if (c == '%') {
        if (i == format.length() - 1) {
          abort();
        }
        char c2 = format[i+1];
        ++i;
        switch(c2) {
          case 'n':
            newline(out);
            break;
          case 'i':
            indent++;
            localIndent++;
            break;
          case 'd':
            indent--;
            localIndent--;
            break;
          case 'c':
            if (data.colors.count(name)) {
              if ((size_t)localColor >= data.colors.at(name).size() ) {
                abort();
              }
              color(out, data.colors.at(name)[localColor++], data);
            }
            break;
          case 'r':
            if (data.hasColor) {
              append(out, RESET_COLOR);
            }
            break;
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9': {
            std::string buf;
            for(; i < format.length() && format[i] >= '0' && format[i] <= '9'; i++) {
              buf.push_back(format[i]);
            }
            i--;
            int idx = std::stoi(buf);
            if (idx == 0 || (size_t)idx > arguments.size()) {
              abort();
            }
            KOREPattern *inner = arguments[idx-1].get();
            bool assoc = false;
            if (auto app = dynamic_cast<KORECompositePattern *>(inner)) {
              if (app->getConstructor()->getName() == constructor->getName() && data.assoc.count(name)) {
                assoc = true;
              }
              if (assoc) {
                for (int j = 0; j < localIndent; j++) {
                  indent--;
                }
              }
              inner->prettyPrint(out, data);
              if (assoc) {
                for (int j = 0; j < localIndent; j++) {
                  indent++;
                }
              }
            } else {
              inner->prettyPrint(out, data);
            }
            break;
          } default:
            append(out, c2);
        }
      } else {
        append(out, c);
      }
    }
  } else {
    abort();
  }
}

struct CompareFirst {
  bool isDigit(char c) {
    return c >= '0' && c <= '9';
  }

  std::string getChunk(std::string s, size_t slength, size_t marker) {
    std::string chunk;
    char c = s[marker];
    chunk.push_back(c);
    marker++;
    if (isDigit(c)) {
      while (marker < slength) {
        c = s[marker];
        if (!isDigit(c)) {
          break;
        }
        chunk.push_back(c);
        marker++;
      }
    } else {
      while (marker < slength) {
        c = s[marker];
        if (isDigit(c)) {
          break;
        }
        chunk.push_back(c);
        marker++;
      }
    }
    return chunk;
  }

  template <typename T>
  bool operator()(std::pair<std::string, T> a, std::pair<std::string, T> b) {
    std::string s1 = a.first;
    std::string s2 = b.first;
    size_t thisMarker = 0;
    size_t thatMarker = 0;
    size_t s1length = s1.length();
    size_t s2length = s2.length();
    while (thisMarker < s1length && thatMarker < s2length) {
      std::string thisChunk = getChunk(s1, s1length, thisMarker);
      thisMarker += thisChunk.length();
      std::string thatChunk = getChunk(s2, s2length, thatMarker);
      thatMarker += thatChunk.length();
      int result = 0;
      if (isDigit(thisChunk[0]) && isDigit(thatChunk[0])) {
        size_t thisChunkLength = thisChunk.length();
        result = thisChunkLength - thatChunk.length();
        if (result == 0) {
          for (size_t i = 0; i < thisChunkLength; i++) {
            result = thisChunk[i] - thatChunk[i];
            if (result != 0) {
              return result < 0;
            }
          }
        }
      } else {
        result = thisChunk.compare(thatChunk);
      }
      if (result != 0) {
        return result < 0;
      }
    }
    return s1length < s2length;
  }
};

static void flatten(KORECompositePattern *pat, std::string name, std::vector<sptr<KOREPattern>> &result) {
  for (auto &arg : pat->getArguments()) {
    if (auto pat2 = dynamic_cast<KORECompositePattern *>(arg.get())) {
      if (pat2->getConstructor()->getName() == name) {
        flatten(pat2, name, result);
      } else {
        result.push_back(arg);
      }
    } else {
      result.push_back(arg);
    }
  }
}

sptr<KOREPattern> KORECompositePattern::sortCollections(PrettyPrintData const& data) {
  if (arguments.empty()) {
    return shared_from_this();
  }
  std::string name = getConstructor()->getName();
  if (data.comm.count(name) && data.assoc.count(name)) {
    std::vector<sptr<KOREPattern>> items;
    flatten(this, name, items);
    std::vector<std::pair<std::string, sptr<KOREPattern>>> printed;
    int oldIndent = indent;
    bool oldAtNewLine = atNewLine;
    atNewLine = true;
    indent = 0;
    PrettyPrintData newData = data;
    newData.hasColor = false;
    for (auto &item : items) {
      std::ostringstream Out;
      item->prettyPrint(Out, newData);
      printed.push_back({Out.str(), item});
    }
    indent = oldIndent;
    atNewLine = oldAtNewLine;
    std::sort(printed.begin(), printed.end(), CompareFirst{});
    items.clear();
    for (auto &item : printed) {
      items.push_back(item.second);
    }
    sptr<KOREPattern> result = items[0];
    for (size_t i = 1; i < items.size(); ++i) {
      sptr<KORECompositePattern> tmp = KORECompositePattern::Create(constructor.get());
      tmp->addArgument(result);
      tmp->addArgument(items[i]);
      result = tmp;
    }
    return result;
  }
  sptr<KORECompositePattern> result = KORECompositePattern::Create(constructor.get());
  for (auto &arg : arguments) {
    result->addArgument(arg->sortCollections(data));
  }
  return result;
}

std::set<std::string> KOREPattern::gatherSingletonVars(void) {
  auto counts = gatherVarCounts(); 
  std::set<std::string> result;
  for (auto entry : counts) {
    if (entry.second == 1) {
      result.insert(entry.first);
    }
  }
  return result;
}

std::map<std::string, int> KORECompositePattern::gatherVarCounts(void) {
  std::map<std::string, int> result;
  for (auto &arg : arguments) {
    auto childResult = arg->gatherVarCounts();
    for (auto entry : childResult) {
      result[entry.first] += entry.second;
    }
  }
  return result;
}

sptr<KOREPattern> KORECompositePattern::filterSubstitution(PrettyPrintData const& data, std::set<std::string> const& vars) {
  if (constructor->getName() == "\\equals") {
    if (auto var = dynamic_cast<KOREVariablePattern *>(arguments[0].get())) {
      std::ostringstream ss;
      int oldIndent = indent;
      bool oldAtNewLine = atNewLine;
      atNewLine = true;
      indent = 0;
      var->prettyPrint(ss, data);
      indent = oldIndent;
      atNewLine = oldAtNewLine;
      std::string name = ss.str();
      if (vars.count(var->getName()) && (name[0] == '_' || (name.size() > 1 && (name[0] == '@' || name[0] == '!' || name[0] == '?') && name[1] == '_'))) {
        sptr<KORECompositePattern> unit = KORECompositePattern::Create("\\top");
        unit->getConstructor()->addFormalArgument(constructor->getFormalArguments()[1]);
        return unit;
      } else {
        return shared_from_this();
      }
    } else {
      return shared_from_this();
    }
  } else if (constructor->getName() == "\\and" || constructor->getName() == "\\or") {
    sptr<KORECompositePattern> result = KORECompositePattern::Create(constructor.get());
    for (auto &arg : arguments) {
      if (constructor->getName() == "\\or") {
        std::set<std::string> vars = arg->gatherSingletonVars();
        result->addArgument(arg->filterSubstitution(data, vars));
      } else {
        result->addArgument(arg->filterSubstitution(data, vars));
      }
    }
    if (constructor->getName() == "\\and") {
     if (auto composite = dynamic_cast<KORECompositePattern *>(result->getArguments()[0].get())) {
        if (composite->getConstructor()->getName() == "\\top") {
          return result->getArguments()[1];
        }
      }
      if (auto composite = dynamic_cast<KORECompositePattern *>(result->getArguments()[1].get())) {
        if (composite->getConstructor()->getName() == "\\top") {
          return result->getArguments()[0];
        }
      }
    }
    return result;
  }
  return shared_from_this();
}

sptr<KOREPattern> KORECompositePattern::expandMacros(SubsortMap const& subsorts, SymbolMap const& overloads, std::vector<ptr<KOREDeclaration>> const& macros, bool reverse, std::set<size_t> &appliedRules) {
  sptr<KORECompositePattern> applied = KORECompositePattern::Create(constructor.get());
  for (auto &arg : arguments) {
    applied->addArgument(arg->expandMacros(subsorts, overloads, macros, reverse));
  }

  size_t i = 0;
  for (auto &decl : macros) {
    if ((decl->getAttributes().count("macro") || decl->getAttributes().count("macro-rec")) && reverse) {
      i++;
      continue;
    }
    auto axiom = dynamic_cast<KOREAxiomDeclaration *>(decl.get());
    auto equals = dynamic_cast<KORECompositePattern *>(axiom->getPattern().get());
    auto lhs = equals->arguments[reverse ? 1 : 0];
    auto rhs = equals->arguments[reverse ? 0 : 1];
    substitution subst;
    bool matches = lhs->matches(subst, subsorts, overloads, applied);
    if (matches && (decl->getAttributes().count("macro-rec") || decl->getAttributes().count("alias-rec") || !appliedRules.count(i))) {
      std::set<size_t> oldAppliedRules = appliedRules;
      appliedRules.insert(i);
      auto result = rhs->substitute(subst)->expandMacros(subsorts, overloads, macros, reverse, appliedRules);
      appliedRules = oldAppliedRules;
      return result;
    }
    i++;
  }
  return applied;
}

bool KOREVariablePattern::matches(substitution &subst, SubsortMap const& subsorts, SymbolMap const& overloads, sptr<KOREPattern> subject) {
  if (subst[name->getName()]) {
    std::ostringstream Out1, Out2;
    subst[name->getName()]->print(Out1);
    subject->print(Out2);
    return Out1.str() == Out2.str();
  } else {
    subst[name->getName()] = subject;
    return true;
  }
}

bool KORECompositePattern::matches(substitution &subst, SubsortMap const& subsorts, SymbolMap const& overloads, sptr<KOREPattern> subject) {
  auto subj = dynamic_cast<KORECompositePattern *>(subject.get());
  if (!subj) {
    return false;
  }
  if (*subj->getConstructor() != *getConstructor()) {
    if (subj->getConstructor()->getName() == "inj" && getConstructor()->getName() == "inj") {
      if (*subj->getConstructor()->getFormalArguments()[1] != *getConstructor()->getFormalArguments()[1]) {
        return false;
      }
      sptr<KORESort> a = subj->getConstructor()->getFormalArguments()[0];
      sptr<KORESort> b = getConstructor()->getFormalArguments()[0];
      if (subsorts.count(b.get()) && subsorts.at(b.get()).count(a.get())) {
        sptr<KORECompositePattern> ba = KORECompositePattern::Create("inj");
        ba->getConstructor()->addFormalArgument(b);
        ba->getConstructor()->addFormalArgument(a);
        ba->getConstructor()->addArgument(b);
        ba->addArgument(arguments[0]);
        return ba->matches(subst, subsorts, overloads, subj->getArguments()[0]);
      } else if (subsorts.count(a.get()) && subsorts.at(a.get()).count(b.get())) {
        sptr<KORECompositePattern> ab = KORECompositePattern::Create("inj");
        ab->getConstructor()->addFormalArgument(a);
        ab->getConstructor()->addFormalArgument(b);
        ab->getConstructor()->addArgument(a);
        ab->addArgument(subj->getArguments()[0]);
        return arguments[0]->matches(subst, subsorts, overloads, ab);
      } else {
        return false;
      }
    } else if (subj->getConstructor()->getName() == "inj") { 
      sptr<KOREPattern> child = subj->getArguments()[0];
      if (auto composite = dynamic_cast<KORECompositePattern *>(child.get())) {
        if (overloads.count(composite->getConstructor()) && overloads.at(composite->getConstructor()).count(getConstructor())) {
          sptr<KORECompositePattern> greater = KORECompositePattern::Create(getConstructor());
          for (int i = 0; i < arguments.size(); i++) {
            if (*getConstructor()->getArguments()[i] != *composite->getConstructor()->getArguments()[i]) {
              sptr<KORECompositePattern> inj = KORECompositePattern::Create("inj");
              inj->getConstructor()->addFormalArgument(composite->getConstructor()->getArguments()[i]);
              inj->getConstructor()->addFormalArgument(getConstructor()->getArguments()[i]);
              inj->getConstructor()->addArgument(composite->getConstructor()->getArguments()[i]);
              inj->addArgument(composite->getArguments()[i]);
              greater->addArgument(inj);
            } else {
              greater->addArgument(composite->getArguments()[i]);
            }
          }
          return this->matches(subst, subsorts, overloads, greater);
        } else {
          return false;
        }
      } else {
        return false;
      }
    } else {
      return false;
    }
  }
  if (subj->arguments.size() != arguments.size()) {
    return false;
  }
  bool match = true;
  for (int i = 0; i < subj->arguments.size(); i++) {
    match = match && arguments[i]->matches(subst, subsorts, overloads, subj->arguments[i]);
  }
  return match;
}

bool KOREStringPattern::matches(substitution &subst, SubsortMap const& subsorts, SymbolMap const& overloads, sptr<KOREPattern> subject) {
  auto subj = dynamic_cast<KOREStringPattern *>(subject.get());
  if (!subj) {
    return false;
  }
  return subj->contents == contents;
}

void KOREDeclaration::addAttribute(ptr<KORECompositePattern> Attribute) {
  std::string name = Attribute->getConstructor()->getName();
  attributes.insert({name, std::move(Attribute)});
}

void
KOREDeclaration::addObjectSortVariable(sptr<KORESortVariable> SortVariable) {
  objectSortVariables.push_back(SortVariable);
}

std::string KOREDeclaration::getStringAttribute(std::string name) const { 
  KORECompositePattern *attr = attributes.at(name).get();
  assert(attr->getArguments().size() == 1);
  auto strPattern = dynamic_cast<KOREStringPattern *>(attr->getArguments()[0].get());
  return strPattern->getContents();
}
 

void KOREAxiomDeclaration::addPattern(ptr<KOREPattern> Pattern) {
  pattern = std::move(Pattern);
}

static const std::string ASSOC = "assoc";
static const std::string COMM = "comm";
static const std::string IDEM = "idem";
static const std::string UNIT = "unit";
static const std::string FUNCTIONAL = "functional";
static const std::string SUBSORT = "subsort";
static const std::string CONSTRUCTOR = "constructor";
static const std::string CEIL = "ceil";

bool KOREAxiomDeclaration::isRequired() {
  return !attributes.count(ASSOC) && !attributes.count(COMM) && !attributes.count(IDEM) && !attributes.count(UNIT) && !attributes.count(FUNCTIONAL) && !attributes.count(CONSTRUCTOR) && !attributes.count(SUBSORT) && !attributes.count(CEIL);
}

bool KOREAxiomDeclaration::isTopAxiom() {
  if (auto top = dynamic_cast<KORECompositePattern *>(pattern.get())) {
    if (top->getConstructor()->getName() == "\\implies" && top->getArguments().size() == 2) {
      if (auto bottomPattern = dynamic_cast<KORECompositePattern *>(top->getArguments()[0].get())) {
        if (bottomPattern->getConstructor()->getName() == "\\bottom" && bottomPattern->getArguments().empty()) {
          return true;
        }
      }
      return false;
    } else if (top->getConstructor()->getName() == "\\rewrites" && top->getArguments().size() == 2) {
      return true;
    } else if (top->getConstructor()->getName() == "\\and" && top->getArguments().size() == 2) {
      return true;
    }
  }
  return false;
}

KOREPattern *KOREAxiomDeclaration::getRightHandSide() const {
  if (auto top = dynamic_cast<KORECompositePattern *>(pattern.get())) {
    if (top->getConstructor()->getName() == "\\implies" && top->getArguments().size() == 2) {
      if (auto andPattern = dynamic_cast<KORECompositePattern *>(top->getArguments()[1].get())) {
        if (andPattern->getConstructor()->getName() == "\\and" && andPattern->getArguments().size() == 2) {
          if (auto bottomPattern = dynamic_cast<KORECompositePattern *>(top->getArguments()[0].get())) {
            if (bottomPattern->getConstructor()->getName() == "\\bottom" && bottomPattern->getArguments().empty()) {
              if (auto andPattern2 = dynamic_cast<KORECompositePattern *>(andPattern->getArguments()[1].get())) {
                if (andPattern2->getConstructor()->getName() == "\\and" && andPattern2->getArguments().size() == 2) {
                  if (auto rewrites = dynamic_cast<KORECompositePattern *>(andPattern2->getArguments()[1].get())) {
                    if (rewrites->getConstructor()->getName() == "\\rewrites" && rewrites->getArguments().size() == 2) {
                      return rewrites->getArguments()[1].get();
                    }
                  }
                } else if (andPattern2->getConstructor()->getName() == "\\rewrites" && andPattern2->getArguments().size() == 2) {
                  if (auto andPattern3 = dynamic_cast<KORECompositePattern *>(andPattern2->getArguments()[1].get())) {
                    if (andPattern3->getConstructor()->getName() == "\\and" && andPattern3->getArguments().size() == 2) {
                      return andPattern3->getArguments()[1].get();
                    }
                  }
                }
              }
            } else if (auto equals = dynamic_cast<KORECompositePattern *>(andPattern->getArguments()[0].get())) {
              if (equals->getConstructor()->getName() == "\\equals" && equals->getArguments().size() == 2) {
                return equals->getArguments()[1].get();
              }
            }
          }
        } else if (andPattern->getConstructor()->getName() == "\\rewrites" && andPattern->getArguments().size() == 2) {
          if (auto andPattern2 = dynamic_cast<KORECompositePattern *>(andPattern->getArguments()[1].get())) {
            if (andPattern2->getConstructor()->getName() == "\\and" && andPattern2->getArguments().size() == 2) {
              return andPattern2->getArguments()[1].get();
            }
          }
        }
      }
    } else if (top->getConstructor()->getName() == "\\and" && top->getArguments().size() == 2) {
      if (auto andPattern = dynamic_cast<KORECompositePattern *>(top->getArguments()[1].get())) {
        if (andPattern->getConstructor()->getName() == "\\and" && andPattern->getArguments().size() == 2) {
          if (auto rewrites = dynamic_cast<KORECompositePattern *>(andPattern->getArguments()[1].get())) {
            if (rewrites->getConstructor()->getName() == "\\rewrites" && rewrites->getArguments().size() == 2) {
              return rewrites->getArguments()[1].get();
            }
          }
        }
      }
    } else if (top->getConstructor()->getName() == "\\equals" && top->getArguments().size() == 2) {
      return top->getArguments()[1].get();
    } else if (top->getConstructor()->getName() == "\\rewrites" && top->getArguments().size() == 2) {
      if (auto andPattern = dynamic_cast<KORECompositePattern *>(top->getArguments()[1].get())) {
        if (andPattern->getConstructor()->getName() == "\\and" && andPattern->getArguments().size() == 2) {
          return andPattern->getArguments()[1].get();
        }
      }
    }
  }
  assert(false && "could not compute right hand side of axiom");
  abort();
}

KOREPattern *KOREAxiomDeclaration::getRequires() const {
  if (auto top = dynamic_cast<KORECompositePattern *>(pattern.get())) {
    if (top->getConstructor()->getName() == "\\implies" && top->getArguments().size() == 2) {
      if (auto outer = dynamic_cast<KORECompositePattern *>(top->getArguments()[0].get())) {
        if (outer->getConstructor()->getName() == "\\and" && outer->getArguments().size() == 2) {
          if (auto _not = dynamic_cast<KORECompositePattern *>(outer->getArguments()[0].get())) {
            if (_not->getConstructor()->getName() == "\\not" && _not->getArguments().size() == 1) {
              outer = dynamic_cast<KORECompositePattern *>(outer->getArguments()[1].get());
              assert(outer);
            }
          }
        }
        if (outer->getConstructor()->getName() == "\\equals" && outer->getArguments().size() == 2) {
          return outer->getArguments()[0].get();
        } else if (outer->getConstructor()->getName() == "\\top" && outer->getArguments().empty()) {
          return nullptr;
        } else if (outer->getConstructor()->getName() == "\\bottom" && outer->getArguments().empty()) {
          // strategy axiom hack
          if (auto trueTop = dynamic_cast<KORECompositePattern *>(top->getArguments()[1].get())) {
            if (trueTop->getConstructor()->getName() == "\\and" && trueTop->getArguments().size() == 2) {
              if (auto equals = dynamic_cast<KORECompositePattern *>(trueTop->getArguments()[0].get())) {
                if (equals->getConstructor()->getName() == "\\equals" && equals->getArguments().size() == 2) {
                  return equals->getArguments()[0].get();
                } else if (equals->getConstructor()->getName() == "\\top" && equals->getArguments().empty()) {
                  return nullptr;
                }
              }
            } else if (trueTop->getConstructor()->getName() == "\\rewrites" && trueTop->getArguments().size() == 2) {
              if (auto andPattern = dynamic_cast<KORECompositePattern *>(trueTop->getArguments()[0].get())) {
                if (andPattern->getConstructor()->getName() == "\\and" && andPattern->getArguments().size() == 2) {
                  if (auto equals = dynamic_cast<KORECompositePattern *>(andPattern->getArguments()[0].get())) {
                    if (equals->getConstructor()->getName() == "\\equals" && equals->getArguments().size() == 2) {
                      return equals->getArguments()[0].get();
                    } else if (equals->getConstructor()->getName() == "\\top" && equals->getArguments().empty()) {
                      return nullptr;
                    }
                  }
                }
              }
            }
          }
        } else if (outer->getConstructor()->getName() == "\\and" && outer->getArguments().size() == 2) {
          if (auto inner = dynamic_cast<KORECompositePattern *>(outer->getArguments()[0].get())) {
            if (inner->getConstructor()->getName() == "\\top" && inner->getArguments().empty()) {
              return nullptr;
            } else if (inner->getConstructor()->getName() == "\\equals" && inner->getArguments().size() == 2) {
              return inner->getArguments()[0].get();
            }
          }
        }
      }
    } else if (top->getConstructor()->getName() == "\\and" && top->getArguments().size() == 2) {
      if (auto equals = dynamic_cast<KORECompositePattern *>(top->getArguments()[0].get())) {
        if (equals->getConstructor()->getName() == "\\equals" && equals->getArguments().size() == 2) {
          return equals->getArguments()[0].get();
        } else if (equals->getConstructor()->getName() == "\\top" && equals->getArguments().empty()) {
          return nullptr;
        }
      }
    } else if (top->getConstructor()->getName() == "\\equals" && top->getArguments().size() == 2) {
      return nullptr;
    } else if (top->getConstructor()->getName() == "\\rewrites" && top->getArguments().size() == 2) {
      if (auto andPattern = dynamic_cast<KORECompositePattern *>(top->getArguments()[0].get())) {
        if (andPattern->getConstructor()->getName() == "\\and" && andPattern->getArguments().size() == 2) {
          if (auto equals = dynamic_cast<KORECompositePattern *>(andPattern->getArguments()[0].get())) {
            if (equals->getConstructor()->getName() == "\\equals" && equals->getArguments().size() == 2) {
              return equals->getArguments()[0].get();
            } else if (equals->getConstructor()->getName() == "\\top" && equals->getArguments().empty()) {
              return nullptr;
            } else if (equals->getConstructor()->getName() == "\\not" && equals->getArguments().size() == 1) {
              if (auto andPattern2 = dynamic_cast<KORECompositePattern *>(andPattern->getArguments()[1].get())) {
                if (andPattern2->getConstructor()->getName() == "\\and" && andPattern2->getArguments().size() == 2) {
                  if (auto equals2 = dynamic_cast<KORECompositePattern *>(andPattern2->getArguments()[0].get())) {
                    if (equals2->getConstructor()->getName() == "\\equals" && equals2->getArguments().size() == 2) {
                      return equals2->getArguments()[0].get();
                    } else if (equals2->getConstructor()->getName() == "\\top" && equals2->getArguments().empty()) {
                      return nullptr;
		    }
		  }
		}
	      }
	    }
          }
        }
      }
    }
  }
  assert(false && "ill-formed axiom");
  abort();
}


void
KOREAliasDeclaration::addVariables(ptr<KORECompositePattern> Variables) {
  boundVariables = std::move(Variables);
}

void KOREAliasDeclaration::addPattern(ptr<KOREPattern> Pattern) {
  pattern = std::move(Pattern);
}

KOREPattern::substitution KOREAliasDeclaration::getSubstitution(KORECompositePattern *subject) {
  int i = 0;
  KOREPattern::substitution result;
  for (auto &arg : boundVariables->getArguments()) {
    KOREVariablePattern *var = dynamic_cast<KOREVariablePattern *>(arg.get());
    if (!var) {
      abort();
    }
    result[var->getName()] = subject->getArguments()[i++];
  }
  return result;
}

bool KORESymbolDeclaration::isAnywhere() {
  return getAttributes().count("anywhere");
}

void KOREModule::addAttribute(ptr<KORECompositePattern> Attribute) {
  std::string name = Attribute->getConstructor()->getName();
  attributes.insert({name, std::move(Attribute)});
}

void KOREModule::addDeclaration(ptr<KOREDeclaration> Declaration) {
  declarations.push_back(std::move(Declaration));
}

void KOREDefinition::addModule(ptr<KOREModule> Module) {
  for (auto &decl : Module->getDeclarations()) {
    if (auto sortDecl = dynamic_cast<KORECompositeSortDeclaration *>(decl.get())) {
      sortDeclarations.insert({sortDecl->getName(), sortDecl});
      auto sort = KORECompositeSort::Create(sortDecl->getName());
    } else if (auto symbolDecl = dynamic_cast<KORESymbolDeclaration *>(decl.get())) {
      symbolDeclarations.insert({symbolDecl->getSymbol()->getName(), symbolDecl});
    } else if (auto aliasDecl = dynamic_cast<KOREAliasDeclaration *>(decl.get())) {
      aliasDeclarations.insert({aliasDecl->getSymbol()->getName(), aliasDecl});
    } else if (auto axiom = dynamic_cast<KOREAxiomDeclaration *>(decl.get())) {
      axioms.push_back(axiom);
    }
  }
  modules.push_back(std::move(Module));
}

void KOREDefinition::addAttribute(ptr<KORECompositePattern> Attribute) {
  std::string name = Attribute->getConstructor()->getName();
  attributes.insert({name, std::move(Attribute)});
}

void KOREDefinition::preprocess() {
  for (auto iter = axioms.begin(); iter != axioms.end(); ++iter) {
    auto axiom = *iter;
    axiom->pattern = axiom->pattern->expandAliases(this);
  }
  auto symbols = std::map<std::string, std::vector<KORESymbol *>>{};
  unsigned nextOrdinal = 0;
  for (auto iter = symbolDeclarations.begin(); iter != symbolDeclarations.end(); ++iter) {
    auto decl = *iter;
    if (decl.second->getAttributes().count("freshGenerator")) {
      auto sort = decl.second->getSymbol()->getSort();
      if (sort->isConcrete()) {
        freshFunctions[dynamic_cast<KORECompositeSort *>(sort.get())->getName()] = decl.second->getSymbol();
      }
    }
  }
  for (auto iter = axioms.begin(); iter != axioms.end();) {
    auto axiom = *iter;
    axiom->ordinal = nextOrdinal;
    ordinals[nextOrdinal++] = axiom;
    axiom->pattern->markSymbols(symbols);
    if (!axiom->isRequired()) {
      iter = axioms.erase(iter);
    } else {
      ++iter;
    }
  }
  for (auto moditer = modules.begin(); moditer != modules.end(); ++moditer) {
    auto &declarations = (*moditer)->getDeclarations();
    for (auto iter = declarations.begin(); iter != declarations.end(); ++iter) {
      KORESymbolDeclaration * decl = dynamic_cast<KORESymbolDeclaration *>(iter->get());
      if (decl == nullptr) {
        continue;
      }
      if (decl->isHooked() && decl->getObjectSortVariables().empty()) {
        KORESymbol * symbol = decl->getSymbol();
        symbols.emplace(symbol->getName(), std::vector<KORESymbol *>{symbol});
      }
    }
  }
  for (auto iter = symbols.begin(); iter != symbols.end(); ++iter) {
    auto entry = *iter;
    for (auto iter = entry.second.begin(); iter != entry.second.end(); ++iter) {
      KORESymbol *symbol = *iter;
      auto decl = symbolDeclarations.at(symbol->getName());
      symbol->instantiateSymbol(decl);
    }
  }
  uint32_t nextSymbol = 0;
  uint16_t nextLayout = 1;
  auto instantiations = std::unordered_map<KORESymbol, uint32_t, HashSymbol>{};
  auto layouts = std::unordered_map<std::string, uint16_t>{};
  auto variables = std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>{};
  for (auto iter = symbols.begin(); iter != symbols.end(); ++iter) {
    auto entry = *iter;
    uint32_t firstTag = nextSymbol;
    for (auto iter = entry.second.begin(); iter != entry.second.end(); ++iter) {
      KORESymbol *symbol = *iter;
      if (symbol->isConcrete()) {
        if (!instantiations.count(*symbol)) {
          instantiations.emplace(*symbol, nextSymbol++);
        }
        std::string layoutStr = symbol->layoutString(this);
        if (!layouts.count(layoutStr)) {
          layouts.emplace(layoutStr, nextLayout++);
        }
        symbol->firstTag = symbol->lastTag = instantiations.at(*symbol);
        symbol->layout = layouts.at(layoutStr);
        objectSymbols[symbol->firstTag] = symbol;
        std::ostringstream Out;
        symbol->print(Out);
        allObjectSymbols[Out.str()] = symbol;
      }
    }
    uint32_t lastTag = nextSymbol-1;
    if (!entry.second.empty()) {
      variables.emplace(entry.first, std::pair<uint32_t, uint32_t>{firstTag, lastTag});
    }
  }
  for (auto iter = symbols.begin(); iter != symbols.end(); ++iter) {
    auto entry = *iter;
    auto range = variables.at(entry.first);
    for (auto iter = entry.second.begin(); iter != entry.second.end(); ++iter) {
      KORESymbol *symbol = *iter;
      for (auto &sort : symbol->getArguments()) {
        if (sort->isConcrete()) {
          hookedSorts[dynamic_cast<KORECompositeSort *>(sort.get())->getCategory(this)] = std::dynamic_pointer_cast<KORECompositeSort>(sort);
        }
      }
      if (symbol->getSort()->isConcrete()) {
        hookedSorts[dynamic_cast<KORECompositeSort *>(symbol->getSort().get())->getCategory(this)] = std::dynamic_pointer_cast<KORECompositeSort>(symbol->getSort());
      }
      if (!symbol->isConcrete()) {
        if (symbol->isPolymorphic()) {
          symbol->firstTag = range.first;
          symbol->lastTag = range.second;
          auto decl = symbolDeclarations.at(symbol->getName());
          if (decl->getAttributes().count("sortInjection")) {
            injSymbol = symbol;
          }
        }
      }
    }
  }
}

// Pretty printer
void KORESortVariable::print(std::ostream &Out, unsigned indent) const {
  std::string Indent(indent, ' ');
  Out << Indent << name;
}

void KORECompositeSort::print(std::ostream &Out, unsigned indent) const {
  std::string Indent(indent, ' ');
  Out << Indent << name << "{";
  bool isFirst = true;
  for (auto &Argument : arguments) {
    if (!isFirst)
      Out << ",";
    Argument->print(Out);
    isFirst = false;
  }
  Out << "}";
}

void KORESymbol::print(std::ostream &Out, unsigned indent) const {
  print(Out, indent, true);
}

void KORESymbol::print(std::ostream &Out, unsigned indent, bool formal) const {
  std::string Indent(indent, ' ');
  Out << Indent << name << "{";
  bool isFirst = true;
  for (auto &Argument : (formal ? formalArguments : arguments)) {
    if (!isFirst)
      Out << ", ";
    Argument->print(Out);
    isFirst = false;
  }
  Out << "}";
}

void KOREVariable::print(std::ostream &Out, unsigned indent) const {
  std::string Indent(indent, ' ');
  Out << Indent << name;
}

void
KOREVariablePattern::print(std::ostream &Out, unsigned indent) const {
  std::string Indent(indent, ' ');
  Out << Indent;
  name->print(Out);
  Out << " : ";
  sort->print(Out);
}

void
KORECompositePattern::print(std::ostream &Out, unsigned indent) const {
  std::string Indent(indent, ' ');
  Out << Indent;
  constructor->print(Out);
  Out << "(";
  bool isFirst = true;
  for (auto &Argument : arguments) {
    if (!isFirst)
      Out << ",";
    Argument->print(Out);
    isFirst = false;
  }
  Out << ")";
}

static std::string escapeString(const std::string &str) {
  std::string result;
  for (char c : str) {
    if (c == '"' || c == '\\' || !isprint(c)) {
      result.push_back('\\');
      result.push_back('x');
      char code[3];
      snprintf(code, 3, "%02x", (unsigned char)c);
      result.push_back(code[0]);
      result.push_back(code[1]);
    } else {
      result.push_back(c);
    }
  }
  return result;
}

void KOREStringPattern::print(std::ostream &Out, unsigned indent) const {
  std::string Indent(indent, ' ');
  Out << Indent << "\"" << escapeString(contents) << "\"";
}

static void printAttributeList(
  std::ostream &Out, const std::map<std::string, ptr<KORECompositePattern>> &attributes,
  unsigned indent = 0) {

  std::string Indent(indent, ' ');
  Out << Indent << "[";
  bool isFirst = true;
  for (auto &Pattern : attributes) {
    if (!isFirst)
      Out << ",";
    Pattern.second->print(Out);
    isFirst = false;
  }
  Out << "]";
}

void KOREDeclaration::printSortVariables(std::ostream &Out) const {
  Out << "{";
  bool isFirst = true;
  for (auto &Variable : objectSortVariables) {
    if (!isFirst)
      Out << ",";
    Variable->print(Out);
    isFirst = false;
  }
  Out << "}";
}

void KORECompositeSortDeclaration::print(
  std::ostream &Out, unsigned indent) const {
  std::string Indent(indent, ' ');
  Out << Indent << (_isHooked ? "hooked-sort " : "sort ") << sortName;
  printSortVariables(Out);
  Out << " ";
  printAttributeList(Out, attributes);
}

void
KORESymbolDeclaration::print(std::ostream &Out, unsigned indent) const {
  std::string Indent(indent, ' ');
  Out << Indent << (_isHooked ? "hooked-symbol " : "symbol ")
      << symbol->getName();
  printSortVariables(Out);
  Out << "(";
  bool isFirst = true;
  for (auto &Argument : symbol->getArguments()) {
    if (!isFirst)
      Out << ",";
    Argument->print(Out);
    isFirst = false;
  }
  Out << ") : ";
  symbol->getSort()->print(Out);
  Out << " ";
  printAttributeList(Out, attributes);
}

void
KOREAliasDeclaration::print(std::ostream &Out, unsigned indent) const {
  std::string Indent(indent, ' ');
  Out << Indent << "alias " << symbol->getName();
  printSortVariables(Out);
  Out << "(";
  bool isFirst = true;
  for (auto &Argument : symbol->getArguments()) {
    if (!isFirst)
      Out << ",";
    Argument->print(Out);
    isFirst = false;
  }
  Out << ") : ";
  symbol->getSort()->print(Out);
  Out << " where ";
  boundVariables->print(Out);
  Out << " := ";
  pattern->print(Out);
  Out << " ";
  printAttributeList(Out, attributes);
}

void KOREAxiomDeclaration::print(std::ostream &Out, unsigned indent) const {
  std::string Indent(indent, ' ');
  Out << Indent << "axiom ";
  printSortVariables(Out);
  pattern->print(Out);
  Out << " ";
  printAttributeList(Out, attributes);
}

void
KOREModuleImportDeclaration::print(std::ostream &Out, unsigned indent) const {
  std::string Indent(indent, ' ');
  Out << Indent << "import " << moduleName;
  Out << " ";
  printAttributeList(Out, attributes);
}

void KOREModule::print(std::ostream &Out, unsigned indent) const {
  std::string Indent(indent, ' ');
  Out << Indent << "module " << name << "\n";
  bool isFirst = true;
  for (auto &Declaration : declarations) {
    if (!isFirst)
      Out << "\n";
    Declaration->print(Out, indent + 2);
    Out << "\n";
    isFirst = false;
  }
  Out << Indent << "endmodule\n";
  printAttributeList(Out, attributes, indent);
}

void KOREDefinition::print(std::ostream &Out, unsigned indent) const {
  printAttributeList(Out, attributes, indent);
  Out << "\n";
  for (auto &Module : modules) {
    Out << "\n";
    Module->print(Out, indent);
    Out << "\n";
  }
}

void kllvm::readMultimap(std::string name, KORESymbolDeclaration *decl, std::map<std::string, std::set<std::string>> &output, std::string attName) {
  if (decl->getAttributes().count(attName)) {
    KORECompositePattern *att = decl->getAttributes().at(attName).get();
    for (auto &pat : att->getArguments()) {
      auto child = dynamic_cast<KORECompositePattern *>(pat.get());
      output[name].insert(child->getConstructor()->getName());
    }
  }
}

// Normally, destruction of KOREPattern would call destructor
// of all its subpatterns. This can sometimes exhaust all the stack space.
// This function deallocates a pattern iteratively, without recursion.
void kllvm::deallocateSPtrKorePattern(sptr<KOREPattern> pattern) {
    std::vector<sptr<KOREPattern>> vec;
    vec.push_back(std::move(pattern));
    while(!vec.empty()) {
        sptr<KOREPattern> curr = std::move(vec.back());
        vec.pop_back();
        if (auto composite = std::dynamic_pointer_cast<KORECompositePattern>(curr)) {
            vec.insert(vec.end(), std::make_move_iterator(composite->arguments.begin()), std::make_move_iterator(composite->arguments.end()));
        }
    }
}
