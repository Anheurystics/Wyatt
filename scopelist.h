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

        ScopeList(string name, LogWindow* logger);

        Scope_ptr current();
        Scope_ptr attach(string name);
        void detach();
        Expr_ptr get(string name);
        void assign(string name, Expr_ptr value);

    private:
        vector<Scope_ptr> chain;
        LogWindow* logger;
};

typedef shared_ptr<ScopeList> ScopeList_ptr;

}

#endif // SCOPELIST_H
