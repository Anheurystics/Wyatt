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

            	Scope(string name) {
                    this->name = name;
                }

                void clear() {
                    for(map<string, Expr_ptr>::iterator it = variables.begin(); it != variables.end(); ++it) {
                        variables.erase(it);
                    }
                }

                void declare(string name, string type, Expr_ptr value) {
                    types[name] = type;
                    variables[name] = null_expr;
                }

                void assign(string name, Expr_ptr value) {
                    //if(types.find(name) != types.end()) {
                        variables[name] = value;
                    //}
                }

                Expr_ptr get(string name) {
                    if(variables.find(name) != variables.end()) {
                        return variables[name];
                    }

                    return NULL;
                }

            private:
                map<string, Expr_ptr> variables;
                map<string, string> types;
        };

        typedef shared_ptr<Layout> Layout_ptr;
        typedef shared_ptr<Program> Program_ptr;
        typedef shared_ptr<Buffer> Buffer_ptr;
        typedef shared_ptr<Scope> Scope_ptr;

        shared_ptr<Scope> globalScope;
        std::stack<shared_ptr<Scope>> functionScopeStack;
        map<string, shared_ptr<Buffer>> buffers;
        map<string, shared_ptr<Program>> programs;
        map<string, shared_ptr<ShaderPair>> shaders;
        map<string, FuncDef_ptr> functions;

        map<string, FuncDef_ptr> builtins;

        string current_program_name;
        shared_ptr<Program> current_program = NULL;

        FuncDef_ptr init = NULL;
        FuncDef_ptr loop = NULL;
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
