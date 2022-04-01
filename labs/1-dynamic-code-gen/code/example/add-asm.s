# solve for rs2
go:
add r0, r0, r0
add r0, r0, r15
add r0, r15, r0
add r15, r0, r0
bl after

sub r0, r0, r0
sub r0, r0, r15
sub r0, r15, r0
sub r15, r0, r0
after:
