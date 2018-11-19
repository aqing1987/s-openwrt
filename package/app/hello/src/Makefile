CFLAGS += -Wall -g -O0
RM = /bin/rm -f

OBJS = hello.o
SRCS = $(OBJS:.o=.c)
DEPS := $(OBJS:.o=.d)
APP = hello

$(APP): $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) -o $(APP)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	$(RM) $(APP) $(OBJS) $(DEPS)

clean_deps:
	$(DEPS)

-include $(DEPS)
