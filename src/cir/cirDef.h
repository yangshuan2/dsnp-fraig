/****************************************************************************
  FileName     [ cirDef.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic data or var for cir package ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_DEF_H
#define CIR_DEF_H

#include <vector>
#include "myHashMap.h"

using namespace std;

// TODO: define your own typedef or enum

class CirGate;
class CirMgr;
class SatSolver;

typedef vector<CirGate*>           GateList;
typedef vector<unsigned>           IdList;

enum GateType
{
   UNDEF_GATE = 0,
   PI_GATE    = 1,
   PO_GATE    = 2,
   AIG_GATE   = 3,
   CONST_GATE = 4,

   TOT_GATE
};

class TwoFanins
{
public:
   TwoFanins(size_t a , size_t b) : fanin1(a), fanin2(b) {}
   ~TwoFanins() {}
   bool operator == (const TwoFanins& t) const {
      if(fanin1 / 2 == t.fanin1 / 2 && fanin2 / 2 == t.fanin2 / 2)
         if((fanin1 % 2 == t.fanin1 % 2) && (fanin2 % 2 == t.fanin2 % 2))
            return true;
      if(fanin1 / 2 == t.fanin2 / 2 && fanin2 / 2 == t.fanin1 / 2)
         if((fanin1 % 2 == t.fanin2 % 2) && (fanin2 % 2 == t.fanin1 % 2))
            return true;
      return false;
   }
   size_t operator () () const {
      size_t x = fanin1 * 0.578125;
      size_t y = fanin2 * 0.578125;
      return (x ^ y);
   }
private:
   size_t fanin1;
   size_t fanin2;
};

#endif // CIR_DEF_H
