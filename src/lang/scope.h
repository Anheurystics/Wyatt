#ifndef SCOPE_H
#define SCOPE_H

#include <string>
#include <map>
#include <set>
#include "logwindow.h"
#include "nodes.h"
#include "helper.h"

namespace Wyatt
{

class Scope
{
  public:
    string name;
    LogWindow *logger;
    string *workingDir;

    Scope(string name, LogWindow *logger, string *workingDir);

    void clear();
    void declare(Stmt_ptr decl, Ident_ptr ident, string type, Expr_ptr value);
    void declare_const(string type, Expr_ptr value);
    bool assign(Stmt_ptr assign, Ident_ptr ident, Expr_ptr value);
    void fast_assign(string name, Expr_ptr value);

    Expr_ptr get(string name);

  private:
    map<string, Expr_ptr> variables;
    map<string, string> types;
    set<string> constants;
};

typedef shared_ptr<Scope> Scope_ptr;
}

#endif // SCOPE_H
