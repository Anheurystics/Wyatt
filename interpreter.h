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
        Expr* execute_stmts(Stmts*);
        void prepare();
        void execute_init();
        void execute_loop();
        void compile_program();
        void compile_shader(GLuint*, ShaderSource*);
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

            Layout* layout = NULL;
        };

        struct Program {
            GLuint handle;
            GLuint vert, frag;

            ShaderSource* vertSource;
            ShaderSource* fragSource;
        };

        class Scope {
            public:
           		string name;

            	Scope(string name) {
                    this->name = name;
                }

                void clear() {
                    for(map<string, Expr*>::iterator it = variables.begin(); it != variables.end(); ++it) {
                        variables.erase(it);
                        delete it->second;
                    }
                }

                void declare(string name, Expr* value) {
                    variables[name] = value;
                }

                Expr* get(string name) {
                    if(variables.find(name) != variables.end()) {
                        return variables[name];
                    }

                    return NULL;
                }

            private:
            	map<string, Expr*> variables;
        };

        Scope* globalScope;
        stack<Scope*> functionScopeStack;
        map<string, Buffer*> buffers;
        map<string, Program*> programs;
        map<string, ShaderPair*> shaders;
        map<string, FuncDef*> functions;

        map<string, FuncDef*> builtins;

        string current_program_name;
        Program* current_program = NULL;

        FuncDef* init = NULL;
        FuncDef* loop = NULL;
        QOpenGLFunctions* gl;

        Expr* eval_expr(Expr*);
        Expr* eval_binary(Binary*);
        Expr* invoke(Invoke*);
        Expr* eval_stmt(Stmt*);
        Expr* resolve_vector(vector<Expr*>);

        LogWindow* logger;
};

#endif //MYPARSER_H
