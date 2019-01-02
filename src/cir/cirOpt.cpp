/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void
CirMgr::sweep()
{
   GateList sortedDFSList = getSortedDFSList();
   for(unsigned i = 1; i < gateMap.size(); i++) {
      if(gateMap[i] != sortedDFSList[i]) {
         assert(sortedDFSList[i] == 0);
         string type = gateMap[i]->getTypeStr();
         if(type == "PI") continue;
         assert(type != "PO");
         cout << "Sweeping: " << type << "(" 
              << i << ") removed..." << endl;
         gateMap[i]->rmRelatingFanouts();
         gateMap[i] = 0;
      }
   }

   updateGateLists();
}

// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()
{
   for(unsigned i = 1; i < gateMap.size(); i++) {
      if(gateMap[i] == 0) continue;
      if(gateMap[i]->getTypeStr() == "AIG")
         gateMap[i]->trivialOpt(gateMap);
   }
   updateGateLists();
   sortAllFanouts();
   DFS();
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/
void
CirMgr::updateGateLists()
{
   GateList tmp;
   GateList::iterator it = AIGs.begin();
   for(; it != AIGs.end(); ++it) {
      unsigned id = (*it)->getID();
      if(gateMap[id] == 0) delete *it;
      else tmp.push_back(*it);
   }
   AIGs = tmp;
   tmp.clear();
   it = UNDEFs.begin();
   for(; it != UNDEFs.end(); ++it) {
      unsigned id = (*it)->getID();
      if(gateMap[id] == 0) delete *it;
      else tmp.push_back(*it);
   }
}

extern CirMgr *cirMgr;

void
AIGGate::trivialOpt(GateList& gateMap)
{
   bool const0    = false;
   bool replacing = false;
   bool replaceL  = false;
   
   //printGate();

   if(unmask(fanin1)->getTypeStr() == "CONST") { // Fanin1 is constant
      if(isInverting(fanin1)) replacing = true;
      else const0 = true;
   }
   else if(unmask(fanin2)->getTypeStr() == "CONST") {
      replaceL = true;
      if(isInverting(fanin2)) replacing = true;
      else const0 = true;
   }
   else if(unmask(fanin1) == unmask(fanin2)) {
      if(isInverting(fanin1) != isInverting(fanin2))
         const0 = true;
      else
         replacing = true;
   }

   size_t fanin;
   if(const0) {
      rmRelatingFanouts();
      for(unsigned i = 0; i < fanouts.size(); i++) {
         unmask(fanouts[i])->newFanin(this, cirMgr->getGate(0), false);
         cirMgr->getGate(0)->setFanout(unmask(fanouts[i]), isInverting(fanouts[i]));
      }
   }
   else if(replacing) {
      rmRelatingFanouts();
      if(replaceL) fanin = fanin1;
      else         fanin = fanin2;
      for(unsigned i = 0; i < fanouts.size(); i++) {
         unmask(fanouts[i])->newFanin(this, unmask(fanin), isInverting(fanin));
         unmask(fanin)->setFanout(unmask(fanouts[i]), isInverting(fanin) != isInverting(fanouts[i]));
      }
   }
   else return;

   cout << "Simplifying: ";
   if(const0) cout << 0;
   else       cout << unmask(fanin)->getID();
   cout << " merging ";
   if(!const0 && isInverting(fanin)) cout << "!";
   cout << getID() << "..." << endl;

   gateMap[getID()] = 0;
}

GateList
CirMgr::getSortedDFSList()
{
   GateList sortedDFSList;
   sortedDFSList.resize(gateMap.size(), 0);
   for(unsigned i = 0; i < _dfsList.size(); i++) {
      unsigned id = _dfsList[i]->getID();
      sortedDFSList[id] = _dfsList[i];
   }
   return sortedDFSList;
}
