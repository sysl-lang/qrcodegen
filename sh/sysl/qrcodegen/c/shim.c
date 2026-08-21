/* The whole shim, and it is two functions.
 *
 * qrcodegen is the smallest binding surface in this organisation for the reason
 * the package exists to demonstrate: it never allocates. Every entry point takes
 * the buffers it will write into, so the ownership question that shapes most
 * bindings here -- qoi's arena, regex's `free` method -- simply does not arise,
 * and everything else in the header is scalars, pointers and enums that sysl
 * declares directly.
 *
 * What is left is a function-like macro, which has no symbol, and one entry
 * point whose `const char *` would have cost a caller an allocation. Both are
 * below, and neither reimplements anything upstream does.
 */

#include <string.h>

#include "qrcodegen.h"
#include "shim.h"

/* Answers 0 for a version outside the standard's 1..40 rather than computing a
 * meaningless size from it. The macro's own contract requires the range and says
 * nothing about what it does outside it, so the check belongs on this side; the
 * sysl layer turns the 0 into a refusal a caller can read.
 */
size_t sysl_qrcodegen_buffer_len(int version) {
	if (version < qrcodegen_VERSION_MIN || version > qrcodegen_VERSION_MAX)
		return 0;

	return (size_t)qrcodegen_BUFFER_LEN_FOR_VERSION(version);
}

/* `qrcodegen_encodeText` with a pointer and a length instead of a terminator.
 *
 * **Why this exists rather than a `cstring` on the sysl side.** `encodeText` is
 * the only entry point that chooses the encoding mode for you -- digits go in
 * numeric mode at ten bits per three, an upper-case payload in alphanumeric at
 * eleven bits per two, and everything else in byte mode -- so skipping it costs
 * a numeric payload three times the space. Reaching it needs a NUL, a sysl
 * `string` has none, and `cstring` allocates one. This package would then need a
 * heap for its most ordinary call, which is the whole thing it is trying not to
 * need.
 *
 * **The terminator's byte comes out of the caller's scratch, which has room for
 * it by construction.** `encodeText` wants a temporary buffer of
 * `BUFFER_LEN_FOR_VERSION(max_version)` bytes that must not overlap either the
 * text or the output; this splits `scratch` in two and puts the NUL-terminated
 * copy in front of it. So the caller supplies `len + 1 + buffer_len(max)` rather
 * than `buffer_len(max)`, and the sysl layer computes that figure with the same
 * two functions rather than restating it.
 *
 * An interior NUL is *not* checked here. `strlen` would simply stop early and
 * encode a prefix, which is the silent truncation `04 §Passing one to C` warns
 * about -- so the sysl layer refuses it by name before the call, where a caller
 * can be told which byte was the problem.
 */
bool sysl_qrcodegen_encode_auto(const uint8_t *data, size_t len,
                                uint8_t *scratch, size_t scratch_len,
                                uint8_t *qrcode, int ecl,
                                int min_version, int max_version,
                                int mask, bool boost_ecl) {
	size_t needed = sysl_qrcodegen_buffer_len(max_version);

	if (needed == 0)
		return false;

	if (scratch_len < len + 1 + needed)
		return false;

	/* `memcpy` is undefined for a null source even with a zero length, and an
	 * empty payload is a perfectly good QR code -- so the copy is skipped
	 * rather than made conditional inside it.
	 */
	if (len > 0)
		memcpy(scratch, data, len);

	scratch[len] = 0;

	return qrcodegen_encodeText((const char *)scratch, scratch + len + 1, qrcode,
	                            (enum qrcodegen_Ecc)ecl, min_version, max_version,
	                            (enum qrcodegen_Mask)mask, boost_ecl);
}
