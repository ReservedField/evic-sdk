/* Library configurations */
INPUT(libevicsdk_fpu_crt0.o)
GROUP(
	armv7e-m/fpu/libgcc.a
	armv7e-m/fpu/libc.a
	armv7e-m/fpu/libm.a
	libevicsdk_fpu.a
)

INCLUDE common.ld
