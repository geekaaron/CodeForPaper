global-incdirs-y += include
srcs-y += seg_with_optee_ta.c poly.c queue.c mymath.c game.c crypto.c attestation.c nonce.c

# To remove a certain compiler flag, add a line like this
#cflags-template_ta.c-y += -Wno-strict-prototypes
