#include "scope.h"

#include <stb_image.h>

namespace Prototype {
    Scope::Scope(string name, LogWindow* logger): name(name), logger(logger) { }

    void Scope::clear() {
        for(map<string, Expr_ptr>::iterator it = variables.begin(); it != variables.end(); ++it) {
            variables.erase(it);
        }
    }

    void Scope::declare(Stmt_ptr decl, Ident_ptr ident, string type, Expr_ptr value) {
        types[ident->name] = type;
        assign(decl, ident, (value != nullptr? value : null_expr));
    }

    bool Scope::assign(Stmt_ptr assign, Ident_ptr ident, Expr_ptr value) {
        string name = ident->name;

        if(types.find(name) == types.end()) {
            return false;
        }
        string value_type = type_to_name(value->type);

        if(types[name] == "float" && value_type == "int") {
            variables[name] = make_shared<Float>(static_pointer_cast<Int>(value)->value);
            return true;
        }
        if(types[name] == "int" && value_type == "float") {
            variables[name] = make_shared<Int>(int(static_pointer_cast<Float>(value)->value));
            return true;
        }
        if(types[name] == "texture2D") {
            if(value_type == "string") {
                Texture_ptr tex = make_shared<Texture>();
                string filename = static_pointer_cast<String>(value)->value;
                tex->image = stbi_load(filename.c_str(), &(tex->width), &(tex->height), &(tex->channels), 4);
                if(tex->image == 0) {
                    string message = "Cannot load " + filename + ": ";
                    message += stbi_failure_reason();
                    logger->log(assign, "ERROR", message);
                    return false;
                }
                variables[name] = tex;
                return true;
            } else
            if(value_type == "texture2D") {

            }
        }
        if(types[name] != "var" && types[name] != value_type) {
            logger->log(assign, "ERROR", "Cannot assign value of type " + value_type + " to variable of type " + types[name]);
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
