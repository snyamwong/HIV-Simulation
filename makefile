FCC = tau_cc.sh
LDC = tau_cc.sh

LD_FLAGS = -std=c11 -lm
FLAGS = -std=c11 -lm

RM = /bin/rm

PROGS = hiv_bcast.cx hiv_halo.cx
OBJS = hiv_bcast.o hiv_halo.o

#all rule
all: $(PROGS)

#$(PROGS) : $(OBJS)
#	$(LDC) $(LD_FLAGS) $(OBJS) -o $(PROGS)

export TAU_MAKEFILE=/opt/sw/tau/x86_64/lib/Makefile.tau-mpi-pdt

hiv_bcast.o:	hiv_bcast.c
	$(FCC) $(FLAGS) -c $^ -o $@

hiv_bcast.cx:	hiv_bcast.o
	$(LDC) $(LD_FLAGS) $< -o $@

hiv_halo.o:	hiv_halo.c
	$(FCC) $(FLAGS) -c $^ -o $@

hiv_halo.cx:	hiv_halo.o
	$(LDC) $(LD_FLAGS) $< -o $@

#clean rule
clean:
	$(RM) -rf *.o $(PROGS) *.mod
