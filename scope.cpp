#include "scope.h"

namespace Prototype {
    Scope::Scope(string name, LogWindow* logger): name(name), logger(logger) { }

    void Scope::clear() {
        for(map<string, Expr_ptr>::iterator it = variables.begin(); it != variables.end(); ++it) {
            variables.erase(it);
        }
    }

    void Scope::declare(string name, string type, Expr_ptr value) {
        types[name] = type;
        variables[name] = (value != NULL? value : null_expr);
    }

    bool Scope::assign(string name, Expr_ptr value) {
        if(types.find(name) == types.end()) {
            return false;
        }
        if(types[name] != "var" && types[name] != type_to_name(value->type)) {
            logger->log(value, "ERROR", "Cannot assign value of type " + type_to_name(value->type) + " to variable of type " + types[name]);
            return true;
        }

        variables[name] = value;
        return true;
    }

    Expr_ptr Scope::get(string name) {
        if(variables.find(name) != variables.end()) {
            return variables[name];
        }

        return nullptr;
    }
}
