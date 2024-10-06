b proc.c:433
b trap.c:64
continue
p cpus[$tp]->proc->name
delete breakpoint 1
continue
p cpus[$tp]->proc->name
da