/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include <string>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void
CirMgr::randomSim()
{
}

void
CirMgr::fileSim(ifstream& patternFile)
{
   string buf;
   unsigned patternNumber = 0;

   while(true) {
      // read pattern files
      bool _quit = false;
      vector<SimValue> patterns;
      patterns.resize(PIs.size());
      for(unsigned i = 0; i < sizeof(void*) * 8; i++) {
         patternFile >> buf;
         if(!patternFile) {
            if(i == 0) _quit = true;
            for(unsigned j = 0; j < buf.size(); j++) 
               patterns[j] << 1;
            continue;
         }
         if(buf.size() != PIs.size()) {
            cerr << "\nError: Pattern(" << buf
                 << ") length(" << buf.size()
                 << ") does not match the number of inputs("
                 << PIs.size() << ") in a circuit!!\n";
            patternNumber -= i;
            _quit = true;
         }
         for(unsigned j = 0; j < buf.size(); j++) {
            if(buf[j] == '0')
               patterns[j] << 1;
            else if(buf[j] == '1') {
               patterns[j] << 1;
               patterns[j] += 1;
            }
            else {
               cerr << "\nError: Pattern(" << buf
                    << ") contains a non-0/1 character(\'"
                    << buf[j] << "\').\n";
               patternNumber -= i;
               _quit = true;
            }
         }

         if(_quit) break;
         patternNumber++;
      }

      if(_quit) break;
      simulateAll(patterns);
      cout << '\r';
      identifyFECs();

      simulated = true;
   }

   cout << "\r" << patternNumber << " patterns simulated." << endl;
   sortFECGrps();
}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/
void
AIGGate::simulate(SimValue)
{
   SimValue val1 = unmask(fanin1)->getSimValue();
   SimValue val2 = unmask(fanin2)->getSimValue();
   val1 = val1 ^ isInverting(fanin1);
   val2 = val2 ^ isInverting(fanin2);
   //if(isInverting(fanin1)) val1 = ~val1;
   //if(isInverting(fanin2)) val2 = ~val2;
   setSimValue(val1 & val2, false);
}

void
CirMgr::sortFECGrps()
{
   for(unsigned i = 0; i < fecGrps.size(); i++)
      sort(fecGrps[i]->begin(), fecGrps[i]->end(), CirGate::compareByID);
   struct {
      bool operator () (FECGroup* i, FECGroup* j) const {
         return CirGate::unmask((*i)[0])->getID() < CirGate::unmask((*j)[0])->getID();
      }
   } compare;
   sort(fecGrps.begin(), fecGrps.end(), compare);

   // generate fecGrpMap
   fecGrpMap.clear();
   fecGrpMap.resize(gateMap.size());
   for(unsigned i = 0; i < fecGrps.size(); i++) {
      for(unsigned j = 0; j < fecGrps[i]->size(); j++) {
         fecGrpMap[CirGate::unmask((*fecGrps[i])[j])->getID()] = i + 1;
      }
   }
}

void
CirMgr::resetFECGrps()
{
   FECGroup* fecGrp = new FECGroup;

   fecGrp->push_back(size_t(constGate));
   for(unsigned i = 0; i < _dfsList.size(); i++) {
      if(_dfsList[i]->getTypeStr() != "AIG") continue;
      fecGrp->push_back(size_t(_dfsList[i]));
   }

   fecGrps.push_back(fecGrp);
}

void
CirMgr::simulateAll(const vector<SimValue>& patterns)
{
   constGate->simulate();
   for(unsigned i = 0; i < PIs.size(); i++) {
      PIs[i]->simulate(patterns[i]);
   }
   for(unsigned i = 0; i < _dfsList.size(); i++) {
      if(_dfsList[i]->getTypeStr() == "PI") continue;
      else _dfsList[i]->simulate();
   }
}

void
CirMgr::identifyFECs()
{
   if(!simulated) resetFECGrps();

   vector<FECGroup*> tmpFecGrps;
   for(unsigned i = 0; i < fecGrps.size(); i++) {
      HashMap<SimValue, FECGroup*> newFecGrps(fecGrps[i]->size());
      for(unsigned j = 0; j < fecGrps[i]->size(); j++) {
         size_t gate = (*fecGrps[i])[j];
         
         if(gate == 0) continue;

         FECGroup* grp;
         SimValue val = CirGate::unmask(gate)->getSimValue() ^ CirGate::isInverting(gate);
          
         if(newFecGrps.query(val, grp)) {
            grp->push_back(gate);
            newFecGrps.update(val, grp);
         }
         else if(newFecGrps.query(val ^ true, grp)) {
            grp->push_back(gate ^= 0x1);
            newFecGrps.update(val ^ true, grp);
         }
         else {
            grp = new FECGroup;
            grp->push_back(gate);
            newFecGrps.insert(val, grp);
         }
      }
      HashMap<SimValue, FECGroup*>::iterator it = newFecGrps.begin();
      for(; it != newFecGrps.end(); ++it) {
         if((*it).second->size() > 1)
            tmpFecGrps.push_back((*it).second);
         else delete (*it).second;
      }
      delete fecGrps[i];
   }
   fecGrps.clear();
   fecGrps = tmpFecGrps;
   cout << "Total #FEC Group = " << fecGrps.size();
   cout.flush();
}
