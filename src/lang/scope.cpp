#include "scope.h"

#include <stb_image.h>

namespace Wyatt {
    Scope::Scope(string name, LogWindow* logger, string* workingDir): name(name), logger(logger), workingDir(workingDir) {}

    void Scope::clear() {
        variables.clear();
        types.clear();
        constants.clear();
    }

    void Scope::declare(Stmt::ptr decl, Ident::ptr ident, string type, Expr::ptr value) {
        types[ident->name] = type;
        assign(decl, ident, value == nullptr? null_expr : value);
        if(decl != nullptr && decl->type == NODE_DECL) {
            if(static_pointer_cast<Decl>(decl)->constant) {
                constants.insert(ident->name);
            }
        }
    }

    bool Scope::assign(Stmt::ptr assign, Ident::ptr ident, Expr::ptr value) {
        string name = ident->name;

        if(constants.find(name) != constants.end()) {
            logger->log(assign, "ERROR", "Cannot modify const value " + name);
            return true;
        }

        if(types.find(name) == types.end()) {
            return false;
        }

        NodeType value_type = value->type;
        string type_string = type_to_name(value_type);

        if(types[name] == "float" && type_string == "int") {
            variables[name] = make_shared<Float>(static_pointer_cast<Int>(value)->value);
            return true;
        }
        if(types[name] == "int" && type_string == "float") {
            variables[name] = make_shared<Int>(int(static_pointer_cast<Float>(value)->value));
            return true;
        }
        if(types[name] == "texture2D") {
            if(value_type == NODE_STRING) {
                Texture::ptr tex = make_shared<Texture>();
                string filename = static_pointer_cast<String>(value)->value;
                string realfilename = "";
                if(file_exists(*workingDir + "/" + filename)) {
                    realfilename = *workingDir + "/" + filename;
                } else {
                    realfilename = filename;
                }
                tex->image = stbi_load(realfilename.c_str(), &(tex->width), &(tex->height), &(tex->channels), 4);
                if(tex->image == 0) {
                    string message = "Cannot load " + filename + ": ";
                    message += stbi_failure_reason();
                    logger->log(assign, "ERROR", message);
                    return true;
                }
                variables[name] = tex;
                return true;
            } else
            if(value_type == NODE_TEXTURE) {
                variables[name] = value;
                return true;
            } else
            if(value_type == NODE_NULL) {
                return true;
            }
        }
        if(types[name] != "var" && types[name] != type_string) {
            logger->log(assign, "ERROR", "Cannot assign value of type " + type_string + " to variable of type " + types[name]);
            return true;
        }

        variables[name] = value;
        return true;
    }

    // to be used internally
    void Scope::fast_assign(string name, Expr::ptr value) {
        variables[name] = value;
    }

    Expr::ptr Scope::get(string name) {
        if(variables.find(name) != variables.end()) {
            return variables[name];
        }

        return nullptr;
    }
}
