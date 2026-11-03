# Chip-8
a simple Chip-8 emulator/interpreter implementation in C using SDL3.
supports inputs, audio and display.

# Quirks profile
this is the result from [Timendus's Quirks Test](https://github.com/Timendus/chip8-test-suite/raw/main/bin/5-quirks.ch8)

| Feature    | Enabled |
|------------|---------|
| VF Reset   | NO      |
| Memory     | NO      |
| Disp. Wait | NO      |
| Clipping   | YES     |
| Shifting   | NO      |
| Jumping    | YES     |

# Keyboard mapping
```
Keyboard:       CHIP-8 Keypad:
1  2  3  4  ->  1  2  3  C
Q  W  E  R  ->  4  5  6  D
A  S  D  F  ->  7  8  9  E
Z  X  C  V  ->  A  0  B  F
```

# Building
```bash
gcc main.c vm.c -lSDL3 -o chip8
```

# Acknowledgements
- https://github.com/Timendus/chip8-test-suite
- https://github.com/loktar00/chip8/tree/master/roms
- https://www.cs.columbia.edu/~sedwards/classes/2016/4840-spring/designs/Chip8.pdf
- https://wojlin.github.io/js-chip8/docs.html

