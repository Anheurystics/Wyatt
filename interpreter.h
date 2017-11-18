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

#include "parser.h"
#include "scanner.h"
#include "logwindow.h"
#include "helper.h"

using namespace std;

class Interpreter {
    public:
        Interpreter(LogWindow*);

        int status = -1;

        void parse(string);
        shared_ptr<Expr> execute_stmts(shared_ptr<Stmts>);
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
                    for(map<string, shared_ptr<Expr>>::iterator it = variables.begin(); it != variables.end(); ++it) {
                        variables.erase(it);
                    }
                }

                void declare(string name, shared_ptr<Expr> value) {
                    variables[name] = value;
                }

                shared_ptr<Expr> get(string name) {
                    if(variables.find(name) != variables.end()) {
                        return variables[name];
                    }

                    return NULL;
                }

            private:
                map<string, shared_ptr<Expr>> variables;
        };

        shared_ptr<Scope> globalScope;
        stack<shared_ptr<Scope>> functionScopeStack;
        map<string, shared_ptr<Buffer>> buffers;
        map<string, shared_ptr<Program>> programs;
        map<string, shared_ptr<ShaderPair>> shaders;
        map<string, shared_ptr<FuncDef>> functions;

        map<string, shared_ptr<FuncDef>> builtins;

        string current_program_name;
        shared_ptr<Program> current_program = NULL;

        shared_ptr<FuncDef> init = NULL;
        shared_ptr<FuncDef> loop = NULL;
        QOpenGLFunctions* gl;

        shared_ptr<Expr> eval_expr(shared_ptr<Expr>);
        shared_ptr<Expr> eval_binary(shared_ptr<Binary>);
        shared_ptr<Expr> invoke(shared_ptr<Invoke>);
        shared_ptr<Expr> eval_stmt(shared_ptr<Stmt>);
        shared_ptr<Expr> resolve_vector(vector<shared_ptr<Expr>>);

        LogWindow* logger;
};

#endif //MYPARSER_H
