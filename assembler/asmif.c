/* asmif.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Befehle zur bedingten Assemblierung                                       */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "bpemu.h"
#include "chunks.h"
#include "strutil.h"
#include "errmsg.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"

#include "asmif.h"


PIfSave FirstIfSave;
Boolean IfAsm;       /* FALSE: in einer neg. IF-Sequenz-->kein Code */

static Boolean ActiveIF;

static LongInt GetIfVal(const tStrComp *pCond)
{
  Boolean IfOK;
  LongInt Tmp;

  FirstPassUnknown = False;
  Tmp = EvalStrIntExpression(pCond, Int32, &IfOK);
  if ((FirstPassUnknown) || (!IfOK))
  {
    Tmp = 1;
    if (FirstPassUnknown) WrError(ErrNum_FirstPassCalc);
    else if (!IfOK) WrError(ErrNum_InvOpType);
  }

  return Tmp;
}


static void AddBoolFlag(Boolean Flag)
{
  strmaxcpy(ListLine, Flag ? "=>TRUE" : "=>FALSE", STRINGSIZE);
}


static void PushIF(LongInt IfExpr)
{
  PIfSave NewSave;

  NewSave = (PIfSave) malloc(sizeof(TIfSave));
  NewSave->NestLevel = SaveIFs() + 1;
  NewSave->Next = FirstIfSave;
  NewSave->SaveIfAsm = IfAsm;
  NewSave->State = IfState_IFIF;
  NewSave->CaseFound = (IfExpr != 0);
  NewSave->StartLine = CurrLine;
  FirstIfSave = NewSave;
  IfAsm = IfAsm && (IfExpr != 0);
}


static void CodeIF(void)
{
  LongInt IfExpr;

  ActiveIF = IfAsm;

  if (!IfAsm)
    IfExpr = 1;
  else if (!ChkArgCnt(1, 1))
    IfExpr = 1;
  else
    IfExpr = GetIfVal(&ArgStr[1]);
  if (IfAsm)
    AddBoolFlag(IfExpr != 0);
  PushIF(IfExpr);
}


static void CodeIFDEF(Word Negate)
{
  LongInt IfExpr;
  Boolean Defined;

  ActiveIF = IfAsm;

  if (!IfAsm) IfExpr = 1;
  else if (!ChkArgCnt(1, 1))
    IfExpr = 1;
  else
  {
    Defined = IsSymbolDefined(&ArgStr[1]);
    if (IfAsm)
      strmaxcpy(ListLine, (Defined) ? "=>DEFINED" : "=>UNDEFINED", STRINGSIZE);
    if (!Negate)
      IfExpr = (Defined) ? 1 : 0;
    else
      IfExpr = (Defined) ? 0 : 1;
  }
  PushIF(IfExpr);
}


static void CodeIFUSED(Word Negate)
{
  LongInt IfExpr;
  Boolean Used;

  ActiveIF = IfAsm;

  if (!IfAsm)
    IfExpr = 1;
  else if (!ChkArgCnt(1, 1))
    IfExpr = 1;
  else
  {
    Used = IsSymbolUsed(&ArgStr[1]);
    if (IfAsm)
      strmaxcpy(ListLine, (Used) ? "=>USED" : "=>UNUSED", STRINGSIZE);
    if (!Negate)
      IfExpr = (Used) ? 1 : 0;
    else
      IfExpr = (Used) ? 0 : 1;
  }
  PushIF(IfExpr);
}


void CodeIFEXIST(Word Negate)
{
  LongInt IfExpr;
  Boolean Found;
  String NPath;

  ActiveIF = IfAsm;

  if (!IfAsm)
    IfExpr = 1;
  else if (!ChkArgCnt(1, 1))
    IfExpr = 1;
  else
  {
    String FileName, Dummy;

    strmaxcpy(FileName, (ArgStr[1].Str[0] == '"') ? ArgStr[1].Str + 1 : ArgStr[1].Str, STRINGSIZE);
    if (FileName[strlen(FileName) - 1] == '"')
      FileName[strlen(FileName) - 1] = '\0';
    AddSuffix(FileName, IncSuffix);
    strmaxcpy(NPath, IncludeList, STRINGSIZE);
    strmaxprep(NPath, SDIRSEP, STRINGSIZE);
    strmaxprep(NPath, ".", STRINGSIZE);
    Found = !FSearch(Dummy, sizeof(Dummy), FileName, CurrFileName, NPath);
    if (IfAsm)
      strmaxcpy(ListLine, Found ? "=>FOUND" : "=>NOT FOUND", STRINGSIZE);
    IfExpr = Negate ? !Found : Found;
  }
  PushIF(IfExpr);
}


static void CodeIFB(Word Negate)
{
  Boolean Blank = True;
  LongInt IfExpr;
  int z;

  ActiveIF = IfAsm;

  if (!IfAsm)
    IfExpr = 1;
  else
  {
    for (z = 1; z <= ArgCnt; z++)
      if (strlen(ArgStr[z++].Str) > 0)
        Blank = False;
    if (IfAsm)
      strmaxcpy(ListLine, (Blank) ? "=>BLANK" : "=>NOT BLANK", STRINGSIZE);
    IfExpr = Negate ? !Blank : Blank;
  }
  PushIF(IfExpr);
}


static void CodeELSEIF(void)
{
  LongInt IfExpr;

  if (!FirstIfSave || (FirstIfSave->State != IfState_IFIF)) WrError(ErrNum_MissingIf);
  else if (ArgCnt == 0)
  {
    if (FirstIfSave->SaveIfAsm)
      AddBoolFlag(IfAsm = (!FirstIfSave->CaseFound));
    FirstIfSave->State = IfState_IFELSE;
  }
  else if (ArgCnt == 1)
  {
    if (!FirstIfSave->SaveIfAsm)
      IfExpr = 1;
    else if (FirstIfSave->CaseFound)
      IfExpr = 0;
    else
      IfExpr = GetIfVal(&ArgStr[1]);
    IfAsm = ((FirstIfSave->SaveIfAsm) && (IfExpr != 0) && (!FirstIfSave->CaseFound));
    if (FirstIfSave->SaveIfAsm)
      AddBoolFlag(IfExpr != 0);
    if (IfExpr != 0)
      FirstIfSave->CaseFound = True;
  }
  else
    (void)ChkArgCnt(0, 1);

  ActiveIF = (!FirstIfSave) || (FirstIfSave->SaveIfAsm);
}


static void CodeENDIF(void)
{
  PIfSave NewSave;

  if (!ChkArgCnt(0, 0));
  else if (!FirstIfSave || ((FirstIfSave->State != IfState_IFIF) && (FirstIfSave->State != IfState_IFELSE))) WrError(ErrNum_MissingIf);
  else
  {
    IfAsm = FirstIfSave->SaveIfAsm;
    NewSave = FirstIfSave;
    FirstIfSave = NewSave->Next;
    sprintf(ListLine, "[%u]", (unsigned)NewSave->StartLine);
    free(NewSave);
  }

  ActiveIF = IfAsm;
}


static void EvalIfExpression(const tStrComp *pCond, TempResult *erg)
{
  FirstPassUnknown = False;
  EvalStrExpression(pCond, erg);
  if ((erg->Typ == TempNone) || (FirstPassUnknown))
  {
    erg->Typ = TempInt;
    erg->Contents.Int = 1;
    if (FirstPassUnknown)
      WrError(ErrNum_FirstPassCalc);
  }
}


static void CodeSWITCH(void)
{
  PIfSave NewSave;

  ActiveIF = IfAsm;

  NewSave = (PIfSave) malloc(sizeof(TIfSave));
  NewSave->NestLevel = SaveIFs() + 1;
  NewSave->Next = FirstIfSave;
  NewSave->SaveIfAsm = IfAsm;
  NewSave->CaseFound = False;
  NewSave->State = IfState_CASESWITCH;
  NewSave->StartLine = CurrLine;
  if (ArgCnt != 1)
  {
    NewSave->SaveExpr.Typ = TempInt;
    NewSave->SaveExpr.Contents.Int = 1;
    if (IfAsm)
      (void)ChkArgCnt(1, 1);
  }
  else
  {
    EvalIfExpression(&ArgStr[1], &(NewSave->SaveExpr));
    SetListLineVal(&(NewSave->SaveExpr));
  }
  FirstIfSave = NewSave;
}


static void CodeCASE(void)
{
  Boolean eq;
  int z;
  TempResult t;

  if (!FirstIfSave) WrError(ErrNum_MissingIf);
  else if (ChkArgCnt(1, ArgCntMax))
  {
    if ((FirstIfSave->State != IfState_CASESWITCH) && (FirstIfSave->State != IfState_CASECASE)) WrError(ErrNum_InvIfConst);
    else
    {
      if (!FirstIfSave->SaveIfAsm)
        eq = True;
      else if (FirstIfSave->CaseFound)
        eq = False;
      else
      {
        eq = False;
        z = 1;
        do
        {
          EvalIfExpression(&ArgStr[z], &t);
          eq = (FirstIfSave->SaveExpr.Typ == t.Typ);
          if (eq)
           switch (t.Typ)
           {
             case TempInt:
               eq = (t.Contents.Int == FirstIfSave->SaveExpr.Contents.Int);
               break;
             case TempFloat:
               eq = (t.Contents.Float == FirstIfSave->SaveExpr.Contents.Float);
               break;
             case TempString:
               eq = (DynStringCmp(&t.Contents.Ascii, &FirstIfSave->SaveExpr.Contents.Ascii) == 0);
               break;
             default:
               eq = False;
               break;
           }
          z++;
        }
        while ((!eq) && (z <= ArgCnt));
      }
      IfAsm = ((FirstIfSave->SaveIfAsm) && (eq) & (!FirstIfSave->CaseFound));
      if (FirstIfSave->SaveIfAsm)
        AddBoolFlag(eq && (!FirstIfSave->CaseFound));
      if (eq)
        FirstIfSave->CaseFound = True;
      FirstIfSave->State = IfState_CASECASE;
    }
  }

  ActiveIF = (!FirstIfSave) || (FirstIfSave->SaveIfAsm);
}


static void CodeELSECASE(void)
{
  if (ChkArgCnt(0, 0))
  {
    if ((FirstIfSave->State != IfState_CASESWITCH) && (FirstIfSave->State != IfState_CASECASE)) WrError(ErrNum_InvIfConst);
    else
      IfAsm = (FirstIfSave->SaveIfAsm && (!FirstIfSave->CaseFound));
    if (FirstIfSave->SaveIfAsm)
      AddBoolFlag(!FirstIfSave->CaseFound);
    FirstIfSave->CaseFound = True;
    FirstIfSave->State = IfState_CASEELSE;
  }

  ActiveIF = (!FirstIfSave) || (FirstIfSave->SaveIfAsm);
}


static void CodeENDCASE(void)
{
  PIfSave NewSave;

  if (!ChkArgCnt(0, 0));
  else if (!FirstIfSave) WrError(ErrNum_MissingIf);
  else
  {
    if ((FirstIfSave->State != IfState_CASESWITCH)
    && (FirstIfSave->State != IfState_CASECASE)
    && (FirstIfSave->State != IfState_CASEELSE)) WrError(ErrNum_InvIfConst);
    else
    {
      IfAsm = FirstIfSave->SaveIfAsm;
      if (!FirstIfSave->CaseFound) WrError(ErrNum_NoCaseHit);
      NewSave = FirstIfSave;
      FirstIfSave = NewSave->Next;
      sprintf(ListLine, "[%u]", (unsigned)NewSave->StartLine);
      free(NewSave);
    }
  }

  ActiveIF = IfAsm;
}


Boolean CodeIFs(void)
{
  Boolean Result = True;

  ActiveIF = False;

  switch (mytoupper(*OpPart.Str))
  {
    case 'I':
      if (Memo("IF")) CodeIF();
      else if (Memo("IFDEF")) CodeIFDEF(False);
      else if (Memo("IFNDEF")) CodeIFDEF(True);
      else if (Memo("IFUSED")) CodeIFUSED(False);
      else if (Memo("IFNUSED")) CodeIFUSED(True);
      else if (Memo("IFEXIST")) CodeIFEXIST(False);
      else if (Memo("IFNEXIST")) CodeIFEXIST(True);
      else if (Memo("IFB")) CodeIFB(False);
      else if (Memo("IFNB")) CodeIFB(True);
      else Result = False;
      break;
    case 'E':
      if ((Memo("ELSE")) || (Memo("ELSEIF"))) CodeELSEIF();
      else if (Memo("ENDIF")) CodeENDIF();
      else if (Memo("ELSECASE")) CodeELSECASE();
      else if (Memo("ENDCASE")) CodeENDCASE();
      else Result = False;
      break;
    case 'S':
      if (Memo("SWITCH") && !SwitchIsOccupied) CodeSWITCH();
      else if (Memo("SELECT") && SwitchIsOccupied) CodeSWITCH();
      else Result = False;
      break;
    case 'C':
      if (Memo("CASE")) CodeCASE();
      else Result = False;
      break;
    default:
      Result = False;
  }

  return Result;
}

Integer SaveIFs(void)
{
  return (!FirstIfSave) ? 0 : FirstIfSave->NestLevel;
}

void RestoreIFs(Integer Level)
{
  PIfSave OldSave;

  while ((FirstIfSave) && (FirstIfSave->NestLevel != Level))
  {
    OldSave = FirstIfSave;
    FirstIfSave = OldSave->Next;
    IfAsm = OldSave->SaveIfAsm;
    free(OldSave);
  }
}


Boolean IFListMask(void)
{
  switch (ListOn)
  {
    case 0:
      return True;
    case 1:
      return False;
    case 2:
      return ((!ActiveIF) && (!IfAsm));
    case 3:
      return (ActiveIF || (!IfAsm));
  }
  return True;
}


void AsmIFInit(void)
{
  IfAsm = True;
}


void asmif_init(void)
{
}
