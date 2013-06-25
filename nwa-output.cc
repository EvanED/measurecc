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

    void
    declare_timer(Module & m)
    {
        Type * int32 = Type::getInt32Ty(getGlobalContext());
        Type * int64 = Type::getInt64Ty(getGlobalContext());
        Type * void_ = Type::getVoidTy(getGlobalContext());
        Type * double_ = Type::getDoubleTy(getGlobalContext());
        
        std::vector<Type*> timer_types(3);
        timer_types[0] = timer_types[1] = int64;
        timer_types[2] = int32;

        StructType * timer = StructType::create(timer_types, "class._measurecc::Timer");
        PointerType * timer_p = PointerType::get(timer, 0);

        std::vector<Type*> timer_param(1);
        timer_param[0] = timer_p;

        FunctionType * ty_void = FunctionType::get(void_, timer_param, false);
        FunctionType * ty_double = FunctionType::get(double_, timer_param, false);

        Function * start = Function::Create(ty_void,
                                            GlobalValue::ExternalLinkage,
                                            "_ZN10_measurecc5Timer5startEv",
                                            &m);

        Function * stop = Function::Create(ty_void,
                                           GlobalValue::ExternalLinkage,
                                           "_ZN10_measurecc5Timer4stopEv",
                                            &m);

        Function * get_time = Function::Create(ty_double,
                                               GlobalValue::ExternalLinkage,
                                               "_ZNK10_measurecc5Timer10total_timeEv",
                                               &m);
    }

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
    
    cl::opt<std::string> funcsFilename("measure-spec",
                                       cl::desc("Specify functions that should be measured"),
                                       cl::value_desc("filename"));

    struct Hello : public ModulePass {

        std::set<std::string> count_funcs;

        static char ID;
        Hello() : ModulePass(ID) {
#if 0
            std::ifstream func_descs(funcsFilename.c_str());
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
                func = func.substr(1);
                std::cerr << "<" << func << ">\n";
                count_funcs.insert(func);
            }
#endif
        }

        virtual bool runOnModule(Module &m) {
            declare_timer(m);
            for (Module::iterator func = m.begin();
                 func != m.end(); ++func)
            {
                doFunction(m, *func);
            }
            return false;
        }

        bool doFunction(Module & m, Function & f) {
            std::string demangled_name = demangle(f.getName());
            std::cerr << "+++ Considering <" << demangled_name << ">\n";
            if (count_funcs.count(demangled_name) == 0) {
                return false;
            }
            if (f.empty()) {
                std::cerr << "Warning: function " << f.getName().str() << " is empty. Cannot instrument.\n";
                return false;
            }
            std::cerr << "+++    Processing " << demangle(f.getName()) << "\n";

            LLVMContext & context = getGlobalContext();
            IntegerType * int32 = TypeBuilder<types::i<32>, true>::get(context);
            Constant * counter = m.getOrInsertGlobal("_measurecc_counter_" + f.getName().str(), int32);
            ConstantInt * one = ConstantInt::get(int32, 1, true);

            BasicBlock & entry = f.getEntryBlock();
            assert(entry.size() > 0);
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


