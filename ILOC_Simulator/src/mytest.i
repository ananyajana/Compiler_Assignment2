vecton
loadI 1024 => r10
loadI 5 => r11
store r11 => r10
loadI  2 => r1
loadI 10 => r2
loadI 1024 => r3
load r3 => r4
vectoff
add r1, r4 => r5
mult r2, r5 => r6
add r1, r6 => r7
store r7 => r3
output 1024  
