#include <boost/algorithm/string/predicate.hpp>

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/TypeBuilder.h"
#include "llvm/GlobalVariable.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/IRBuilder.h"
#include "llvm/Support/CommandLine.h"

#include <sstream>
#include <iostream>
#include <string>
#include <set>

using namespace llvm;

namespace  {
    cl::opt<std::string> OutputFilename("hello-o", cl::desc("Specify output filename"),
                                        cl::value_desc("filename"));

    struct Hello : public ModulePass {

        static char ID;
        Hello() : ModulePass(ID) {}

        virtual bool runOnModule(Module &m) {
            std::cerr << "hello-o: " << OutputFilename << "\n";
            
            for (Module::iterator func = m.begin();
                 func != m.end(); ++func)
            {
                doFunction(m, *func);
            }
            return false;
        }

        bool doFunction(Module & m, Function & f) {
            LLVMContext & context = getGlobalContext();
            IntegerType * int32 = TypeBuilder<types::i<32>, true>::get(context);
            Constant * counter = m.getOrInsertGlobal("zzzzz_" + f.getName().str(), int32);
            ConstantInt * one = ConstantInt::get(int32, 1, true);

            BasicBlock & entry = f.getEntryBlock();
            IRBuilder<> builder(entry.begin());
            Value * pre = builder.CreateLoad(counter, "");
            Value * add = builder.CreateAdd(pre, one);
            (void) builder.CreateStore(add, counter);
            
            return false;
        }
    };  // end of struct Hello
}  // end of anonymous namespace

char Hello::ID = 0;

static RegisterPass<Hello> X("hello", "Hello World Pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);


