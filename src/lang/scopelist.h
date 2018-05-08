#ifndef SCOPELIST_H
#define SCOPELIST_H

#include <string>
#include <vector>
#include "logwindow.h"
#include "scope.h"
#include "nodes.h"

namespace Wyatt {

class ScopeList {
    public:
        typedef shared_ptr<ScopeList> ptr;

        string name;
        LogWindow* logger;
        string* workingDir;

        ScopeList(string name, LogWindow* logger, string* workingDir);

        Scope::ptr current();
        Scope::ptr attach(string name);
        void detach();
        Expr::ptr get(string name);
        bool assign(Stmt::ptr assign, Ident::ptr, Expr::ptr);

    private:
        vector<Scope::ptr> chain;
};

}

#endif // SCOPELIST_H
