#include "scopelist.h"

namespace Wyatt {

ScopeList::ScopeList(string name, LogWindow* logger, string* workingDir): name(name), logger(logger), workingDir(workingDir)
{
    attach("base");
}

Scope::ptr ScopeList::current() {
    return chain.back();
}

Scope::ptr ScopeList::attach(string name) {
    Scope::ptr newScope = make_shared<Scope>(name, logger, workingDir);
    chain.push_back(newScope);
    return newScope;
}

void ScopeList::detach() {
    chain.pop_back();
}

Expr::ptr ScopeList::get(string name) {
    for(vector<Scope::ptr>::reverse_iterator it = chain.rbegin(); it != chain.rend(); ++it) {
        Scope::ptr scope = *it;
        Expr::ptr value = scope->get(name);
        if(value != nullptr) {
            return value;
        }
    }
    return nullptr;
}

bool ScopeList::assign(Stmt::ptr assign, Ident::ptr ident, Expr::ptr value) {
    for(vector<Scope::ptr>::reverse_iterator it = chain.rbegin(); it != chain.rend(); ++it) {
        Scope::ptr scope = *it;
        if(scope->assign(assign, ident, value == nullptr? null_expr : value)) {
            return true;
        }
    }
    return false;
}

}
