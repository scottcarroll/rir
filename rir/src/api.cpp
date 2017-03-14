/** Enables the use of R internals for us so that we can manipulate R structures
 * in low level.
 */

#include <cassert>

#include "api.h"

#include "ir/Compiler.h"
#include "interpreter/interp_context.h"
#include "interpreter/interp.h"
#include "ir/BC.h"

#include "utils/FunctionHandle.h"

#include "optimizer/Printer.h"
#include "code/analysis.h"
#include "optimizer/cp.h"
#include "optimizer/Signature.h"

#include "ir/Optimizer.h"

#include "R/Sexp.h"
#include "tracing/Tracing.h"

using namespace rir;

REXPORT SEXP rir_disassemble(SEXP what) {

    Rprintf("%p\n", what);
    ::Function * f = TYPEOF(what) == CLOSXP ? isValidClosureSEXP(what) : isValidFunctionSEXP(what);

    if (f == nullptr)
        Rf_error("Not a rir compiled code");

    CodeEditor(what).print();
    return R_NilValue;
}

REXPORT SEXP rir_compile(SEXP what, SEXP env = NULL) {

    // TODO make this nicer
    if (TYPEOF(what) == CLOSXP) {
        SEXP body = BODY(what);
        if (TYPEOF(body) == BCODESXP) {
            R_PreserveObject(body);
            body = VECTOR_ELT(CDR(body), 0);
        }

        if (TYPEOF(body) == EXTERNALSXP)
            Rf_error("closure already compiled");

        SEXP result = allocSExp(CLOSXP);
        PROTECT(result);
        auto res = Compiler::compileClosure(body, FORMALS(what));
        SET_FORMALS(result, res.formals);
        SET_CLOENV(result, CLOENV(what));
        SET_BODY(result, res.bc);
        Rf_copyMostAttrib(what, result);
        UNPROTECT(1);
        return result;
    } else {
        if (TYPEOF(what) == BCODESXP) {
            what = VECTOR_ELT(CDR(what), 0);
        }
        auto res = Compiler::compileExpression(what);
        return res.bc;
    }
}

REXPORT SEXP rir_markOptimize(SEXP what) {
    if (TYPEOF(what) != CLOSXP)
        return R_NilValue;
    SEXP b = BODY(what);
    isValidFunctionSEXP(b);
    Function* fun = (Function*)INTEGER(b);
    fun->markOpt = true;
    return R_NilValue;
}

REXPORT SEXP rir_eval(SEXP what, SEXP env) {
    ::Function * f = isValidFunctionObject(what);
    if (f == nullptr)
        f = isValidClosureSEXP(what);
    if (f == nullptr)
        Rf_error("Not rir compiled code");
    return evalRirCode(functionCode(f), globalContext(), env, 0);
}

REXPORT SEXP rir_body(SEXP cls) {
    ::Function * f = isValidClosureSEXP(cls);
    if (f == nullptr)
        Rf_error("Not a valid rir compiled function");
    return functionSEXP(f);
}

REXPORT SEXP rir_analysis_signature(SEXP what) {
    if (TYPEOF(what) != CLOSXP)
        return R_NilValue;
    CodeEditor ce(what);
    SignatureAnalysis sa;
    sa.analyze(ce);
    return sa.finalState().exportToR();
}

REXPORT SEXP rir_trace(SEXP what, SEXP f) {
    if (TYPEOF(what) != STRSXP) {
        Rf_warning("'what' should be a string");
        return R_NilValue;
    }

    if (TYPEOF(f) != CLOSXP) {
        Rf_warning("'f' should be a function of some kind");
        return R_NilValue;
    }

    SEXP charSxp = STRING_ELT(what, 0);
    std::string s = CHAR(charSxp);

    Tracing& tracing = Tracing::instance();

    if (s.compare("call") == 0) {
        tracing.addTracer(Tracing::Type::RIR_TRACE_CALL, f);
    } else if (s.compare("promise_eval") == 0) {
        tracing.addTracer(Tracing::Type::RIR_TRACE_PROMISE_EVAL, f);
    } else {
        Rf_warning("unknown 'what'");
    }

    return R_NilValue;
}

// startup ---------------------------------------------------------------------

bool startup() {
    initializeRuntime(rir_compile, Optimizer::reoptimizeFunction);
    return true;
}

bool startup_ok = startup();
