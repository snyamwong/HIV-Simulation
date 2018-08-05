FCC = tau_cc.sh
LDC = tau_cc.sh

LD_FLAGS = -std=c11 -lm
FLAGS = -std=c11 -lm

RM = /bin/rm

PROGS = hiv.cx
OBJS = hiv.o

#all rule
all: $(PROGS)

#$(PROGS) : $(OBJS)
#	$(LDC) $(LD_FLAGS) $(OBJS) -o $(PROGS)

export TAU_MAKEFILE=/opt/sw/tau/x86_64/lib/Makefile.tau-mpi-pdt

hiv.o:	hiv.c
	$(FCC) $(FLAGS) -c $^ -o $@

hiv.cx:	hiv.o
	$(LDC) $(LD_FLAGS) $< -o $@

#clean rule
clean:
	$(RM) -rf *.o $(PROGS) *.mod
