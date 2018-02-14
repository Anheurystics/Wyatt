#ifndef SCOPE_H
#define SCOPE_H

#include <string>
#include <map>
#include "logwindow.h"
#include "nodes.h"

namespace Prototype {

class Scope {
    public:
        string name;
        LogWindow* logger;

        Scope(string name, LogWindow* logger);

        void clear();
        void declare(Stmt_ptr decl, Ident_ptr ident, string type, Expr_ptr value);
        bool assign(Stmt_ptr assign, Ident_ptr ident, Expr_ptr value);

        Expr_ptr get(string name);

    private:
        map<string, Expr_ptr> variables;
        map<string, string> types;
};

typedef shared_ptr<Scope> Scope_ptr;

}

#endif // SCOPE_H
