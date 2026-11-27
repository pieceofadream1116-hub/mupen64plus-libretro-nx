/* The Xbox app runs the N64 CPU in the cached interpreter (no dynarec on UWP yet): the dynarec's entry points are
 * still named by the core, but never called. */
#include <stdint.h>

struct r4300_core;

void dyna_jump(void) {}
void dyna_stop(struct r4300_core *r4300) { (void)r4300; }
void dynarec_jump_to(struct r4300_core *r4300, uint32_t address) { (void)r4300; (void)address; }
