/* code47c00.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator Toshiba TLCS-47(0(A))                                       */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <string.h>

#include "bpemu.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "code47c00.h"

#define BitOrderCnt 4

enum
{
  ModNone = -1,
  ModAcc = 0,
  ModL = 1,
  ModH = 2,
  ModHL = 3,
  ModIHL = 4,
  ModAbs = 5,
  ModPort = 6,
  ModImm = 7,
  ModSAbs = 8,
};

#define MModAcc (1 << ModAcc)
#define MModL (1 << ModL)
#define MModH (1 << ModH)
#define MModHL (1 << ModHL)
#define MModIHL (1 << ModIHL)
#define MModAbs (1 << ModAbs)
#define MModPort (1 << ModPort)
#define MModImm (1 << ModImm)
#define MModSAbs (1 << ModSAbs)

#define M_CPU47C00 1
#define M_CPU470C00 2
#define M_CPU470AC00 4

static CPUVar CPU47C00, CPU470C00, CPU470AC00;
static ShortInt AdrType, OpSize;
static Byte AdrVal;
static LongInt DMBAssume;

/*---------------------------------------------------------------------------*/

static void SetOpSize(ShortInt NewSize)
{
  if (OpSize == -1) OpSize = NewSize;
  else if (OpSize != NewSize)
  {
    WrError(ErrNum_ConfOpSizes); AdrType = ModNone;
  }
}

static void DecodeAdr(const tStrComp *pArg, Word Mask)
{
  static char *RegNames[ModIHL + 1] = {"A", "L", "H", "HL", "@HL"};

  Byte z;
  Word AdrWord;
  Boolean OK;

  AdrType = ModNone;

  for (z = 0; z <= ModIHL; z++)
   if (!strcasecmp(pArg->Str, RegNames[z]))
   {
     AdrType = z;
     if (z != ModIHL) SetOpSize(Ord(z == ModHL));
     goto chk;
   }

  if (*pArg->Str == '#')
  {
    switch (OpSize)
    {
      case -1:
        WrError(ErrNum_UndefOpSizes);
        break;
      case 2:
        AdrVal = EvalStrIntExpressionOffs(pArg, 1, UInt2, &OK) & 3;
        if (OK)
          AdrType = ModImm;
        break;
      case 0:
        AdrVal = EvalStrIntExpressionOffs(pArg, 1, Int4, &OK) & 15;
        if (OK)
          AdrType = ModImm;
        break;
      case 1:
        AdrVal = EvalStrIntExpressionOffs(pArg, 1, Int8, &OK);
        if (OK)
          AdrType = ModImm;
        break;
    }
    goto chk;
  }

  if (*pArg->Str == '%')
  {
    AdrVal = EvalStrIntExpressionOffs(pArg, 1, Int5, &OK);
    if (OK)
    {
      AdrType = ModPort;
      ChkSpace(SegIO);
    }
    goto chk;
  }

  FirstPassUnknown = False;
  AdrWord = EvalStrIntExpression(pArg, Int16, &OK);
  if (OK)
  {
    ChkSpace(SegData);

    if (FirstPassUnknown)
      AdrWord &= SegLimits[SegData];
    else if (Hi(AdrWord) != DMBAssume)
      WrError(ErrNum_InAccPage);

    AdrVal = Lo(AdrWord);
    if (FirstPassUnknown)
      AdrVal &= 15;

    AdrType =  (((Mask & MModSAbs) != 0) && (AdrVal<16)) ? ModSAbs : ModAbs;
  }

chk:
  if ((AdrType != ModNone) && (!((1 << AdrType) & Mask)))
  {
    WrError(ErrNum_InvAddrMode);
    AdrType = ModNone;
  }
}

static void ChkCPU(Byte Mask)
{
  if (ChkExactCPUMask(Mask, CPU47C00) < 0)
    CodeLen = 0;
}

static Boolean DualOp(char *s1, char *s2)
{
  return (((!strcasecmp(ArgStr[1].Str, s1)) && (!strcasecmp(ArgStr[2].Str, s2)))
       || ((!strcasecmp(ArgStr[2].Str, s1)) && (!strcasecmp(ArgStr[1].Str, s2))));
}

/*---------------------------------------------------------------------------*/

/* ohne Argument */

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    CodeLen = 1;
    BAsmCode[0] = Code;
  }
}

/* Datentransfer */

static void DecodeLD(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (!strcasecmp(ArgStr[1].Str, "DMB"))
  {
    SetOpSize(2);
    DecodeAdr(&ArgStr[2], MModImm | MModIHL);
    switch (AdrType)
    {
      case ModIHL:
        CodeLen = 3;
        BAsmCode[0] = 0x03;
        BAsmCode[1] = 0x3a;
        BAsmCode[2] = 0xe9;
        ChkCPU(M_CPU470AC00);
        break;
      case ModImm:
        CodeLen = 3;
        BAsmCode[0] = 0x03;
        BAsmCode[1] = 0x2c;
        BAsmCode[2] = 0x09 + (AdrVal << 4);
        ChkCPU(M_CPU470AC00);
        break;
    }
  }
  else
  {
    DecodeAdr(&ArgStr[1], MModAcc | MModHL | MModH | MModL);
    switch (AdrType)
    {
      case ModAcc:
        DecodeAdr(&ArgStr[2], MModIHL | MModAbs | MModImm);
        switch (AdrType)
        {
          case ModIHL:
            CodeLen = 1;
            BAsmCode[0] = 0x0c;
            break;
          case ModAbs:
            CodeLen = 2;
            BAsmCode[0] = 0x3c;
            BAsmCode[1] = AdrVal;
            break;
          case ModImm:
            CodeLen = 1;
            BAsmCode[0] = 0x40 + AdrVal;
            break;
        }
        break;
      case ModHL:
        DecodeAdr(&ArgStr[2], MModAbs | MModImm);
        switch (AdrType)
        {
          case ModAbs:
            if (AdrVal & 3) WrError(ErrNum_NotAligned);
            else
            {
              CodeLen = 2;
              BAsmCode[0] = 0x28;
              BAsmCode[1] = AdrVal;
            }
            break;
          case ModImm:
            CodeLen = 2;
            BAsmCode[0] = 0xc0 + (AdrVal >> 4);
            BAsmCode[1] = 0xe0 + (AdrVal & 15);
            break;
        }
        break;
      case ModH:
      case ModL:
        BAsmCode[0] = 0xc0 + (Ord(AdrType == ModL) << 5);
        DecodeAdr(&ArgStr[2], MModImm);
        if (AdrType != ModNone)
        {
          CodeLen = 1;
          BAsmCode[0] += AdrVal;
        }
        break;
    }
  }
}

static void DecodeLDL(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if ((strcasecmp(ArgStr[1].Str, "A")) || (strcasecmp(ArgStr[2].Str, "@DC"))) WrError(ErrNum_InvAddrMode);
  else
  {
    CodeLen = 1;
    BAsmCode[0] = 0x33;
  }
}

static void DecodeLDH(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if ((strcasecmp(ArgStr[1].Str, "A")) || (strcasecmp(ArgStr[2].Str, "@DC+"))) WrError(ErrNum_InvAddrMode);
  else
  {
    CodeLen = 1;
    BAsmCode[0] = 0x32;
  }
}

static void DecodeST(Word Code)
{
  Byte HReg;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (!strcasecmp(ArgStr[1].Str, "DMB"))
  {
    DecodeAdr(&ArgStr[2], MModIHL);
    if (AdrType != ModNone)
    {
      CodeLen = 3;
      BAsmCode[0] = 0x03;
      BAsmCode[1] = 0x3a;
      BAsmCode[2] = 0x69;
      ChkCPU(M_CPU470AC00);
    }
  }
  else
  {
    OpSize = 0;
    DecodeAdr(&ArgStr[1], MModImm | MModAcc);
    switch (AdrType)
    {
      case ModAcc:
        if (!strcasecmp(ArgStr[2].Str, "@HL+"))
        {
          CodeLen = 1;
          BAsmCode[0] = 0x1a;
        }
        else if (!strcasecmp(ArgStr[2].Str, "@HL-"))
        {
          CodeLen = 1;
          BAsmCode[0] = 0x1b;
        }
        else
        {
          DecodeAdr(&ArgStr[2], MModAbs | MModIHL);
          switch (AdrType)
          {
            case ModAbs:
              CodeLen = 2;
              BAsmCode[0] = 0x3f;
              BAsmCode[1] = AdrVal;
              break;
            case ModIHL:
              CodeLen = 1;
              BAsmCode[0] = 0x0f;
              break;
          }
        }
        break;
      case ModImm:
        HReg = AdrVal;
        if (!strcasecmp(ArgStr[2].Str, "@HL+"))
        {
          CodeLen = 1;
          BAsmCode[0] = 0xf0 + HReg;
        }
        else
        {
          DecodeAdr(&ArgStr[2], MModSAbs);
          if (AdrType != ModNone)
          {
            if (AdrVal>0x0f) WrError(ErrNum_OverRange);
            else
            {
              CodeLen = 2;
              BAsmCode[0] = 0x2d;
              BAsmCode[1] = (HReg << 4) + AdrVal;
            }
          }
        }
        break;
    }
  }
}

static void DecodeMOV(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if ((!strcasecmp(ArgStr[1].Str, "A")) && (!strcasecmp(ArgStr[2].Str, "DMB")))
  {
    CodeLen = 3;
    BAsmCode[0] = 0x03;
    BAsmCode[1] = 0x3a;
    BAsmCode[2] = 0xa9;
    ChkCPU(M_CPU470AC00);
  }
  else if ((!strcasecmp(ArgStr[1].Str, "DMB")) && (!strcasecmp(ArgStr[2].Str, "A")))
  {
    CodeLen = 3;
    BAsmCode[0] = 0x03;
    BAsmCode[1] = 0x3a;
    BAsmCode[2] = 0x29;
    ChkCPU(M_CPU470AC00);
  }
  else if ((!strcasecmp(ArgStr[1].Str, "A")) && (!strcasecmp(ArgStr[2].Str, "SPW13")))
  {
    CodeLen = 2;
    BAsmCode[0] = 0x3a;
    BAsmCode[1] = 0x84;
    ChkCPU(M_CPU470AC00);
  }
  else if ((!strcasecmp(ArgStr[1].Str, "STK13")) && (!strcasecmp(ArgStr[2].Str, "A")))
  {
    CodeLen = 2;
    BAsmCode[0] = 0x3a;
    BAsmCode[1] = 0x04;
    ChkCPU(M_CPU470AC00);
  }
  else if (strcasecmp(ArgStr[2].Str, "A")) WrError(ErrNum_InvAddrMode);
  else
  {
    DecodeAdr(&ArgStr[1], MModH | MModL);
    if (AdrType != ModNone)
    {
      CodeLen = 1;
      BAsmCode[0] = 0x10 + Ord(AdrType == ModL);
    }
  }
}

static void DecodeXCH(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (DualOp("A", "EIR"))
  {
    CodeLen = 1;
    BAsmCode[0] = 0x13;
  }
  else if (DualOp("A", "@HL"))
  {
    CodeLen = 1;
    BAsmCode[0] = 0x0d;
  }
  else if (DualOp("A", "H"))
  {
    CodeLen = 1;
    BAsmCode[0] = 0x30;
  }
  else if (DualOp("A", "L"))
  {
    CodeLen = 1;
    BAsmCode[0] = 0x31;
  }
  else
  {
    tStrComp *pArg1 = &ArgStr[1], *pArg2 = &ArgStr[2];

    if ((strcasecmp(pArg1->Str, "A")) && (strcasecmp(pArg1->Str, "HL")))
    {
      pArg1 = &ArgStr[2];
      pArg2 = &ArgStr[1];
    }
    if ((strcasecmp(pArg1->Str, "A")) && (strcasecmp(pArg1->Str, "HL"))) WrError(ErrNum_InvAddrMode);
    else
    {
      DecodeAdr(pArg2, MModAbs);
      if (AdrType != ModNone)
      {
        if ((!strcasecmp(pArg1->Str, "HL")) && (AdrVal & 3)) WrError(ErrNum_NotAligned);
        else
        {
          CodeLen = 2;
          BAsmCode[0] = 0x29 + (0x14 * Ord(!strcasecmp(pArg1->Str, "A")));
          BAsmCode[1] = AdrVal;
        }
      }
    }
  }
}

static void DecodeIN(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModPort);
    if (AdrType != ModNone)
    {
      Byte HReg = AdrVal;
      DecodeAdr(&ArgStr[2], MModAcc | MModIHL);
      switch (AdrType)
      {
        case ModAcc:
          CodeLen = 2;
          BAsmCode[0] = 0x3a;
          BAsmCode[1] = (HReg & 0x0f) + (((HReg & 0x10) ^ 0x10) << 1);
          break;
        case ModIHL:
          CodeLen = 2;
          BAsmCode[0] = 0x3a;
          BAsmCode[1] = 0x40 + (HReg & 0x0f) + (((HReg & 0x10) ^ 0x10) << 1);
          break;
      }
    }
  }
}

static void DecodeOUT(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModPort);
    if (AdrType != ModNone)
    {
      Byte HReg = AdrVal;
      OpSize = 0;
      DecodeAdr(&ArgStr[1], MModAcc | MModIHL | MModImm);
      switch (AdrType)
      {
        case ModAcc:
          CodeLen = 2;
          BAsmCode[0] = 0x3a;
          BAsmCode[1] = 0x80 + ((HReg & 0x10) << 1) + ((HReg & 0x0f) ^ 4);
          break;
        case ModIHL:
          CodeLen = 2;
          BAsmCode[0] = 0x3a;
          BAsmCode[1] = 0xc0 + ((HReg & 0x10) << 1) + ((HReg & 0x0f) ^ 4);
          break;
        case ModImm:
          if (HReg > 0x0f) WrError(ErrNum_InvAddrMode);
          else
          {
            CodeLen = 2;
            BAsmCode[0] = 0x2c;
            BAsmCode[1] = (AdrVal << 4) + HReg;
          }
          break;
      }
    }
  }
}

static void DecodeOUTB(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (strcasecmp(ArgStr[1].Str, "@HL")) WrError(ErrNum_InvAddrMode);
  else
  {
    CodeLen = 1;
    BAsmCode[0] = 0x12;
  }
}

/* Arithmetik */

static void DecodeCMPR(Word Code)
{
  Byte HReg;

  UNUSED(Code); 

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModAcc | MModSAbs | MModH | MModL);
    switch (AdrType)
    {
      case ModAcc:
        DecodeAdr(&ArgStr[2], MModIHL | MModAbs | MModImm);
        switch (AdrType)
        {
          case ModIHL:
            CodeLen = 1;
            BAsmCode[0] = 0x16;
            break;
          case ModAbs:
            CodeLen = 2;
            BAsmCode[0] = 0x3e;
            BAsmCode[1] = AdrVal;
            break;
          case ModImm:
            CodeLen = 1;
            BAsmCode[0] = 0xd0 + AdrVal;
            break;
        }
        break;
      case ModSAbs:
        OpSize = 0;
        HReg = AdrVal;
        DecodeAdr(&ArgStr[2], MModImm);
        if (AdrType != ModNone)
        {
          CodeLen = 2;
          BAsmCode[0] = 0x2e;
          BAsmCode[1] = (AdrVal << 4) + HReg;
        }
        break;
      case ModH:
      case ModL:
        HReg = AdrType;
        DecodeAdr(&ArgStr[2], MModImm);
        if (AdrType != ModNone)
        {
          CodeLen = 2;
          BAsmCode[0] = 0x38;
          BAsmCode[1] = 0x90 + (Ord(HReg == ModH) << 6) + AdrVal;
        }
        break;
    }
  }
}

static void DecodeADD(Word Code)
{
  Byte HReg;

  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModAcc | MModIHL | MModSAbs | MModL | MModH);
    switch (AdrType)
    {
      case ModAcc:
        DecodeAdr(&ArgStr[2], MModIHL | MModImm);
        switch (AdrType)
        {
          case ModIHL:
            CodeLen = 1;
            BAsmCode[0] = 0x17;
            break;
          case ModImm:
            CodeLen = 2;
            BAsmCode[0] = 0x38;
            BAsmCode[1] = AdrVal;
            break;
        }
        break;
      case ModIHL:
        OpSize = 0;
        DecodeAdr(&ArgStr[2], MModImm);
        if (AdrType != ModNone)
        {
          CodeLen = 2;
          BAsmCode[0] = 0x38;
          BAsmCode[1] = 0x40 + AdrVal;
        }
        break;
      case ModSAbs:
        HReg = AdrVal;
        OpSize = 0;
        DecodeAdr(&ArgStr[2], MModImm);
        if (AdrType != ModNone)
        {
          CodeLen = 2;
          BAsmCode[0] = 0x2f;
          BAsmCode[1] = (AdrVal << 4) + HReg;
        }
        break;
      case ModH:
      case ModL:
        HReg = Ord(AdrType == ModH);
        DecodeAdr(&ArgStr[2], MModImm);
        if (AdrType != ModNone)
        {
          CodeLen = 2;
          BAsmCode[0] = 0x38;
          BAsmCode[1] = 0x80 + (HReg << 6) + AdrVal;
        }
        break;
    }
  }
}

static void DecodeADDC(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if ((strcasecmp(ArgStr[1].Str, "A")) || (strcasecmp(ArgStr[2].Str, "@HL"))) WrError(ErrNum_InvAddrMode);
  else
  {
    CodeLen = 1;
    BAsmCode[0] = 0x15;
  }
}

static void DecodeSUBRC(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if ((strcasecmp(ArgStr[1].Str, "A")) || (strcasecmp(ArgStr[2].Str, "@HL"))) WrError(ErrNum_InvAddrMode);
  else
  {
    CodeLen = 1;
    BAsmCode[0] = 0x14;
  }
}

static void DecodeSUBR(Word Code)
{
  Byte HReg;

  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    OpSize = 0;
    DecodeAdr(&ArgStr[2], MModImm);
    if (AdrType != ModNone)
    {
      HReg = AdrVal;
      DecodeAdr(&ArgStr[1], MModAcc | MModIHL);
      switch (AdrType)
      {
        case ModAcc:
          CodeLen = 2;
          BAsmCode[0] = 0x38;
          BAsmCode[1] = 0x10 + HReg;
          break;
        case ModIHL:
          CodeLen = 2;
          BAsmCode[0] = 0x38;
          BAsmCode[1] = 0x50 + HReg;
          break;
      }
    }
  }
}

static void DecodeINC_DEC(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModAcc | MModIHL | MModL);
    switch (AdrType)
    {
      case ModAcc:
        CodeLen = 1;
        BAsmCode[0] = 0x08 + Code;
        break;
      case ModL:
        CodeLen = 1;
        BAsmCode[0] = 0x18 + Code;
        break;
      case ModIHL:
        CodeLen = 1;
        BAsmCode[0] = 0x0a + Code;
        break;
    }
  }
}

/* Logik */

static void DecodeAND_OR(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModAcc | MModIHL);
    switch (AdrType)
    {
      case ModAcc:
        DecodeAdr(&ArgStr[2], MModImm | MModIHL);
        switch (AdrType)
        {
          case ModIHL:
            CodeLen = 1;
            BAsmCode[0] = 0x1e - Code;
            break;
          case ModImm:
            CodeLen = 2;
            BAsmCode[0] = 0x38;
            BAsmCode[1] = 0x30 - (Code << 4) + AdrVal;
            break;
        }
        break;
      case ModIHL:
        SetOpSize(0);
        DecodeAdr(&ArgStr[2], MModImm);
        if (AdrType != ModNone)
        {
          CodeLen = 2;
          BAsmCode[0] = 0x38;
          BAsmCode[1] = 0x70 - (Code << 4) + AdrVal;
        }
        break;
    }
  }
}

static void DecodeXOR(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if ((strcasecmp(ArgStr[1].Str, "A")) || (strcasecmp(ArgStr[2].Str, "@HL"))) WrError(ErrNum_InvAddrMode);
  else
  {
    CodeLen = 1;
    BAsmCode[0] = 0x1f;
  }
}

static void DecodeROLC_RORC(Word Code)
{
  if (!ChkArgCnt(1, 2));
  else if (strcasecmp(ArgStr[1].Str, "A")) WrError(ErrNum_InvAddrMode);
  else
  {
    Boolean OK;
    Byte HReg;

    if (ArgCnt == 1)
    {
      HReg = 1;
      OK = True;
    }
    else
      HReg = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
    if (OK)
    {
      int z;

      BAsmCode[0] = Code;
      for (z = 1; z < HReg; z++)
        BAsmCode[z] = BAsmCode[0];
      CodeLen = HReg;
      if (HReg >= 4)
        WrError(ErrNum_Senseless);
    }
  }
}

static void DecodeBit(Word Code)
{
  Byte HReg;
  Boolean OK;

  if (ArgCnt == 1)
  {
    if (!strcasecmp(ArgStr[1].Str, "@L"))
    {
      if (Memo("TESTP")) WrError(ErrNum_InvAddrMode);
      else
      {
        if (Code == 2)
          Code = 3;
        CodeLen = 1;
        BAsmCode[0] = 0x34 + Code;
      }
    }
    else if (!strcasecmp(ArgStr[1].Str, "CF"))
    {
      if (Code < 2) WrError(ErrNum_InvAddrMode);
      else
      {
        CodeLen = 1;
        BAsmCode[0] = 10 -2 * Code;
      }
    }
    else if (!strcasecmp(ArgStr[1].Str, "ZF"))
    {
      if (Code != 3) WrError(ErrNum_InvAddrMode);
      else
      {
        CodeLen = 1;
        BAsmCode[0] = 0x0e;
      }
    }
    else if (!strcasecmp(ArgStr[1].Str, "GF"))
    {
      if (Code == 2) WrError(ErrNum_InvAddrMode);
      else
      {
        CodeLen = 1;
        BAsmCode[0] = (Code == 3) ? 1 : 3 - Code;
        ChkCPU(M_CPU47C00);
      }
    }
    else if ((!strcasecmp(ArgStr[1].Str, "DMB")) || (!strcasecmp(ArgStr[1].Str, "DMB0")))
    {
      CodeLen = 2;
      BAsmCode[0] = 0x3b;
      BAsmCode[1] = 0x39 + (Code << 6);
      ChkCPU(strcasecmp(ArgStr[1].Str, "DMB0") ? M_CPU470C00 : M_CPU470AC00);
    }
    else if (!strcasecmp(ArgStr[1].Str, "DMB1"))
    {
      CodeLen = 3;
      BAsmCode[0] = 3;
      BAsmCode[1] = 0x3b;
      BAsmCode[2] = 0x19 + (Code << 6);
      ChkCPU(M_CPU470AC00);
    }
    else if (!strcasecmp(ArgStr[1].Str, "STK13"))
    {
      if (Code > 1) WrError(ErrNum_InvAddrMode);
      else
      {
        CodeLen = 3;
        BAsmCode[0] = 3 - Code;
        BAsmCode[1] = 0x3a;
        BAsmCode[2] = 0x84;
        ChkCPU(M_CPU470AC00);
      }
    }
    else
      WrError(ErrNum_InvAddrMode);
  }
  else if (ArgCnt == 2)
  {
    if (!strcasecmp(ArgStr[1].Str, "IL"))
    {
      if (Code != 1) WrError(ErrNum_InvAddrMode);
      else
      {
        HReg = EvalStrIntExpression(&ArgStr[2], UInt6, &OK);
        if (OK)
        {
          CodeLen = 2;
          BAsmCode[0] = 0x36;
          BAsmCode[1] = 0xc0 + HReg;
        }
      }
    }
    else
    {
      HReg = EvalStrIntExpression(&ArgStr[2], UInt2, &OK);
      if (OK)
      {
        DecodeAdr(&ArgStr[1], MModAcc | MModIHL | MModPort | MModSAbs);
        switch (AdrType)
        {
          case ModAcc:
            if (Code != 2) WrError(ErrNum_InvAddrMode);
            else
            {
              CodeLen = 1;
              BAsmCode[0] = 0x5c + HReg;
            }
            break;
          case ModIHL:
            if (Code == 3) WrError(ErrNum_InvAddrMode);
            else
            {
              CodeLen = 1;
              BAsmCode[0] = 0x50 + HReg + (Code << 2);
            }
            break;
          case ModPort:
            if (AdrVal>15) WrError(ErrNum_OverRange);
            else
            {
              CodeLen = 2;
              BAsmCode[0] = 0x3b;
              BAsmCode[1] = (Code << 6) + (HReg << 4) + AdrVal;
            }
            break;
          case ModSAbs:
            CodeLen = 2;
            BAsmCode[0] = 0x39;
            BAsmCode[1] = (Code << 6) + (HReg << 4) + AdrVal;
            break;
        }
      }
    }
  }
  else
    (void)ChkArgCnt(1, 2);
}

static void DecodeEICLR_DICLR(Word Code)
{
  if (!ChkArgCnt(2, 2));
  else if (strcasecmp(ArgStr[1].Str, "IL")) WrError(ErrNum_InvAddrMode);
  else
  {
    Boolean OK;

    BAsmCode[1] = EvalStrIntExpression(&ArgStr[2], UInt6, &OK);
    if (OK)
    {
      CodeLen = 2;
      BAsmCode[0] = 0x36;
      BAsmCode[1] += Code;
    }
  }
}

/* Spruenge */

static void DecodeBSS(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Word AdrWord;

    FirstPassUnknown = False;
    AdrWord = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
    if (OK && ChkSamePage(AdrWord, EProgCounter() + 1, 6))
    {
      ChkSpace(SegCode);
      CodeLen = 1;
      BAsmCode[0] = 0x80 + (AdrWord & 0x3f);
    }
  }
}

static void DecodeBS(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Word AdrWord;

    FirstPassUnknown = False;
    AdrWord = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
    if (OK && ChkSamePage(AdrWord, EProgCounter() + 2, 12))
    {
      ChkSpace(SegCode);
      CodeLen = 2;
      BAsmCode[0] = 0x60 + (Hi(AdrWord) & 15);
      BAsmCode[1] = Lo(AdrWord);
    }
  }
}

static void DecodeBSL(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Word AdrWord = EvalStrIntExpression(&ArgStr[1], Int16, &OK);

    if (OK)
    {
      if (AdrWord > SegLimits[SegCode]) WrError(ErrNum_OverRange);
      else
      {
        ChkSpace(SegCode);
        CodeLen = 3;
        switch (AdrWord >> 12)
        {
          case 0: BAsmCode[0] = 0x02; break;
          case 1: BAsmCode[0] = 0x03; break;
          case 2: BAsmCode[0] = 0x1c; break;
          case 3: BAsmCode[0] = 0x01; break;
        }
        BAsmCode[1] = 0x60 + (Hi(AdrWord) & 0x0f);
        BAsmCode[2] = Lo(AdrWord);
      }
    }
  }
}

static void DecodeB(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Word AdrWord = EvalStrIntExpression(&ArgStr[1], Int16, &OK);

    if (OK)
    {
      if (AdrWord > SegLimits[SegCode]) WrError(ErrNum_OverRange);
      else
      {
        ChkSpace(SegCode);
        if ((AdrWord >> 6) == ((EProgCounter() + 1) >> 6))
        {
          CodeLen = 1;
          BAsmCode[0] = 0x80 + (AdrWord & 0x3f);
        }
        else if ((AdrWord >> 12) == ((EProgCounter() + 2) >> 12))
        {
          CodeLen = 2;
          BAsmCode[0] = 0x60 + (Hi(AdrWord) & 0x0f);
          BAsmCode[1] = Lo(AdrWord);
        }
        else
        {
          CodeLen = 3;
          switch (AdrWord >> 12)
          {
            case 0: BAsmCode[0] = 0x02; break;
            case 1: BAsmCode[0] = 0x03; break;
            case 2: BAsmCode[0] = 0x1c; break;
            case 3: BAsmCode[0] = 0x01; break;
          }
          BAsmCode[1] = 0x60 + (Hi(AdrWord) & 0x0f);
          BAsmCode[2] = Lo(AdrWord);
        }
      }
    }
  }
}

static void DecodeCALLS(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Word AdrWord = EvalStrIntExpression(&ArgStr[1], Int16, &OK);

    if (OK)
    {
      if (AdrWord == 0x86)
        AdrWord = 0x06;
      if ((AdrWord & 0xff87) != 6) WrError(ErrNum_InvOpType);
      else
      {
        CodeLen = 1;
        BAsmCode[0] = (AdrWord >> 3) + 0x70;
      }
    }
  }
}

static void DecodeCALL(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Word AdrWord;

    FirstPassUnknown = False;
    AdrWord = EvalStrIntExpression(&ArgStr[1], Int16, &OK);
    if (OK && ChkSamePage(AdrWord, EProgCounter(), 11))
    {
      ChkSpace(SegCode);
      CodeLen = 2;
      BAsmCode[0] = 0x20 + (Hi(AdrWord) & 7);
      BAsmCode[1] = Lo(AdrWord);
    }
  }
}

static void DecodePORT(Word Code)
{
  UNUSED(Code);

  CodeEquate(SegIO, 0, SegLimits[SegIO]);
}

/*---------------------------------------------------------------------------*/

static void AddFixed(char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(203);
  AddInstTable(InstTable, "LD", 0, DecodeLD);
  AddInstTable(InstTable, "LDL", 0, DecodeLDL);
  AddInstTable(InstTable, "LDH", 0, DecodeLDH);
  AddInstTable(InstTable, "ST", 0, DecodeST);
  AddInstTable(InstTable, "MOV", 0, DecodeMOV);
  AddInstTable(InstTable, "XCH", 0, DecodeXCH);
  AddInstTable(InstTable, "IN", 0, DecodeIN);
  AddInstTable(InstTable, "OUT", 0, DecodeOUT);
  AddInstTable(InstTable, "OUTB", 0, DecodeOUTB);
  AddInstTable(InstTable, "CMPR", 0, DecodeCMPR);
  AddInstTable(InstTable, "ADD", 0, DecodeADD);
  AddInstTable(InstTable, "ADDC", 0, DecodeADDC);
  AddInstTable(InstTable, "SUBRC", 0, DecodeSUBRC);
  AddInstTable(InstTable, "SUBR", 0, DecodeSUBR);
  AddInstTable(InstTable, "INC", 0, DecodeINC_DEC);
  AddInstTable(InstTable, "DEC", 1, DecodeINC_DEC);
  AddInstTable(InstTable, "AND", 0, DecodeAND_OR);
  AddInstTable(InstTable, "OR", 1, DecodeAND_OR);
  AddInstTable(InstTable, "XOR", 0, DecodeXOR);
  AddInstTable(InstTable, "ROLC", 0x05, DecodeROLC_RORC);
  AddInstTable(InstTable, "RORC", 0x07, DecodeROLC_RORC);
  AddInstTable(InstTable, "EICLR", 0x40, DecodeEICLR_DICLR);
  AddInstTable(InstTable, "DICLR", 0x80, DecodeEICLR_DICLR);
  AddInstTable(InstTable, "BSS", 0, DecodeBSS);
  AddInstTable(InstTable, "BS", 0, DecodeBS);
  AddInstTable(InstTable, "BSL", 0, DecodeBSL);
  AddInstTable(InstTable, "B", 0, DecodeB);
  AddInstTable(InstTable, "CALLS", 0, DecodeCALLS);
  AddInstTable(InstTable, "CALL", 0, DecodeCALL);
  AddInstTable(InstTable, "PORT", 0, DecodePORT);

  AddFixed("RET" , 0x2a);
  AddFixed("RETI", 0x2b);
  AddFixed("NOP" , 0x00);

  InstrZ = 0;
  AddInstTable(InstTable, "SET", InstrZ++, DecodeBit);
  AddInstTable(InstTable, "CLR", InstrZ++, DecodeBit);
  AddInstTable(InstTable, "TEST", InstrZ++, DecodeBit);
  AddInstTable(InstTable, "TESTP", InstrZ++, DecodeBit);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*--------------------------------------------------------------------------*/

static void MakeCode_47C00(void)
{
  CodeLen = 0;
  DontPrint = False;
  OpSize = -1;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(False))
    return;

  if (!LookupInstTable(InstTable, OpPart.Str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_47C00(void)
{
  return (Memo("PORT"));
}

static void SwitchFrom_47C00(void)
{
  DeinitFields();
}

static Boolean TrueFnc(void)
{
  return True;
}

static void SwitchTo_47C00(void)
{
#define ASSUME47Count (sizeof(ASSUME47s) / sizeof(*ASSUME47s))
  static ASSUMERec ASSUME47s[] =
  {
    { "DMB", &DMBAssume, 0, 3, 4, NULL }
  };

  TurnWords = False;
  ConstMode = ConstModeIntel;
  SetIsOccupiedFnc = TrueFnc;

  PCSymbol = "$";
  HeaderID = 0x55;
  NOPCode = 0x00;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData) | (1 << SegIO);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  Grans[SegData] = 1; ListGrans[SegData] = 1; SegInits[SegData] = 0;
  Grans[SegIO  ] = 1; ListGrans[SegIO  ] = 1; SegInits[SegIO  ] = 0;
  if (MomCPU == CPU47C00)
  {
    SegLimits[SegCode] = 0xfff;
    SegLimits[SegData] = 0xff;
    SegLimits[SegIO] = 0x0f;
  }
  else if (MomCPU == CPU470C00)
  {
    SegLimits[SegCode] = 0x1fff;
    SegLimits[SegData] = 0x1ff;
    SegLimits[SegIO] = 0x1f;
  }
  else if (MomCPU == CPU470AC00)
  { 
    SegLimits[SegCode] = 0x3fff;
    SegLimits[SegData] = 0x3ff;
    SegLimits[SegIO] = 0x1f;
  }

  pASSUMERecs = ASSUME47s;
  ASSUMERecCnt = ASSUME47Count;

  MakeCode = MakeCode_47C00;
  IsDef = IsDef_47C00;
  SwitchFrom = SwitchFrom_47C00;
  InitFields();
}

static void InitCode_47C00(void)
{
  DMBAssume = 0;
}

void code47c00_init(void)
{
  CPU47C00 = AddCPU("47C00", SwitchTo_47C00);
  CPU470C00 = AddCPU("470C00", SwitchTo_47C00);
  CPU470AC00 = AddCPU("470AC00", SwitchTo_47C00);

  AddInitPassProc(InitCode_47C00);
}
