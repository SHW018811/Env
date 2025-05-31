CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lpthread -lm
INCLUDES = -I.

SRCS = main.c dbc.c ./ocv_soc/ocv_soc_t.c
OBJS = $(SRCS:.c=.o)

TARGET = bms_simulator

.PHONY: all clean clean_objs

all: $(TARGET)
	@$(MAKE) clean_objs

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean_objs:
	rm -f $(OBJS)

clean:
	rm -f $(OBJS) $(TARGET)