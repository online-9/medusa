#include "medusa/architecture.hpp"

MEDUSA_NAMESPACE_BEGIN

  Architecture::Architecture(Tag ArchTag) : m_Tag(ArchTag)
{
  m_CfgMdl.InsertBoolean("Disassembly only basic block", false);
}

u8 Architecture::GetModeByName(std::string const& rModeName) const
{
  auto const& rModes = GetModes();
  for (auto itMode = std::begin(rModes), itEnd = std::end(rModes); itMode != itEnd; ++itMode)
    if (rModeName == std::get<0>(*itMode))
      return std::get<1>(*itMode);
  return 0;
}

bool Architecture::FormatCell(
  Document      const& rDoc,
  Address       const& rAddr,
  Cell          const& rCell,
  PrintData          & rPrintData) const
{
  switch (rCell.GetType())
  {
  case Cell::InstructionType: return FormatInstruction(rDoc, rAddr, static_cast<Instruction const&>(rCell), rPrintData);
  case Cell::ValueType:       return FormatValue      (rDoc, rAddr, static_cast<Value       const&>(rCell), rPrintData);
  case Cell::CharacterType:   return FormatCharacter  (rDoc, rAddr, static_cast<Character   const&>(rCell), rPrintData);
  case Cell::StringType:      return FormatString     (rDoc, rAddr, static_cast<String      const&>(rCell), rPrintData);
  default:                    return false;
  }
}

bool Architecture::FormatInstruction(
  Document      const& rDoc,
  Address       const& rAddr,
  Instruction   const& rInsn,
  PrintData          & rPrintData) const
{
  char Sep = '\0';

  rPrintData.AppendMnemonic(rInsn.GetName());

  for (unsigned int i = 0; i < OPERAND_NO; ++i)
  {
    Operand const* pOprd = rInsn.Operand(i);
    if (pOprd == nullptr)
      break;
    if (pOprd->GetType() == O_NONE)
      break;

    if (Sep != '\0')
      rPrintData.AppendOperator(",").AppendSpace();

    if (!FormatOperand(rDoc, rAddr, rInsn, *pOprd, i, rPrintData))
      return false;
  }

  return true;
}

bool Architecture::FormatOperand(
  Document      const& rDoc,
  Address       const& rAddr,
  Instruction   const& rInsn,
  Operand       const& rOprd,
  u8                   OperandNo,
  PrintData          & rPrintData) const
{
  auto const& rBinStrm = rDoc.GetBinaryStream();
  return false;
}


bool Architecture::FormatCharacter(
  Document      const& rDoc,
  Address       const& rAddr,
  Character     const& rChar,
  PrintData          & rPrintData) const
{
  auto const& rBinStrm = rDoc.GetBinaryStream();
  std::ostringstream oss;
  TOffset Off;

  if (!rDoc.ConvertAddressToFileOffset(rAddr, Off))
    return false;

  switch (rChar.GetSubType())
  {
  case Character::AsciiCharacterType: default:
    {
      s8 Char;
      if (!rBinStrm.Read(Off, Char))
        return false;

      switch (Char)
      {
      case '\0': oss << "\\0"; break;
      case '\a': oss << "\\a"; break;
      case '\b': oss << "\\b"; break;
      case '\t': oss << "\\t"; break;
      case '\n': oss << "\\n"; break;
      case '\v': oss << "\\v"; break;
      case '\f': oss << "\\f"; break;
      case '\r': oss << "\\r"; break;
      default:   oss << Char;  break;
      }
    }
  }
  rPrintData.AppendCharacter(oss.str());
  return true;
}

bool Architecture::FormatString(
  Document      const& rDoc,
  Address       const& rAddr,
  String        const& rStr,
  PrintData          & rPrintData) const
{
  auto const&        rBinStrm  = rDoc.GetBinaryStream();
  TOffset            FileOff;
  size_t             StrLen    = rStr.GetLength();
  StringTrait const* pStrTrait = rStr.GetStringTrait();

  if (pStrTrait == nullptr)
    return false;

  if (rDoc.ConvertAddressToFileOffset(rAddr, FileOff) == false)
    return false;

  char* pStrBuf = new char[StrLen];
  if (rDoc.GetBinaryStream().Read(FileOff, pStrBuf, StrLen) == false)
  {
    delete[] pStrBuf;
    return false;
  }

  std::string OrgStr = pStrTrait->ConvertToUtf8(pStrBuf, StrLen);
  delete[] pStrBuf;
  if (OrgStr.empty())
    return false;
  std::string FmtStr;

  if (rStr.GetSubType() == String::Utf16Type)
    rPrintData.AppendKeyword("L");

  rPrintData.AppendOperator("\"");

  auto itCharEnd = std::end(OrgStr);
  size_t FmtStrBeg = FmtStr.length();
  for (auto itChar = std::begin(OrgStr); itChar != itCharEnd; ++itChar)
  {
    switch (*itChar)
    {
    case '\0': FmtStr += "\\0";   break;
    case '\\': FmtStr += "\\\\";  break;
    case '\a': FmtStr += "\\a";   break;
    case '\b': FmtStr += "\\b";   break;
    case '\t': FmtStr += "\\t";   break;
    case '\n': FmtStr += "\\n";   break;
    case '\v': FmtStr += "\\v";   break;
    case '\f': FmtStr += "\\f";   break;
    case '\r': FmtStr += "\\r";   break;
    default:   FmtStr += *itChar; break;
    }
  }

  rPrintData.AppendString(FmtStr);
  rPrintData.AppendOperator("\"");
  return true;
}

bool Architecture::FormatValue(
  Document      const& rDoc,
  Address       const& rAddr,
  Value         const& rVal,
  PrintData          & rPrintData) const
{
  auto const&         rBinStrm = rDoc.GetBinaryStream();
  TOffset             Off;
  u8                  ValueModifier = rVal.GetSubType() & ValueDetail::ModifierMask;
  u8                  ValueType     = rVal.GetSubType() & ValueDetail::BaseMask;
  std::string         BasePrefix    = "";
  bool                IsUnk = false;

  switch (rVal.GetLength())
  {
  case 1:  rPrintData.AppendKeyword("db").AppendSpace(); break;
  case 2:  rPrintData.AppendKeyword("dw").AppendSpace(); break;
  case 4:  rPrintData.AppendKeyword("dd").AppendSpace(); break;
  case 8:  rPrintData.AppendKeyword("dq").AppendSpace(); break;
  default: rPrintData.AppendKeyword("d?").AppendSpace(); break;
  }

  if (!rDoc.ConvertAddressToFileOffset(rAddr, Off))
  {
    rPrintData.AppendOperator("(?)");
    return true;
  }

  u8 Base;
  switch (ValueType)
  {
  case ValueDetail::BinaryType:               Base = 2;  break;
  case ValueDetail::DecimalType:              Base = 10; break;
  case ValueDetail::HexadecimalType: default: Base = 16; break;
  }

  switch (ValueModifier)
  {
  case ValueDetail::NotType:    rPrintData.AppendOperator("~"); break;
  case ValueDetail::NegateType: rPrintData.AppendOperator("-"); break;
  default: break;
  }

  switch (rVal.GetLength())
  {
  case 1:
    {
      u8 Data;
      if (!rBinStrm.Read(Off, Data))
        return false;
      rVal.Modify(Data);
      if (ValueType == ValueDetail::CharacterType)
      {
        std::string FmtChr;
        switch (Data)
        {
        case '\0': FmtChr = "\\0";   break;
        case '\\': FmtChr = "\\\\";  break;
        case '\a': FmtChr = "\\a";   break;
        case '\b': FmtChr = "\\b";   break;
        case '\t': FmtChr = "\\t";   break;
        case '\n': FmtChr = "\\n";   break;
        case '\v': FmtChr = "\\v";   break;
        case '\f': FmtChr = "\\f";   break;
        case '\r': FmtChr = "\\r";   break;
        default:   FmtChr = Data; break;
        }
        rPrintData.AppendOperator("'").AppendCharacter(FmtChr).AppendOperator("'");
      }
      else
        rPrintData.AppendImmediate(Data, 8, Base);
      break;
    }
  case 2:
    {
      u16 Data;
      if (!rBinStrm.Read(Off, Data))
        return false;
      rVal.Modify(Data);

/*      if (ValueType == ValueDetail::CharacterType)
      {
        std::string FmtChr;
        switch (Data)
        {
        case '\0': FmtChr = "\\0";   break;
        case '\\': FmtChr = "\\\\";  break;
        case '\a': FmtChr = "\\a";   break;
        case '\b': FmtChr = "\\b";   break;
        case '\t': FmtChr = "\\t";   break;
        case '\n': FmtChr = "\\n";   break;
        case '\v': FmtChr = "\\v";   break;
        case '\f': FmtChr = "\\f";   break;
        case '\r': FmtChr = "\\r";   break;
        default:   FmtChr = Data & 0xff; break;
        }
        rPrintData.AppendKeyword("L").AppendOperator("'").AppendCharacter(FmtChr).AppendOperator("'");
      }

      else */if (ValueType & ValueDetail::ReferenceType)
      {
        Address Addr = rAddr;
        Addr.SetOffset(Data);
        Label Lbl = rDoc.GetLabelFromAddress(Addr);
        if (Lbl.GetType() != Label::Unknown)
          rPrintData.AppendLabel(Lbl.GetLabel());
        else
          rPrintData.AppendAddress(Addr);
      }

      else
      {
        rPrintData.AppendImmediate(Data, 16, Base);
      }
      break;
    }
  case 4:
    {
      u32 Data;
      if (!rBinStrm.Read(Off, Data))
        return false;
      rVal.Modify(Data);

/*      if (ValueType == ValueDetail::CharacterType)
      {
        std::string FmtChr;
        switch (Data)
        {
        case '\0': FmtChr = "\\0";   break;
        case '\\': FmtChr = "\\\\";  break;
        case '\a': FmtChr = "\\a";   break;
        case '\b': FmtChr = "\\b";   break;
        case '\t': FmtChr = "\\t";   break;
        case '\n': FmtChr = "\\n";   break;
        case '\v': FmtChr = "\\v";   break;
        case '\f': FmtChr = "\\f";   break;
        case '\r': FmtChr = "\\r";   break;
        default:   FmtChr = Data; break;
        }
        rPrintData.AppendKeyword("L").AppendOperator("'").AppendCharacter(FmtChr).AppendOperator("'");
      }

      else */if (ValueType == ValueDetail::ReferenceType)
      {
        Address Addr = rAddr;
        Addr.SetOffset(Data);
        Label Lbl = rDoc.GetLabelFromAddress(Addr);
        if (Lbl.GetType() != Label::Unknown)
          rPrintData.AppendLabel(Lbl.GetLabel());
        else
          rPrintData.AppendAddress(Addr);
      }

      else
      {
        rPrintData.AppendImmediate(Data, 32, Base);
      }
      break;
    }
  case 8:
    {
      u64 Data;
      if (!rBinStrm.Read(Off, Data))
        return false;
      rVal.Modify(Data);

/*      if (ValueType == ValueDetail::CharacterType)
      {
        std::string FmtChr;
        switch (Data)
        {
        case '\0': FmtChr = "\\0";   break;
        case '\\': FmtChr = "\\\\";  break;
        case '\a': FmtChr = "\\a";   break;
        case '\b': FmtChr = "\\b";   break;
        case '\t': FmtChr = "\\t";   break;
        case '\n': FmtChr = "\\n";   break;
        case '\v': FmtChr = "\\v";   break;
        case '\f': FmtChr = "\\f";   break;
        case '\r': FmtChr = "\\r";   break;
        default:   FmtChr.assign(1, Data); break;
        }
        rPrintData.AppendKeyword("L").AppendOperator("'").AppendCharacter(FmtChr).AppendOperator("'");
      }

      else */if (ValueType == ValueDetail::ReferenceType)
      {
        Address Addr = rAddr;
        Addr.SetOffset(Data);
        Label Lbl = rDoc.GetLabelFromAddress(Addr);
        if (Lbl.GetType() != Label::Unknown)
          rPrintData.AppendLabel(Lbl.GetLabel());
        else
          rPrintData.AppendAddress(Addr);
      }

      else
      {
        rPrintData.AppendImmediate(Data, 64, Base);
      }
      break;
    }

  default:
    return false;
  }

  return true;
}

bool Architecture::FormatMultiCell(
  Document      const& rDoc,
  Address       const& rAddress,
  MultiCell     const& rMultiCell,
  PrintData          & rPrintData) const
{
  switch (rMultiCell.GetType())
  {
  case MultiCell::FunctionType: return FormatFunction(rDoc, rAddress, static_cast<Function const&>(rMultiCell), rPrintData);
  default:                      return false;
  }
}

bool Architecture::FormatFunction(
  Document      const& rDoc,
  Address       const& rAddr,
  Function      const& rFunc,
  PrintData          & rPrintData) const
{
  std::ostringstream oss;
  oss << std::hex << std::showbase << std::left;
  auto FuncLabel = rDoc.GetLabelFromAddress(rAddr);

  oss
    << "; " << FuncLabel.GetLabel()
    << ": size=" << rFunc.GetSize()
    << ", insn_cnt=" << rFunc.GetInstructionCounter();

  rPrintData.AppendComment(oss.str());
  return true;
}

bool Architecture::FormatStructure(
    Document      const& rDoc,
    Address       const& rAddr,
    Structure     const& rStruct,
    PrintData          & rPrintData) const
{
  std::ostringstream oss;
  oss << std::hex << std::showbase << std::left;

  oss
    << "; " << "<structure_name>"
    << ": size=" << rStruct.GetSize();

  rPrintData.AppendComment(oss.str());
  return true;
}

MEDUSA_NAMESPACE_END
