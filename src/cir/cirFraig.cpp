/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include <queue>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include "myHashMap.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
// _floatList may be changed.
// _unusedList and _undefList won't be changed
void
CirMgr::strash()
{
   HashMap<TwoFanins, CirGate*> hashMap(_dfsList.size());
   for(unsigned i = 0; i < _dfsList.size(); i++) {
      if(_dfsList[i]->getTypeStr() != "AIG") continue;
      TwoFanins fanins = _dfsList[i]->getFanins();
      CirGate* mergeGate;
      if(hashMap.query(fanins, mergeGate)) {
         // merge
         _dfsList[i]->mergeSTR(mergeGate);
         gateMap[_dfsList[i]->getID()] = 0;
      }
      else
         hashMap.insert(fanins, _dfsList[i]);
   }
   updateGateLists();
   sortAllFanouts();
   DFS();
}

void
CirMgr::fraig()
{
   // initialize SATModel
   SATModel satModel(gateMap.size());
   satModel.setGate(constGate);
   for(unsigned i = 0; i < _dfsList.size(); i++) {
      if(_dfsList[i]->getTypeStr() != "PO")
         satModel.setGate(_dfsList[i]);
   }

   // collect SAT patterns
   vector<SimValue> patterns;
   patterns.resize(PIs.size());
   unsigned patternNumber = 0;

   // the list of pairs of gates to merge
   vector<pair<size_t, size_t>> mergeList;
   
   // to iterate a single FEC group
   unsigned checkTimes = 0;

   // to avoid proving identical statements
   vector<bool> checkMap;
   checkMap.resize(gateMap.size(), false);
   checkMap[0] = true;

   // check by order of _dfsList
   // if UNSAT => push merge list, target merging thisGate
   // if SAT => push patterns
   for(unsigned i = 0; i < _dfsList.size(); i++) {
      if(_dfsList[i]->getTypeStr() != "AIG") continue;

      size_t fecGrp_size_t = cirMgr->getFECGrp(_dfsList[i]->getID());
      if(fecGrp_size_t == 0) continue;

      FECGroup* fecGrp = (FECGroup*)(fecGrp_size_t / 2 * 2);
      if(checkTimes >= fecGrp->size()) { checkTimes = 0; continue; }

      bool inv = CirGate::isInverting(fecGrp_size_t) ^ 
         CirGate::isInverting((*fecGrp)[0]);
      size_t thisGate = size_t(_dfsList[i]) ^ inv;
      size_t target = (*fecGrp)[checkTimes];

      if(thisGate == target) { 
         checkMap[CirGate::unmask(thisGate)->getID()] = true;
         checkTimes++; i--; continue; 
      }
      if(target == 0) { checkTimes++; i--; continue; }
      if(checkMap[CirGate::unmask(target)->getID()] == false) {
         checkTimes++; i--; continue;
      }

      checkMap[CirGate::unmask(thisGate)->getID()] = true;
      cout << "\r                                   \r";
      
      cout << "Proving (";
      if(CirGate::isInverting(target)) cout << '!';
      cout << CirGate::unmask(target)->getID() << ", ";
      if(CirGate::isInverting(thisGate)) cout << '!';
      cout << CirGate::unmask(thisGate)->getID() << ")...";
      cout.flush();
      
      if(satModel.prove(thisGate, target)) {
         cout << "SAT!!";
         cout.flush();

         for(unsigned j = 0; j < PIs.size(); j++) {
            int satVal = satModel.getValue(PIs[j]->getID());
            assert(satVal != -1);
            patterns[j] << 1;
            if(satVal == 1)
               patterns[j] += 1;
         }
         patternNumber++;

         // checkTimes++; i--;

         if(checkTimes >= 8) { checkTimes++; i--; }
         else { deleteFromFECGrp(CirGate::unmask(thisGate)); checkTimes = 0; }
      }
      else {
         cout << "UNSAT!!";
         cout.flush();

         mergeList.push_back(make_pair(target, thisGate));
         deleteFromFECGrp(CirGate::unmask(thisGate));
         checkTimes = 0;
      }
      
      if(patternNumber == 32) {
         cout << "\rUpdating by SAT... ";
         simulateAll(patterns);
         identifyFECs();
         sortFECGrps();
         cout << endl;
         patternNumber = 0;
         checkTimes = 0;
      }
      else if(mergeList.size() > 400) {
         cout << "\r                                   \r";
         for(unsigned j = 0; j < mergeList.size(); j++) {
            CirGate* thisG = CirGate::unmask(mergeList[j].second);
            CirGate* trgtG = CirGate::unmask(mergeList[j].first);
            bool inv =  CirGate::isInverting(mergeList[j].second) ^
               CirGate::isInverting(mergeList[j].first);
            thisG->mergeFRAIG(trgtG, inv);
            gateMap[thisG->getID()] = 0;
         }
         cout << "Updating by UNSAT... ";
         identifyFECs();
         sortFECGrps();
         mergeList.clear();
         cout << endl;

         cout << "Updating by SAT... ";
         simulateAll(patterns);
         identifyFECs();
         sortFECGrps();
         cout << endl;
         patternNumber = 0;
         checkTimes = 0;
      }
   }

   cout << "\r                                   \r";
   for(unsigned j = 0; j < mergeList.size(); j++) {
      CirGate* thisG = CirGate::unmask(mergeList[j].second);
      CirGate* trgtG = CirGate::unmask(mergeList[j].first);
      bool inv =  CirGate::isInverting(mergeList[j].second) ^
         CirGate::isInverting(mergeList[j].first);
      thisG->mergeFRAIG(trgtG, inv);
      gateMap[thisG->getID()] = 0;
   }
   cout << "Updating by UNSAT... ";
   identifyFECs();
   sortFECGrps();
   mergeList.clear();
   cout << endl;

   cout << "Updating by SAT... ";
   simulateAll(patterns);
   identifyFECs();
   sortFECGrps();
   cout << endl;

   simulated = false;

   updateGateLists();
   sortAllFanouts();
   DFS();

   strash();

}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
void
CirGate::mergeSTR(CirGate* mergeGate)
{
   rmRelatingFanouts();
   for(unsigned i = 0; i < fanouts.size(); i++) {
      unmask(fanouts[i])->newFanin(this, mergeGate, false);
      mergeGate->setFanout(unmask(fanouts[i]), isInverting(fanouts[i]));
   }
   // a merging b => b be deleted
   cout << "Strashing: " << mergeGate->getID()
        << " merging " << getID() << "..." << endl;
}

void
CirGate::mergeFRAIG(CirGate* mergeGate, bool inv)
{
   rmRelatingFanouts();
   for(unsigned i = 0; i < fanouts.size(); i++) {
      unmask(fanouts[i])->newFanin(this, mergeGate, inv);
      mergeGate->setFanout(unmask(fanouts[i]), isInverting(fanouts[i]) ^ inv);
   }
   // a merging b => b be deleted
   cout << "Fraig: " << mergeGate->getID() << " merging ";
   if(inv) cout << '!';
   cout << getID() << "..." << endl;
}

void
CirMgr::deleteFromFECGrp(CirGate* gate)
{
   if(fecGrpMap[gate->getID()] == 0) return;
   FECGroup* fecGrp = fecGrps[fecGrpMap[gate->getID()] - 1];
   for(unsigned i = 0; i < fecGrp->size(); i++) {
      if(CirGate::unmask((*fecGrp)[i]) == gate)
         (*fecGrp)[i] = 0;
   }
   fecGrpMap[gate->getID()] = 0;
}

/********************************************/
/*      Member functions of SATModel        */
/********************************************/
void
SATModel::setGate(CirGate* gate)
{
   Var vf = solver.newVar();
   varMap[gate->getID()] = vf;
   if(gate->getTypeStr() == "CONST") {
      Var vv = solver.newVar();
      varMap[0] = vv;
      solver.addAigCNF(vv, vf, false, vf, true);
   }
   if(gate->getTypeStr() != "AIG") return;

   Var  va = varMap[gate->getFaninLit(1) / 2];
   Var  vb = varMap[gate->getFaninLit(2) / 2];
   bool fa = gate->getFaninLit(1) % 2;
   bool fb = gate->getFaninLit(2) % 2;

   solver.addAigCNF(vf, va, fa, vb, fb);
}

bool
SATModel::prove(size_t a, size_t b)
{
   Var  vf = solver.newVar();
   Var  va = varMap[CirGate::unmask(a)->getID()];
   Var  vb = varMap[CirGate::unmask(b)->getID()];
   bool fa = CirGate::isInverting(a);
   bool fb = CirGate::isInverting(b);

   solver.addXorCNF(vf, va, fa, vb, fb);
   solver.assumeRelease();
   solver.assumeProperty(vf, true);

   return solver.assumpSolve();
}
