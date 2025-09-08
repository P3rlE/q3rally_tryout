#include "vm_local.h"

/*
 * Minimal stub for ARM64 JIT support.
 * Currently falls back to the interpreted VM since the
 * full AArch64 backend has not yet been implemented.
 */

static void VM_Destroy_Compiled( vm_t *vm ) {
    (void)vm;
}

void VM_Compile( vm_t *vm, vmHeader_t *header ) {
    (void)header;
    Com_Printf("arm64 VM JIT not implemented, falling back to interpreter\n");
    vm->destroy = VM_Destroy_Compiled;
    vm->compiled = qfalse;
}

int VM_CallCompiled( vm_t *vm, int *args ) {
    return VM_CallInterpreted( vm, args );
}

