This document serves as a manual/tutorial for the Wyatt programming language/development environment

# Introduction
Wyatt is a beginner-friendly programming language for the creation of 3D graphics applications. It abstracts away OpenGL's boilerplate code and exposes the graphics concepts (such as the shaders, vertex buffers, etc.) in a simpler manner. 

# Rationale
## Easier setup 
One reason for why WebGL is favored for teaching the OpenGL API is its relatively easy setup. The only requirements are a WebGL-compliant browser and a text editor. Once the context is acquired (through the HTML5 canvas API) then the programmer can start writing 3D graphics applications.

Desktop OpenGL development environments can be harder to set-up, especially if the language used is C++. Development is highly reliant on third-party libraries (GLFW/SDL for windowing, glew/glad for function loading, glm for math, etc.), and trying to teach how to include and link against those libraries can take a lot of time.

The IDE included with Wyattm with its own built-in code editor, a screen for outputting results, and error handling, makes setup instantaneous.

## Less boilerplate
The OpenGL API's focus on configurability, flexibility, and speed also contributes to its high barrier to entry. For example, uploading vertex data to the GPU has the following steps:
```c++
GLuint triangle;
glGenBuffers(1, &triangle);
glBindBuffer(GL_ARRAY_BUFFER, triangle);
glBufferData(GL_ARRAY_BUFFER, sizeof(data), &data[0], GL_STATIC_DRAW);

GLint a_pos = glGetAttribLocation(program);
glVertexAttribPointer(a_pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
glEnableVertexAttribArray(a_pos);
```
While this level of control allows for optimizations on the programmer's part, it sometimes ends up confusing beginners. Wyatt aims to simplify these concepts, shortening the vertex data code to something like this:
```js
buffer buf;
buf.pos += data;
```
This not only makes it easier for the user to understand that vertex data is being uploaded, but also removes the need to write out all the boilerplate code needed.

# Language Specification
## Basic Syntax
Wyatt is modelled after languages such as JavaScript and Python. Statements are delimited by a semicolon.
## Looping constructs
The `for` statements in the language can either iterate using an index in a range, or iterate through an explicitly defined list
```js
for(i in 0, 10, 1) { // start, end (exclusive), interval
    print i;
}

list a = {1, 2, 3, 4, 5};
for(i in a) {
    print i;
}
```
Wyatt has the ```vector``` and ```matrix``` types built-in. Usual operations between them, such as the dot product, cross product, and matrix multiplication is supported.
### Vector
Vector types (of length 2, 3, or 4) are declared as such
```js
vec2 a = [1, 2];
vec3 b = [4, 5, 6];
vec4 c = [1, 0, 0, 1];
```
Binary operations for vector types are as follows:
- addition `+`, subtraction `-`, dot product `*`, cross product `%` (between vectors with similar lengths)
    - dot products evaluate to a `float`, while the rest evaluate to a `vector`
- multiplication `*` and division `\` (between a vector and a scalar) for scaling vectors
```js
vec3 a = [1, 1, 0];
vec3 b = [0, 1, 1];
float c = 2;
print a + b;    // [1, 2, 0]
print a - b;    // [1, 0, -1]
print a * b;    // 1
print a % b;    // [1, -1, 1]
print a * c;    // [2, 2, 0]
print a / c;    // [0.5, 0.5, 0]
```
Unary operations for vectors are negation `-` and magnitude `||`
```js
print -[1, 0, -1]   //[-1, 0, 1];
print |[1, 1, 1]|   //1.7320
```
### Matrix
Matrix types can be thought of as two-dimensional vector types, and are specified in **row-major** order
```js
mat2 a = [[1, 2], [3, 4]];
mat3 b = [[1, 0, 0], [0, 1, 0], [0, 0, 1]];
vec4 v = [1, 2, 3, 4];
mat4 c = [v, v, v, v];
```
The only valid binary operation between matrices of similar dimensions are only multiplication
```js
mat3 a = [[1, 2, 3], [4, 5, 6], [7, 8, 9]];
mat3 s = [[2, 0, 0], [0, 2, 0], [0, 0, 2]];
print a * s;    // [[2, 4, 6], [8, 10, 12], [14, 16, 18]];
```