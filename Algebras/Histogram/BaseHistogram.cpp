/*
----
This file is part of SECONDO.

Copyright (C) 2004, University in Hagen, Department of Computer Science,
Database Systems for New Applications.

SECONDO is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

SECONDO is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with SECONDO; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
----

//paragraph [1] Title: [{\Large \bf \begin {center}] [\end {center}}]
//[TOC] [\tableofcontents]

[1] Header File of the Histogram Algebra

December 2007, S.H[oe]cher,M.H[oe]ger,A.Belz,B.Poneleit


[TOC]

1 Overview

2 Defines and includes

*/
#include "Histogram.h"
#include "ListUtils.h"

namespace hgr
{
  BaseHistogram::BaseHistogram()
  {
  }

  BaseHistogram::BaseHistogram(bool _defined, size_t size) :
    bin(size)
  {
    SetDefined(_defined);
  }

  BaseHistogram::BaseHistogram(const BaseHistogram& rhs) :
    bin(rhs.bin.Size())
  {
    SetDefined(rhs.IsDefined());

    for (int j = 0; j < rhs.GetNoBin(); j++)
      bin.Append( rhs.GetBin(j) );
  }

  BaseHistogram::~BaseHistogram()
  {
  }

  HIST_REAL BaseHistogram::GetBin(int i) const
  {
    assert( 0 <= i && i < GetNoBin() );

    HIST_REAL p;
    bin.Get(i, &p);
    return p;
  }

  void BaseHistogram::AppendBin(const HIST_REAL& b)
  {
    bin.Append(b);
  }

  int BaseHistogram::GetNoBin() const
  {
    return bin.Size();
  }

  void BaseHistogram::ResizeBin(const int newSize)
  {
    if (newSize > 0)
    {
      bin.clean();
      bin.resize(newSize);
    }
  }

  HIST_REAL BaseHistogram::Distance(const BaseHistogram* h)
  {
    // Vorbedingung: die Histogramme sind vom gleichen Typ
    const Histogram1d* hist = static_cast<const Histogram1d*>(h);

    // Differenz der bins (entspricht Fläche bzw. Volumen) berechnen
    HIST_REAL squareDistance = 0.0;
    HIST_REAL distance;
    for (int i = 0; i < GetNoBin(); i++) {
      distance = GetBin(i) - hist->GetBin(i);
      squareDistance += distance*distance;
    }
    return squareDistance;
  }

  /*
   * Gibt den ersten Index des Behaelters mit minimalem
   * Fuellstand zurueck.
   *
   */
  CcInt BaseHistogram::GetMinBin() const
  {
    if (!IsDefined() || IsEmpty() || !IsConsistent(false))
      return CcInt(false, -1);

    HIST_REAL min = GetBin(0);
    int index = 0;

    for (int i = 1; i < GetNoBin(); i++)
    {
      if (CmpReal(GetBin(i), min) == -1)
      {
        min = GetBin(i);
        index = i;
      }
    }

    return CcInt(true, index);
  }

  /*
   * Gibt den ersten Index des Behaelters mit maximalem
   * Fuellstand zurueck.
   *
   */
  CcInt BaseHistogram::GetMaxBin() const
  {
    if (!IsDefined() || IsEmpty() || !IsConsistent(false))
      return CcInt(false, -1);

    HIST_REAL max = GetBin(0);
    int index = 0;

    for (int i = 1; i < GetNoBin(); i++)
    {
      if (CmpReal(GetBin(i), max) == 1)
      {
        max = GetBin(i);
        index = i;
      }
    }

    return CcInt(true, index);
  }

  /*
   * Vergleichsfunktion fuer DbArray<HIST_REAL>::Find.
   * Verwendet zur Suche nach dem passenden Behaelter
   * fuer einen Wert.
   *
   */
  int BaseHistogram::CmpBinSearch(const void* v1, const void* v2)
  {
    HIST_REAL* f1 = (HIST_REAL*)v1;
    HIST_REAL* f2 = (HIST_REAL*)v2;

    if (*f1 < *f2)
      return -1;

    return 1;
  }

  void BaseHistogram::CopyBinsFrom(const BaseHistogram* h)
  {
    bin.clean();

    for (int i = 0; i < h->GetNoBin(); i++)
      this->AppendBin(h->GetBin(i));
  }

  ListExpr is_undefinedTypeMap( ListExpr args )
  {
    string errMsg = "Expecting either a histogram1d or a histogram2d.";
    NList list(args);

    if (nl->ListLength(args) == 1)
    {
      ListExpr arg = nl->First(args);
      if (    (nl->IsAtom(arg) && nl->IsEqual(arg, "histogram1d"))
          ||  (nl->IsAtom(arg) && nl->IsEqual(arg, "histogram2d"))  )
      {
        return NList(symbols::BOOL).listExpr();
      } // if (nl->IsAtom(arg) && nl->IsEqual(arg, "histogram1d"))
    } // if (nl->ListLength(args) == 1)
    return list.typeError(errMsg);
  } // ListExpr is_undefinedTypeMap( ListExpr args )


  //  Selection mapping function

  int is_undefinedSelect( ListExpr args )
  {
    NList list(args);
    if (list.first().isSymbol(symbols::HISTOGRAM1D))
      return 0;
    if (list.first().isSymbol(symbols::HISTOGRAM2D))
      return 1;
    else
      return 2;
  } // int is_undefinedSelect( ListExpr args )


  // Value mapping function
  int is_undefined1dFun ( Word* args,
                          Word& result,
                          int message,
                          Word& local,
                          Supplier s )
  {
    Histogram1d *hg = static_cast<Histogram1d*>(args[0].addr);
    result = qp->ResultStorage(s);
    CcBool *b = static_cast<CcBool*>(result.addr);

    if(!hg->IsDefined())
      b->Set(true, true);
    else
      b->Set(true, false);

    return 0;
  } // int is_undefined1dFun ( ...


  // Value mapping function
  int is_undefined2dFun ( Word* args,
                          Word& result,
                          int message,
                          Word& local,
                          Supplier s )
  {
    Histogram2d *hg = static_cast<Histogram2d*>(args[0].addr);
    result = qp->ResultStorage(s);
    CcBool *b = static_cast<CcBool*>(result.addr);

    if(!hg->IsDefined())
      b->Set(true, true);
    else
      b->Set(true, false); // b->SetDefined(false);

    return 0;
  } // int is_undefined1dFun ( ...

  int IsRefinementFun(Word* args, Word& result, int message, Word& local,
      Supplier s)
  {
    BaseHistogram* h1 = static_cast<BaseHistogram*>(args[0].addr );
    BaseHistogram* h2 = static_cast<BaseHistogram*>(args[1].addr );

    result = qp->ResultStorage(s);
    CcBool* b = static_cast<CcBool*>(result.addr );

    if(h1->IsDefined() && h2->IsDefined()){
      b->Set( true, h1->IsRefinement(*h2));
    } else {
      b->Set( false, false);
    }

    return 0;
  }

  int IsEqualFun(Word* args, Word& result, int message, Word& local,
      Supplier s)
  {
    BaseHistogram* h1 = static_cast<BaseHistogram*>(args[0].addr );
    BaseHistogram* h2 = static_cast<BaseHistogram*>(args[1].addr );

    result = qp->ResultStorage(s);

    CcBool* b = static_cast<CcBool*>(result.addr );
    b->Set( true, *h1 == *h2);

    return 0;
  }

  int IsLessFun(Word* args, Word& result, int message, Word& local, Supplier s)
  {
    BaseHistogram* h1 = static_cast<BaseHistogram*>(args[0].addr );
    BaseHistogram* h2 = static_cast<BaseHistogram*>(args[1].addr );

    result = qp->ResultStorage(s);

    CcBool* b = static_cast<CcBool*>(result.addr );
    b->Set( true, *h1 < *h2);

    return 0;
  }

  ListExpr HistHistBoolTypeMap(ListExpr args)
  {
    NList list(args);
    const string errMsg = "Expecting (" + symbols::HISTOGRAM1D
        + " "
        + symbols::HISTOGRAM1D + ")"
        + " or (" + symbols::HISTOGRAM2D + " "
        + symbols::HISTOGRAM2D + ")";

    if (list.length() != 2)
      return list.typeError(errMsg);

    NList arg1 = list.first();
    NList arg2 = list.second();

    // histogram1d x histogram1d -> bool
    if (    arg1.isSymbol(symbols::HISTOGRAM1D)
         && arg2.isSymbol(symbols::HISTOGRAM1D) )
      return NList(symbols::BOOL).listExpr();

    // histogram2d x histogram2d -> bool
    if (    arg1.isSymbol(symbols::HISTOGRAM2D)
         && arg2.isSymbol(symbols::HISTOGRAM2D) )
      return NList(symbols::BOOL).listExpr();

    return list.typeError(errMsg);
  }

  /*
   Argument 0 histogram to translate, 1 pattern histogram
   */
  int TranslateFun(Word* args, Word& result, int message, Word& local,
      Supplier s)
  {
    BaseHistogram* histBase = (BaseHistogram*)args[0].addr;
    BaseHistogram* histPattern = (BaseHistogram*)args[1].addr;

    result = qp->ResultStorage(s);
    BaseHistogram* hist = (BaseHistogram*)result.addr;
    hist->Clear();
    hist->CopyFrom(histBase);

    if (hist->IsRefinement(*histPattern))
    {
      hist->Coarsen(histPattern);
    }
    else
    {
      hist->SetDefined(false);
    }
    return 0;
  } // TranslateFun(Word* args, Word& result, int message, Word& local,


  ListExpr TranslateTypeMap(ListExpr args)
  {
    NList argList = NList(args);

    if(!argList.length() == 2){
      return listutils::typeError("Expects 2 arguments.");
    }

    NList histA = argList.first();
    NList histB = argList.second();

    if(!((histA.isSymbol(symbols::HISTOGRAM1D) &&
	        histB.isSymbol(symbols::HISTOGRAM1D))
        || (histA.isSymbol(symbols::HISTOGRAM2D) &&
	    histB.isSymbol(symbols::HISTOGRAM2D)))){
      return listutils::typeError("Expecting two histograms of the same type.");
    }

    return histA.listExpr();
  } // TranslateTypeMap(ListExpr args)


  int UseFun(Word* args, Word& result, int message, Word& local, Supplier s)
  {
    /*
     * args[0]: The input-Histogram.
     * args[1]: The list of optional parameters.
     * args[2]: The parameter function.
     *
     */

    BaseHistogram* inputHg = (BaseHistogram*)args[0].addr;

    // The query processor provided an empty BaseHistogram-instance:
    result = qp->ResultStorage(s);
    BaseHistogram* resultHg = (BaseHistogram*)result.addr;
    resultHg->Clear();

    if (!inputHg->IsDefined())
    {
      resultHg->SetDefined(false);
      return 0;
    }

    // Get the list of optional parameters:
    Supplier optParamList = args[1].addr;
    int noOfOptParams = qp->GetNoSons(optParamList);

    // Get the parameter function:
    Supplier fun = args[2].addr;
    ArgVectorPointer funargs = qp->Argument(fun);

    // 'Load' the function with the opt. parameters:
    Supplier node;
    Word argVal;

    for (int i = 0; i < noOfOptParams; i++)
    {
      // Get the value of the parameter:
      node = qp->GetSupplier(optParamList, i);
      qp->Request(node, argVal);

      // Check, if the parameter is defined:
      if ( !((Attribute*)argVal.addr)->IsDefined() )
      {
        resultHg->SetDefined(false);
        return 0;
      }

      // Load the argumentvector with the parameter:
      (*funargs)[i + 1] = argVal;
    }

    Word funResult;
    CcReal binVal;
    CcReal* funResultBinVal;

    for (int i = 0; i < inputHg->GetNoBin(); i++)
    {
      // Get the current bin-value from the input-histogram:
      binVal.Set(true, inputHg->GetBin(i));

      // Pass it as first argument to the function and
      // evaluate the function:
      (*funargs)[0] = SetWord(&binVal);
      qp->Request(fun, funResult);

      // The function should return a realvalue:
      funResultBinVal = (CcReal*)funResult.addr;

      // Store the result in the output-histogram:
      resultHg->AppendBin(funResultBinVal->GetValue());
    }

    // Copy the ranges 1:1
    resultHg->CopyRangesFrom(inputHg);

    return 0;
  }

  ListExpr UseTypeMap(ListExpr args)
  {
    string errorMsg;
    ListExpr dummy;

    NList list(args);

    // Check the list:
    errorMsg = "Expecting a list of length three.";

    if (list.length() != 3)
      return list.typeError(errorMsg);

    NList arg1 = list.first(); // histogramxd
    NList arg2 = list.second(); // (T*)
    NList arg3 = list.third(); // (map real T* real)


    // Check the first argument:
    // histogramxd
    errorMsg = "Expecting "
      "histogram1d or histogram2d as first argument.";

    if (!arg1.isSymbol(symbols::HISTOGRAM1D) &&
        !arg1.isSymbol(symbols::HISTOGRAM2D))
      return list.typeError(errorMsg);


    // Check the second argument:
    // (T*)
    errorMsg = "Expecting a list with structure (T*) "
          "as second argument, where T is in kind DATA.";

    // Check, if each T is in kind DATA:
    for (unsigned int i = 1; i <= arg2.length(); i++)
    {
      if (!am->CheckKind("DATA", arg2.elem(i).listExpr(), dummy))
        return list.typeError(errorMsg);
    }


    // Check the third argument:
    // (map real T* real)
    errorMsg = "Expecting a list with structure (map real T* real) "
      "as third argument, where T is in kind DATA.";

    if (arg3.length() < 3)
      return list.typeError(errorMsg);

    NList arg3_1 = arg3.first();
    NList arg3_2 = arg3.second();
    NList arg3_last = arg3.elem(arg3.length());

    if (!arg3_1.isSymbol(symbols::MAP) ||
        !arg3_2.isSymbol(symbols::REAL) ||
        !arg3_last.isSymbol(symbols::REAL))
      return list.typeError(errorMsg);

    // Check, if each parameter T is in kind DATA:
    for (unsigned int i = 3; i < arg3.length(); i++)
    {
      if (!am->CheckKind("DATA", arg3.elem(i).listExpr(), dummy))
        return list.typeError(errorMsg);
    }

    errorMsg = "The parameterlist given in the second argument "
          "does not match the parameterlist of the function.";

    if (arg2.length() != arg3.length() - 3)
      return list.typeError(errorMsg);

    for (unsigned int i = 1; i <= arg2.length(); i++)
    {
      if (arg2.elem(i).str() != arg3.elem(i + 2).str())
        return list.typeError(errorMsg);
    }

    return arg1.listExpr();
  }

  int Use2Fun(Word* args, Word& result, int message, Word& local, Supplier s)
  {
    /*
     * args[0]: The first input-Histogram.
     * args[1]: The second input-Histogram.
     * args[2]: The list of optional parameters.
     * args[3]: The parameter function.
     *
     */

    BaseHistogram* inputHg1 = (BaseHistogram*)args[0].addr;
    BaseHistogram* inputHg2 = (BaseHistogram*)args[1].addr;

    // The query processor provided an empty BaseHistogram-instance:
    result = qp->ResultStorage(s);
    BaseHistogram* resultHg = (BaseHistogram*)result.addr;
    resultHg->Clear();

    if (!inputHg1->IsDefined() || !inputHg2->IsDefined()) {
      resultHg->SetDefined(false);
      return 0;
    }

    // XRIS: Problem: The arguments are modified here!
    //       Should use new histigrams instead!
    if (inputHg1->IsRefinement(*inputHg2)) {
      inputHg1->Coarsen(inputHg2);
    } else if (inputHg2->IsRefinement(*inputHg1)) {
      inputHg2->Coarsen(inputHg1);
    } else {
      resultHg->SetDefined(false);
      return 0;
    }

    // Get the list of optional parameters:
    Supplier optParamList = args[2].addr;
    int noOfOptParams = qp->GetNoSons(optParamList);

    // Get the parameter function:
    Supplier fun = args[3].addr;
    ArgVectorPointer funargs = qp->Argument(fun);

    // 'Load' the function with the opt. parameters:
    Supplier node;
    Word argVal;

    for (int i = 0; i < noOfOptParams; i++)
    {
      // Get the value of the parameter:
      node = qp->GetSupplier(optParamList, i);
      qp->Request(node, argVal);

      // Check, if the parameter is defined:
      if ( !((Attribute*)argVal.addr)->IsDefined() )
      {
        resultHg->SetDefined(false);
        return 0;
      }

      // Load the argumentvector:
      (*funargs)[i + 2] = argVal;
    }

    Word funResult;
    CcReal binVal1, binVal2;
    CcReal* funResultBinVal;

    for (int i = 0; i < inputHg1->GetNoBin(); i++)
    {
      // Get the current bin-values from the input-histograms:
      binVal1.Set(true, (inputHg1->GetBin(i)));
      binVal2.Set(true, (inputHg2->GetBin(i)));

      // Pass them as first and second argument to the function and
      // evaluate the function:
      (*funargs)[0] = SetWord(&binVal1);
      (*funargs)[1] = SetWord(&binVal2);
      qp->Request(fun, funResult);

      // The function should return a realvalue:
      funResultBinVal = (CcReal*)funResult.addr;

      // Store the result in the output-histogram:
      resultHg->AppendBin(funResultBinVal->GetValue());
    }

    // Copy the ranges 1:1
    resultHg->CopyRangesFrom(inputHg1);

    return 0;
  }

  ListExpr Use2TypeMap(ListExpr args)
  {
    string errorMsg;
    ListExpr dummy;

    NList list(args);

    // Check the list:
    errorMsg = "Expecting a list of length four.";

    if (list.length() != 4)
      return list.typeError(errorMsg);

    NList arg1 = list.first(); // histogramxd
    NList arg2 = list.second(); // histogramxd
    NList arg3 = list.third(); // (T*)
    NList arg4 = list.fourth(); // (map real real T* real)

    // Check the first argument:
    // histogramxd
    errorMsg = "Expecting "
      "histogram1d or histogram2d as first argument.";

    if (!arg1.isSymbol(symbols::HISTOGRAM1D) &&
        !arg1.isSymbol(symbols::HISTOGRAM2D))
      return list.typeError(errorMsg);

    // Check the second argument:
    // histogramxd
    errorMsg = "Expecting " + arg1.convertToString() + " as second argument.";

    if (!arg2.isEqual(arg1.convertToString()))
      return list.typeError(errorMsg);


    // Check the third argument:
    // (T*)
    errorMsg = "Expecting a list with structure (T*) "
      "as third argument, where T is in kind DATA.";

    // Check, if each T is in kind DATA:
    for (unsigned int i = 1; i <= arg3.length(); i++)
    {
      if (!am->CheckKind("DATA", arg3.elem(i).listExpr(), dummy))
        return list.typeError(errorMsg);
    }

    // Check the fourth argument:
    // (map real real T* real)
    errorMsg = "Expecting a list with structure (map real real T* real) "
      "as fourth argument, where T is in kind DATA.";

    if (arg4.length() < 4)
      return list.typeError(errorMsg);

    NList arg4_1 = arg4.first();
    NList arg4_2 = arg4.second();
    NList arg4_3 = arg4.third();
    NList arg4_last = arg4.elem(arg4.length());

    if (!arg4_1.isSymbol(symbols::MAP) || !arg4_2.isSymbol(symbols::REAL) ||
        !arg4_3.isSymbol(symbols::REAL) || !arg4_last.isSymbol(symbols::REAL))
      return list.typeError(errorMsg);

    // Check, if each parameter T is in kind DATA:
    for (unsigned int i = 4; i < arg4.length(); i++)
    {
      if (!am->CheckKind("DATA", arg4.elem(i).listExpr(), dummy))
        return list.typeError(errorMsg);
    }

    errorMsg = "The parameterlist given in the second argument "
      "does not match the parameterlist of the function.";

    if (arg3.length() != arg4.length() - 4)
      return list.typeError(errorMsg);

    for (unsigned int i = 1; i <= arg3.length(); i++)
    {
      if (arg3.elem(i).str() != arg4.elem(i + 3).str())
        return list.typeError(errorMsg);
    }


    return arg1.listExpr();
  }

  /*
   Argument 0 histogram, 1 Fold function, 2 initial value,
   */
  int FoldFun(Word* args, Word& result, int message, Word& local, Supplier s)
  {
    result = qp->ResultStorage(s);
    Attribute* res = static_cast<Attribute*>(result.addr);
    BaseHistogram* hist = (BaseHistogram*)args[0].addr;    // get histogram

    if( !hist->IsDefined() ) {
      res->SetDefined(false);
      return 0;
    }

    Word fctres;
    // Store initial value to res
    res->CopyFrom(static_cast<Attribute*>(args[2].addr));
    // Store initial value to tmpres
    Attribute* tmpres = (static_cast<Attribute*>(args[2].addr))->Clone();
    ArgVectorPointer vector = qp->Argument(args[1].addr);
    CcReal* bin = new CcReal(true);
    for(int i=0; i < hist->GetNoBin(); i++){
      tmpres->CopyFrom(res);
      bin->Set(true, hist->GetBin(i));
      ((*vector)[0]).setAddr(tmpres);
      ((*vector)[1]).setAddr(bin);
      qp->Request(args[1].addr, fctres);
      res->CopyFrom(static_cast<Attribute*>(fctres.addr));
    }
    bin->DeleteIfAllowed();
    tmpres->DeleteIfAllowed();
    return 0;

  }
  ListExpr FoldTypeMap(ListExpr args)
  {
    NList argList = NList(args);

    //cout << "argList " << argList << endl;

    if(!argList.length() == 3){
      return listutils::typeError("Expects 3 arguments.");
    }

    NList hist = argList.first();
    NList fun = argList.second();
    NList initVal = argList.third();

    if(!(    hist.isSymbol(symbols::HISTOGRAM1D)
          || hist.isSymbol(symbols::HISTOGRAM2D))){
      return listutils::typeError("Expects an histogram or histogram2d "
                                  "as 1st argument.");
    }

    if(fun.length() != 4 || !fun.first().isSymbol(symbols::MAP)){
      return listutils::typeError("Expects vaild function as 3rd argument.");
    }

    if(!fun.third().isSymbol(symbols::REAL)){
      return listutils::typeError("Expects function parameter of type "
                                  + symbols::REAL);
    }

    NList type = fun.fourth();
    if(fun.second() != type || initVal != type){
      return listutils::typeError("First function parameter or initial value "
        "not of type " + type.convertToString());
    }
    ListExpr dummy;

    if(!am->CheckKind("DATA", type.listExpr(), dummy)){
      return listutils::typeError("Type of parameter is not in kind DATA.");
    }
    //cout << "fun " << fun << endl;
    //cout << "initVal " << initVal << endl;
    return NList(initVal).listExpr();
  } // FoldTypeMap(ListExpr args)

  /*
   Argument 0 Histogram1, Argument 1 Histogram2
   */
  int DistanceFun(Word* args, Word& result, int message, Word& local,
      Supplier s)
  {
    BaseHistogram* hist1 = (BaseHistogram*)args[0].addr;
    BaseHistogram* hist2 = (BaseHistogram*)args[1].addr;

    result = qp->ResultStorage(s);
    CcReal* distance = static_cast<CcReal*>(result.addr);

    // Abstand von undefinierten Histogrammen ist undef
    if (!hist1->IsDefined() || !hist2->IsDefined()) {
      distance->Set(false, 0.0);
      return 0;
    }

    // Gleiche Histogramme haben Abstand 0
    if (*hist1 == *hist2) {
      distance->Set(true, 0.0);
      return 0;
    }

    // Falls das eine Histogramm Verfeinerung des anderen ist, eine
    // Vergröberung Durchführen und das Quadrat der Flächen- bzw.
    // Volumendifferenzen

    // XRIS: This changes the arguments!
    //       Should use new histigrams instead!
    if (hist1->IsRefinement(*hist2)) {
      hist1->Coarsen(hist2);
      distance->Set(hist1->Distance(hist2));
    } // if (hist1->IsRefinement(*hist2))
    else if (hist2->IsRefinement(*hist1)) {
      hist2->Coarsen(hist1);
      distance->Set(hist2->Distance(hist1));
    } // else if (hist2->IsRefinement(*hist1))
    else  {
      distance->Set(false, 0.0);
    }
    return 0;
  }

  ListExpr DistanceTypeMap(ListExpr args)
  {
    NList argList = NList(args);

    if(argList.length() != 2) {
      return listutils::typeError("Expects 2 arguments.");
    }
    NList histA = argList.first();
    NList histB = argList.second();

    if(!(    (    histA.isSymbol(symbols::HISTOGRAM1D)
               && histB.isSymbol(symbols::HISTOGRAM1D))
          || (    histA.isSymbol(symbols::HISTOGRAM2D)
               && histB.isSymbol(symbols::HISTOGRAM2D)))) {
      return listutils::typeError("Expects two histograms of the same type.");
    }
    return NList(symbols::REAL).listExpr();
  }


  template<bool histogram1d>
  ListExpr FindBinTypeMap(ListExpr args)
  {
    NList argList(args);

    if(argList.length() != 2) {
      return listutils::typeError("Expects 2 arguments.");
    }
    NList hist = argList.first();
    NList value = argList.second();

    if (histogram1d)
    {
      if(!hist.isSymbol(symbols::HISTOGRAM1D)) {
        return listutils::typeError("Expects " +
                symbols::HISTOGRAM1D + " as 1st argument");
      }
    } else {
      if(!hist.isSymbol(symbols::HISTOGRAM2D)) {
        return listutils::typeError("Expects " +
            symbols::HISTOGRAM2D + " as 1st argument");
      }
    }

    if(!value.isSymbol(symbols::REAL)) {
      return listutils::typeError("Expects real as 2nd argument");
    }

    NList result = NList(symbols::INT, false);
    return result.listExpr();
  }

  ListExpr GetResultTuple2d()
  {
    // The structure is: (tuple ((X int)(Y int)))

    NList part1(symbols::TUPLE);

    NList part2_1;
    part2_1.append(NList("X"));
    part2_1.append(NList(symbols::INT));

    NList part2_2;
    part2_2.append(NList("Y"));
    part2_2.append(NList(symbols::INT));

    NList part2(part2_1, part2_2);

    return NList(part1, part2).listExpr();
  }

  ListExpr GetResultTupleTypeInfo2d()
  {
    SecondoCatalog sc;
    sc.Open();

    return sc.NumericType(GetResultTuple2d());
  }

  ListExpr FindMinMaxBinTypeMap(ListExpr args)
  {
    NList in(args);

    const string errMsg = "Expecting (" + symbols::HISTOGRAM1D + ")"
        + " or (" + symbols::HISTOGRAM2D + ")";

    if (in.length() != 1)
      return in.typeError(errMsg);

    in = in.first();

    NList out1d = NList(NList(symbols::STREAM), NList(symbols::INT));
    NList out2d = NList(NList(symbols::STREAM), NList(GetResultTuple2d()));

    // (histogram1d) -> (stream int)
    if (in.isSymbol(symbols::HISTOGRAM1D))
      return out1d.listExpr();

    // (histogram2d) -> (stream (tuple ((x int)(y int))))
    if (in.isSymbol(symbols::HISTOGRAM2D))
      return out2d.listExpr();

    return in.typeError(errMsg);
  }

  /*
  3 Instantiation of Template Functions

  The compiler cannot expand these template functions in
  the file ~HistogramAlgebra.cpp~.

  */

  template ListExpr
  FindBinTypeMap<true>(ListExpr args);

  template ListExpr
  FindBinTypeMap<false>(ListExpr args);

}
