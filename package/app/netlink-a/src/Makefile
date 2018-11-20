CFLAGS += -Wall -g -O0
RM = /bin/rm -f

#OBJS = netlink-a.o
OBJS = netlink-a-rx.o
SRCS = $(OBJS:.o=.c)
DEPS := $(OBJS:.o=.d)
APP = netlink-a

$(APP): $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) -o $(APP)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

install:
	cp $(APP) ../../res

clean:
	$(RM) $(APP) $(OBJS) $(DEPS)

clean_deps:
	$(DEPS)

-include $(DEPS)
