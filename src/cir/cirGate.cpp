/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirGate::reportGate()", "CirGate::reportFanin()" and
//       "CirGate::reportFanout()" for cir cmds. Feel free to define
//       your own variables and functions.

extern CirMgr *cirMgr;

/**************************************/
/*   class CirGate member functions   */
/**************************************/
size_t CirGate::_global_ref = 1;

void
CirGate::reportGate() const
{
   cout << "================================================================================" << endl;

   //ios init(NULL);
   //init.copyfmt(cout);

   stringstream ss;
   ss << "= " << getTypeStr() << "(" << id << ")";
   if(gateName.size()) ss << "\"" << gateName << "\"";
   ss << ", line " << lineNo;
   string output = ss.str();
   cout << output << endl;

   size_t fecGrp_size_t = cirMgr->getFECGrp(id);
   FECGroup* fecGrp = (FECGroup*)(fecGrp_size_t / 2 * 2);
   bool inv = isInverting(fecGrp_size_t);
   cout << "= FECs:";
   if(fecGrp_size_t != 0)
      for(unsigned i = 0; i < fecGrp->size(); i++) {
         if(unmask((*fecGrp)[i]) == this) continue;
         cout << ' ';
         if(isInverting((*fecGrp)[i]) != inv) cout << '!';
         cout << unmask((*fecGrp)[i])->id;
      }
   cout << endl;

   cout << "= Value: " << value << endl;
   
   //cout.copyfmt(init);
   
   cout << "================================================================================" << endl;
}

void
CirGate::reportFanin(int level) const
{
   assert (level >= 0);
   resetGlobalRef();
   printFanin(level, 0, false);
}

void
CirGate::reportFanout(int level) const
{
   assert (level >= 0);
   resetGlobalRef();
   printFanout(level, 0, false);
}

bool
CirGate::setFanout(CirGate* cg, bool inv)
{
   size_t tmp = (size_t)cg;
   if(inv) tmp |= 0x1;
   fanouts.push_back(tmp);
   return true;
}

void
CirGate::printFanin(int level, int spaces, bool mark) const
{
   for(int i = 0; i < spaces; i++) cout << "  ";
   if(mark) cout << "!";
   cout << getTypeStr() << " " << getID() << endl;
}

void
CirGate::printFanout(int level, int spaces, bool mark) const
{
   for(int i = 0; i < spaces; i++) cout << "  ";
   if(mark) cout << "!";
   cout << getTypeStr() << " " << getID();
   
   if(fanouts.size() == 0) { cout << endl; return; }

   if(level != 0 && isVisited()) { cout << " (*)" << endl; return; }

   cout << endl;
   if(level != 0) visit();

   if(level == 0) return;

   for(unsigned i = 0; i < fanouts.size(); i++)
      unmask(fanouts[i])->printFanout(level - 1, spaces + 1, isInverting(fanouts[i]));
}

void
CirGate::removeFanout(const CirGate* torm)
{
   vector<size_t>::iterator it = fanouts.begin();
   for(; it != fanouts.end(); ++it)
      if(unmask(*it) == torm){
         // cerr << unmask(*it)->getID() << endl;
         it = fanouts.erase(it);
         --it;
      }
}

void
CirGate::bfsTraversal(GateList& _bfsList, queue<CirGate*>& _queue) const
{
   if(isVisited()) return;
   else visit();

   _bfsList.push_back((CirGate*)this);
   for(unsigned i = 0; i < fanouts.size(); i++) {
      _queue.push(unmask(fanouts[i]));
   }
}

/********************
***      AIG      ***
********************/

void
AIGGate::dfsTraversal(GateList& _dfsList) const
{
   if(isVisited()) return;
   else visit();

   unmask(fanin1)->dfsTraversal(_dfsList);
   unmask(fanin2)->dfsTraversal(_dfsList);

   _dfsList.push_back((CirGate*)this);
}

void
AIGGate::printGate() const
{
   cout << "AIG " << getID() << " ";
   if(unmask(fanin1)->getTypeStr() == "UNDEF") cout << "*";
   if(isInverting(fanin1)) cout << "!";
   cout << unmask(fanin1)->getID() << " ";
   if(unmask(fanin2)->getTypeStr() == "UNDEF") cout << "*";
   if(isInverting(fanin2)) cout << "!";
   cout << unmask(fanin2)->getID() << endl;
   
}

void
AIGGate::writeGate(ostream& os) const
{
   if(isVisited()) return;
   else visit();

   unmask(fanin1)->writeGate(os);
   if(unmask(fanin1) != unmask(fanin2))
      unmask(fanin2)->writeGate(os);

   os << 2 * getID() << " "
      << getFaninLit(1) << " "
      << getFaninLit(2) << "\n";
}

void
AIGGate::countGate(unsigned& cnt) const
{
   if(isVisited()) return;
   else visit();

   unmask(fanin1)->countGate(cnt);
   if(unmask(fanin1) != unmask(fanin2))
      unmask(fanin2)->countGate(cnt);

   cnt++;
}

unsigned
AIGGate::getFaninLit(int num) const
{
   if(num == 1) return (2 * unmask(fanin1)->getID() + isInverting(fanin1));
   if(num == 2) return (2 * unmask(fanin2)->getID() + isInverting(fanin2));
   return 0;
}

void
AIGGate::getFloatingFanin(CirGate*& a, CirGate*& b) const
{
   if(unmask(fanin1)->getTypeStr() == "UNDEF") a = unmask(fanin1);
   if(unmask(fanin2)->getTypeStr() == "UNDEF") b = unmask(fanin2);
}

bool
AIGGate::setFanin(CirGate* cg, bool inv, int num)
{
   size_t tmp = (size_t)cg;
   if(inv) tmp |= 0x1;
   if(num == 1) fanin1 = tmp;
   else if(num == 2) fanin2 = tmp;
   else return false;
   return true;
}

void
AIGGate::newFanin(CirGate* o, CirGate* n, bool i)
{
   assert(unmask(fanin1) == o || unmask(fanin2) == o);
   if(unmask(fanin1) == o) setFanin(n, i != isInverting(fanin1), 1);
   if(unmask(fanin2) == o) setFanin(n, i != isInverting(fanin2), 2);
}

void
AIGGate::rmRelatingFanouts()
{
   unmask(fanin1)->removeFanout(this);
   unmask(fanin2)->removeFanout(this);
}

bool
AIGGate::haveFloatingFanin() const
{
   if(unmask(fanin1)->getTypeStr() == "UNDEF") return true;
   if(unmask(fanin2)->getTypeStr() == "UNDEF") return true;
   return false;
}

void
AIGGate::printFanin(int level, int spaces, bool mark) const
{
   for(int i = 0; i < spaces; i++) cout << "  ";
   if(mark) cout << "!";
   cout << "AIG " << getID();

   if(level != 0 && isVisited()) { cout << " (*)" << endl; return; }

   cout << endl;
   if(level != 0) visit();

   if(level == 0) return;

   unmask(fanin1)->printFanin(level - 1, spaces + 1, isInverting(fanin1));
   unmask(fanin2)->printFanin(level - 1, spaces + 1, isInverting(fanin2));
}

/********************
***       PI      ***
********************/

void
PIGate::dfsTraversal(GateList& _dfsList) const
{
   if(isVisited()) return;
   else visit();

   _dfsList.push_back((CirGate*)this);
}

void
PIGate::printGate() const
{
   cout << "PI  " << getID();
   if(gateName.size())
      cout << " (" << gateName << ")";
   cout << endl;
   
}

/********************
***       PO      ***
********************/

void
POGate::dfsTraversal(GateList& _dfsList) const
{
   if(isVisited()) return;
   else visit();

   unmask(fanin)->dfsTraversal(_dfsList);

   _dfsList.push_back((CirGate*)this);
}

void
POGate::printGate() const
{
   cout << "PO  " << getID() << " ";
   if(unmask(fanin)->getTypeStr() == "UNDEF") cout << "*";
   if(isInverting(fanin)) cout << "!";
   cout << unmask(fanin)->getID();
   if(gateName.size())
      cout << " (" << gateName << ")";
   cout << endl;
   
}

void
POGate::printFanin(int level, int spaces, bool mark) const
{
   for(int i = 0; i < spaces; i++) cout << "  ";
   if(mark) cout << "!";
   cout << "PO " << getID();

   if(level != 0 && isVisited()) { cout << " (*)" << endl; return; }

   cout << endl;
   if(level != 0) visit();

   if(level == 0) return;

   unmask(fanin)->printFanin(level - 1, spaces + 1, isInverting(fanin));
}

void
POGate::printFanout(int level, int spaces, bool mark) const
{
   for(int i = 0; i < spaces; i++) cout << "  ";
   if(mark) cout << "!";
   cout << getTypeStr() << " " << getID() << endl;
}

bool
POGate::setFanin(CirGate* cg, bool inv, int num) 
{
   size_t tmp = (size_t)cg;
   if(inv) tmp |= 0x1;
   fanin = tmp;
   return true;
}

bool
POGate::haveFloatingFanin() const 
{ 
   if(unmask(fanin)->getTypeStr() == "UNDEF") return true;
   return false; 
}

/********************
***     OTHERS    ***
********************/

void
CONSTGate::dfsTraversal(GateList& _dfsList) const
{
   if(isVisited()) return;
   else visit();

   _dfsList.push_back((CirGate*)this);
}

void
CONSTGate::printGate() const
{
   cout << "CONST0" << endl;
   
}

/********************
***    cirDef.h   ***
********************/

bool
TwoFanins::operator == (const TwoFanins& t) const
{
   if(fanin1 / 2 == t.fanin1 / 2 && fanin2 / 2 == t.fanin2 / 2)
      if((fanin1 % 2 == t.fanin1 % 2) && (fanin2 % 2 == t.fanin2 % 2))
         return true;
   if(fanin1 / 2 == t.fanin2 / 2 && fanin2 / 2 == t.fanin1 / 2)
      if((fanin1 % 2 == t.fanin2 % 2) && (fanin2 % 2 == t.fanin1 % 2))
         return true;
   return false;
}

ostream& 
operator << (ostream& os, const SimValue& v) 
{
   size_t tmp = v._value;
   for(unsigned i = 0; i < sizeof(void*) * 8; i++) {
      if(i != 0 && i % 8 == 0) os << "_";
      os << tmp % 2;
      tmp /= 2;
   }
   return os;
}
