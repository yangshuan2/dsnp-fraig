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
   GateList _bfsList;
   BFS(_bfsList);
   cout << "size of _bfsList: " << _bfsList.size() << endl;
   cout << "size of _dfsList: " << _dfsList.size() << endl;

   // initialize SATModel
   SATModel satModel(gateMap.size());
   satModel.setGate(constGate);
   for(unsigned i = 0; i < PIs.size(); i++) {
      satModel.setGate(PIs[i]);
   }
   
   // collect SAT patterns
   vector<SimValue> patterns;
   patterns.resize(PIs.size());
   unsigned patternNumber = 0;

   // the list of pairs of gates to merge
   vector<size_t> mergeMap;
   mergeMap.resize(gateMap.size(), 0);
   unsigned mergeCount = 0;
   
   // to iterate a single FEC group
   unsigned checkTimes = 0;

   // to avoid proving identical statements
   vector<bool> checkMap;
   checkMap.resize(gateMap.size(), false);
   checkMap[0] = true;

   // check by order of _bfsList
   // if UNSAT => push merge list, target merging thisGate
   // if SAT => push patterns
   GateList& iterateList = _dfsList;

   for(unsigned i = 0; i < iterateList.size(); i++) {
      satModel.setGate(iterateList[i]);
      if(iterateList[i]->getTypeStr() != "AIG") continue;

      if(fecGrpMap[iterateList[i]->getID()] == 0) { checkTimes = 0; continue; }
      FECGroup* fecGrp = fecGrps[fecGrpMap[iterateList[i]->getID()] - 1];
      if(checkTimes >= fecGrp->size()) { checkTimes = 0; continue; }

      size_t thisGate = 0;
      for(unsigned j = 0; j < fecGrp->size(); j++) {
         if(CirGate::unmask((*fecGrp)[j]) == iterateList[i])
            thisGate = (*fecGrp)[j];
      }
      assert(thisGate != 0);

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

         // if(CirGate::unmask(target) != constGate) { checkTimes++; i--; }
         // else checkTimes = 0;
         checkTimes++; i--;
      }
      else {
         cout << "UNSAT!!";
         cout.flush();

         if(CirGate::isInverting(thisGate)) target ^= 0x1;
         mergeMap[iterateList[i]->getID()] = target;
         mergeCount++;
         deleteFromFECGrp(CirGate::unmask(thisGate));
         checkTimes = 0;
      }
      
      if(patternNumber == sizeof(void*) * 8) {
         cout << "\rUpdating by SAT... ";
         simulateAll(patterns);
         identifyFECs();
         sortFECGrps();
         cout << endl;
         patterns.assign(PIs.size(), 0);
         patternNumber = 0;
         checkTimes = 0;
      }
      else if(mergeCount > 400) {
         cout << "\r                                   \r";
         for(unsigned j = 0; j < _dfsList.size(); j++) {
            CirGate* thisG = _dfsList[j];
            if(mergeMap[thisG->getID()] == 0) continue;
            CirGate* trgtG = CirGate::unmask(mergeMap[thisG->getID()]);
            bool inv =  CirGate::isInverting(mergeMap[thisG->getID()]);
            thisG->mergeFRAIG(trgtG, inv);
            satModel.setGate(trgtG);
            gateMap[thisG->getID()] = 0;
         }
         cout << "Updating by UNSAT... ";
         identifyFECs();
         sortFECGrps();
         mergeMap.assign(gateMap.size(), 0);
         mergeCount = 0;
         cout << endl;

         cout << "Updating by SAT... ";
         simulateAll(patterns);
         identifyFECs();
         sortFECGrps();
         cout << endl;
         patterns.assign(PIs.size(), 0);
         patternNumber = 0;
         checkTimes = 0;
      }

   }

   cout << "\r                                   \r";
   for(unsigned j = 0; j < _dfsList.size(); j++) {
      CirGate* thisG = _dfsList[j];
      if(mergeMap[thisG->getID()] == 0) continue;
      CirGate* trgtG = CirGate::unmask(mergeMap[thisG->getID()]);
      bool inv =  CirGate::isInverting(mergeMap[thisG->getID()]);
      thisG->mergeFRAIG(trgtG, inv);
      satModel.setGate(trgtG);
      gateMap[thisG->getID()] = 0;
   }
   cout << "Updating by UNSAT... ";
   identifyFECs();
   sortFECGrps();
   mergeMap.assign(gateMap.size(), 0);
   mergeCount = 0;
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

   _bfsList.clear();
   BFS(_bfsList);
   assert(_bfsList.size() == _dfsList.size());

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

void
CirMgr::BFS(GateList& _bfsList) const
{
   if(PIs.size() == 0) return;
   GateList dfsMap = getSortedDFSList();
   vector<bool> traverseMap;
   traverseMap.resize(gateMap.size(), false);
   CirGate::resetGlobalRef();
   queue<CirGate*> _queue;
   if(_dfsList[0] != 0)
      _queue.push(constGate);
   while(!_queue.empty()) {
      if(_queue.front()->getTypeStr() == "AIG" &&
         traverseMap[_queue.front()->getID()] == false)
         traverseMap[_queue.front()->getID()] = true;
      else if(dfsMap[_queue.front()->getID()] != 0)
         _queue.front()->bfsTraversal(_bfsList, _queue);
      _queue.pop();
   }
   for(unsigned i = 0; i < PIs.size(); i++) {
      _queue.push(PIs[i]);
      while(!_queue.empty()) {
         if(_queue.front()->getTypeStr() == "AIG" &&
            traverseMap[_queue.front()->getID()] == false)
            traverseMap[_queue.front()->getID()] = true;
         else if(dfsMap[_queue.front()->getID()] != 0)
            _queue.front()->bfsTraversal(_bfsList, _queue);
         _queue.pop();
      }
   }
   for(unsigned i = 0; i < UNDEFs.size(); i++) {
      _queue.push(UNDEFs[i]);
      while(!_queue.empty()) {
         if(_queue.front()->getTypeStr() == "AIG" &&
            traverseMap[_queue.front()->getID()] == false)
            traverseMap[_queue.front()->getID()] = true;
         else if(dfsMap[_queue.front()->getID()] != 0)
            _queue.front()->bfsTraversal(_bfsList, _queue);
         _queue.pop();
      }
   }
}

/********************************************/
/*      Member functions of SATModel        */
/********************************************/
void
SATModel::setGate(CirGate* gate)
{
   Var vf = solver.newVar();
   if(gate->getTypeStr() == "CONST" || 
      gate->getTypeStr() == "UNDEF") {
      Var vv = solver.newVar();
      varMap[gate->getID()] = vv;
      solver.addAigCNF(vv, vf, false, vf, true);
      return;
   }
   if(gate->getTypeStr() != "AIG") return;

   Var  va = varMap[gate->getFaninLit(1) / 2];
   Var  vb = varMap[gate->getFaninLit(2) / 2];
   bool fa = gate->getFaninLit(1) % 2;
   bool fb = gate->getFaninLit(2) % 2;

   solver.addAigCNF(vf, va, fa, vb, fb);
   varMap[gate->getID()] = vf;
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
