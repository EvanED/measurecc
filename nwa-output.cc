// @_ZL1x = internal global i32 0, align 4
// @t1 = global %"class._measurecc::Timer" zeroinitializer, align 8


#include <boost/algorithm/string/predicate.hpp>

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/TypeBuilder.h"
#include "llvm/GlobalVariable.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/IRBuilder.h"
#include "llvm/User.h"
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
            return mangled;
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
        FunctionType * _ty_timer_to_void, * _ty_timer_to_double, * _ty_timer_string_to_void;
        Function * _timer_start, * _timer_stop, * _timer_total_time, * _measurecc_ctor, * _measurecc_dtor;

        std::map<std::string, Constant*> _timers;

        void
        define_xtors(Module & m)
        {
            Type * void_ = Type::getVoidTy(getGlobalContext());
            FunctionType * fn_type = FunctionType::get(void_, false);

            _measurecc_ctor = Function::Create(fn_type,
                                               GlobalValue::InternalLinkage,
                                               "_measurecc_ctor",
                                               &m);
            _measurecc_dtor = Function::Create(fn_type,
                                               GlobalValue::InternalLinkage,
                                               "_measurecc_dtor",
                                               &m);

            assert(_measurecc_ctor->empty());
            assert(_measurecc_dtor->empty());

            BasicBlock * ctor_block = BasicBlock::Create(getGlobalContext(),
                                                         "entry",
                                                         _measurecc_ctor);
            BasicBlock * dtor_block = BasicBlock::Create(getGlobalContext(),
                                                         "entry",
                                                         _measurecc_dtor);

            Constant * timer_ctor = m.getOrInsertFunction("_ZN10_measurecc5TimerC1EPKc", _ty_timer_string_to_void);
            Constant * timer_dtor = m.getOrInsertFunction("_ZN10_measurecc5TimerD1Ev", _ty_timer_to_void);

            for(std::map<std::string, Constant*>::const_iterator timerit = _timers.begin();
                timerit != _timers.end(); ++timerit)
            {
                std::string const & func_name = timerit->first;
                Constant * timer = timerit->second;

                Constant * init = ConstantDataArray::getString(getGlobalContext(), demangle(func_name));
                Constant * name_holder = new GlobalVariable(m,
                                                            init->getType(),
                                                            false,
                                                            GlobalValue::InternalLinkage,
                                                            init,
                                                            "_measurecc_holder_" + func_name);
                

                Type * int32 = Type::getInt32Ty(getGlobalContext());
                Constant * zero = ConstantInt::get(int32, 0, true);
                std::vector<Value*> gepi_params(2);
                gepi_params[0] = zero;
                gepi_params[1] = zero;
                Instruction * name_holder_address =
                    GetElementPtrInst::CreateInBounds(name_holder, gepi_params, "", ctor_block);

                std::vector<Value*> ctor_params(2);
                ctor_params[0] = timer;
                ctor_params[1] = name_holder_address;

                std::vector<Value*> dtor_params(1);
                dtor_params[0] = timer;

                CallInst::Create(timer_ctor, ctor_params, "", ctor_block);
                CallInst::Create(timer_dtor, dtor_params, "", dtor_block);
            }
            ReturnInst::Create(getGlobalContext(), ctor_block);
            ReturnInst::Create(getGlobalContext(), dtor_block);
        }

        void
        add_xtor_call(Module & m, Function * f, std::string const & varname)
        {
            GlobalVariable * xtors = m.getGlobalVariable(varname, true);
            std::vector<Constant*> xtor_list;

            if (xtors != NULL) {
                Constant * init = xtors->getInitializer();

                for (User::value_op_iterator op = init->value_op_begin();
                     op != init->value_op_end(); ++op)
                {
                    xtor_list.push_back(cast<Constant>(*op));
                }
            }

            // Make a new entry:
            Type * void_ = Type::getVoidTy(getGlobalContext());
            Type * int32 = Type::getInt32Ty(getGlobalContext());

            std::vector<Type*> xtor_entry_vec(2);
            xtor_entry_vec[0] = int32;
            xtor_entry_vec[1] = PointerType::get(FunctionType::get(void_, false), 0);
            StructType * xtor_entry_type = StructType::get(getGlobalContext(), xtor_entry_vec);

            std::vector<Constant*> entry_vec(2);
            entry_vec[0] = ConstantInt::get(int32, 65535, true);
            entry_vec[1] = f;
            assert(entry_vec[1]);
            Constant * xtor_entry = ConstantStruct::get(xtor_entry_type, entry_vec);
            
            // Append it to the list, then make that the new global_xtors
            xtor_list.push_back(xtor_entry);

            ArrayType * global_xtors_type = ArrayType::get(xtor_entry_type, xtor_list.size());
            Constant * new_xtors = ConstantArray::get(global_xtors_type, xtor_list);

            if (xtors != NULL) {
                xtors->removeFromParent();
                delete xtors;
            }
            GlobalVariable * new_global_xtors = new GlobalVariable(m,
                                                                   global_xtors_type,
                                                                   false,
                                                                   GlobalValue::AppendingLinkage,
                                                                   new_xtors,
                                                                   varname);
        }
        

        Constant *
        declare_counter(Module & m, Function & f)
        {
            Type * int32 = Type::getInt32Ty(getGlobalContext());
            Constant * zero = ConstantInt::get(int32, 0, true);
            GlobalVariable * g = new GlobalVariable(m,
                                                    int32,
                                                    false,
                                                    GlobalValue::ExternalLinkage,
                                                    zero,
                                                    "_measurecc_counter_" + f.getName().str());
            return g;
        }

        Constant *
        declare_timer(Module & m, Function & f)
        {
            Constant * zero = ConstantAggregateZero::get(_timer_type);
            GlobalVariable * g = new GlobalVariable(m,
                                                    _timer_type,
                                                    false,
                                                    GlobalValue::InternalLinkage,
                                                    zero,
                                                    "_measurecc_timer_" + f.getName().str());
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

            std::vector<Type*> ctor_param(2);
            ctor_param[0] = timer_p;
            ctor_param[1] = PointerType::get(Type::getInt8Ty(getGlobalContext()), 0);

            _ty_timer_to_void = FunctionType::get(void_, timer_param, false);
            _ty_timer_string_to_void = FunctionType::get(void_, ctor_param, false);
            _ty_timer_to_double = FunctionType::get(double_, timer_param, false);

            _timer_start = Function::Create(_ty_timer_to_void,
                                            GlobalValue::ExternalLinkage,
                                            "_ZN10_measurecc5Timer5startEv",
                                            &m);

            _timer_stop = Function::Create(_ty_timer_to_void,
                                           GlobalValue::ExternalLinkage,
                                           "_ZN10_measurecc5Timer4stopEv",
                                           &m);

            _timer_total_time = Function::Create(_ty_timer_to_double,
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
                if (command == "time") {
                    time_funcs.insert(func);
                }
                else if (command == "count") {
                    count_funcs.insert(func);
                }
                else {
                    assert(false && "did not say time or count");
                }
            }
        }

        virtual bool runOnModule(Module &m) {
            _timers.clear();
            declare_timer_stuff(m);
            for (Module::iterator func = m.begin();
                 func != m.end(); ++func)
            {
                doFunction(m, *func);
            }

            define_xtors(m);
            add_xtor_call(m, _measurecc_ctor, "llvm.global_ctors");
            add_xtor_call(m, _measurecc_dtor, "llvm.global_dtors");
            return false;
        }

        bool doFunction(Module & m, Function & f) {
            std::string demangled_name = demangle(f.getName());
            if (count_funcs.count(demangled_name) == 0
                && time_funcs.count(demangled_name) == 0)
            {
                return false;
            }
            if (f.empty()) {
                std::cerr << "Warning: function " << f.getName().str() << " is empty. Cannot instrument.\n";
                return false;
            }

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
                Constant * timer = declare_timer(m, f);
                std::vector<Value*> params(1);
                params[0] = timer;

                _timers[f.getName().str()] = timer;

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


