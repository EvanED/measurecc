#include <boost/algorithm/string/predicate.hpp>

#include "llvm/Pass.h"
#include "llvm/Support/DebugLoc.h"
#include "llvm/DebugInfo.h"
#include "llvm/Function.h"
#include "llvm/InstrTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Support/raw_ostream.h"

#include <sstream>
#include <iostream>
#include <string>
#include <set>

using namespace llvm;

namespace  {

    struct Hello : public FunctionPass {

        static char ID;
        Hello() : FunctionPass(ID) {}

        virtual bool runOnFunction(Function &f) {
            return false;
        }
    };  // end of struct Hello
}  // end of anonymous namespace

char Hello::ID = 0;

static RegisterPass<Hello> X("hello", "Hello World Pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);


