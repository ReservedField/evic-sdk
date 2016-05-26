/* Library configurations */
INPUT(libevicsdk_crt0.o)
GROUP(
	armv7e-m/libgcc.a
	armv7e-m/libc.a
	armv7e-m/libm.a
	libevicsdk.a
)

INCLUDE common.ld
