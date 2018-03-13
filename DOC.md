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
Wyatt is modelled after languages such as JavaScript and Python.

## Type System and Declaration
The built-in primitives are `int`, `float` (both 32-bit), `bool`, and `string`.

Types such as `buffer` and `texture2D` (used for 3D rendering operations), and linear algebra constructs such as vectors and matrices, and will be discussed later in this document.

The `var` keyword can be used for implicit declaration.
```js
int a = 0; 
float b = 0.25;
string c = "hello world";
bool a = true;
a = "hello"; // not valid
var d = b; // d is implicitly defined a float
```

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
## Linear Algebra Types
Wyatt has the vector and matrix types built-in. Usual operations between them, such as the dot product, cross product, and matrix multiplication is supported.
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
print -[1, 0, -1]   // [-1, 0, 1];
print |[1, 1, 1]|   // 1.7320
```
### Matrix
Matrix types can be thought of as two-dimensional vector types, and are specified in **row-major** order
```js
mnat2 a = [[1, 2], [3, 4]];
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
## 3D programming constructs
Wyatt abstracts away from the programmer boilerplate code relating to creating, modifying, and using vertex shaders, buffers, etc. Many of the common OpenGL operations are built into the language
### Shader creation
Instead of storing shader code in strings/files and loading them, shader code in Wyatt is part of the program itself, with the use of the `vert` and `frag` keywords. In lieu of GLSL, a subset of Wyatt is transpiled to GLSL. This makes sure that the user will only have to keep one language in mind.
```js
vert basic(vec3 pos, vec3 col) { // Inputs go here (in / attribute)

    // Uniforms go here
    mat4 model;
    mat4 view;
    mat4 proj;

    func main() {
        // FinalPosition is equivalent to gl_Position
        FinalPosition = proj * view * model * [proj, 1.0]; 
        Color = col;
    }

}(vec4 FinalPosition, vec3 Color) // Outputs go here (out / varying)

frag basic(vec3 Color) { // Inputs go here (in / varying)

    // Uniforms go here

    func main() {
        // FinalColor is equivalent to gl_FragColor
        FinalColor = [Color, 1.0];
    }

}(vec4 FinalColor) // Outputs goes here
```

Function inputs are specified llike inputs to any normal function-- the same goes for function outputs, but at the end of the definition instead. Uniforms are specified as global variables, accessible only to the main() function and any other declared functions.

Instead of explicitly declaring a program to which the two shaders will be linked to, a vertex and fragment shader sharing the same name will automatically be bound to a program with that name.

Modifying and accessing a shader's uniform is analogous to modifying its member
```js
shader.model = [[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]];
print shader.model; // This prints out the uploaded model matrix
```

### Attribute buffers
Vertex data is stored in buffers. Buffers can be thought of as objects containing one or more lists of data (typically vectors)

Given the shaders above, declaring a white triangle would be as simple as
```js
buffer tri;
// three vertices, where pos and color are inputs in the above shader
tri.pos += [1, -1, 0], [0, 1, 0], [-1, -1, 0];
tri.color += [1, 1, 1], [1, 1, 1], [1, 1, 1];
```

This automatically performs the uploading of data (`glBufferData`) and the assigning of attribute layouts (`glVertexAttribPointer`)

Buffer objects also have built-in support for indexed vertex specification, using the `indices` member
```
tri.indices += 1, 2, 3;
```
### Simple drawing
To `draw` the contents of a buffer using a specific shader program
```js
draw <buffer> using <program>
```

If the buffer's `indices` field is not empty, it will use the indices accordingly.


### Textures
Textures can be loaded by assigning the filename to a `texture2D` object, and are assigned to shaders as uniforms
```js
texxture2D diffuse = "test.jpg";
program.diffuseTex = diffuse;
```

### Rendering to texture