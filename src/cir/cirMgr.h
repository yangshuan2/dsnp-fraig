/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

#include "cirDef.h"

extern CirMgr *cirMgr;

class CirMgr
{
public:
   CirMgr() {}
   ~CirMgr();

   // Access functions
   // return '0' if "gid" corresponds to an undefined gate.
   CirGate* getGate(unsigned gid) const { 
      if(gid >= gateMap.size()) return 0;
      return gateMap[gid];
   }

   // Member functions about circuit construction
   bool readCircuit(const string&);

   // Member functions about circuit optimization
   void sweep();
   void optimize();

   // Member functions about simulation
   void randomSim();
   void fileSim(ifstream&);
   void setSimLog(ofstream *logFile) { _simLog = logFile; }

   // Member functions about fraig
   void strash();
   void printFEC() const;
   void fraig();

   // Member functions about circuit reporting
   void printSummary() const;
   void printNetlist() const;
   void printPIs() const;
   void printPOs() const;
   void printFloatGates() const;
   void printFECPairs() const;
   void writeAag(ostream&) const;
   void writeGate(ostream&, CirGate*) const;


private:
   ofstream           *_simLog;

   GateList           PIs;
   GateList           POs;
   GateList           AIGs;
   GateList           UNDEFs;
   CirGate            *constGate;

   GateList           gateMap;
   GateList           _dfsList;

   vector<FECGroup*>  fecGrps;

   // Update info of gates
   void DFS();
   void updateGateLists();
   void sortAllFanouts();

   // Member functions about simulation
   void sortFECGrps();
   void resetFECGrps();
   void simulateAll(const vector<SimValue>&);
   void identifyFECs();

   // Helper access methods
   GateList getSortedDFSList();
};

#endif // CIR_MGR_H
