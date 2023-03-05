CFLAGS=-Wall -pedantic -std=c11
APP=rmtabs
SRCS=main.c
OBJS=$(SRCS:.c=.o)

all: debug

debug: CFLAGS += -g3 -O0
debug: $(APP)

release: CFLAGS += -O3
release: $(APP)

$(APP): $(OBJS)
	$(CC) $^ -o $@

.PHONY: clean
clean:
	$(RM) $(APP) $(OBJS)
