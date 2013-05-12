#ifndef __OS_WINDOWS_STACK_ANALYZER__
#define __OS_WINDOWS_STACK_ANALYZER__

#include <medusa/namespace.hpp>
#include <medusa/expression.hpp>
#include <medusa/analyzer.hpp>

MEDUSA_NAMESPACE_USE

class ExpressionVisitor_FindOperations : public ExpressionVisitor
{
public:
  struct RegisterOffset
  {
    RegisterOffset(u32 Id = 0, s64 Offset = 0) : m_Id(Id), m_Offset(Offset) {}
    u32 m_Id;
    s64 m_Offset;

    bool operator==(u32 Id)
    {
      return m_Id == Id;
    }
  };

  ExpressionVisitor_FindOperations(std::list<RegisterOffset>& rRegisterOffsetList)
    : m_rRegisterOffsetList(rRegisterOffsetList), m_CurrentRegisterOffset() {}
  virtual Expression* VisitBind(Expression::List const& rExprList);
  virtual Expression* VisitOperation(u32 Type, Expression const* pLeftExpr, Expression const* pRightExpr);
  virtual Expression* VisitConstant(u32 Type, u64 Value);
  virtual Expression* VisitIdentifier(u32 Id, CpuInformation const* pCpuInfo);
  virtual Expression* VisitMemory(u32 AccessSizeInBit, Expression const* pBaseExpr, Expression const* pOffsetExpr, bool Deref);

  RegisterOffset& GetCurrentRegisterOffset(void) { return m_CurrentRegisterOffset; }
private:
  std::list<RegisterOffset>& m_rRegisterOffsetList;
  RegisterOffset             m_CurrentRegisterOffset;
};

class X86StackAnalyzerTracker : public Analyzer::Tracker
{
public:
  X86StackAnalyzerTracker(CpuInformation const* pCpuInfo);
  virtual bool Track(Analyzer& rAnlz, Database& rDb, Address const& rAddr);
private:
  std::list<ExpressionVisitor_FindOperations::RegisterOffset> m_RegisterOffsetList;
  CpuInformation const* m_pCpuInfo;
};

#endif // !__OS_WINDOWS_STACK_ANALYZER__