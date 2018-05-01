# Wyatt 
Wyatt is a programming language and a development environment for graphics programming. 

> **WARNING**: While the basic features are there, it is still in an early and buggy alpha state

# Getting Started
Starting the program is as simple as running the binary executable. The `docs` directory contain useful information on how to use the programming language.

If you are an experienced OpenGL developer, then `docs/MANUAL.md` will teach you how the language abstracts the OpenGL API. If you are new to graphics progrmaming, then `docs/TUTORIAL.md` will introduce you to the concepts.

## Hello Triangle
```python
vert basic(vec3 pos) {

    func main() {
        FinalPosition = [pos, 1.0];
    }

}(vec4 FinalPosition)

frag basic() {

    func main() {
        FinalColor = [1.0, 1.0, 1.0, 1.0];	
    }

}(vec4 FinalColor)

buffer tri;

func init(){
    tri.pos += [1.0, -1.0, 0.0], [0.0, 1.0, 0.0], [-1.0, -1.0, 0.0];
}

func loop(){
    draw tri using basic;
}
```

# Building 
Building Wyatt requires
1. `g++` (MinGW for Windows systems) and `make`
1. Qt Creator and Qt5 SDK
1. Flex 2.6
1. Bison 3.0.4
1. `stb_image.h` (place it in src/lang)

(While other version of the above tools might work, the ones listed above are the ones I personally use to build)

To first generate the Makefiles, run `qmake` on the root directory. Then, run `make`.

The build type (Release or Debug) can also be specified (the build defaults to Release).
```
> make -f Makefile.Release
> make -f Makefile.Debug
```

The resulting executable can be found in the release and debug directories.

Alternatively, if you have QtCreator installed, you can open the project file (`wyatt.pro`) and build from there.

# License
Wyatt (both the IDE and language) is licensed under the GPLv3 license.
