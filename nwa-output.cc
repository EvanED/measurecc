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

#include <cxxabi.h>

#include <sstream>
#include <iostream>
#include <string>
#include <fstream>
#include <set>

using namespace llvm;

namespace  {

    std::string
    demangle(std::string const & mangled)
    {
        int out;
        char* demangled = abi::__cxa_demangle(mangled.c_str(), NULL, NULL, &out);
        if (!demangled) {
            return "???";
        }
        std::string ret = demangled;
        free(demangled);
        return ret;
    }
    
    cl::opt<std::string> countFilename("count-funcs", cl::desc("Specify functions that should be counted"),
				       cl::value_desc("filename"));

    struct Hello : public ModulePass {

	std::set<std::string> count_funcs;

        static char ID;
        Hello() : ModulePass(ID) {
	    std::ifstream func_descs(countFilename.c_str());
	    if (!func_descs.good()) {
		std::cerr << "Error: could not open " << func_descs << "\n";
		std::exit(1);
	    }
	    std::string line;
	    while(getline(func_descs, line)) {
		std::stringstream ss(line);
		std::string command, func;
		ss >> command;
		assert(command == "count");
		getline(ss, func);
		std::cerr << "<" << func.substr(1) << ">\n";
		count_funcs.insert(func);
	    }
	}

        virtual bool runOnModule(Module &m) {
            for (Module::iterator func = m.begin();
                 func != m.end(); ++func)
            {
                doFunction(m, *func);
            }
            return false;
        }

        bool doFunction(Module & m, Function & f) {
            std::cerr << "+++ Considering " << demangle(f.getName()) << "\n";
	    if (count_funcs.count(f.getName()) == 0) {
		return false;
	    }
            if (f.empty()) {
		std::cerr << "Warning: function " << f.getName().str() << " is empty. Cannot instrument.\n";
                return false;
            }
            std::cerr << "+++ Processing " << demangle(f.getName()) << "\n";

            LLVMContext & context = getGlobalContext();
            IntegerType * int32 = TypeBuilder<types::i<32>, true>::get(context);
            Constant * counter = m.getOrInsertGlobal("zzzzz_" + f.getName().str(), int32);
            ConstantInt * one = ConstantInt::get(int32, 1, true);

            BasicBlock & entry = f.getEntryBlock();
            if (entry.size() > 0) {
                IRBuilder<> builder(entry.begin());
                Value * pre = builder.CreateLoad(counter, "");
                Value * add = builder.CreateAdd(pre, one);
                (void) builder.CreateStore(add, counter);
            }
            
            return false;
        }
    };  // end of struct Hello

}  // end of anonymous namespace

char Hello::ID = 0;

static RegisterPass<Hello> X("hello", "Hello World Pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);


