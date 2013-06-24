#include <boost/algorithm/string/predicate.hpp>

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/TypeBuilder.h"
#include "llvm/GlobalVariable.h"
#include "llvm/Constants.h"

#include <sstream>
#include <iostream>
#include <string>
#include <set>

using namespace llvm;

namespace  {

    struct Hello : public FunctionPass {

        static char ID;
        Hello() : FunctionPass(ID) {}

        virtual bool doInitialization(Module &m) {
            LLVMContext & context = getGlobalContext();
            IntegerType * int32 = TypeBuilder<types::i<32>, true>::get(context);
            ConstantInt * zero = ConstantInt::get(int32, 0, true);
            GlobalVariable * g = new GlobalVariable(int32, // type
                                                    false, // is constant
                                                    GlobalValue::InternalLinkage,
                                                    zero,  // initializer
                                                    "zzzzz");
            return false;
        }

        virtual bool runOnFunction(Function &f) {
            return false;
        }
    };  // end of struct Hello
}  // end of anonymous namespace

char Hello::ID = 0;

static RegisterPass<Hello> X("hello", "Hello World Pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);

