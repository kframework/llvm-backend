#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <functional>

#include "runtime/header.h"
#include "runtime/alloc.h"

void printInt(writer *file, mpz_t i, const char *sort) {
  char *str = mpz_get_str(NULL, 10, i);
  sfprintf(file, "\\dv{%s}(\"%s\")", sort, str);
}

void printFloat(writer *file, floating *f, const char *sort) {
  std::string str = floatToString(f);
  sfprintf(file, "\\dv{%s}(\"%s\")", sort, str.c_str());
}

void printBool(writer *file, bool b, const char *sort) {
  const char *str = b ? "true" : "false";
  sfprintf(file, "\\dv{%s}(\"%s\")", sort, str);
}

void printStringBuffer(writer *file, stringbuffer *b, const char *sort) {
  std::string str(b->contents->data, b->strlen);
  sfprintf(file, "\\dv{%s}(\"%s\")", sort, str.c_str());
}

void printMInt(writer *file, size_t *i, size_t bits, const char *sort) {
  if (i == nullptr) {
    sfprintf(file, "\\dv{%s}(\"0p%zd\")", sort, bits);
  } else {
    mpz_ptr z = hook_MINT_import(i, bits, false);
    char *str = mpz_get_str(NULL, 10, z);
    sfprintf(file, "\\dv{%s}(\"%sp%zd\")", sort, str, bits);
  }
}

void sfprintf(writer *file, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  if (file->file) {
    vfprintf(file->file, fmt, args);
  } else {
    char buf[8192];
    char *finalBuf = buf;
    int res = vsnprintf(buf + sizeof(blockheader), sizeof(buf) - sizeof(blockheader), fmt, args);
    if ((size_t)res >= sizeof(buf) - sizeof(blockheader)) {
      size_t size = sizeof(buf)*2;
      finalBuf = (char *)malloc(size);
      memcpy(finalBuf, buf, sizeof(buf));
      res = vsnprintf(finalBuf + sizeof(blockheader), size - sizeof(blockheader), fmt, args);
      if ((size_t)res >= size - sizeof(blockheader)) {
        do {
          size *= 2;
	      finalBuf = (char *)realloc(finalBuf, size);
	      res = vsnprintf(finalBuf + sizeof(blockheader), size - sizeof(blockheader), fmt, args);
	    } while ((size_t)res >= size - sizeof(blockheader));
      }
    }
    string *str = (string *)finalBuf;
    set_len(str, (size_t)res);
    hook_BUFFER_concat(file->buffer, str);
  }
}

void printComma(writer *file) {
  sfprintf(file, ",");
}

struct StringHash {
  size_t operator() (string * const& k) const {
    return std::hash<std::string>{}(std::string(k->data, len(k)));
  }
};

struct StringEq {
  bool operator() (string * const& lhs, string * const& rhs) const {
    return hook_STRING_eq(lhs, rhs);
  }
};

static thread_local std::vector<block *> boundVariables;
static thread_local std::unordered_map<string *, std::string, StringHash, StringEq> varNames;
static thread_local std::set<std::string> usedVarNames;
static thread_local uint64_t varCounter = 0;

void printConfigurationInternal(writer *file, block *subject, const char *sort, bool isVar) {
  uint8_t isConstant = ((uintptr_t)subject) & 3;
  if (isConstant) {
    uint32_t tag = ((uintptr_t)subject) >> 32;
    if (isConstant == 3) {
      // bound variable
      printConfigurationInternal(file, boundVariables[boundVariables.size()-1-tag], sort, true);
      return;
    }
    const char *symbol = getSymbolNameForTag(tag);
    sfprintf(file, "%s()", symbol);
    return;
  }
  uint16_t layout = layout(subject);
  if (!layout) {
    string *str = (string *)subject;
    size_t len = len(subject);
    sfprintf(file, "\\dv{%s}(\"", sort);
    for (size_t i = 0; i < len; ++i) {
      char c = str->data[i];
      switch(c) {
      case '\\':
        sfprintf(file, "\\\\");
        break;
      case '"':
        sfprintf(file, "\\\"");
        break;
      case '\n':
        sfprintf(file, "\\n");
        break;
      case '\t':
        sfprintf(file, "\\t");
        break;
      case '\r':
        sfprintf(file, "\\r");
        break;
      case '\f':
        sfprintf(file, "\\f");
        break;
      default:
        if ((unsigned char)c >= 32 && (unsigned char)c < 127) {
          sfprintf(file, "%c", c);
        } else {
          sfprintf(file, "\\x%02x", (unsigned char)c);
        }
        break;
      }
    }
    if (isVar && !varNames.count(str)) {
      std::string stdStr = std::string(str->data, len(str));
      std::string suffix = "";
      while (usedVarNames.count(stdStr + suffix)) {
        suffix = std::to_string(varCounter++);
      }
      stdStr = stdStr + suffix;
      sfprintf(file, "%s", suffix.c_str());
      usedVarNames.insert(stdStr);
      varNames[str] = suffix;
    } else if (isVar) {
      sfprintf(file, "%s", varNames[str].c_str());
    }
    sfprintf(file, "\")");
    return;
  }
  uint32_t tag = tag_hdr(subject->h.hdr);
  bool isBinder = isSymbolABinder(tag);
  if (isBinder) {
    boundVariables.push_back(*(block **)(((char *)subject) + sizeof(blockheader)));
  }
  const char *symbol = getSymbolNameForTag(tag);
  std::string symbolStr(symbol);
  if (symbolStr.rfind("inj{", 0) == 0) {	
    std::string prefix = symbolStr.substr(0, symbolStr.find_first_of(','));
    sfprintf(file, "%s, %s}(", prefix.c_str(), sort);
  } else {
    sfprintf(file, "%s(", symbol);
  }
  visitChildren(subject, file, printConfigurationInternal, printMap, printList, printSet, printInt, printFloat,
      printBool, printStringBuffer, printMInt, printComma);
  if (isBinder) {
    boundVariables.pop_back();
  }
  sfprintf(file, ")");
}

void printStatistics(const char *filename, uint64_t steps) {
  FILE *file = fopen(filename, "w");
  fprintf(file, "%" PRIu64 "\n", steps-1); //off by one adjustment
  fclose(file);
}

void printConfiguration(const char *filename, block *subject) {
  FILE *file = fopen(filename, "a");
  boundVariables.clear();
  varCounter = 0;
  writer w = {file,nullptr};
  printConfigurationInternal(&w, subject, nullptr, false);
  varNames.clear();
  usedVarNames.clear();
  fclose(file);
}

string *printConfigurationToString(block *subject) {
  boundVariables.clear();
  varCounter = 0;
  stringbuffer *buf = hook_BUFFER_empty();
  writer w = {nullptr,buf};
  printConfigurationInternal(&w, subject, nullptr, false);
  varNames.clear();
  usedVarNames.clear();
  return hook_BUFFER_toString(buf);
}

void printConfigurationToFile(FILE *file, block *subject) {
  boundVariables.clear();
  varCounter = 0;
  writer w = {file,nullptr};
  printConfigurationInternal(&w, subject, nullptr, false);
  varNames.clear();
  usedVarNames.clear();
}

extern "C" void* getStderr(void) {
  return stderr;
}

struct MatchLog {
  enum {
    SUCCESS=0, FUNCTION, FAIL
  } kind;

  char *function;
  char *debugName;
  void *result;
  std::vector<void *> args;

  char *pattern;
  void *subject;
  char *sort;
};

static std::vector<MatchLog> matchLog;

void resetMatchReason(void) {
  matchLog.clear();
}

MatchLog *getMatchLog(void) {
  return &matchLog[0];
}

void **getMatchFnArgs(MatchLog *log) {
  return &log->args[0];
}

size_t getMatchLogSize(void) {
  return matchLog.size();
}

extern "C" {
  
void addMatchSuccess(void) {
  matchLog.push_back({MatchLog::SUCCESS, NULL, NULL, NULL, {}, NULL, NULL, NULL});
}

void addMatchFailReason(void *subject, char *pattern, char *sort) {
  matchLog.push_back({MatchLog::FAIL, NULL, NULL, NULL, {}, pattern, subject, sort});
}

void addMatchFunction(char *debugName, char *function, void *result, ...) {
  va_list ap;
  va_start(ap, result);

  std::vector<void *> args;
  while (true) {
    void *arg = va_arg(ap, void *);
    if (!arg) break;
    args.push_back(arg);
  }

  matchLog.push_back({MatchLog::FUNCTION, function, debugName, result, args, NULL, NULL, NULL});

  va_end(ap);
}

}

#define DEFINE_GDB_PY_SCRIPT(script_name) \
  asm("\
.pushsection \".debug_gdb_scripts\", \"MS\",@progbits,1\n\
.byte 1 /* Python */\n\
.asciz \"" script_name "\"\n\
.popsection \n\
");

#ifndef __APPLE__
DEFINE_GDB_PY_SCRIPT(INSTALL_PREFIX "/lib/kllvm/gdb/interpreter-gdb.py")
#endif
