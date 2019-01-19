/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include <string>
#include <limits>
#include <queue>
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
   unsigned patternNumber = 0;

   bool _quit = false;
   while(!_quit) {
      vector<SimValue> patterns;
      patterns.resize(PIs.size());

      unsigned intbits = sizeof(int) * 8;
      unsigned sztbits = sizeof(size_t) * 8;

      for(unsigned i = 0; i < PIs.size(); i++) {
         for(unsigned j = 0; j < sztbits; j += intbits) {
            patterns[i] <<= intbits;
            patterns[i] += rnGen(INT_MAX);
         }
      }
      patternNumber += sztbits;
      
      simulateAll(patterns);
      cout << '\r';
      identifyFECs();
      sortFECGrps();

      if(randomCheckPoint()) _quit = true;

      writeSimulationLog(patternNumber);

      simulated = true;
   }

   cout << "\r" << patternNumber << " patterns simulated." << endl;
   sortFECGrps();

   _simLog = 0;
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
            for(unsigned j = 0; j < buf.size(); j++) 
               patterns[j] <<=1;
            if(i == 0) _quit = true;
            else continue;
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
               patterns[j] <<=1;
            else if(buf[j] == '1') {
               patterns[j] <<=1;
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

      writeSimulationLog(patternNumber);

      simulated = true;
   }

   cout << "\r" << patternNumber << " patterns simulated." << endl;
   sortFECGrps();
   _simLog = 0;
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
   if(fecGrps.size() != 0) {
      for(unsigned i = 0; i < fecGrps.size(); i++) {
         if(fecGrps[i] != 0) delete fecGrps[i];
      }
   }
   fecGrps.clear();

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

bool
CirMgr::randomCheckPoint() const
{
   static vector<unsigned> fecs;
   static vector<unsigned> fec0;

   if(!simulated) {
      fecs.clear(); fec0.clear();
   }

   unsigned magicNumber;
   switch(_effort) {
      case LOW_EFF:
         magicNumber = 3; break;
      case MEDIUM_EFF:
         magicNumber = 5; break;
      case HIGH_EFF:
         magicNumber = 10; break;
      case SUPER_EFF:
         magicNumber = 15; break;
      default:
         magicNumber = 5;
   }

   if(fecGrps.size() == 0) return true;
   if(fecs.size() < magicNumber && fec0.size() < magicNumber) {
      fecs.push_back(fecGrps.size());
      fec0.push_back(fecGrps[0]->size());
      return false;
   }
   for(unsigned i = 0; i < magicNumber - 1; i++) {
      fecs[i] = fecs[i+1];
      fec0[i] = fec0[i+1];
   }
   fecs[magicNumber - 1] = fecGrps.size();
   fec0[magicNumber - 1] = fecGrps[0]->size();
   for(unsigned i = 0; i < magicNumber - 1; i++) {
      if((fecs[i] != fecs[i+1]) ||
         (fec0[i] != fec0[i+1])) return false;
   }
   return true;
}

void
CirMgr::writeSimulationLog(unsigned patternNumber)
{
   if(_simLog == 0) return;

   patternNumber %= (sizeof(size_t) * 8);
   if(patternNumber == 0) patternNumber = (sizeof(size_t) * 8);

   vector<string> patternsPI;
   patternsPI.resize(patternNumber);
   vector<string> patternsPO;
   patternsPO.resize(patternNumber);

   for(unsigned i = 0; i < PIs.size(); i++) {
      stringstream buf;
      buf << PIs[i]->getSimValue();

      string tmp = buf.str();
      string str;
      for(unsigned j = 0; j < tmp.size(); j++) {
         if(tmp[j] != '_') str += tmp[j];
      }

      for(unsigned j = 0; j < patternNumber; j++) {
         patternsPI[j] += str[str.size() - j - 1];
      }
   }

   for(unsigned i = 0; i < POs.size(); i++) {
      stringstream buf;
      buf << POs[i]->getSimValue();

      string tmp = buf.str();
      string str;
      for(unsigned j = 0; j < tmp.size(); j++) {
         if(tmp[j] != '_') str += tmp[j];
      }

      for(unsigned j = 0; j < patternNumber; j++) {
         patternsPO[j] += str[str.size() - j - 1];
      }
   }

   for(unsigned i = 0; i < patternNumber; i++) {
      (*_simLog) << patternsPI[i] << ' ' << patternsPO[i] << endl;
   }
}
