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
#include "scope.h"
#include "scopelist.h"

namespace Prototype {

class Interpreter {
    public:
        Interpreter(LogWindow*);

        int status = -1;

        unsigned int width, height;

        void parse(string);
        Expr_ptr execute_stmts(Stmts_ptr);
        void prepare();
        void load_imports();
        void load_import(string);
        void execute_init();
        void execute_loop();
        void compile_program();
        void compile_shader(GLuint*, Shader_ptr);
        void setFunctions(QOpenGLFunctions* gl) {
            this->gl = gl;
        }
        void reset();

    private:
        Prototype::Scanner scanner;
        Prototype::Parser parser;

        typedef shared_ptr<Layout> Layout_ptr;
        typedef shared_ptr<Scope> Scope_ptr;

        Scope_ptr globalScope;
        std::stack<ScopeList_ptr> functionScopeStack;
        map<string, shared_ptr<ShaderPair>> shaders;
        map<string, FuncDef_ptr> functions;
        map<string, FuncDef_ptr> builtins;

        vector<string> imports;
        vector<Decl_ptr> globals;

        int activeTextureSlot = 0;

        string current_program_name;
        Program_ptr current_program = nullptr;

        FuncDef_ptr init = nullptr;
        FuncDef_ptr loop = nullptr;
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
