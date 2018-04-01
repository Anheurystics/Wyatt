# Tutorial
This short tutorial series aims to walk you through the language's features while providing an introduction to basic OpenGL programming concepts. It copies its structure from other OpenGL tutorials out there, sucha as [this](https://learnopengl.com/) and [this](https://open.gl)

## Part 0: Set-up and Theory
Open up the Wyatt development environment. The initial code should look like this
```js
func init() {

}

func loop() {
}
```
From here we can begin to render our first triangle

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
With our attributes set, it's time to write our shaders. We will create a vertex shader that simply takes the input position attribute and outputs it as the final position output.
```js
vert simple(vec3 pos) {
    func main() {
        FinalPosition = [pos, 1.0];
    }
}(vec4 FinalPosition)
```
The FinalPosition will always be a `vec4`, so we have to "convert" our `vec3` into one beforehand.

Our fragment shader wil determine the output color. Since we want to have a flat-colored triangle, we hardcode the output color as white.
```js
frag simple() {
    func main() {
        FinalColor = [1.0, 1.0, 1.0, 1.0];
    }
}(vec4 FinalColor)
```
In later examples, the fragment shader will get interpolated data from the vertex shader.

The last step would be to draw the contents of the `triangle` buffer using the pipeline described by the `simple` shaders. This is to be done in the loop function (so it'll draw every frame)
```js
func loop() {
    draw triangle using simple;
}
```

![Flat white triangle][flat_tri]

[flat_tri]: flat_tri.png

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
vert simple(vec3 pos, vec3 col) {
    func main() {
        FinalPosition = [pos, 1.0];
        Color = col;
    }
}(vec4 FinalPosition, vec3 Color)

frag simple(vec3 Color) {
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