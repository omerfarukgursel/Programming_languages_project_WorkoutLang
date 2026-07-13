CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g
SRCDIR  = src
SRCS    = $(SRCDIR)/main.c $(SRCDIR)/lexer.c $(SRCDIR)/parser.c $(SRCDIR)/ast.c
TARGET  = workoutlang

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
