#ifndef SCOPE_H
#define SCOPE_H

#include <string>
#include <map>
#include "logwindow.h"
#include "nodes.h"

#include <SOIL/SOIL.h>

namespace Prototype {

class Scope {
    public:
        string name;
        LogWindow* logger;

        Scope(string name, LogWindow* logger);

        void clear();
        void declare(string name, string type, Expr_ptr value);
        bool assign(string name, Expr_ptr value);

        Expr_ptr get(string name);

    private:
        map<string, Expr_ptr> variables;
        map<string, string> types;
};

typedef shared_ptr<Scope> Scope_ptr;

}

#endif // SCOPE_H
