#ifndef KLLVM_UTIL_H
#define KLLVM_UTIL_H

#include "llvm/IR/Module.h"

namespace kllvm {

// Returns a reference to the function declaration for a memory allocation function with the given
// name, adding a declaration to the current module if one does not yet exist
llvm::Function* koreHeapAlloc(std::string name, llvm::Module *module);

// If Value is an instance of llvm::Function, cast and return. Otherwise, print errors and abort.
llvm::Function* castToFunctionOrAbort(llvm::Value* value);


// getOrInsertFunction on module, aborting on failure
template<class...Ts>
static llvm::Function* getOrInsertFunction(llvm::Module *module, Ts... Args) {
  llvm::Value *callee;
  auto ret = module->getOrInsertFunction(Args...);
#if LLVM_LIB_VERSION_MAJOR >= 9
  callee = ret.getCallee();
#else
  callee = ret;
#endif
  return castToFunctionOrAbort(callee);
}

}

#endif // KLLVM_UTIL_H 
