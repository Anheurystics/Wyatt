#include "myparser.h"

#define resolve_int(n) ((Int*)n)->value
#define resolve_float(n) ((Float*)n)->value
#define resolve_scalar(n) ((n->type == NODE_INT)? (float)(resolve_int(n)) : resolve_float(n))
#define resolve_vec3(v) resolve_scalar(v->x), resolve_scalar(v->y), resolve_scalar(v->z)

#define LOOP_TIMEOUT 5

MyParser::MyParser() {

}

Expr* MyParser::eval_binary(Binary* bin) {
    Expr* lhs = eval_expr(bin->lhs);
    if(!lhs) return 0;

    OpType op = bin->op;

    Expr* rhs = eval_expr(bin->rhs);
    if(!rhs) return 0;

    NodeType ltype = lhs->type;
    NodeType rtype = rhs->type;

    if(ltype == NODE_BOOL && rtype == NODE_BOOL) {
        bool a = ((Bool*)lhs)->value;
        bool b = ((Bool*)rhs)->value;

        switch(op) {
            case OP_AND: return new Bool(a && b);
            case OP_OR: return new Bool(a || b);
            default: return 0;
        }
    }

    if(ltype == NODE_INT && rtype == NODE_INT) {
        int a = resolve_int(lhs);
        int b = resolve_int(rhs);

        switch(op) {
            case OP_PLUS: return new Int(a + b);
            case OP_MINUS: return new Int(a - b);
            case OP_MULT: return new Int(a * b);
            case OP_DIV: return new Float(a / (float)b);
            case OP_MOD: return new Int(a % b);
            case OP_EQUAL: return new Bool(a == b);
            case OP_LESSTHAN: return new Bool(a < b);
            case OP_GREATERTHAN: return new Bool(a > b);
            case OP_NEQUAL: return new Bool(a != b);
            case OP_LEQUAL: return new Bool(a <= b);
            case OP_GEQUAL: return new Bool(a >= b);
            default: return 0;
        }
    }

    if(ltype == NODE_FLOAT && rtype == NODE_FLOAT) {
        float a = resolve_float(lhs);
        float b = resolve_float(rhs);

        switch(op) {
            case OP_PLUS: return new Float(a + b);
            case OP_MINUS: return new Float(a - b);
            case OP_MULT: return new Float(a * b);
            case OP_DIV: return new Float(a / b);
            case OP_EQUAL: return new Bool(a == b);
            case OP_LESSTHAN: return new Bool(a < b);
            case OP_GREATERTHAN: return new Bool(a > b);
            case OP_NEQUAL: return new Bool(a != b);
            case OP_LEQUAL: return new Bool(a <= b);
            case OP_GEQUAL: return new Bool(a >= b);
            default: return 0;
        }
    }

    if((ltype == NODE_FLOAT && rtype == NODE_INT) || (ltype == NODE_INT && rtype == NODE_FLOAT)) {
        float a = resolve_scalar(lhs);
        float b = resolve_scalar(rhs);

        switch(op) {
            case OP_PLUS: return new Float(a + b);
            case OP_MINUS: return new Float(a - b);
            case OP_MULT: return new Float(a * b);
            case OP_DIV: return new Float(a / b);
            case OP_EQUAL: return new Bool(a == b);
            case OP_LESSTHAN: return new Bool(a < b);
            case OP_GREATERTHAN: return new Bool(a > b);
            case OP_NEQUAL: return new Bool(a != b);
            case OP_LEQUAL: return new Bool(a <= b);
            case OP_GEQUAL: return new Bool(a >= b);
            default: return 0;
        }
    }

    if(ltype == NODE_VECTOR3 && rtype == NODE_VECTOR3) {
        Vector3* a = (Vector3*)eval_expr(lhs);
        Vector3* b = (Vector3*)eval_expr(rhs);

        float ax = resolve_scalar(a->x); 
        float ay = resolve_scalar(a->y);
        float az = resolve_scalar(a->z);
        float bx = resolve_scalar(b->x);
        float by = resolve_scalar(b->y);
        float bz = resolve_scalar(b->z);

        switch(op) {
            case OP_PLUS: return new Vector3(new Float(ax+bx), new Float(ay+by), new Float(az+bz));
            case OP_MINUS: return new Vector3(new Float(ax-bx), new Float(ay-by), new Float(az-bz));
            case OP_MULT: return new Float(ax*bx + ay*by + az*bz);
            case OP_MOD: return new Vector3(new Float(ay*bz-az*by), new Float(az*bx-ax*bz), new Float(ax*by-ay*bx));
            default: return 0;
        }
    }

    if(ltype == NODE_VECTOR3 && (rtype == NODE_INT || rtype == NODE_FLOAT)) {
        Vector3* a = (Vector3*)eval_expr(lhs);
        float ax = resolve_scalar(a->x), ay = resolve_scalar(a->y), az = resolve_scalar(a->z);
        float b = resolve_scalar(rhs);

        switch(op) {
            case OP_MULT: return new Vector3(new Float(ax*b), new Float(ay*b), new Float(az*b));
            case OP_DIV: return new Vector3(new Float(ax/b), new Float(ay/b), new Float(az/b));
            default: return 0;
        }
    }

    if((ltype == NODE_INT || ltype == NODE_FLOAT) && rtype == NODE_VECTOR3) {
        float a = resolve_scalar(lhs);
        Vector3* b = (Vector3*)eval_expr(rhs);
        float bx = resolve_scalar(b->x), by = resolve_scalar(b->y), bz = resolve_scalar(b->z);

        switch(op) {
            case OP_MULT: return new Vector3(new Float(bx*a), new Float(by*a), new Float(bz*a));
            case OP_DIV: return new Vector3(new Float(bx/a), new Float(by/a), new Float(bz/a));
            default: return 0;
        }
    }

    if(ltype == NODE_MATRIX3 && (rtype == NODE_INT || rtype == NODE_FLOAT)) {
        Matrix3* a = (Matrix3*)eval_expr(lhs);

        if(op != OP_MULT && op != OP_DIV) return 0;

        Vector3* v0 = (Vector3*)eval_binary(new Binary(a->v0, op, rhs));
        Vector3* v1 = (Vector3*)eval_binary(new Binary(a->v1, op, rhs));
        Vector3* v2 = (Vector3*)eval_binary(new Binary(a->v2, op, rhs));

        return new Matrix3(v0, v1, v2);
    }

    if((ltype == NODE_INT || ltype == NODE_FLOAT) && (rtype == NODE_MATRIX3)) {
        Matrix3* a = (Matrix3*)eval_expr(rhs);

        if(op != OP_MULT && op != OP_DIV) return 0;

        Vector3* v0 = (Vector3*)eval_binary(new Binary(a->v0, op, lhs));
        Vector3* v1 = (Vector3*)eval_binary(new Binary(a->v1, op, lhs));
        Vector3* v2 = (Vector3*)eval_binary(new Binary(a->v2, op, lhs));

        return new Matrix3(v0, v1, v2);
    }

    if(ltype == NODE_MATRIX3 && rtype == NODE_MATRIX3) {
        Matrix3* a = (Matrix3*)eval_expr(lhs);
        Matrix3* b = (Matrix3*)eval_expr(rhs);
        
        if(op != OP_MULT) return 0;

        Vector3* r0 = new Vector3(new Binary(a->v0, OP_MULT, b->c0), new Binary(a->v0, OP_MULT, b->c1), new Binary(a->v0, OP_MULT, b->c2));
        Vector3* r1 = new Vector3(new Binary(a->v1, OP_MULT, b->c0), new Binary(a->v1, OP_MULT, b->c1), new Binary(a->v1, OP_MULT, b->c2));
        Vector3* r2 = new Vector3(new Binary(a->v2, OP_MULT, b->c0), new Binary(a->v2, OP_MULT, b->c1), new Binary(a->v2, OP_MULT, b->c2));

        return eval_expr(new Matrix3(r0, r1, r2));
    }

    if(ltype == NODE_VECTOR3 && rtype == NODE_MATRIX3) {
        Vector3* a = (Vector3*)eval_expr(lhs);
        Matrix3* b = (Matrix3*)eval_expr(rhs);

        if(op != OP_MULT) return 0;

        return eval_expr(new Vector3(new Binary(a, OP_MULT, b->c0), new Binary(a, OP_MULT, b->c1), new Binary(a, OP_MULT, b->c2)));
    }

    return 0;
}

Expr* MyParser::eval_expr(Expr* node) {
    switch(node->type) {
        case NODE_IDENT: 
            {
                Ident* ident = (Ident*)node;
                if(variables.find(ident->name) == variables.end()) {
                    std::cout << "ERROR: Variable " << ident->name << " does not exist!\n";
                    return 0;
                } else {
                    return variables[ident->name];
                }
            }

        case NODE_BOOL:
            return node;

        case NODE_INT:
            return node;

        case NODE_FLOAT:
            return node;

        case NODE_VECTOR3:
            {
                Vector3* vec3 = (Vector3*)(node);
                Expr* x = eval_expr(vec3->x);
                Expr* y = eval_expr(vec3->y);
                Expr* z = eval_expr(vec3->z);
                
                if((x->type == NODE_INT || x->type == NODE_FLOAT) && (y->type == NODE_INT || y->type == NODE_FLOAT) && (z->type == NODE_INT || z->type == NODE_FLOAT)) {
                    vec3->x = x; vec3->y = y; vec3->z = z;
                    return vec3;
                }

                if(x->type == NODE_VECTOR3 && y->type == NODE_VECTOR3 && z->type == NODE_VECTOR3) {
                    return new Matrix3((Vector3*)x, (Vector3*)y, (Vector3*)z);
                }

                return 0;
            }

        case NODE_MATRIX3:
            {
                Matrix3* mat3 = (Matrix3*)node;
                mat3->v0 = (Vector3*)eval_expr(mat3->v0);
                mat3->v1 = (Vector3*)eval_expr(mat3->v1);
                mat3->v2 = (Vector3*)eval_expr(mat3->v2);
                return mat3;
            }

        case NODE_UNARY:
            {
                Unary* un = (Unary*)node;
                Expr* rhs = eval_expr(un->rhs);

                if(!rhs) return 0;

                if(un->op == OP_MINUS) {
                    if(rhs->type == NODE_INT) {
                        Int* i = (Int*)rhs;
                        return new Int(-(i->value));
                    }

                    if(rhs->type == NODE_FLOAT) {
                        Float* fl = (Float*)rhs;
                        return new Float(-(fl->value));
                    }
                    if(rhs->type == NODE_VECTOR3) {
                        Vector3* vec3 = (Vector3*)rhs;

                        //SHAMEFUL HACK
                        return eval_binary(new Binary(vec3, OP_MULT, new Float(-1))); 
                    }
                }
                if(un->op == OP_NOT) {
                    if(rhs->type == NODE_BOOL) {
                        Bool* b = (Bool*)rhs;
                        return new Bool(!(b->value));
                    }
                }
            }

        case NODE_BINARY:
            {
                Binary* bin = (Binary*)(node);
                return eval_binary(bin);
            }

        default: return 0;
    }

    return 0;
}

void MyParser::eval_stmt(Stmt* stmt) {
    switch(stmt->type) {
        case NODE_FUNCSTMT:
            {
                FuncStmt* func = (FuncStmt*)stmt;
                std::string funcName = func->invoke->ident->name;
                if(functions.find(funcName) != functions.end()) {
                    FuncDef* def = functions[funcName];
                    execute_stmts(def->stmts);
                } else {
                    std::cout << "ERROR: Call to undefined function " << funcName << std::endl;
                }
                return;
            }
        case NODE_ASSIGN:
            {
                Assign* assign = (Assign*)stmt;
                Expr* rhs = eval_expr(assign->value);
                if(rhs) {
                    Ident* ident = assign->ident;
                    if(ident->type == NODE_IDENT) {
                        variables[assign->ident->name] = rhs;
                    } else if(ident->type == NODE_UNIFORM) {
                        Uniform* uniform = (Uniform*)ident;
                        if(current_program->vertSource->name == uniform->shader) {
                            ShaderSource* src = current_program->vertSource;
                            std::string type = "";
                            if(src->uniforms.find(uniform->name) != src->uniforms.end()) {
                                type = src->uniforms[uniform->name];
                            } else {
                                src = current_program->fragSource;
                                if(src->uniforms.find(uniform->name) != src->uniforms.end()) {
                                    type = src->uniforms[uniform->name];
                                } else {
                                    std::cout << "Uniform does not exist!\n";
                                    return;
                                }
                            }

                            if(type == "vec3") {
                                if(rhs->type == NODE_VECTOR3) {
                                    Vector3* vec3 = (Vector3*)rhs;
                                    gl->glUniform3f(gl->glGetUniformLocation(current_program->handle, uniform->name.c_str()), resolve_vec3(vec3));
                                } else {
                                    std::cout << "Uniform uploadm mismatch: vec3 required\n"; 
                                }
                            } else if(type == "mat4") {
                            }

                        }
                    }
                } else {
                    std::cout << "ERROR: Invalid assignment\n";
                }

                return;
            }
        case NODE_ALLOC:
            {
                Alloc* alloc = (Alloc*)stmt;

                if(!buffers[alloc->ident->name]) {
                    Buffer* buf = new Buffer;
                    buf->layout = new Layout;

                    gl->glGenBuffers(1, &(buf->handle));
                    buffers[alloc->ident->name] = buf;
                } else {
                    std::cout << "ERROR: Can't allocate to " << alloc->ident->name << ": buffer already exists!\n";
                }

                return;
            }
        case NODE_UPLOAD:
            {
                Upload* upload = (Upload*)stmt;

                Buffer* buffer = buffers[upload->ident->name];
                Layout* layout = buffer->layout;

                if(layout->attributes.find(upload->attrib->name) == layout->attributes.end()) {
                    layout->attributes[upload->attrib->name] = 3;
                    layout->list.push_back(upload->attrib->name);
                }

                std::vector<float>* target = &(buffer->data[upload->attrib->name]);
                for(unsigned int i = 0; i < upload->list->list.size(); i++) {
                    Expr* expr = eval_expr(upload->list->list[i]);
                    if(!expr) {
                        std::cout << "ERROR: Can't upload illegal value into buffer\n";
                        break;
                    }

                    if(expr->type == NODE_VECTOR3) {
                        Vector3* vec3 = (Vector3*)expr;
                        target->insert(target->end(), resolve_scalar(vec3->x));
                        target->insert(target->end(), resolve_scalar(vec3->y));
                        target->insert(target->end(), resolve_scalar(vec3->z));
                    }

                    if(expr->type == NODE_FLOAT) {
                        Float* f = (Float*)expr;
                        target->insert(target->begin(), resolve_scalar(f));
                    }
                }

                buffer->sizes[upload->attrib->name] = target->size() / buffer->layout->attributes[upload->attrib->name];

                return;
            }
        case NODE_DRAW:
            {
                Draw* draw = (Draw*)stmt;

                if(current_program == NULL) {
                    std::cout << "ERROR: Cannot bind program with name " << current_program_name << std::endl;
                    return;
                }

                Buffer* buffer = buffers[draw->ident->name];
                if(buffer) {
                    Layout* layout = buffer->layout;
                    std::vector<float> final_vector;

                    std::map<std::string, unsigned int> attributes = layout->attributes;

                    gl->glBindBuffer(GL_ARRAY_BUFFER, buffer->handle);
                    for(unsigned int i = 0; i < buffer->sizes[layout->list[0]]; i++) {
                        for(unsigned int j = 0; j < layout->list.size(); j++) {
                            std::string attrib = layout->list[j];
                            for(unsigned int k = 0; k < layout->attributes[attrib]; k++) {
                                final_vector.insert(final_vector.end(), buffer->data[attrib][(i * layout->attributes[attrib]) + k]);
                            }
                        }
                    }

                    gl->glBufferData(GL_ARRAY_BUFFER, final_vector.size() * sizeof(float), &final_vector[0], GL_STATIC_DRAW);

                    int total_size = 0;
                    for(std::map<std::string, unsigned int>::iterator it = layout->attributes.begin(); it != layout->attributes.end(); ++it) {
                        total_size += it->second;
                    }

                    int cumulative_size = 0;
                    for(unsigned int i = 0; i < layout->list.size(); i++) {
                        std::string attrib = layout->list[i];
                        GLint location = gl->glGetAttribLocation(current_program->handle, attrib.c_str());
                        gl->glVertexAttribPointer(location, layout->attributes[attrib], GL_FLOAT, false, total_size * sizeof(float), (void*)(cumulative_size * sizeof(float)));
                        gl->glEnableVertexAttribArray(location);

                        cumulative_size += layout->attributes[attrib];
                    }

                    gl->glDrawArrays(GL_TRIANGLES, 0, final_vector.size() / total_size);
                } else {
                    std::cout << "ERROR: Can't draw non-existent buffer " << draw->ident->name << std::endl;
                }

                return;
            }
        case NODE_USE:
            {
                Use* use = (Use*)stmt;
                current_program_name = use->ident->name;
                current_program = programs[current_program_name];

                if(current_program == NULL) {
                    std::cout << "ERROR: No valid vertex/fragment pair for program name " << current_program_name << std::endl;
                    return;
                }

                gl->glUseProgram(current_program->handle);
                return;
            }
        case NODE_IF:
            {
                If* ifstmt = (If*)stmt;
                Expr* condition = eval_expr(ifstmt->condition);
                if(!condition) return;
                if(condition->type == NODE_BOOL) {
                    bool b = ((Bool*)condition)->value;
                    if(b) {
                        execute_stmts(ifstmt->block);
                    }
                }
                return;
            }
        case NODE_WHILE:
            {
                While* whilestmt = (While*)stmt;
                Expr* condition = eval_expr(whilestmt->condition);
                if(!condition) return;
                if(condition->type == NODE_BOOL) {
                    std::time_t start = std::time(nullptr);
                    while(true) {
                        condition = eval_expr(whilestmt->condition);
                        bool b = ((Bool*)condition)->value;
                        if(!b) break;

                        execute_stmts(whilestmt->block);
                        std::time_t now = std::time(nullptr);
                        
                        int diff = std::difftime(now, start);
                        if(diff > LOOP_TIMEOUT) { break; }
                    }
                }

                return;
            }
        case NODE_PRINT:
            {
                Print* print = (Print*)stmt;
                Expr* output = eval_expr(print->expr);
                switch(output->type) {
                    case NODE_INT:
                        std::cout << resolve_int(output) << std::endl;
                        break;
                    case NODE_FLOAT:
                        std::cout << resolve_float(output) << std::endl;
                        break;
                    case NODE_BOOL:
                        std::cout << (((Bool*)output)->value? "true" : "false") << std::endl;
                        break;
                    case NODE_VECTOR3:
                        {
                            Vector3* vec3 = (Vector3*)output;
                            std::cout << "[" << resolve_scalar(vec3->x) << ", " << resolve_scalar(vec3->y) << ", " << resolve_scalar(vec3->z) << "]\n";
                            break;
                        }
                    case NODE_MATRIX3:
                        {
                            Matrix3* mat3 = (Matrix3*)output;
                            std::cout << "|" << resolve_scalar(mat3->v0->x) << ", " << resolve_scalar(mat3->v0->y) << ", " << resolve_scalar(mat3->v0->z) << "|\n";
                            std::cout << "|" << resolve_scalar(mat3->v1->x) << ", " << resolve_scalar(mat3->v1->y) << ", " << resolve_scalar(mat3->v1->z) << "|\n";
                            std::cout << "|" << resolve_scalar(mat3->v2->x) << ", " << resolve_scalar(mat3->v2->y) << ", " << resolve_scalar(mat3->v2->z) << "|\n";
                            break;
                        }
                    default: break;
                }
                return;
            }
        default: return;
    }
}

void MyParser::execute_stmts(Stmts* stmts) {
    for(unsigned int it = 0; it < stmts->list.size(); it++) { 
        eval_stmt(stmts->list.at(it));
    }
}

void MyParser::compile_shader(GLuint* handle, ShaderSource* source) {
    const char* src = source->code.c_str();
    gl->glShaderSource(*handle, 1, &src, NULL);
    gl->glCompileShader(*handle);

    GLint success;
    char log[256];

    gl->glGetShaderInfoLog(*handle, 256, 0, log);
    gl->glGetShaderiv(*handle, GL_COMPILE_STATUS, &success);
    if(success != GL_TRUE) {
        std::cout << source->name << " shader error\n" << success << " " << log << std::endl;
    }
}

void MyParser::compile_program() {
    programs.clear();
    for(std::map<std::string, ShaderPair*>::iterator it = shaders.begin(); it != shaders.end(); ++it) {
        Program* program = new Program;
        program->handle = gl->glCreateProgram();
        program->vert = gl->glCreateShader(GL_VERTEX_SHADER);
        program->frag = gl->glCreateShader(GL_FRAGMENT_SHADER);

        gl->glAttachShader(program->handle, program->vert);
        gl->glAttachShader(program->handle, program->frag);

        program->vertSource = it->second->vertex;
        program->fragSource = it->second->fragment;

        if(program->vertSource == NULL) {
            std::cout << "ERROR: Missing vertex shader source for program " << it->first << std::endl;
            continue;
        }

        if(program->fragSource == NULL) {
            std::cout << "ERROR: Missing fragment shader source for program " << it->first << std::endl;
            continue;
        }

        compile_shader(&(program->vert), program->vertSource);
        compile_shader(&(program->frag), program->fragSource);

        GLint success;
        char log[256];
        gl->glLinkProgram(program->handle);
        gl->glGetProgramiv(program->handle, GL_LINK_STATUS, &success);
        gl->glGetProgramInfoLog(program->handle, 256, 0, log);
        if(success != GL_TRUE) {
            std::cout << "program error\n" << log << std::endl;
        }

        programs[it->first] = program;
    }
}

void MyParser::execute_init() {
    if(!init || status) return;
    buffers.clear();
    variables.clear();

    execute_stmts(init->stmts);
}

void MyParser::execute_loop() {
    if(!loop || status) return;

    execute_stmts(loop->stmts);
}

void MyParser::parse(std::string code) {
    YY_BUFFER_STATE state = yy_scan_string(code.c_str());

    shaders.clear();
    functions.clear();
    status = yyparse(&shaders, &functions);

    init = functions["init"];
    if(init == NULL) {
        std::cout << "ERROR: init function required!\n";
    }

    loop = functions["loop"];
    if(loop == NULL) {
        std::cout << "ERROR: loop function required!\n";
    }

    yy_delete_buffer(state);
}

