/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Implement memeber functions for class CirMgr

/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;

enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE,
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static unsigned lineNo = 0;  // in printint, lineNo needs to ++
static unsigned colNo  = 0;  // in printing, colNo needs to ++
static char buf[1024];
static string errMsg;
static int errInt;
static CirGate *errGate;

static bool
parseError(CirParseError err)
{
   switch (err) {
      case EXTRA_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Extra space character is detected!!" << endl;
         break;
      case MISSING_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing space character!!" << endl;
         break;
      case ILLEGAL_WSPACE: // for non-space white space character
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal white space char(" << errInt
              << ") is detected!!" << endl;
         break;
      case ILLEGAL_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
              << errMsg << "!!" << endl;
         break;
      case ILLEGAL_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
              << errMsg << "\"!!" << endl;
         break;
      case ILLEGAL_SYMBOL_TYPE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal symbol type (" << errMsg << ")!!" << endl;
         break;
      case ILLEGAL_SYMBOL_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Symbolic name contains un-printable char(" << errInt
              << ")!!" << endl;
         break;
      case MISSING_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing " << errMsg << "!!" << endl;
         break;
      case MISSING_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
              << errMsg << "\"!!" << endl;
         break;
      case MISSING_NEWLINE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": A new line is expected here!!" << endl;
         break;
      case MISSING_DEF:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
              << " definition!!" << endl;
         break;
      case CANNOT_INVERTED:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": " << errMsg << " " << errInt << "(" << errInt/2
              << ") cannot be inverted!!" << endl;
         break;
      case MAX_LIT_ID:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
              << endl;
         break;
      case REDEF_GATE:
         cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
              << "\" is redefined, previously defined as "
              << errGate->getTypeStr() << " in line " << errGate->getLineNo()
              << "!!" << endl;
         break;
      case REDEF_SYMBOLIC_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
              << errMsg << errInt << "\" is redefined!!" << endl;
         break;
      case REDEF_CONST:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Cannot redefine const (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_SMALL:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too small (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_BIG:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too big (" << errInt << ")!!" << endl;
         break;
      default: break;
   }
   return false;
}

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/
bool
CirMgr::readCircuit(const string& fileName)
{
   constGate = new CONSTGate();

   lineNo = 0; colNo = 0;
   errMsg = ""; errInt = 0;
   memset(buf, 0, 1024);

   // Open file
   ifstream ifs(fileName);
   if(!ifs) { 
      cerr << "Cannot open design \"" << fileName << "\"!!\n";
      return false; }

   // Header
   string token;
   unsigned mvi, inNo, latch, outNo, andNo;
   unsigned NoInt[5];
   // ifs >> aag >> mvi >> inNo >> latch >> outNo >> andNo;
   errMsg = "aag"; 
   if(!ifs.getline(buf, 1024)) return parseError(MISSING_IDENTIFIER);
   errInt = buf[colNo];
   if(buf[colNo] == 0)   return parseError(MISSING_IDENTIFIER);
   if(buf[colNo] == ' ') return parseError(EXTRA_SPACE);
   if(isspace(buf[colNo]))  return parseError(ILLEGAL_WSPACE);
   errMsg = ""; for(int i = 0; buf[i] != 0 && !isspace(buf[i]); i++) errMsg += buf[i];
   if(buf[colNo] != 'a')   return parseError(ILLEGAL_IDENTIFIER);
   if(buf[++colNo] != 'a') return parseError(ILLEGAL_IDENTIFIER);
   if(buf[++colNo] != 'g') return parseError(ILLEGAL_IDENTIFIER);

   if(buf[++colNo] >= '0' && buf[colNo] <= '9') return parseError(MISSING_SPACE);
   if(errMsg != "aag") return parseError(ILLEGAL_IDENTIFIER);

   for(int i = 0; i < 5; i++) {
      string type;
      switch(i) {
         case 0:
            type = "variables"; break;
         case 1:
            type = "PIs";       break;
         case 2:
            type = "latches";   break;
         case 3:
            type = "POs";       break;
         case 4:
            type = "AIGs";      break; }

      errMsg = "number of " + type;
      if(buf[colNo] == 0)  return parseError(MISSING_NUM);
      if(buf[colNo] != ' ') return parseError(MISSING_SPACE);

      token = ""; int tmp = 0; errInt = buf[++colNo];
      if(buf[colNo] == 0)   return parseError(MISSING_NUM);
      if(buf[colNo] == ' ') return parseError(EXTRA_SPACE);
      if(isspace(buf[colNo]))  return parseError(ILLEGAL_WSPACE);
      while(buf[colNo] != 0 && !isspace(buf[colNo])) token += buf[colNo++];
      errMsg = "number of " + type + "(" + token + ")";
      if(!myStr2Int(token, tmp)) return parseError(ILLEGAL_NUM);
      if(tmp < 0) return parseError(ILLEGAL_NUM);
      NoInt[i] = tmp;
   }

   mvi   = NoInt[0]; 
   inNo  = NoInt[1];
   latch = NoInt[2];
   outNo = NoInt[3];
   andNo = NoInt[4];

   if(buf[colNo] != 0)
      return parseError(MISSING_NEWLINE);

   if(mvi < inNo + andNo) {
      errMsg = "Number of variables";
      errInt = mvi;
      return parseError(NUM_TOO_SMALL); }

   if(latch != 0) {
      errMsg = "latches";
      return parseError(ILLEGAL_NUM); }

   // gateMap = new CirGate* [mvi + outNo + 1] {0};
   gateMap.resize(mvi + outNo + 1, 0);
   gateMap[0] = constGate;

   // Input pins
   for(unsigned i = 0; i < inNo; i++) {

      lineNo++; colNo = 0;
      errMsg = ""; errInt = 0; errGate = 0;
      memset(buf, 0, 1024);

      unsigned id;
      token = ""; int tmp = 0;
      // ifs >> id;

      errMsg = "PI";
      if(!ifs.getline(buf, 1024)) return parseError(MISSING_DEF);
      errMsg = "PI literal ID"; errInt = buf[colNo];
      if(buf[colNo] == 0)   return parseError(MISSING_NUM);
      if(buf[colNo] == ' ') return parseError(EXTRA_SPACE);
      if(isspace(buf[colNo]))  return parseError(ILLEGAL_WSPACE);

      int j = 0; while(buf[j] != 0 && !isspace(buf[j])) token += buf[j++];
      errMsg += ( "(" + token + ")" );
      if(!myStr2Int(token, tmp)) return parseError(ILLEGAL_NUM);
      if(tmp < 0)                return parseError(ILLEGAL_NUM);
      errInt = tmp; errMsg = "PI";
      if(tmp < 2)      return parseError(REDEF_CONST);
      if(tmp % 2 != 0) return parseError(CANNOT_INVERTED);
      
      id = tmp;

      if(id / 2 > mvi)   return parseError(MAX_LIT_ID);
      errGate = getGate(id / 2);
      if(errGate != 0) return parseError(REDEF_GATE);

      colNo = j;
      if(buf[colNo] != 0) return parseError(MISSING_NEWLINE);

      PIs.push_back(new PIGate(id / 2, i + 2));
      gateMap[id / 2] = PIs.back();
   }

   // Output pins
   vector<unsigned> outID;
   for(unsigned i = 0; i < outNo; i++) {
      
      lineNo++; colNo = 0;
      errMsg = ""; errInt = 0; errGate = 0;
      memset(buf, 0, 1024);

      unsigned id;
      token = ""; int tmp = 0;
      // ifs >> id;

      errMsg = "PO";
      if(!ifs.getline(buf, 1024)) return parseError(MISSING_DEF);
      errMsg = "PO literal ID"; errInt = buf[colNo];
      if(buf[colNo] == 0)   return parseError(MISSING_NUM);
      if(buf[colNo] == ' ') return parseError(EXTRA_SPACE);
      if(isspace(buf[colNo]))  return parseError(ILLEGAL_WSPACE);

      int j = 0; while(buf[j] != 0 && !isspace(buf[j])) token += buf[j++];
      errMsg += ( "(" + token + ")" );
      if(!myStr2Int(token, tmp)) return parseError(ILLEGAL_NUM);
      if(tmp < 0)                return parseError(ILLEGAL_NUM);
      errInt = tmp;
      id = tmp;

      if(id / 2 > mvi)   return parseError(MAX_LIT_ID);

      colNo = j;
      if(buf[colNo] != 0) return parseError(MISSING_NEWLINE);

      POs.push_back(new POGate(mvi + i + 1, i + inNo + 2));
      gateMap[mvi + i + 1] = POs.back();
      outID.push_back(id);
   }

   // And Gates
   vector<unsigned> andID, fin1ID, fin2ID;
   for(unsigned i = 0; i < andNo; i++) {
      
      lineNo++; colNo = 0;
      errMsg = ""; errInt = 0; errGate = 0;
      memset(buf, 0, 1024);

      unsigned id, fin1, fin2;
      token = ""; int tmp = 0;
      // ifs >> id >> fin1 >> fin2;

      errMsg = "AIG";
      if(!ifs.getline(buf, 1024)) return parseError(MISSING_DEF);
      errMsg = "AIG gate literal ID"; errInt = buf[colNo];
      if(buf[colNo] == 0)   return parseError(MISSING_NUM);
      if(buf[colNo] == ' ') return parseError(EXTRA_SPACE);
      if(isspace(buf[colNo]))  return parseError(ILLEGAL_WSPACE);

      int j = 0; while(buf[j] != 0 && !isspace(buf[j])) token += buf[j++];
      errMsg += ( "(" + token + ")" );
      if(!myStr2Int(token, tmp)) return parseError(ILLEGAL_NUM);
      if(tmp < 0)                return parseError(ILLEGAL_NUM);
      errInt = tmp; errMsg = "AIG gate";
      if(tmp < 2)      return parseError(REDEF_CONST);
      if(tmp % 2 != 0) return parseError(CANNOT_INVERTED);
      
      id = tmp;

      if(id / 2 > mvi)   return parseError(MAX_LIT_ID);
      errGate = getGate(id / 2);
      if(errGate != 0) return parseError(REDEF_GATE);

      colNo = j; token = ""; tmp = 0;

      if(buf[colNo] < ' ') return parseError(MISSING_SPACE);

      errMsg = "AIG input literal ID"; 
      colNo++; errInt = buf[colNo];
      if(buf[colNo] == 0)   return parseError(MISSING_NUM);
      if(buf[colNo] == ' ') return parseError(EXTRA_SPACE);
      if(isspace(buf[colNo]))  return parseError(ILLEGAL_WSPACE);

      j = colNo; while(buf[j] != 0 && !isspace(buf[j])) token += buf[j++];
      errMsg += ( "(" + token + ")" );
      if(!myStr2Int(token, tmp)) return parseError(ILLEGAL_NUM);
      if(tmp < 0)                return parseError(ILLEGAL_NUM);

      errInt = tmp;
      fin1 = tmp;

      if(fin1 / 2 > mvi)   return parseError(MAX_LIT_ID);

      colNo = j; token = ""; tmp = 0;

      if(buf[colNo] < ' ') return parseError(MISSING_SPACE);

      errMsg = "AIG input literal ID"; 
      colNo++; errInt = buf[colNo];
      if(buf[colNo] == 0)   return parseError(MISSING_NUM);
      if(buf[colNo] == ' ') return parseError(EXTRA_SPACE);
      if(buf[colNo] < ' ')  return parseError(ILLEGAL_WSPACE);

      j = colNo; while(buf[j] > ' ') token += buf[j++];
      errMsg += ( "(" + token + ")" );
      if(!myStr2Int(token, tmp)) return parseError(ILLEGAL_NUM);
      if(tmp < 0)                return parseError(ILLEGAL_NUM);

      errInt = tmp;
      fin2 = tmp;

      if(fin2 / 2 > mvi)   return parseError(MAX_LIT_ID);

      colNo = j;

      if(buf[colNo] != 0) return parseError(MISSING_NEWLINE);

      AIGs.push_back(new AIGGate(id / 2, i + inNo + outNo + 2));
      gateMap[id / 2] = AIGs.back();
      andID.push_back(id);
      fin1ID.push_back(fin1);
      fin2ID.push_back(fin2);

   }

   // Pin Names
   string _name;

   memset(buf, 0, 1024); colNo = 0;

   while(ifs.getline(buf, 1024)) {
      lineNo++; colNo = 0;
      errMsg = ""; errInt = 0; errGate = 0;
      token = ""; _name = "";

      if(buf[0] == 'c') break;
      // ifs >> _name;

      bool inpin = false, outpin = false;
      unsigned pinNo = 0; int tmp = 0;

      errMsg = buf[colNo]; errInt = buf[colNo];
      if(buf[colNo] == ' ')  return parseError(EXTRA_SPACE);
      if(isspace(buf[colNo]))return parseError(ILLEGAL_WSPACE);
      if(buf[0] == 'i')      inpin = true;
      else if(buf[0] == 'o') outpin = true;
      else                   return parseError(ILLEGAL_SYMBOL_TYPE);

      errMsg = "symbol index"; errInt = buf[++colNo];
      if(buf[colNo] == 0)    return parseError(MISSING_NUM);
      if(buf[colNo] == ' ')  return parseError(EXTRA_SPACE);
      if(isspace(buf[colNo]))   return parseError(ILLEGAL_WSPACE);

      int j = colNo; while(buf[j] > ' ') token += buf[j++];
      errMsg = "symbol index(" + token + ")";
      if(!myStr2Int(token, tmp)) return parseError(ILLEGAL_NUM);

      pinNo = tmp;

      if(inpin) {
         errMsg = "PI index"; errInt = pinNo;
         if(pinNo >= inNo) return parseError(NUM_TOO_BIG);
         errMsg = "i";
         if(PIs[pinNo]->getGateName().size() != 0)
            return parseError(REDEF_SYMBOLIC_NAME); }
      if(outpin) {
         errMsg = "PO index"; errInt = pinNo;
         if(pinNo >= outNo) return parseError(NUM_TOO_BIG);
         errMsg = "o";
         if(POs[pinNo]->getGateName().size() != 0)
            return parseError(REDEF_SYMBOLIC_NAME); }

      colNo = j; errMsg = "symbolic name";
      if(buf[colNo] == 0)  return parseError(MISSING_IDENTIFIER);
      if(buf[colNo] < ' ') return parseError(MISSING_SPACE);

      while(buf[++colNo] != 0) {
         errInt = buf[colNo];
         if(!isprint(buf[colNo])) return parseError(ILLEGAL_SYMBOL_NAME);
         _name += buf[colNo];
      }

      if(_name.size() == 0) return parseError(MISSING_IDENTIFIER);

      if(inpin) PIs[pinNo]->setGateName(_name);
      else if(outpin) POs[pinNo]->setGateName(_name);

      memset(buf, 0, 1024);
   }

   if(buf[++colNo] != 0) return parseError(MISSING_NEWLINE);

   // Set fanins of AND gates
   for(unsigned i = 0; i < andNo; i++) {
      unsigned fanin1ID = fin1ID[i] / 2;
      unsigned fanin2ID = fin2ID[i] / 2;
      bool fanin1Inv = fin1ID[i] % 2;
      bool fanin2Inv = fin2ID[i] % 2;

      CirGate* fin1 = getGate(fanin1ID);
      CirGate* fin2 = getGate(fanin2ID);

      if(fin1 == 0) {
         fin1 = new UNDEFGate(fanin1ID);
         UNDEFs.push_back(fin1);
         gateMap[fanin1ID] = UNDEFs.back();
      }
      if(fin2 == 0) {
         fin2 = new UNDEFGate(fanin2ID);
         UNDEFs.push_back(fin2);
         gateMap[fanin2ID] = UNDEFs.back();
      }
      
      AIGs[i]->setFanin(fin1, fanin1Inv, 1);
      fin1->setFanout(AIGs[i], fanin1Inv);

      AIGs[i]->setFanin(fin2, fanin2Inv, 2);
      fin2->setFanout(AIGs[i], fanin2Inv);
   }

   // Set fanins of outputs
   for(unsigned i = 0; i < outNo; i++) {
      unsigned faninID = outID[i] / 2;
      bool faninInv = outID[i] % 2;

      CirGate* fin = getGate(faninID);

      if(fin == 0) {
         fin = new UNDEFGate(faninID);
         UNDEFs.push_back(fin);
         gateMap[faninID] = UNDEFs.back();
      }
      
      POs[i]->setFanin(fin, faninInv, 1);
      fin->setFanout(POs[i], faninInv);
   }

   sortAllFanouts();
   DFS();

   return true;
}

void
CirMgr::DFS() {
   CirGate::resetGlobalRef();
   _dfsList.clear();
   for(unsigned i = 0; i < POs.size(); i++) {
      POs[i]->dfsTraversal(_dfsList);
   }
}

void
CirMgr::sortAllFanouts()
{
   for(unsigned i = 0; i < PIs.size(); i++)
      PIs[i]->sortFanouts();
   for(unsigned i = 0; i < AIGs.size(); i++)
      AIGs[i]->sortFanouts();
   for(unsigned i = 0; i < UNDEFs.size(); i++)
      UNDEFs[i]->sortFanouts();
   constGate->sortFanouts();
}

/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
/*********************
Circuit Statistics
==================
  PI          20
  PO          12
  AIG        130
------------------
  Total      162
*********************/
void
CirMgr::printSummary() const
{
   cout << endl;
   cout << "Circuit Statistics" << endl;
   cout << "==================" << endl;

   ios init(NULL);
   init.copyfmt(cout);

   cout << "  PI" << setw(12) << PIs.size() << endl;
   cout << "  PO" << setw(12) << POs.size() << endl;
   cout << "  AIG"<< setw(11) <<AIGs.size() << endl;
   
   cout.copyfmt(init);

   cout << "------------------" << endl;

   cout << "  Total" << setw(9)
        << PIs.size() + POs.size() + AIGs.size() << endl;

   cout.copyfmt(init);
}

void
CirMgr::printNetlist() const
{
   cout << endl;
   for (unsigned i = 0, n = _dfsList.size(); i < n; ++i) {
      cout << "[" << i << "] ";
      _dfsList[i]->printGate();
   }
}

void
CirMgr::printPIs() const
{
   cout << "PIs of the circuit:";
   for(unsigned i = 0; i < PIs.size(); i++) {
      cout << " " << PIs[i]->getID();
   }
   cout << endl;
}

void
CirMgr::printPOs() const
{
   cout << "POs of the circuit:";
   for(unsigned i = 0; i < POs.size(); i++) {
      cout << " " << POs[i]->getID();
   }
   cout << endl;
}

void
CirMgr::printFloatGates() const
{
   vector<unsigned> floating;
   
   for(unsigned i = 0; i < POs.size(); i++) {
      if(POs[i]->haveFloatingFanin())
         floating.push_back(POs[i]->getID());
   }
   for(unsigned i = 0; i < AIGs.size(); i++) {
      if(AIGs[i]->haveFloatingFanin())
         floating.push_back(AIGs[i]->getID());
   }
   sort(floating.begin(), floating.end());
   if(floating.size()) {
      cout << "Gates with floating fanin(s):";
      for(unsigned i = 0; i < floating.size(); i++)
         cout << " " << floating[i];
      cout << endl;
   }

   vector<unsigned> unused;

   for(unsigned i = 0; i < PIs.size(); i++) {
      if(PIs[i]->definedNotUsed())
         unused.push_back(PIs[i]->getID());
   }
   for(unsigned i = 0; i < AIGs.size(); i++) {
      if(AIGs[i]->definedNotUsed())
         unused.push_back(AIGs[i]->getID());
   }
   sort(unused.begin(), unused.end());
   if(unused.size()) {
      cout << "Gates defined but not used  :";
      for(unsigned i = 0; i < unused.size(); i++)
         cout << " " << unused[i];
      cout << endl;
   }
}

void
CirMgr::printFECPairs() const
{
}

void
CirMgr::writeAag(ostream& outfile) const
{
   // Header
   unsigned aigcnt = 0;
   CirGate::resetGlobalRef();
   for(unsigned i = 0; i < POs.size(); i++) 
      POs[i]->countGate(aigcnt);

   outfile << "aag " << gateMap.size() - POs.size() - 1 << " "
           << PIs.size() << " 0 "
           << POs.size() << " "
           << aigcnt << "\n";

   CirGate::resetGlobalRef();
   // Inputs
   for(unsigned i = 0; i < PIs.size(); i++)
      outfile << 2 * PIs[i]->getID() << "\n";

   // Outputs
   for(unsigned i = 0; i < POs.size(); i++)
      outfile << POs[i]->getFaninLit() << "\n";

   // And gates
   for(unsigned i = 0; i < POs.size(); i++) 
      POs[i]->writeGate(outfile);

   // Symbolic names
   for(unsigned i = 0; i < PIs.size(); i++) {
      if(PIs[i]->getGateName().size() != 0) {
         outfile << "i" << i << " "
                 << PIs[i]->getGateName() << "\n";
      }
   }
   for(unsigned i = 0; i < POs.size(); i++) {
      if(POs[i]->getGateName().size() != 0) {
         outfile << "o" << i << " "
                 << POs[i]->getGateName() << "\n";
      }
   }

   outfile << "c" << endl;
}

void
CirMgr::writeGate(ostream& outfile, CirGate *g) const
{
}
