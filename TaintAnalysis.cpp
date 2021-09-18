#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

struct taintInfo
{
  unsigned line;
  StringRef fnName;
};

std::vector<std::string> sinkFn = {"memcpy", "strcpy", "strcat"};
std::vector<std::string> sanitizeFn = {"strlen"};
std::vector<taintInfo> taintFn;
bool g_debug = 0;

namespace {

  void printMap(std::map<StringRef, bool> taintMap)
  {
    for(auto it = taintMap.cbegin(); it != taintMap.cend(); ++it)
    {
      errs() << it->first << " " << it->second << "\n";
    }
  }

  bool isTaintedArg(llvm::CallInst &C, std::map<StringRef, bool> taintMap)
  {
    for(auto iter = C.arg_begin(); iter != C.arg_end(); ++iter)
    {
      StringRef var = iter->get()->getName();
      if((taintMap.find(var) != taintMap.end()) && taintMap.find(var)->second && !var.empty())
      {
        return 1;
      }

      if(g_debug)
      {
        errs() << "in taintedArg " << C.getCalledFunction()->getName() << "\n";
        errs() << var << "\n";
      }
    }
    return 0;
  }

  void sanitizeArg(llvm::CallInst &C, std::map<StringRef, bool> taintMap)
  {
    for(auto iter = C.arg_begin(); iter != C.arg_end(); ++iter)
    {
      StringRef var = iter->get()->getName();
      if((taintMap.find(var) != taintMap.end()) && !var.empty())
      {
        taintMap.find(var)->second = 0;
      }

      if(g_debug)
        errs() << var << " " << taintMap.find(var)->second << "\n";
    }
  }

  void taintStore(Instruction &I, std::map<StringRef, bool> &taintMap)
  {
    StringRef op1 = I.getOperand(0)->getName();
    StringRef op2 = I.getOperand(1)->getName();

    if(taintMap.find(op1)->second && !op2.empty())
      taintMap.insert(std::pair<StringRef, bool>(op2, 1));
    else if(!op2.empty())
      taintMap.insert(std::pair<StringRef, bool>(op2, 0));
    if(g_debug)
    {
      errs() << I << "\n";
      errs() << "store " << op1 << " " << op2 <<"\n";
      if(taintMap.find(op1) != taintMap.end() && taintMap.find(op2) != taintMap.end())
      {
        errs() << "taint " << taintMap.find(op1)->first << " " <<taintMap.find(op1)->second << " ";
        errs() << taintMap.find(op2)->first << " " << taintMap.find(op2)->second << "\n";
      }
    }
  }

  void taintLoad(Instruction &I, std::map<StringRef, bool> &taintMap)
  {
    StringRef op1 = I.getOperand(0)->getName();
    StringRef op2 = I.getName();

    if(taintMap.find(op1)->second && !op2.empty())
      taintMap.insert(std::pair<StringRef, bool>(op2, 1));
    else if(!op2.empty())
      taintMap.insert(std::pair<StringRef, bool>(op2, 0));

    if(g_debug)
    {
      errs() << I <<"\n";
      errs() << "load " << op1 << " " << op2 <<"\n";
      if(taintMap.find(op1) != taintMap.end() && taintMap.find(op2) != taintMap.end())
      {
        errs() << "taint " << taintMap.find(op1)->first << " " <<taintMap.find(op1)->second << " ";
        errs() << taintMap.find(op2)->first << " " << taintMap.find(op2)->second << "\n";
      }
    }
  }

  void taintCall(Instruction &I, std::map<StringRef, bool> &taintMap)
  {
    llvm::CallInst *call_inst = llvm::dyn_cast<llvm::CallInst>(&I); 
    Function* fn = call_inst->getCalledFunction();
    StringRef fnName = fn->getName();

    if(g_debug)
    {
      errs() << I << "\n" << "name " << fnName << "\n";
    }
    if(std::find(sinkFn.begin(), sinkFn.end(), fnName) != sinkFn.end())
    {
      bool flag = isTaintedArg(*call_inst, taintMap);
      taintInfo temp;
      if(flag)
      {
        unsigned line = I.getDebugLoc().getLine();
        temp.line = line;
        temp.fnName = fnName;
        taintFn.push_back(temp);
      }

      if(g_debug)
        errs() << "Tainted fn " << temp.fnName << " line " << temp.line;
    }
    else if(std::find(sanitizeFn.begin(), sanitizeFn.end(), fnName) != sanitizeFn.end())
    {
      sanitizeArg(*call_inst, taintMap);
    }
  }

  void printAnalysis()
  {
    errs() << "WARNING: Tainted arguments passed to these functions:\n";
    for(int i = 0; i < taintFn.size(); i++)
    {
      errs() << taintFn[i].fnName << " at line " << taintFn[i].line <<"\n";
    }
  }
  // This method implements what the pass does
  void visitor(Function &F) {
    std::map<StringRef, bool> taintMap;

    if(g_debug)
      errs() << "Hello from: "<< F.getName() << "\n";

    // taint all the function arguments passed in the function
    for(auto iter = F.arg_begin(); iter != F.arg_end(); ++iter)
    {
      StringRef varName = iter->getName();

      if(!varName.empty())
        taintMap.insert(std::pair<StringRef, bool>(varName, 1));
      if(g_debug)
        errs() << varName << "\n";
    }

    // Check the opcode and take
    // action accordingly
    for (BasicBlock &B: F)
    {
      for (Instruction &I: B)
      {
        unsigned opcode = I.getOpcode();
        if (opcode == llvm::Instruction::Store)
        {
          taintStore(I, taintMap);
        }
        else if (opcode == llvm::Instruction::Load)
        {
          taintLoad(I, taintMap);
        }
        else if (opcode == llvm::Instruction::Call)
        {
          taintCall(I, taintMap);
        }
      }
    }
    printMap(taintMap);
  }

  // Legacy PM implementation
  struct TaintAnalyis : public ModulePass {
    static char ID;
    TaintAnalyis() : ModulePass(ID) {}
    // Main entry point - the name conveys what unit of IR this is to be run on.
    bool runOnModule(Module &M) override {
      for(Function &F: M)
      {
        visitor(F);
      }
      printAnalysis();
      // Doesn't modify the input unit of IR, hence 'false'
      return false;
    }
  };
} // namespace

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------
// The address of this variable is used to uniquely identify the pass. The
// actual value doesn't matter.
char TaintAnalyis::ID = 0;

// This is the core interface for pass plugins. It guarantees that 'opt' will
// recognize LegacyHelloWorld when added to the pass pipeline on the command
// line, i.e.  via '--legacy-hello-world'
static RegisterPass<TaintAnalyis>
X("taint-analysis", "Taint Analysis Pass",
    true, // This pass doesn't modify the CFG => true
    true // This pass is not a pure analysis pass => false
 );
