#include "Symbols.h"

namespace rir {

namespace symbol {
#define DECLARE(name, txt) SEXP name = Rf_install(txt)

DECLARE(SuperAssignBracket, "[<<-");
DECLARE(SuperAssignDoubleBracket, "[[<<-");
DECLARE(AssignBracket, "[<-");
DECLARE(AssignDoubleBracket, "[[<-");
DECLARE(DoubleBracket, "[[");
DECLARE(Bracket, "[");
DECLARE(Block, "{");
DECLARE(Parenthesis, "(");
DECLARE(Assign, "<-");
DECLARE(Assign2, "=");
DECLARE(SuperAssign, "<<-");
DECLARE(If, "if");
DECLARE(Function, "function");
DECLARE(Return, "return");
DECLARE(For, "for");
DECLARE(While, "while");
DECLARE(Repeat, "repeat");
DECLARE(Break, "break");
DECLARE(Next, "next");
DECLARE(Switch, "switch");
DECLARE(Add, "+");
DECLARE(Sub, "-");
DECLARE(Mul, "*");
DECLARE(Div, "/");
DECLARE(Pow, "^");
DECLARE(Idiv, "%/%");
DECLARE(Mod, "%%");
DECLARE(Sqrt, "sqrt");
DECLARE(Exp, "exp");
DECLARE(Eq, "==");
DECLARE(Ne, "!=");
DECLARE(Lt, "<");
DECLARE(Le, "<=");
DECLARE(Ge, ">=");
DECLARE(Gt, ">");
DECLARE(BitAnd, "&");
DECLARE(BitOr, "|");
DECLARE(Not, "!");
DECLARE(Ellipsis, "...");
DECLARE(Colon, ":");
DECLARE(Internal, ".Internal");
DECLARE(tmp, "*tmp*");
DECLARE(vtmp, "*vtmp*");
DECLARE(value, "value");
DECLARE(isnull, "is.null");
DECLARE(islist, "is.list");
DECLARE(ispairlist, "is.pairlist");
DECLARE(quote, "quote");
DECLARE(And, "&&");
DECLARE(Or, "||");
DECLARE(Missing, "missing");
DECLARE(seq, "seq");
DECLARE(lapply, "lapply");
DECLARE(aslist, "as.list");
DECLARE(isvector, "is.vector");
DECLARE(substr, "substr");
DECLARE(Class, "class");
DECLARE(OldClass, "oldClass");
DECLARE(at, "@");
DECLARE(names, "names");
DECLARE(attr, "attr");
DECLARE(body, "body");
DECLARE(slot, "slot");
DECLARE(as, "as");
DECLARE(packageSlot, "packageSlot");
DECLARE(attributes, "attributes");
DECLARE(c, "c");
DECLARE(standardGeneric, "standardGeneric");
DECLARE(debugBreak, ".debug.break");
DECLARE(UseMethod, "UseMethod");
DECLARE(printInvocation, ".printInvocation");

#undef DECLARE
} // namespace symbol

} // namespace rjit
