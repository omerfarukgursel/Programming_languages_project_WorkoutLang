# WorkoutLang

A domain-specific language (DSL) for designing, structuring, and documenting resistance-training and cardio workout programs. Written in C as part of the CSE 341 — Concepts of Programming Languages course.

---

## Overview

WorkoutLang lets fitness coaches and athletes describe workout sessions — exercises, sets, repetitions, rest periods, and progression rules — in a readable, unambiguous format without learning a general-purpose programming language.

```
workout UpperBody {
    day Monday {
        set benchpress reps 4 x 10 rest 90s
        set overhead_press reps 3 x 8 rest 60s
    }
}

routine myWeek {
    int totalSets = 5
    load UpperBody
    if totalSets > 3 {
        print "Heavy week"
    }
    repeat 4 weeks {
        load UpperBody
    }
}
```

---

## Language Features

### Types
| Type | Description | Example |
|---|---|---|
| `int` | 32-bit signed integer | `3`, `10` |
| `float` | IEEE 754 double precision | `1.5`, `72.5` |
| `bool` | Boolean value | `true`, `false` |
| `duration` | Time with unit suffix | `90s`, `2min`, `1hr` |
| `weight` | Weight with unit suffix | `80kg`, `135lb` |
| `Day` | Record type (setCount field) | declared via `day` keyword |

### Control Structures
- `if / else` — conditional execution; condition must be `bool`
- `while` — condition re-evaluated each iteration (W-True / W-False semantics)
- `repeat N weeks` — fixed-count loop, no iteration counter exposed

### Domain-Specific Constructs
- `workout / day / set` — structured workout declaration hierarchy
- `load` — execute all days of a named workout
- `print` — output any value to stdout

### Functions
- Typed parameters and return types
- Call-by-value parameter passing (Sebesta §9.2)
- Recursive calls supported

### Type System
- **Strongly typed** — every type error detected before execution
- **No implicit coercion** — not even `int → float`
- **Name equivalence** — `duration` and `weight` are distinct even though both store a number
- **Static (lexical) scoping** — name resolution follows textual structure, not call sequence

---

## Build

```bash
git clone <your-repo-url>
cd workoutlang
make
```

Requires `gcc` with C11 support. Compiles with zero warnings under `-Wall -Wextra -std=c11`.

To clean and rebuild:

```bash
make clean && make
```

---

## Usage

```bash
# Parse + type check + execute (default)
./workoutlang <file.wl>

# Parse and print AST only
./workoutlang --dump-ast <file.wl>

# Parse + type check only (no execution)
./workoutlang --type-check <file.wl>

# Print token stream only
./workoutlang --lex <file.wl>
```

---

## Example Output

Running `./workoutlang tests/valid1.wl`:

```
[UpperBody — Monday]
  set benchpress: 4 x 10, rest 90s
  set overhead_press: 3 x 8, rest 1min
  set tricep_dip: 3 x 12, rest 45s
[UpperBody — Wednesday]
  set pullup: 4 x 8, rest 90s
  set row: 3 x 10, rest 1min
Heavy week
[UpperBody — Monday]
  set benchpress: 4 x 10, rest 90s
  ...
```

---

## Test Programs

```bash
# Valid programs — parse, type check, and execute successfully
./workoutlang tests/valid1.wl   # workout + day + set + routine + if/else + repeat
./workoutlang tests/valid2.wl   # func + return + while + func_call
./workoutlang tests/valid3.wl   # func returning bool + && operator + unary !

# Parse error programs — rejected with line numbers
./workoutlang tests/error1.wl   # missing closing brace
./workoutlang tests/error2.wl   # missing 'x' between sets and reps
./workoutlang tests/error3.wl   # unknown character '@'
./workoutlang tests/error4.wl   # missing rest duration in set statement
./workoutlang tests/error5.wl   # statement at top level

# Type / runtime error programs
./workoutlang tests/type_error1.wl   # bool variable initialised with int  → [line 3] Type error
./workoutlang tests/type_error2.wl   # division by zero                    → [line 2] Runtime error
./workoutlang tests/type_error3.wl   # if condition is int, not bool       → [line 3] Type error
```

---

## Project Structure

```
workoutlang/
├── Makefile
├── README.md
├── src/
│   ├── main.c              # Entry point — pipeline orchestration
│   ├── token.h             # Token type definitions
│   ├── lexer.h / lexer.c   # Tokeniser — DURATION_LIT, WEIGHT_LIT, keywords
│   ├── parser.h / parser.c # Recursive descent parser — AST + error messages
│   ├── ast.h / ast.c       # AST node definitions (tagged union) + printer
│   ├── symbol_table.h / symbol_table.c  # Environment chain — static scoping
│   ├── type_checker.h / type_checker.c  # Two-pass static type checker
│   └── interpreter.h / interpreter.c    # Tree-walking interpreter
└── tests/
    ├── valid1.wl           # Upper body workout — if/else, repeat
    ├── valid2.wl           # Leg day — while loop, functions
    ├── valid3.wl           # Full body — bool func, &&, !
    ├── error1.wl .. error5.wl       # Parser error cases
    └── type_error1.wl .. type_error3.wl  # Type / runtime error cases
```

---

## Pipeline

```
source.wl
    │
    ▼
  Lexer          →  Token stream
    │
    ▼
  Parser         →  AST (with line numbers)
    │
    ▼
  Type Checker   →  Pass 1: register globals
  (two passes)   →  Pass 2: check all statement bodies
    │
    ▼
  Interpreter    →  stdout / stderr
```

---

## Error Format

All errors use the format:

```
[line N] <Error type>: <message>
```

Examples:
```
[line 3] Parse error at '10': expected 'x' between sets and reps
[line 3] Type error: cannot initialise 'result' (type bool) with expression of type int
[line 2] Runtime error: division by zero
```

---

## Key Design Decisions

**No implicit coercion.** Every operand must already have the exact type expected. Not even `int → float` widening is permitted. This eliminates an entire class of silent errors for non-programmer users.

**`duration` and `weight` are comparison-only.** Arithmetic on these types (`90s + 60s`) is blocked at the type-checker level. Adding two rest periods is plausible; multiplying two weights (kg²) is dimensionally nonsensical.

**Assignment is a statement, not an expression.** `if (x = 5)` is a parse error in WorkoutLang. The `=` and `==` operators are syntactically separated — `=` only appears in `var_decl` and `assign_stmt`, never inside an expression.

**`repeat N weeks` has no iteration counter.** The body cannot observe which week it is on. This prevents index-arithmetic errors and keeps the semantics close to how coaches think about training cycles.

**Static scoping via environment chain.** Scope frames are a linked list. `scope_lookup` walks from innermost to outermost, implementing static (lexical) scoping. Each `if`, `while`, and `repeat` body pushes a new `SCOPE_BLOCK` frame; the frame is destroyed on exit (stack-dynamic lifetime).

---

## Known Limitations

- `Day.setCount` dot notation is specified in the grammar but not yet implemented in the parser. The type checker and interpreter handle it; adding it requires ~10 lines in `parse_primary`.
- A `func` that reaches the end of its body without a `return` statement silently returns `0`. Detecting this requires control-flow analysis beyond the current two-pass type checker.
- The `weight` type uses integer-only literal syntax (`80kg`, not `3.5kg`). Internally stored as `double` for lb→kg conversion.

---

## Academic Context

| Item | Detail |
|---|---|
| Course | CSE 341 — Concepts of Programming Languages |
| Term | Spring 2026 |
| Reference | Robert W. Sebesta, *Concepts of Programming Languages*, 12th ed. |
