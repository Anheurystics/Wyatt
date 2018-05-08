# Wyatt TODO list
- [ ] Stop using AST nodes for storage
    - [ ] Create separate wrapper types for int, float, bool, vectors, matrices, buffers, textures, lists
        - [ ] Use glm for vector and matrix internal types
    - [ ] Modify scope lists to store those wrapper types
        - [ ] Formalize pass by value vs. pass by reference semantics
            - int, float, bool as value, everything else as reference
            - copy constructors for reference objects
    - [ ] Modify eval_* expressions to return just those
    - [ ] Modify transpiler to use those types (side effect: remove need for resolve_* functions?)

- [ ] Speed up certain parts
    - [ ] Make inverse, transpose, etc. built-in functions (ie. not in standard library)
        - [ ] Use glm to handle those

- [ ] Decouple renderer from interpreter
    - [ ] Create separate GLRenderer class
    - [ ] Add functions for creating graphics objects, manipulating them, calling functions
    - [ ] Remove GL* references from AST nodes and transpiler, use GLRenderer
    - [ ] Create dummy renderer for headless interpreter (pass renderer to interpreter constructor)

