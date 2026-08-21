/* The two things in qrcodegen's interface a sysl caller cannot reach directly.
 *
 * The first is `qrcodegen_BUFFER_LEN_FOR_VERSION(n)`, a *function-like* macro,
 * which is `15 §7`'s second C-only shape: there is nothing for an `extern` to
 * name, and a `c const` block can only read a constant rather than call one.
 * Restating the arithmetic in sysl would work and is exactly the transcription
 * this arrangement exists to avoid -- the buffer size a caller must supply would
 * then agree with the encoder's idea of it only by luck.
 *
 * The second is `qrcodegen_encodeText`, which is reachable but only from a
 * caller holding a NUL-terminated string. A sysl `string` carries a length and
 * has no terminator (`04 §Passing one to C`), so the conversion allocates -- and
 * an allocation here would cost this package the one property that makes it
 * interesting, that it can be built for a target with no heap. The wrapper takes
 * a pointer and a length instead and finds the terminator's byte inside scratch
 * the caller already owns.
 */
#ifndef SYSL_QRCODEGEN_SHIM_H
#define SYSL_QRCODEGEN_SHIM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

size_t sysl_qrcodegen_buffer_len(int version);

bool sysl_qrcodegen_encode_auto(const uint8_t *data, size_t len,
                                uint8_t *scratch, size_t scratch_len,
                                uint8_t *qrcode, int ecl,
                                int min_version, int max_version,
                                int mask, bool boost_ecl);

#endif
