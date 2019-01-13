/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <queue>
#include "cirDef.h"
#include "sat.h"

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

class CirGate;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
class CirGate
{
public:
   CirGate(unsigned i, unsigned ln, GateType ts) : id(i), lineNo(ln), type(ts), _ref(0) {}
   virtual ~CirGate() {}

   // Basic access methods
   string   getTypeStr() const { 
      switch(type) {
         case UNDEF_GATE: return "UNDEF";
         case PI_GATE:    return "PI";
         case PO_GATE:    return "PO";
         case AIG_GATE:   return "AIG";
         case CONST_GATE: return "CONST";
         default:         return "";
      }
   }
   unsigned getLineNo() const { return lineNo; }
   unsigned getID() const { return id; }
   string   getGateName() const { return gateName; }
   SimValue getSimValue() const { return value; }
   bool     isAig() const { return type == AIG_GATE; }

   virtual unsigned getFaninLit(int=0) const { return 0; }
   virtual void getFloatingFanin(CirGate*&, CirGate*&) const {}
   virtual TwoFanins getFanins() const { return TwoFanins(0, 0); }

   // Printing functions
   virtual void printGate() const = 0;
   virtual void writeGate(ostream&) const {}
   virtual void countGate(unsigned&) const {}
   virtual void printFanin(int, int, bool) const;
   virtual void printFanout(int, int, bool) const;
   void reportGate() const;
   void reportFanin(int level) const;
   void reportFanout(int level) const;

   // Returning gate status
   virtual bool haveFloatingFanin() const { return false; }
   bool definedNotUsed() const { return fanouts.empty(); }

   // Setting fanins/fanouts
   virtual bool setFanin(CirGate*, bool=false, int=0) { return false; }
   virtual bool setFanout(CirGate*, bool=false);
   virtual void newFanin(CirGate*, CirGate*, bool) {}
   virtual void rmRelatingFanouts() {}
   void removeFanout(const CirGate*);

   // Optimizing functions
   virtual void trivialOpt(GateList&, CirGate*) {}
   void mergeSTR(CirGate*);
   void mergeFRAIG(CirGate*, bool);

   // For DFS and BFS Traversing
   virtual void dfsTraversal(GateList&) const = 0;
   void bfsTraversal(GateList&, queue<CirGate*>&) const;
   static  void resetGlobalRef() { _global_ref++; }

   // For CirMgr's use
   void setGateName(const string& gn) { gateName = gn; }
   void sortFanouts() { sort(fanouts.begin(), fanouts.end(), compareByID); }

   // Run simulation
   virtual void simulate(SimValue=0) { value = 0; }
   void setSimValue(SimValue v, bool i) { value = v ^ i; }

   // Static helper methods
   static CirGate* unmask(size_t ptr) { return (CirGate*)(ptr / 2 * 2); }
   static bool isInverting(size_t ptr) { return ptr % 2; }
   static struct { 
      bool operator()(size_t i, size_t j) { return unmask(i)->id < unmask(j)->id; } } compareByID;

private:
   unsigned id;
   unsigned lineNo;
   GateType type;
   mutable size_t _ref;
   static  size_t _global_ref;
   
protected:
   string           gateName;
   vector<size_t>   fanouts;
   SimValue         value;
   
   // For DFS Traversing
   bool isVisited() const { return _ref == _global_ref; }
   void visit() const { _ref = _global_ref; }
};

class AIGGate : public CirGate
{
public:
   AIGGate(unsigned i, unsigned ln) : CirGate(i, ln, AIG_GATE) {}
   ~AIGGate() {}
   unsigned getFaninLit(int) const;
   void getFloatingFanin(CirGate*&, CirGate*&) const;
   TwoFanins getFanins() const { return TwoFanins(fanin1, fanin2);}
   void dfsTraversal(GateList&) const;
   void printGate() const;
   void writeGate(ostream&) const;
   void countGate(unsigned&) const;
   void printFanin(int, int, bool) const;
   bool haveFloatingFanin() const;
   bool setFanin(CirGate*, bool, int);
   void newFanin(CirGate*, CirGate*, bool);
   void trivialOpt(GateList&, CirGate*);
   void rmRelatingFanouts();
   void simulate(SimValue);
private:
   size_t fanin1;
   size_t fanin2;
};

class PIGate : public CirGate
{
public:
   PIGate(unsigned i, unsigned ln) : CirGate(i, ln, PI_GATE) {}
   ~PIGate() {}
   void dfsTraversal(GateList&) const;
   void printGate() const;
   void simulate(SimValue v) { value = v; }
};

class POGate : public CirGate
{
public:
   POGate(unsigned i, unsigned ln) : CirGate(i, ln, PO_GATE) {}
   ~POGate() {}
   unsigned getFaninLit(int num) const { return (2 * unmask(fanin)->getID() + isInverting(fanin)); }
   void getFloatingFanin(CirGate*& a, CirGate*&) const { if(unmask(fanin)->getTypeStr() == "UNDEF") a = unmask(fanin); }
   void dfsTraversal(GateList&) const;
   void printGate() const;
   void writeGate(ostream& os) const { unmask(fanin)->writeGate(os); }
   void countGate(unsigned& cnt) const { unmask(fanin)->countGate(cnt); }
   void printFanin(int, int, bool) const;
   void printFanout(int, int, bool) const;
   bool haveFloatingFanin() const;
   bool setFanin(CirGate*, bool, int);
   bool setFanout(CirGate* cg, bool inv) { return false; }
   void newFanin(CirGate* o, CirGate* n, bool i) { setFanin(n, i ^ isInverting(fanin), 0); }
   void rmRelatingFanouts() { unmask(fanin)->removeFanout(this); }
   void simulate(SimValue) { setSimValue(unmask(fanin)->getSimValue(), isInverting(fanin)); }
private:
   size_t fanin;
};

class UNDEFGate : public CirGate
{
public:
   UNDEFGate(unsigned i) : CirGate(i, 0, UNDEF_GATE) {}
   ~UNDEFGate() {}
   void dfsTraversal(GateList&) const { return; }
   void printGate() const { return; }
};

class CONSTGate : public CirGate
{
public:
   CONSTGate() : CirGate(0, 0, CONST_GATE) {}
   ~CONSTGate() {}
   void dfsTraversal(GateList&) const;
   void printGate() const;
};

#endif // CIR_GATE_H
