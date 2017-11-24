#include "scopelist.h"

namespace Prototype {

ScopeList::ScopeList(string name, LogWindow* logger): name(name), logger(logger)
{
    attach("base");
}

Scope_ptr ScopeList::current() {
    return chain.back();
}

Scope_ptr ScopeList::attach(string name) {
    Scope_ptr newScope = make_shared<Scope>(name, logger);
    chain.push_back(newScope);
    return newScope;
}

void ScopeList::detach() {
    chain.pop_back();
}

Expr_ptr ScopeList::get(string name) {
    for(vector<Scope_ptr>::reverse_iterator it = chain.rbegin(); it != chain.rend(); ++it) {
        Scope_ptr scope = *it;
        Expr_ptr value = scope->get(name);
        if(value != nullptr) {
            return value;
        }
    }
    return nullptr;
}

void ScopeList::assign(string name, Expr_ptr value) {
    for(vector<Scope_ptr>::reverse_iterator it = chain.rbegin(); it != chain.rend(); ++it) {
        Scope_ptr scope = *it;
        if(scope->assign(name, value)) {
            return;
        }
    }
    logger->log(value, "ERROR", "Variable " + name + " does not exist!");
}

}
