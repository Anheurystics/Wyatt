#ifndef MYPARSER_H
#define MYPARSER_H

#include <cstdio>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <map>
#include <stack>
#include <vector>
#include <string>
#include <ctime>
#include <regex>
#include <memory>

#include <QOpenGLFunctions>

#include "scanner.h"
#include "parser.hpp"
#include "logwindow.h"
#include "helper.h"
#include "glsltranspiler.h"

namespace Prototype {

struct Layout {
    map<string, unsigned int> attributes;
    vector<string> list;
};

struct Buffer {
    GLuint handle;
    map<string, vector<float>> data;
    map<string, unsigned int> sizes;

    shared_ptr<Layout> layout = NULL;
};

struct Program {
    GLuint handle;
    GLuint vert, frag;

    shared_ptr<ShaderSource> vertSource;
    shared_ptr<ShaderSource> fragSource;
};

class Scope {
    public:
        string name;
        LogWindow* logger;

        Scope(string name, LogWindow* logger): name(name), logger(logger) {}

        void clear() {
            for(map<string, Expr_ptr>::iterator it = variables.begin(); it != variables.end(); ++it) {
                variables.erase(it);
            }
        }

        void declare(string name, string type, Expr_ptr value) {
            types[name] = type;
            variables[name] = (value != NULL? value : null_expr);
        }

        bool assign(string name, Expr_ptr value) {
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

        Expr_ptr get(string name) {
            if(variables.find(name) != variables.end()) {
                return variables[name];
            }

            return nullptr;
        }

    private:
        map<string, Expr_ptr> variables;
        map<string, string> types;
};

typedef shared_ptr<Scope> Scope_ptr;

class ScopeList {
    public:
        string name;
        ScopeList(string name, LogWindow* logger): name(name), logger(logger) {
            attach("base");
        }

        Scope_ptr current() {
            return chain.back();
        }

        Scope_ptr attach(string name) {
            Scope_ptr newScope = make_shared<Scope>(name, logger);
            chain.push_back(newScope);
            return newScope;
        }

        void detach() {
            chain.pop_back();
        }

        Expr_ptr get(string name) {
            for(vector<Scope_ptr>::reverse_iterator it = chain.rbegin(); it != chain.rend(); ++it) {
                Scope_ptr scope = *it;
                Expr_ptr value = scope->get(name);
                if(value != nullptr) {
                    return value;
                }
            }
            return nullptr;
       }

        void assign(string name, Expr_ptr value) {
            for(vector<Scope_ptr>::reverse_iterator it = chain.rbegin(); it != chain.rend(); ++it) {
                Scope_ptr scope = *it;
                if(scope->assign(name, value)) {
                    return;
                }
            }
            logger->log(value, "ERROR", "Variable " + name + " does not exist!");
        }

    private:
        vector<Scope_ptr> chain;
        LogWindow* logger;
};

typedef shared_ptr<ScopeList> ScopeList_ptr;

class Interpreter {
    public:
        Interpreter(LogWindow*);

        int status = -1;

        void parse(string);
        Expr_ptr execute_stmts(shared_ptr<Stmts>);
        void prepare();
        void execute_init();
        void execute_loop();
        void compile_program();
        void compile_shader(GLuint*, shared_ptr<ShaderSource>);
        void setFunctions(QOpenGLFunctions* gl) {
            this->gl = gl;
        }
        void reset();

    private:
        Prototype::Scanner scanner;
        Prototype::Parser parser;

        typedef shared_ptr<Layout> Layout_ptr;
        typedef shared_ptr<Program> Program_ptr;
        typedef shared_ptr<Buffer> Buffer_ptr;
        typedef shared_ptr<Scope> Scope_ptr;

        Scope_ptr globalScope;
        std::stack<ScopeList_ptr> functionScopeStack;
        map<string, shared_ptr<Buffer>> buffers;
        map<string, shared_ptr<Program>> programs;
        map<string, shared_ptr<ShaderPair>> shaders;
        map<string, FuncDef_ptr> functions;

        map<string, FuncDef_ptr> builtins;

        string current_program_name;
        shared_ptr<Program> current_program = NULL;

        FuncDef_ptr init = NULL;
        FuncDef_ptr loop = NULL;
        Invoke_ptr init_invoke;
        Invoke_ptr loop_invoke;
        QOpenGLFunctions* gl;

        Expr_ptr eval_expr(Expr_ptr);
        Expr_ptr eval_binary(shared_ptr<Binary>);
        Expr_ptr invoke(shared_ptr<Invoke>);
        Expr_ptr eval_stmt(shared_ptr<Stmt>);
        Expr_ptr resolve_vector(vector<Expr_ptr>);

        LogWindow* logger;

        GLSLTranspiler* transpiler;

        unsigned int line = 1;
        unsigned int column = 1;
};
}

#endif //MYPARSER_H
