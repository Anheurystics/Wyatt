#include "scopelist.h"

namespace Wyatt
{

ScopeList::ScopeList(string name, LogWindow *logger, string *workingDir) : name(name), logger(logger), workingDir(workingDir)
{
    attach("base");
}

Scope_ptr ScopeList::current()
{
    return chain.back();
}

Scope_ptr ScopeList::attach(string name)
{
    Scope_ptr newScope = make_shared<Scope>(name, logger, workingDir);
    chain.push_back(newScope);
    return newScope;
}

void ScopeList::detach()
{
    chain.pop_back();
}

Expr_ptr ScopeList::get(string name)
{
    for (vector<Scope_ptr>::reverse_iterator it = chain.rbegin(); it != chain.rend(); ++it)
    {
        Scope_ptr scope = *it;
        Expr_ptr value = scope->get(name);
        if (value != nullptr)
        {
            return value;
        }
    }
    return nullptr;
}

bool ScopeList::assign(Stmt_ptr assign, Ident_ptr ident, Expr_ptr value)
{
    for (vector<Scope_ptr>::reverse_iterator it = chain.rbegin(); it != chain.rend(); ++it)
    {
        Scope_ptr scope = *it;
        if (scope->assign(assign, ident, value == nullptr ? null_expr : value))
        {
            return true;
        }
    }
    return false;
}
}
