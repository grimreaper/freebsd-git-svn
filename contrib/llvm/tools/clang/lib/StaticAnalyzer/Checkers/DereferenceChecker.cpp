//== NullDerefChecker.cpp - Null dereference checker ------------*- C++ -*--==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This defines NullDerefChecker, a builtin check in ExprEngine that performs
// checks for null pointers at loads and stores.
//
//===----------------------------------------------------------------------===//

#include "ClangSACheckers.h"
#include "clang/AST/ExprObjC.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "llvm/ADT/SmallString.h"

using namespace clang;
using namespace ento;

namespace {
class DereferenceChecker
    : public Checker< check::Location,
                      check::Bind,
                      EventDispatcher<ImplicitNullDerefEvent> > {
  mutable OwningPtr<BuiltinBug> BT_null;
  mutable OwningPtr<BuiltinBug> BT_undef;

  void reportBug(ProgramStateRef State, const Stmt *S, CheckerContext &C,
                 bool IsBind = false) const;

public:
  void checkLocation(SVal location, bool isLoad, const Stmt* S,
                     CheckerContext &C) const;
  void checkBind(SVal L, SVal V, const Stmt *S, CheckerContext &C) const;

  static const MemRegion *AddDerefSource(raw_ostream &os,
                             SmallVectorImpl<SourceRange> &Ranges,
                             const Expr *Ex, const ProgramState *state,
                             const LocationContext *LCtx,
                             bool loadedFrom = false);
};
} // end anonymous namespace

const MemRegion *
DereferenceChecker::AddDerefSource(raw_ostream &os,
                                   SmallVectorImpl<SourceRange> &Ranges,
                                   const Expr *Ex,
                                   const ProgramState *state,
                                   const LocationContext *LCtx,
                                   bool loadedFrom) {
  Ex = Ex->IgnoreParenLValueCasts();
  const MemRegion *sourceR = 0;
  switch (Ex->getStmtClass()) {
    default:
      break;
    case Stmt::DeclRefExprClass: {
      const DeclRefExpr *DR = cast<DeclRefExpr>(Ex);
      if (const VarDecl *VD = dyn_cast<VarDecl>(DR->getDecl())) {
        os << " (" << (loadedFrom ? "loaded from" : "from")
           << " variable '" <<  VD->getName() << "')";
        Ranges.push_back(DR->getSourceRange());
        sourceR = state->getLValue(VD, LCtx).getAsRegion();
      }
      break;
    }
    case Stmt::MemberExprClass: {
      const MemberExpr *ME = cast<MemberExpr>(Ex);
      os << " (" << (loadedFrom ? "loaded from" : "via")
         << " field '" << ME->getMemberNameInfo() << "')";
      SourceLocation L = ME->getMemberLoc();
      Ranges.push_back(SourceRange(L, L));
      break;
    }
  }
  return sourceR;
}

void DereferenceChecker::reportBug(ProgramStateRef State, const Stmt *S,
                                   CheckerContext &C, bool IsBind) const {
  // Generate an error node.
  ExplodedNode *N = C.generateSink(State);
  if (!N)
    return;

  // We know that 'location' cannot be non-null.  This is what
  // we call an "explicit" null dereference.
  if (!BT_null)
    BT_null.reset(new BuiltinBug("Dereference of null pointer"));

  SmallString<100> buf;
  SmallVector<SourceRange, 2> Ranges;

  // Walk through lvalue casts to get the original expression
  // that syntactically caused the load.
  if (const Expr *expr = dyn_cast<Expr>(S))
    S = expr->IgnoreParenLValueCasts();

  const MemRegion *sourceR = 0;

  if (IsBind) {
    if (const BinaryOperator *BO = dyn_cast<BinaryOperator>(S)) {
      if (BO->isAssignmentOp())
        S = BO->getRHS();
    } else if (const DeclStmt *DS = dyn_cast<DeclStmt>(S)) {
      assert(DS->isSingleDecl() && "We process decls one by one");
      if (const VarDecl *VD = dyn_cast<VarDecl>(DS->getSingleDecl()))
        if (const Expr *Init = VD->getAnyInitializer())
          S = Init;
    }
  }

  switch (S->getStmtClass()) {
  case Stmt::ArraySubscriptExprClass: {
    llvm::raw_svector_ostream os(buf);
    os << "Array access";
    const ArraySubscriptExpr *AE = cast<ArraySubscriptExpr>(S);
    sourceR = AddDerefSource(os, Ranges, AE->getBase()->IgnoreParenCasts(),
                             State.getPtr(), N->getLocationContext());
    os << " results in a null pointer dereference";
    break;
  }
  case Stmt::UnaryOperatorClass: {
    llvm::raw_svector_ostream os(buf);
    os << "Dereference of null pointer";
    const UnaryOperator *U = cast<UnaryOperator>(S);
    sourceR = AddDerefSource(os, Ranges, U->getSubExpr()->IgnoreParens(),
                             State.getPtr(), N->getLocationContext(), true);
    break;
  }
  case Stmt::MemberExprClass: {
    const MemberExpr *M = cast<MemberExpr>(S);
    if (M->isArrow()) {
      llvm::raw_svector_ostream os(buf);
      os << "Access to field '" << M->getMemberNameInfo()
         << "' results in a dereference of a null pointer";
      sourceR = AddDerefSource(os, Ranges, M->getBase()->IgnoreParenCasts(),
                               State.getPtr(), N->getLocationContext(), true);
    }
    break;
  }
  case Stmt::ObjCIvarRefExprClass: {
    const ObjCIvarRefExpr *IV = cast<ObjCIvarRefExpr>(S);
    if (const DeclRefExpr *DR =
        dyn_cast<DeclRefExpr>(IV->getBase()->IgnoreParenCasts())) {
      if (const VarDecl *VD = dyn_cast<VarDecl>(DR->getDecl())) {
        llvm::raw_svector_ostream os(buf);
        os << "Instance variable access (via '" << VD->getName()
           << "') results in a null pointer dereference";
      }
    }
    Ranges.push_back(IV->getSourceRange());
    break;
  }
  default:
    break;
  }

  BugReport *report =
    new BugReport(*BT_null,
                  buf.empty() ? BT_null->getDescription() : buf.str(),
                  N);

  bugreporter::addTrackNullOrUndefValueVisitor(N, bugreporter::GetDerefExpr(N),
                                               report);

  for (SmallVectorImpl<SourceRange>::iterator
       I = Ranges.begin(), E = Ranges.end(); I!=E; ++I)
    report->addRange(*I);

  if (sourceR) {
    report->markInteresting(sourceR);
    report->markInteresting(State->getRawSVal(loc::MemRegionVal(sourceR)));
  }

  C.EmitReport(report);
}

void DereferenceChecker::checkLocation(SVal l, bool isLoad, const Stmt* S,
                                       CheckerContext &C) const {
  // Check for dereference of an undefined value.
  if (l.isUndef()) {
    if (ExplodedNode *N = C.generateSink()) {
      if (!BT_undef)
        BT_undef.reset(new BuiltinBug("Dereference of undefined pointer value"));

      BugReport *report =
        new BugReport(*BT_undef, BT_undef->getDescription(), N);
      bugreporter::addTrackNullOrUndefValueVisitor(N,
                                                   bugreporter::GetDerefExpr(N),
                                                   report);
      report->disablePathPruning();
      C.EmitReport(report);
    }
    return;
  }

  DefinedOrUnknownSVal location = cast<DefinedOrUnknownSVal>(l);

  // Check for null dereferences.
  if (!isa<Loc>(location))
    return;

  ProgramStateRef state = C.getState();

  ProgramStateRef notNullState, nullState;
  llvm::tie(notNullState, nullState) = state->assume(location);

  // The explicit NULL case.
  if (nullState) {
    if (!notNullState) {
      reportBug(nullState, S, C);
      return;
    }

    // Otherwise, we have the case where the location could either be
    // null or not-null.  Record the error node as an "implicit" null
    // dereference.
    if (ExplodedNode *N = C.generateSink(nullState)) {
      ImplicitNullDerefEvent event = { l, isLoad, N, &C.getBugReporter() };
      dispatchEvent(event);
    }
  }

  // From this point forward, we know that the location is not null.
  C.addTransition(notNullState);
}

void DereferenceChecker::checkBind(SVal L, SVal V, const Stmt *S,
                                   CheckerContext &C) const {
  // If we're binding to a reference, check if the value is known to be null.
  if (V.isUndef())
    return;

  const MemRegion *MR = L.getAsRegion();
  const TypedValueRegion *TVR = dyn_cast_or_null<TypedValueRegion>(MR);
  if (!TVR)
    return;

  if (!TVR->getValueType()->isReferenceType())
    return;

  ProgramStateRef State = C.getState();

  ProgramStateRef StNonNull, StNull;
  llvm::tie(StNonNull, StNull) = State->assume(cast<DefinedOrUnknownSVal>(V));

  if (StNull) {
    if (!StNonNull) {
      reportBug(StNull, S, C, /*isBind=*/true);
      return;
    }

    // At this point the value could be either null or non-null.
    // Record this as an "implicit" null dereference.
    if (ExplodedNode *N = C.generateSink(StNull)) {
      ImplicitNullDerefEvent event = { V, /*isLoad=*/true, N,
                                       &C.getBugReporter() };
      dispatchEvent(event);
    }
  }

  // Unlike a regular null dereference, initializing a reference with a
  // dereferenced null pointer does not actually cause a runtime exception in
  // Clang's implementation of references.
  //
  //   int &r = *p; // safe??
  //   if (p != NULL) return; // uh-oh
  //   r = 5; // trap here
  //
  // The standard says this is invalid as soon as we try to create a "null
  // reference" (there is no such thing), but turning this into an assumption
  // that 'p' is never null will not match our actual runtime behavior.
  // So we do not record this assumption, allowing us to warn on the last line
  // of this example.
  //
  // We do need to add a transition because we may have generated a sink for
  // the "implicit" null dereference.
  C.addTransition(State, this);
}

void ento::registerDereferenceChecker(CheckerManager &mgr) {
  mgr.registerChecker<DereferenceChecker>();
}
