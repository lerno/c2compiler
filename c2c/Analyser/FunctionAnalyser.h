/* Copyright 2013-2018 Bas van den Berg
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANALYSER_FUNCTION_ANALYSER_H
#define ANALYSER_FUNCTION_ANALYSER_H

#include <string>
#include <vector>
#include <llvm/ADT/APSInt.h>
#include "Clang/SourceLocation.h"
#include "AST/Type.h"
#include "Analyser/ExprTypeAnalyser.h"

namespace c2lang {
class DiagnosticsEngine;
class DiagnosticBuilder;
}

namespace C2 {

class TypeResolver;
class Scope;
class Decl;
class VarDecl;
class FunctionDecl;
class EnumConstantDecl;
class LabelDecl;
class BreakStmt;
class ContinueStmt;
class Stmt;
class LabelStmt;
class SwitchStmt;
class DeferStmt;
class Expr;
class IdentifierExpr;
class InitListExpr;
class MemberExpr;
class CallExpr;
class BuiltinExpr;
class ASTContext;
class GotoStmt;
class ReturnStmt;

constexpr size_t MAX_STRUCT_INDIRECTION_DEPTH = 256;
const size_t MAX_DEFERS = 256;
class FunctionAnalyser {
public:
    FunctionAnalyser(Scope& scope_,
                    TypeResolver& typeRes_,
                    ASTContext& context_,
                    c2lang::DiagnosticsEngine& Diags_,
                    bool isInterface_);

    void check(FunctionDecl* F);
    void checkVarInit(VarDecl* V);
    void checkArraySizeExpr(VarDecl* V);
    unsigned checkEnumValue(EnumConstantDecl* E, llvm::APSInt& nextValue);
private:
    void checkFunction(FunctionDecl* F);

    void analyseStmt(Stmt* stmt, bool haveScope = false);
    void analyseCompoundStmt(Stmt* stmt);
    void analyseIfStmt(Stmt* stmt);
    void analyseWhileStmt(Stmt* stmt);
    void analyseDoStmt(Stmt* stmt);
    void analyseForStmt(Stmt* stmt);
    void analyseSwitchStmt(Stmt* stmt);
    void analyseBreakStmt(Stmt* S);
    void analyseContinueStmt(Stmt* S);
    void analyseLabelStmt(Stmt* S);
    void analyseGotoStmt(Stmt* S);
    void analyseCaseStmt(Stmt* stmt);
    void analyseDefaultStmt(Stmt* stmt);
    void analyseReturnStmt(Stmt* stmt);
    void analyseDeclStmt(Stmt* stmt);
    void analyseAsmStmt(Stmt* stmt);
    void analyseDeferStmt(Stmt* stmt);
    bool analyseCondition(Stmt* stmt);
    void analyseStmtExpr(Stmt* stmt);

    QualType analyseExpr(Expr* expr, unsigned side);
    QualType analyseIntegerLiteral(Expr* expr);
    QualType analyseBinaryOperator(Expr* expr, unsigned side);
    QualType analyseConditionalOperator(Expr* expr);
    QualType analyseUnaryOperator(Expr* expr, unsigned side);
    QualType analyseBuiltinExpr(Expr* expr);
    QualType analyseArraySubscript(Expr* expr, unsigned side);
    QualType analyseMemberExpr(Expr* expr, unsigned side);
    QualType analyseStructMember(QualType T, MemberExpr* M, unsigned side, bool isStatic);
    bool checkStructTypeArg(QualType T,  FunctionDecl* func) const;
    Decl* getStructDecl(QualType T) const;
    bool exprIsType(const Expr* E) const;
    QualType analyseParenExpr(Expr* expr);
    bool analyseBitOffsetIndex(Expr* expr, llvm::APSInt* Result, BuiltinType* BaseType);
    QualType analyseBitOffsetExpr(Expr* expr, QualType BaseType, c2lang::SourceLocation base);
    QualType analyseExplicitCastExpr(Expr* expr);
    QualType analyseCall(Expr* expr);
    bool checkCallArgs(FunctionDecl* func, CallExpr* call, Expr *structFunction);
    Decl* analyseIdentifier(IdentifierExpr* expr);

    void analyseInitExpr(Expr* expr, QualType expectedType);
    void analyseInitList(InitListExpr* expr, QualType expectedType);
    void analyseDesignatorInitExpr(Expr* expr, QualType expectedType);
    void analyseSizeOfExpr(BuiltinExpr* expr);
    QualType analyseElemsOfExpr(BuiltinExpr* B);
    QualType analyseEnumMinMaxExpr(BuiltinExpr* B, bool isMin);
    void analyseArrayType(VarDecl* V, QualType T);
    void analyseArraySizeExpr(ArrayType* AT);
    DeferStmt** deferVectorToArray();

    c2lang::DiagnosticBuilder Diag(c2lang::SourceLocation Loc, unsigned DiagID) const;
    void pushMode(unsigned DiagID);
    void popMode();

    class ConstModeSetter {
    public:
        ConstModeSetter (FunctionAnalyser& analyser_, unsigned DiagID)
            : analyser(analyser_)
        {
            analyser.pushMode(DiagID);
        }
        ~ConstModeSetter()
        {
            analyser.popMode();
        }
    private:
        FunctionAnalyser& analyser;
    };

    LabelDecl* LookupOrCreateLabel(const char* name, c2lang::SourceLocation loc);

    bool checkAssignee(Expr* expr) const;
    void checkAssignment(Expr* assignee, QualType TLeft);
    void checkDeclAssignment(Decl* decl, Expr* expr);
    void checkArrayDesignators(InitListExpr* expr, int64_t* size);
    void checkEnumCases(const SwitchStmt* SS, const EnumType* ET) const;
    QualType getStructType(QualType T) const;
    QualType getConditionType(const Stmt* C) const;

    void outputStructDiagnostics(QualType T, IdentifierExpr *member, unsigned msg);
    QualType analyseStaticStructFunction(QualType T, MemberExpr *M, const StructTypeDecl *S, unsigned side);

    // conversions
    QualType UsualUnaryConversions(Expr* expr) const;

    Scope& scope;
    TypeResolver& TR;
    ASTContext& Context;
    ExprTypeAnalyser EA;

    c2lang::DiagnosticsEngine& Diags;

    FunctionDecl* CurrentFunction;
    VarDecl* CurrentVarDecl;
    unsigned constDiagID;
    bool inConstExpr;
    bool usedPublicly;
    bool isInterface;

    unsigned deferId;

    // Our callstack (statically allocated)
    struct CallStack {
        Expr *structFunction[MAX_STRUCT_INDIRECTION_DEPTH];
        unsigned callDepth;
        void push() { structFunction[callDepth++] = 0; }
        Expr *pop() { return structFunction[--callDepth]; }
        bool reachedMax() { return callDepth == MAX_STRUCT_INDIRECTION_DEPTH; };
        void setStructFunction(Expr* expr) { structFunction[callDepth - 1] = expr; }
    };


    std::vector<unsigned> conditionalDefers {};

    CallStack callStack;

    enum class DeferWalkEntry {
        DEFER,
        GOTO,
        LABEL,
        BREAK,
        CONTINUE,
        RETURN,
        BREAK_START,
        BREAK_CONTINUE_START,
        BREAK_END,
        BREAK_CONTINUE_END,
        EXIT_DEFER,
    };
    struct DeferWalk {
        DeferWalkEntry type;
        unsigned depth;
        union {
            ReturnStmt* returnStmt;
            ContinueStmt* continueStmt;
            BreakStmt* breakStmt;
            DeferStmt* deferStmt;
            GotoStmt* gotoStmt;
            LabelDecl* labelDecl;
            Stmt* entryPoointStmt;
        } as;
        unsigned marked;
    };

    std::vector<DeferWalk> deferWalk {};
    unsigned deferDepth = 0;

    void pushDeferEntry(DeferWalk &&walk) {
        walk.depth = deferDepth;
        switch(walk.type) {
            case DeferWalkEntry::DEFER:
                ++deferDepth;
                break;
            case DeferWalkEntry::EXIT_DEFER:
                --deferDepth;
                break;
            case DeferWalkEntry::CONTINUE:
            case DeferWalkEntry::BREAK:
            case DeferWalkEntry::LABEL:
            case DeferWalkEntry::GOTO:
            case DeferWalkEntry::BREAK_CONTINUE_START:
            case DeferWalkEntry::BREAK_START:
            case DeferWalkEntry::BREAK_CONTINUE_END:
            case DeferWalkEntry::BREAK_END:
            case DeferWalkEntry::RETURN:
                break;
        }
        deferWalk.push_back(std::move(walk));
    }


    typedef std::vector<LabelDecl*> Labels;
    typedef Labels::iterator LabelsIter;
    Labels labels {};
    std::vector<DeferStmt *> defersFound {};

    FunctionAnalyser(const FunctionAnalyser&);
    FunctionAnalyser& operator= (const FunctionAnalyser&);
    void analyseDeferWalk();
    void analyseDeferGoto(unsigned int i);
    void analyseDeferBreak(size_t i);
    void analyseDeferContinue(size_t i);
    void analyseDeferReturn(size_t i);
};

}

#endif

