#ifndef SCOPE_H
#define SCOPE_H

#include <string>
#include <map>
#include <set>
#include "logwindow.h"
#include "nodes.h"
#include "helper.h"

namespace Wyatt {

class Scope {
    public:
        typedef shared_ptr<Scope> ptr;

        string name;
        LogWindow* logger;
        string* workingDir;

        Scope(string name, LogWindow* logger, string* workingDir);

        void clear();
        void declare(Stmt::ptr decl, Ident::ptr ident, string type, Expr::ptr value);
        void declare_const(string type, Expr::ptr value);
        bool assign(Stmt::ptr assign, Ident::ptr ident, Expr::ptr value);
        void fast_assign(string name, Expr::ptr value);

        Expr::ptr get(string name);

    private:
        map<string, Expr::ptr> variables;
        map<string, string> types;
        set<string> constants;
};

}

#endif // SCOPE_H
