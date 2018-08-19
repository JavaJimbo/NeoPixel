// PixelWrite.S 10-27-16 Updated for PORT F on LED MATRIX CONTROLLER V1.0
#include <xc.h>
#define NUMBER_OF_PIXELS 60
#define NUMBER_OF_BITS 24

.data

.text
.set noreorder
/*********************************************************************
* writePixels()
* 
********************************************************************/
.global writePixels
.ent writePixels

writePixels:
sub sp, 0x04  # PUSH RA ONTO STACK
sw ra, 0(sp)

START:
la t7,arrPixels					# Set address pointer to first ring address
li t1, 0
OUTLOOP:
lw a0, 0(t7)
jal sendPixel
nop
addiu t1,t1,1
beq t1, NUMBER_OF_PIXELS, DONE
addiu t7,t7,4
j OUTLOOP
nop
DONE:
lw ra, 0(sp)	# POP RA FROM STACK
add sp, 0x04
j ra
nop
.end writePixels	




.ent sendPixel
sendPixel:
sub sp, 0x04  # PUSH RA ONTO STACK
sw ra, 0(sp)
la t3, LATF		# Load address of PORTF latch - was LARB
lw t2, 0(t3)		# Copy PORTB data to t2
la t3, PORTF		# Now load address of PORTF output data register - was PORTB
li t0, 0
nop

addiu t4,a0,0       # Copy data word

BITLOOP:
ori t2, t2, 0x0001	# SET OUTPUT PIN HIGH
sw t2, 0(t3)

                    # IS BIT A '1' OR A '0'?
and t8,t4,0x800000  # Test data bit with mask
beqz t8,DUTYZERO    # If it is a '0', initialize duty cycle to send a 0

DUTYONE:            # DUTY CYCLE FOR A '1'
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop

nop
nop
nop
nop
nop
nop
nop
nop
nop
nop

nop
nop
nop
nop
nop
nop
nop
nop
nop
nop

nop
nop
nop
nop
nop
nop
nop
nop
nop
nop

nop
nop
nop
nop
nop
nop
nop
nop
nop
nop

nop
nop
nop
nop
nop
nop
nop
andi t2, t2, 0xFFFE 	# SET OUTPUT PIN LOW
sw t2, 0(t3)
nop
nop
nop
j ENDLOOP
nop

DUTYZERO:               # DUTY CYCLE FOR A '0'
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop

nop
nop
nop
nop
nop
nop
nop
nop
nop
nop


andi t2, t2, 0xFFFE 	# SET OUTPUT PIN LOW
sw t2, 0(t3)
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop

nop
nop
nop
nop
nop
nop
nop
nop
nop
nop

nop
nop
nop
nop
nop
nop
nop
nop
nop
nop

nop
nop
nop
nop
nop
nop
nop
nop
nop
nop

nop
nop
nop
nop
nop
nop
nop
nop
nop
nop

nop
nop
nop
nop
nop
nop


ENDLOOP:
addiu t0,t0,1
beq t0, NUMBER_OF_BITS, ENDPIXEL
sll t4,t4,1     # Otherwise, more bits to do, so shift data word up to test next bit
j BITLOOP
nop
ENDPIXEL:
lw ra, 0(sp)	# POP RA FROM STACK
add sp, 0x04
j ra
nop
.end sendPixel


.ent wasteTime
wasteTime:
sub sp, 0x04  # PUSH RA ONTO STACK
sw ra, 0(sp)
addiu t5,a0,0
li t6,0
LOOPY:
addiu t6,t6,1
bne t6,t5,LOOPY
lw ra, 0(sp)	# POP RA FROM STACK
add sp, 0x04
j ra
nop
.end wasteTime