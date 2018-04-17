# Tutorial
This short tutorial series aims to walk you through the language's features while providing an introduction to basic OpenGL programming concepts. It copies its structure from other OpenGL tutorials out there, such as [this](https://learnopengl.com/) and [this](https://open.gl)

## Part 0: Set-up and Theory
Open up the Wyatt development environment. The initial code should look like this
```js
func init() {
}

func loop() {
}
```
From here we can begin to render our first triangle.

OpenGL operate on the **graphics rendering pipeline**. This rendering pipeline is responsible for turning the 3D primitives defined by the user into 2D images on the screen using a series of steps.

The simplified rendering pipeline steps you would need to know would be as follows:

**1. Vertex data preparation**

This is where the programmer specifies the shapes to draw, by specifying the different **attributes** such as position, color, etc. The name and type (float, vector) of each attribute will also be specified by the user. For example, if we want to specify a 3D polygon with lighting effects on it, then we have to specify the position, color, and normal attributes.

**2. Vertex shader**

The vertex shader is responsible for getting and processing the different vertex attributes. The output of the vertex shader is also interpolated and passed down to the fragment shader. At this stage also happens the transformation of each vertex, usually via a transformation matrix.

**3. Fragment shader**

The fragment shader gets the output of the vertex shader and interpolates between them, generating fragments based on the vertices. The color of the fragment is also determined in this step.

**4. Drawing to framebuffer**

The default framebuffer is the screen, but can be specified by the user. This is the step where fragments are either drawn onto the screen or discarded (if they are out of bounds).

The screen follows what is called the **normalized device coordinate** system, where the leftmost and rightmost sides are -X and +X, the topmost and bottommost sides are +Y and -Y. While we will be initially defining our geometry using this system, we will be granted more freedom when we start using matrices to transform them.

<div style="page-break-after: always;"></div>

## Part 1: Flat-colored Triangle
To render a simple white triangle, we first have to specify its appropriate vertex data. A `buffer` object is a container for all vertex data and attributes, so let's make one now.
```js
buffer triangle;

func init() {
}

func loop() {
}
```
Each of the buffer's attributes can be treated as a list, where the first element of attribute `xyz` can be thought of as the attribute of the first vertex, and so on. Attributes can be declared and added to on the spot. 

So if we want to declare our triangle with three vertices at `[1.0, -1.0, 0.0], [0.0, 1.0, 0.0], [-1.0, -1.0, 0.0]` we first have to label it with an attribute name (`pos`, for instance), and then add data to it.
```js
func init() {
    triangle.pos += [1.0, -1.0, 0.0], [0.0, 1.0, 0.0], [-1.0, -1.0, 0.0];   
}
```
With our attributes set, it's time to write our shaders. We will create a vertex shader that simply takes the input position attribute and outputs it as the final position output. This is called a pass-through shader
```js
vert basic(vec3 pos) {
    func main() {
        FinalPosition = [pos, 1.0];
    }
}(vec4 FinalPosition)
```
Shaders should be specified before the init and loop functions, and the global variables.
The FinalPosition will always be a `vec4`, so we have to "convert" our `vec3` into one beforehand.

Our fragment shader wil determine the output color. Since we want to have a flat-colored triangle, we hardcode the output color as white.
```js
frag basic() {
    func main() {
        FinalColor = [1.0, 1.0, 1.0, 1.0];
    }
}(vec4 FinalColor)
```
In later examples, the fragment shader will get interpolated data from the vertex shader.

The last step would be to draw the contents of the `triangle` buffer using the pipeline described by the `basic` shaders. This is to be done in the loop function (so it'll draw every frame)
```js
func loop() {
    draw triangle using basic;
}
```

![Flat white triangle][flat_tri]

[flat_tri]: flat_tri.png

<div style="page-break-after: always;"></div>

## Part 2: Colored triangle
We could simply change FinalColor to the desired color we want, but this would only allow us to produce flat colored triangles. If we want each vertex to have a different color, thus giving the triangle a gradient effect, we have to specify color as another attribute.
```js
func init() {
    triangle.pos += [1.0, -1.0, 0.0], [0.0, 1.0, 0.0], [-1.0, -1.0, 0.0];   
    triangle.col += [1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0];
}
```
The three vertices will now be colored red, green, and blue. We then have to modify our shaders to make sure that the attribute data `col` will be used in determining the final fragment color.
```js
vert basic(vec3 pos, vec3 col) {
    func main() {
        FinalPosition = [pos, 1.0];
        Color = col;
    }
}(vec4 FinalPosition, vec3 Color)

frag basic(vec3 Color) {
    func main() {
        FinalColor = [Color, 1.0];
    }
}(vec4 FinalColor)
```
Three things happened here:
1. `col` is declared as an input of the vertex shader
2. `Color` is declared both as an output of the vertex shader and an input of the fragment shader. Normally, any vertex shader outputs should also be defined as fragment shader inputs.
3. `Color` is used by the fragment shader to determine the final output color.

![Colored triangle][color_tri]

[color_tri]: color_tri.png

<div style="page-break-after: always;"></div>

## Part 2.5: Indexing
Since shapes are defined using triangles, defining more complex shapes (in this example, a square) would require us to specify six vertices. In some cases, this means specifying redundant data (ie. if two vertices are specified with the same attributes)
```js
func init() {
    tri.pos += [1.0, -1.0, 0.0], [-1.0, 1.0, 0.0], [-1.0, -1.0, 0.0],
               [1.0, -1.0, 0.0], [1.0, 1.0, 0.0], [-1.0, 1.0, 0.0];
    tri.color += [1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0],
                 [1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0];
}
```
One solution for this would be to assign the vectors to variables, and upload them to the buffer instead. Another solution is to specify the required vertices only once, and then specify the *indexing* of the buffer. The `index` keyword is used for this.
```js
func init() {
    tri.pos += [1.0, -1.0, 0.0], [-1.0, 1.0, 0.0], [-1.0, -1.0, 0.0],
               [1.0, 1.0, 0.0];
    tri.color += [1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0],
                 [0.0, 0.0, 1.0];

    // the first triangle will use vertices 0, 1, 2, and
    // the second one will use vertices 1, 0, 3
    tri.indices += 0, 1, 2, 1, 0, 3;
}
```
![Quad][quad]

[quad]: quad.png

<div style="page-break-after: always;"></div>

## Part 3: Transformation
If we want to move our geometry, we can't just modify the vertices manually per loop and re-upload. We have to use a construct called a `matrix` to perform operations on each individual vertex position.

There are three fundamental matrix transformation operations:
1. Scale matrix
2. Rotation matrix
3. Translation matrix

While you could specify these matrices yourselves, it is easier to use helper functions located in the `utils.gfx` file (you have to include `utils.gfx` at the very top). Example usages of the helper functions are listed below.
```js
import "utils.gfx"

// Create a matrix that scales the vertices by 0.3, shrinking the object
mat4 scaleMatrix = mat4_scale(0.3);

// Create a matrix that translates the vertices by [0.5, 1.0, 2.0];
mat4 translationMatrix =  mat4_translation(0.5, 1.0, 2.0);
```

However, multiplying these to the vertices directly and re-uploading won't work either. The best practice is to specify them as inputs to the vertex shader, and then multiplying there
```js
vert basic(vec3 pos, vec3 color) {
    mat4 model;

    func main() {
        // This transforms each vertex using model
        FinalPosition = model * [pos, 1.0]; 
        Color = color;
    }
}(vec4 FinalPosition, vec3 Color)
```
Members of the shader declared this way are called `uniforms`-- unlike attributes, they retain their value for each vertex/fragment unless specified by the user. They can be thought of as a shader's global variables, and can be set just like a shader's regular members.
```js
func loop() {
    // Update the model matrix, rotation it by 0.01 radians each frame
    model *= mat4_rotation_z(0.01); 
    basic.model = model; // Assign as uniform
    draw tri using basic;
}
```
Since repeatedly uploading vertex data would be costly, uniforms are the best way to perform such operations

![Rotated quad][spin_quad]

[spin_quad]: spin_quad.png

<div style="page-break-after: always;"></div>

## Part 3.5: Camera setup
If we want to setup a camera in scene to move around and look at different objects, we need to use two more matrices. All-in-all, there are three basic fundamental matrices used in 3D rendering:
1. Model matrix - this is the one introduced last tutorial, which controls the object's position, size, and rotation in the world. Multiplying by the model matrix places an object in **world space**
2. View matrix - the (inverse) model matrix of the camera, and will be used to offset the positions of objects, transforming them from **world space** to **view space**
3. Projection matrix - this is the characteristics of the camera itself, which specifies how the objects are projected. Orthographic projections are used commonly for 2D games, and perspective projections are used for 3D games. This matrix transforms from **view space** roughly to **clip space**, which roughly corresponds to (save for some extra operations) to the normalized device coordinates (NDC) mentioned earlier.

Similar to the helper functions for model matrices, there are also helper functions for view matrices:
```js
// View matrix
vec3 eye = [0, 0, 2]; // The position of the camera
vec3 at = [0, 0, 0]; // Where the camera is looking at
vec3 up = [0, 1, 0]; // The global up direction of the camera
mat4 view = mat4_lookat(eye, at up);

// Perspective projection
float fov = PI * 0.25; // The vertical FOV (field-of-view) of the camera
float aspect = WIDTH / HEIGHT; // The aspect ratio of the screen
float near = 0.1; // distance of the near plane from the eye
float far = 100.0; // distance of the far plane from the eye 
mat4 persp = mat4_perspective(fov, aspect, near, far);

// TODO: Ortho
```

So if we want use the matrices we created above, we repeat the steps we did for the model matrix.
```js
vert basic(vec3 pos, vec3 color) {
    mat4 model;
    mat4 view;
    mat4 proj;

    func main() {
        // This transforms each vertex using model
        FinalPosition = proj * view * model * [pos, 1.0]; 
        Color = color;
    }
}(vec4 FinalPosition, vec3 Color)

func init() {
    ...

    basic.view = view;
    basic.proj = persp;
}

func loop() {
    // Rotate around the X-axis instead to make the projection more apparent
    model *= mat4_rotation_x(0.01);
    basic.model = model;
    ...
}
```
![Quad with Camera][proj_quad]

[proj_quad]: proj_quad.png

<div style="page-break-after: always;"></div>

## Part 4: Textures
2D textures are usually for projecting images onto geometry to give more detail, instead of relying on just flat colors. To use 2D textures, you have to **UV map** each vertex of a geometry to a certain texture coordinate. This specifies how the image should be "drawn" in the triangle. The texture coordinates are in the range [0, 0] to [1, 1], where [0, 0] is the lower left corner and [1, 1] is the upper right corner.

Texture coordinates (also referred to as UV coordinates) are treated as another attributes, and are specified in a way similar to position and color attributes. So given our quad example:
```js
vert basic(vec3 pos, vec3 color, vec2 uv) {
    <uniforms>

    func main() {
        <other operations>
        UV = uv;
    }
}(vec4 FinalPosition, vec2 UV);

func init() {
    tri.pos += [1.0, -1.0, 0.0], [-1.0, 1.0, 0.0], [-1.0, -1.0, 0.0],
               [1.0, 1.0, 0.0];
    tri.color += [1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0],
                 [0.0, 0.0, 1.0];
    tri.uv += [1.0, 0.0], [0.0, 1.0], [0.0, 0.0], [1.0, 1.0];

    // the first triangle will use vertices 0, 1, 2, and
    // the second one will use vertices 1, 0, 3
    tri.indices += 0, 1, 2, 1, 0, 3;
}
```
Next we'll have to load the texture itself. Supported filetypes are PNG and JPG. The variable type of all textures is `texture2D`, and can be loaded by just assigning the string describing the filename of the image to the texture2D object;
```js
<shader code>

texture2D tex = "texture.jpg";

func init() {
```
To actually use the texture's colors, we have to pass it as a uniform to the fragment shader. The UV should also be passed from the vertex to the fragment shader, similar to the Color (it will be interpolated). To get the color of a texture at a certain UV coordinate, the `texture` function is used, and it returns a `vec4` (the color)

```js
frag basic(vec3 Color, vec2 UV) {
    texture2D tex;

    func main() {
        FinalColor = texture(tex, UV); 
    }
}(vec4 FinalColor)
```
![Textured quad][textured]

[textured]: textured.png

Instead of fully replacing Color with the texture, you can also blend the two colors together, tinting the image.
```js
FinalColor = texture(tex, UV) * Color;
```
![Textured, blended quad][textured_blend]

[textured_blend]: textured_blend.png

<div style="page-break-after: always;"></div>

## Part 4.5: Render-to-texture
