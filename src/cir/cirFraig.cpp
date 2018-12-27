/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
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
   cout << "Strashing: " << mergeGate->getID()
        << " merging " << getID() << "..." << endl;
}
