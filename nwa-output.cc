// @_ZL1x = internal global i32 0, align 4


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
#include "llvm/Support/raw_ostream.h"

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
    
    cl::opt<std::string> funcsFilename("measure-spec",
                                       cl::desc("Specify functions that should be measured"),
                                       cl::value_desc("filename"));

    struct Hello : public ModulePass {

        std::set<std::string> count_funcs, time_funcs;


        Type * _timer_type;
        Function * _timer_start, * _timer_stop, * _timer_total_time;

        Constant *
        declare_counter(Module & m, Function & f)
        {
            Type * int32 = Type::getInt32Ty(getGlobalContext());
            Constant * zero = ConstantInt::get(int32, 0, true);
            GlobalVariable * g = new GlobalVariable(m,
                                                    int32,
                                                    false,
                                                    GlobalValue::InternalLinkage,
                                                    zero,
                                                    "_measurecc_counter_" + f.getName().str());
            return g;
        }

        void
        declare_timer_stuff(Module & m)
        {
            Type * int32 = Type::getInt32Ty(getGlobalContext());
            Type * int64 = Type::getInt64Ty(getGlobalContext());
            Type * void_ = Type::getVoidTy(getGlobalContext());
            Type * double_ = Type::getDoubleTy(getGlobalContext());
        
            std::vector<Type*> timer_types(3);
            timer_types[0] = timer_types[1] = int64;
            timer_types[2] = int32;

            _timer_type = StructType::create(timer_types, "class._measurecc::Timer");
            PointerType * timer_p = PointerType::get(_timer_type, 0);

            std::vector<Type*> timer_param(1);
            timer_param[0] = timer_p;

            FunctionType * ty_void = FunctionType::get(void_, timer_param, false);
            FunctionType * ty_double = FunctionType::get(double_, timer_param, false);

            _timer_start = Function::Create(ty_void,
                                            GlobalValue::ExternalLinkage,
                                            "_ZN10_measurecc5Timer5startEv",
                                            &m);

            _timer_stop = Function::Create(ty_void,
                                           GlobalValue::ExternalLinkage,
                                           "_ZN10_measurecc5Timer4stopEv",
                                           &m);

            _timer_total_time = Function::Create(ty_double,
                                                 GlobalValue::ExternalLinkage,
                                                 "_ZNK10_measurecc5Timer10total_timeEv",
                                                 &m);
        }
        

        static char ID;
        Hello() : ModulePass(ID) {
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
                getline(ss, func);
                func = func.substr(1);
                std::cerr << "<" << func << ">: ";
                if (command == "time") {
                    std::cerr << "timing";
                    time_funcs.insert(func);
                }
                else if (command == "count") {
                    std::cerr << "counting";
                    count_funcs.insert(func);
                }
                else {
                    assert(false && "did not say time or count");
                }
                std::cerr << "\n";
            }
        }

        virtual bool runOnModule(Module &m) {
            declare_timer_stuff(m);
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
            if (count_funcs.count(demangled_name) == 0
                && time_funcs.count(demangled_name) == 0)
            {
                return false;
            }
            if (f.empty()) {
                std::cerr << "Warning: function " << f.getName().str() << " is empty. Cannot instrument.\n";
                return false;
            }
            std::cerr << "+++    Processing " << demangle(f.getName()) << "\n";

            if (count_funcs.count(demangled_name) > 0) {
                LLVMContext & context = getGlobalContext();
                IntegerType * int32 = TypeBuilder<types::i<32>, true>::get(context);
                Constant * counter = declare_counter(m, f);
                ConstantInt * one = ConstantInt::get(int32, 1, true);

                BasicBlock & entry = f.getEntryBlock();
                assert(entry.size() > 0);
                if (entry.size() > 0) {
                    IRBuilder<> builder(entry.begin());
                    Value * pre = builder.CreateLoad(counter, "");
                    Value * add = builder.CreateAdd(pre, one);
                    (void) builder.CreateStore(add, counter);
                }
            }

            if (time_funcs.count(demangled_name) > 0) {
                Constant * timer = m.getOrInsertGlobal("_measurecc_timer_" + f.getName().str(), _timer_type);
                std::vector<Value*> params(1);
                params[0] = timer;

                BasicBlock & entry = f.getEntryBlock();
                assert(entry.size() > 0);

                CallInst * call = CallInst::Create(_timer_start, params, "", entry.begin());

                for (Function::iterator bb = f.begin(); bb != f.end(); ++bb) {
                    TerminatorInst * term = bb->getTerminator();
                    if (isa<ReturnInst>(term)) {
                        CallInst * call = CallInst::Create(_timer_stop, params, "", term);
                    }
                }
            }
            
            return false;
        }
    };  // end of struct Hello

}  // end of anonymous namespace

char Hello::ID = 0;

static RegisterPass<Hello> X("hello", "Hello World Pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);


