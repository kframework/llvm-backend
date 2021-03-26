#include "runtime/header.h"

#include "rapidjson/reader.h"
#include "rapidjson/writer.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"

#include <vector>

using namespace rapidjson;

extern "C" {
  floating *move_float(floating *);
  string *hook_STRING_int2string(mpz_t);
  string * makeString(const char * input, ssize_t len = -1);
  char *getTerminatedString(string *);
}

std::string floatToString(const floating *f, const char *suffix);

struct zinj {
  blockheader h;
  mpz_ptr data;
};

struct floatinj {
  blockheader h;
  floating *data;
};

struct stringinj {
  blockheader h;
  string *data;
};

typedef struct boolinj {
  struct blockheader h;
  bool data;
} boolinj;

struct jsonlist {
  blockheader h;
  block *hd;
  jsonlist* tl;
};

struct json {
  blockheader h;
  jsonlist *data;
};

struct jsonmember {
  blockheader h;
  block *key;
  block *val;
};

static block * dotK = leaf_block(getTagForSymbolName("dotk{}"));
static blockheader kseqHeader = {getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("kseq{}"))};

#define get_header(name, symbol) \
static struct blockheader name() {\
  static struct blockheader hdr = {(uint64_t)-1}; \
  if (hdr.hdr == (uint64_t)-1) { \
    hdr = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName(symbol)); \
  } \
  return hdr; \
}

get_header(boolHdr, "inj{SortBool{}, SortJSON{}}")
get_header(intHdr, "inj{SortInt{}, SortJSON{}}")
get_header(floatHdr, "inj{SortFloat{}, SortJSON{}}")
get_header(strHdr, "inj{SortString{}, SortJSON{}}")
get_header(listHdr, "LblJSONs{}")
get_header(membHdr, "LblJSONEntry{}")
get_header(objHdr, "LblJSONObject{}")
get_header(listWrapHdr, "LblJSONList{}")

#define get_block(name, symbol) \
static block *name() {\
  static uint64_t tag = (uint64_t)-1; \
  if (tag == (uint64_t)-1) { \
    tag = (uint64_t)leaf_block(getTagForSymbolName(symbol)); \
  } \
  return (block *)tag; \
}

get_block(dotList, "Lbl'Stop'List'LBraQuot'JSONs'QuotRBraUnds'JSONs{}")
get_block(null, "LblJSONnull{}")

struct KoreHandler : BaseReaderHandler<UTF8<>, KoreHandler> {
  block *result;
  std::vector<block *> stack;

  bool Null() { stack.push_back(null()); return true; }
  bool Bool(bool b) {
    boolinj *inj = (boolinj *)koreAlloc(sizeof(boolinj));
    inj->h = boolHdr();
    inj->data = b;
    result = (block *)inj;
    stack.push_back(result);
    return true;
  }

  bool RawNumber(const char *str, SizeType length, bool copy) {
    mpz_t z;
    int status = mpz_init_set_str(z, str, 10);
    if (status == 0) {
      zinj *inj = (zinj *)koreAlloc(sizeof(zinj));
      inj->h = intHdr();
      inj->data = move_int(z);
      result = (block *)inj;
      stack.push_back(result);
      return true;
    } else {
      mpz_clear(z);
      floating f[1];
      mpfr_init2(f->f, 53);
      f->exp = 11;
      mpfr_set_str(f->f, str, 9, MPFR_RNDN);
      floatinj *inj = (floatinj *)koreAlloc(sizeof(floatinj));
      inj->h = floatHdr();
      inj->data = move_float(f);
      result = (block *)inj;
      stack.push_back(result);
      return true;
    }
  }

  bool String(const char *str, SizeType len, bool copy) {
    stringinj *inj = (stringinj *)koreAlloc(sizeof(stringinj));
    inj->h = strHdr();
    string *token = (string *)koreAllocToken(sizeof(string) + len);
    set_len(token, len);
    memcpy(token->data, str, len);
    inj->data = token;
    result = (block *)inj;
    stack.push_back(result);
    return true;
  }

  bool StartObject() { return true; }
  
  bool Key(const char *str, SizeType len, bool copy) {
    return String(str, len, copy);
  }


  bool EndObject(SizeType memberCount) {
    result = dotList();
    for (SizeType i = 0; i < memberCount; i++) {
      jsonmember *member = (jsonmember *)koreAlloc(sizeof(jsonmember));
      member->h = membHdr();
      member->val = stack.back();
      stack.pop_back();
      member->key = stack.back();
      stack.pop_back();
      jsonlist *list = (jsonlist *)koreAlloc(sizeof(jsonlist));
      list->h = listHdr();
      list->hd = (block *)member;
      list->tl = (jsonlist *)result;
      result = (block *)list;
    }
    json *wrap = (json *)koreAlloc(sizeof(json));
    wrap->h = objHdr();
    wrap->data = (jsonlist *)result;
    stack.push_back((block *)wrap);
    return true;
  }

  bool StartArray() { return true; }

  bool EndArray(SizeType elementCount) {
    result = dotList();
    for (SizeType i = 0; i < elementCount; i++) {
      jsonlist *list = (jsonlist *)koreAlloc(sizeof(jsonlist));
      list->h = listHdr();
      list->hd = stack.back();
      stack.pop_back();
      list->tl = (jsonlist *)result;
      result = (block *)list;
    }
    json *wrap = (json *)koreAlloc(sizeof(json));
    wrap->h = listWrapHdr();
    wrap->data = (jsonlist *)result;
    stack.push_back((block *)wrap);
    return true;
  }
};

template <typename Stream>
struct KoreWriter : Writer<Stream> {
  bool RawNumber(const typename Writer<Stream>::Ch* str, rapidjson::SizeType length, bool copy = false) {
    (void)copy;
    Writer<Stream>::Prefix(rapidjson::kNumberType);
    return Writer<Stream>::EndValue(Writer<Stream>::WriteRawValue(str, length));
  }

  KoreWriter(Stream &os) : Writer<Stream>(os) {}
};

template <typename Stream>
static bool write_json(KoreWriter<Stream> &writer, block *data) {
  bool return_value = true;
  if (data != dotList()) {
    if (data == null()) {
      writer.Null();
    } else if (data->h.hdr == boolHdr().hdr) {
      boolinj *inj = (boolinj *)data;
      writer.Bool(inj->data);
    } else if (data->h.hdr == intHdr().hdr) {
      zinj *inj = (zinj *)data;
      string *str = hook_STRING_int2string(inj->data);
      writer.RawNumber(str->data, len(str), false);
    } else if (data->h.hdr == floatHdr().hdr) {
      floatinj *inj = (floatinj *)data;
      std::string str = floatToString(inj->data, "");
      writer.RawNumber(str.c_str(), str.length(), false);
    } else if (data->h.hdr == strHdr().hdr) {
      stringinj *inj = (stringinj *)data;
      writer.String(inj->data->data, len(inj->data), false);
    } else if (data->h.hdr == objHdr().hdr) {
      writer.StartObject();
      json *obj = (json *)data;
      return_value = write_json(writer, (block *)obj->data);
      writer.EndObject();
    } else if (data->h.hdr == listWrapHdr().hdr) {
      writer.StartArray();
      json *obj = (json *)data;
      return_value = write_json(writer, (block *)obj->data);
      writer.EndArray();
    } else if (data->h.hdr == listHdr().hdr) {
      jsonlist *list = (jsonlist *)data;
      return_value = write_json(writer, list->hd) && write_json(writer, (block *)list->tl);
    } else if (data->h.hdr == membHdr().hdr) {
      jsonmember *memb = (jsonmember *)data;
      stringinj *inj = (stringinj *)memb->key;
      writer.Key(inj->data->data, len(inj->data), false);
      return_value = write_json(writer, memb->val);
    } else {
      return_value = false;
    }
  }
  return return_value;
}

extern "C" {

SortString hook_JSON_json2string(SortJSON json) {
  StringBuffer buffer;
  KoreWriter<StringBuffer> writer(buffer);
  if (! write_json(writer, json)) {
    abort();
  }
  return makeString(buffer.GetString());
}

SortJSON hook_JSON_string2json(SortString str) {
  char *cstr = getTerminatedString(str);
  StringStream s(cstr);
  KoreHandler handler;
  Reader reader;
  bool result = reader.Parse<kParseNumbersAsStringsFlag>(s, handler);
  if (result) {
    return handler.stack.back();
  } else {
    abort();
  }
}

}
