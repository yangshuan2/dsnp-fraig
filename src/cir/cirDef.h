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
#include <bitset>
#include <string>
#include "myHashMap.h"

using namespace std;

// TODO: define your own typedef or enum

class CirGate;
class CirMgr;
class SatSolver;

typedef vector<CirGate*>           GateList;
typedef vector<unsigned>           IdList;
typedef vector<size_t>             FECGroup;

enum GateType
{
   UNDEF_GATE = 0,
   PI_GATE    = 1,
   PO_GATE    = 2,
   AIG_GATE   = 3,
   CONST_GATE = 4,

   TOT_GATE
};

enum Effort
{
   LOW_EFF    = 0,
   MEDIUM_EFF = 1,
   HIGH_EFF   = 2,
   SUPER_EFF  = 3,

   TOT_EFF
};

class TwoFanins
{
public:
   TwoFanins(size_t a , size_t b) : fanin1(a), fanin2(b) {}
   ~TwoFanins() {}
   bool operator == (const TwoFanins& t) const;
   size_t operator () () const {
      size_t x = fanin1 * 0.578125;
      size_t y = fanin2 * 0.578125;
      return (x ^ y);
   }
   
   size_t fanin1;
   size_t fanin2;
};

class SimValue
{
public:
   SimValue() : _value(0) {}
   SimValue(size_t v) : _value(v) {}
   SimValue& operator = (size_t a)   { _value = a; return *this; }
   SimValue& operator <<= (int a)    { _value <<= a; return *this; }
   SimValue& operator += (int a)     { _value += a; return *this; }
   size_t    operator () () const    { return (~_value) ^ (_value >> 8); }

   size_t operator ^ (bool i) const { if(i) return ~_value; else return _value; }
   size_t operator & (const SimValue& v) const { return _value & v._value; }
   bool   operator == (const SimValue& v) const { return _value == v._value; }

   friend ostream& operator << (ostream& os, const SimValue& v);

   size_t _value;
};

#endif // CIR_DEF_H
