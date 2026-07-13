# WorkoutLang — D2 Implementation (Part 1)

A lexer and parser for WorkoutLang, a domain-specific language for describing
workout programs. Written in C (C11), no external dependencies.

## Project Structure

```
workoutlang/
├── Makefile
├── README.md
├── src/
│   ├── token.h      — token type definitions
│   ├── lexer.h/.c   — lexer (tokenizer)
│   ├── ast.h/.c     — AST node definitions and printer
│   ├── parser.h/.c  — recursive-descent parser
│   └── main.c       — entry point
└── tests/
    ├── valid1.wl    — upper body workout with if/else
    ├── valid2.wl    — functions, while loop, expressions
    ├── valid3.wl    — all types, nested logic, bool expressions
    ├── error1.wl    — missing closing brace
    ├── error2.wl    — missing 'x' in set statement
    ├── error3.wl    — unknown character '@'
    ├── error4.wl    — missing rest duration
    └── error5.wl    — statement at top level
```

## Build

Requires: gcc (any version supporting C11), make.

```bash
make
```

This produces the `workoutlang` binary.

To clean:
```bash
make clean
```

## Usage

```bash
# Parse a file — reports success or prints errors with line numbers
./workoutlang <source.wl>

# Parse and print the AST
./workoutlang --dump-ast <source.wl>

# Print the token stream (lexer output only)
./workoutlang --lex <source.wl>
```

## Examples

### Successful parse with AST dump
```bash
./workoutlang --dump-ast tests/valid1.wl
```
Output:
```
Parsing successful.

Program
  WorkoutDecl(UpperBody)
    DayDecl(Monday)
      SetStmt(exercise=benchpress, sets=4, reps=10, rest=90s)
      ...
```

### Parse error with line number
```bash
./workoutlang tests/error2.wl
```
Output:
```
[line 4] Parse error at '10': expected 'x' between sets and reps
Parsing failed — see errors above.
```

## Language Overview

WorkoutLang programs consist of three kinds of top-level declarations:

- `workout NAME { day NAME { set ... } }` — defines exercise days
- `routine NAME { ... }` — defines a training schedule using loads, loops, conditionals
- `func NAME(params) : type { ... }` — user-defined functions

### Set statement (domain-specific)
```
set benchpress reps 4 x 10 rest 90s
```
Means: benchpress, 4 sets × 10 reps, 90 seconds rest.

### Supported types
- `int`, `float`, `bool`, `duration` (90s, 2min, 1hr), `weight` (80kg, 135lb)

### Control structures
- `if expr { } else { }` — conditional (else binds to nearest if)
- `while expr { }` — general loop
- `repeat N weeks { }` — domain-specific fixed-count loop
- `load NAME` — loads a workout into the current routine

## Exit Codes
- `0` — parsing succeeded
- `1` — parse error or file not found
