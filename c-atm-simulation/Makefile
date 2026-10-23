# ==============================================================================
# Makefile for Console-Based ATM Simulation (C11 Standard)
# ==============================================================================

CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2

ifeq ($(OS),Windows_NT)
    TARGET = atm_sim.exe
    RM = del /Q /F
else
    TARGET = atm_sim
    RM = rm -f
endif

SRCS = main.c atm.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c atm.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-$(RM) $(TARGET) $(OBJS) accounts.dat accounts.bak accounts.tmp audit.log statement.csv

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
