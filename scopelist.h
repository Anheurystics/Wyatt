#ifndef SCOPELIST_H
#define SCOPELIST_H

#include <string>
#include <vector>
#include "logwindow.h"
#include "scope.h"
#include "nodes.h"

namespace Prototype {

class ScopeList {
    public:
        string name;
        string* working_dir;

        ScopeList(string name, LogWindow* logger, string* working_dir);

        Scope_ptr current();
        Scope_ptr attach(string name);
        void detach();
        Expr_ptr get(string name);
        bool assign(Stmt_ptr assign, Ident_ptr, Expr_ptr);

    private:
        vector<Scope_ptr> chain;
        LogWindow* logger;
};

typedef shared_ptr<ScopeList> ScopeList_ptr;

}

#endif // SCOPELIST_H
