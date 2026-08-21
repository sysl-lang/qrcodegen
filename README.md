# qrcodegen

QR Code generation for [sysl](https://github.com/sysl-lang/sysl), bound to
[Project Nayuki's qrcodegen](https://www.nayuki.io/page/qr-code-generator-library) — the whole of QR
Code Model 2 in a thousand lines of C: all forty versions, all four correction levels, all four
encoding modes, and the mask selection.

**This package asks the machine for nothing.** There is no `requires` block — no allocator, no
operating system, no file system. Unlike every other binding in this organisation that can say that,
it took no doing at all: qrcodegen never allocates, because every entry point is handed the buffers
it will write into. That is what makes it the easiest thing here to put on a microcontroller.

```
sh/sysl/qrcodegen/
    qrcodegen.sysl      the binding
    tests.sysl          12 tests, two of them whole symbols compared against other encoders
    c/
        c.sysl          the header as C declares it, and the numbers it holds
        tests.sysl      14 tests over the raw layer, which are the API survey's probes kept
        shim.c          the two things sysl cannot reach directly
        shim.h          so that both languages read one declaration of them
        qrcodegen.c     vendored qrcodegen, unmodified — nayuki/QR-Code-generator, MIT
        qrcodegen.h
package.hocon           who this package is, and what it needs of the machine
```

The module is **`sh.sysl.qrcodegen`**, and the directories are that name: a dotted module name
mirrors its path from the library root. The prefix is the reverse-DNS of `sysl.sh`, so that a package
claims a name nobody else will mint rather than the top-level word `qrcodegen`.

**Everything that is C lives in `c/`**, module `sh.sysl.qrcodegen.c` — the two upstream files, the
shim, and the sysl declarations over them. That is the two-layer shape every binding in this
organisation uses, and the point of it is that the two halves have different jobs: the `c` layer has
to be *faithful*, because a signature that disagrees with the header links perfectly and corrupts the
call at run time, and the layer above it has to be *pleasant*, which is a different question that
would otherwise be answered in the same breath. A program imports `sh.sysl.qrcodegen`.

## Using it

Name it in your project's `package.hocon` and `sysl build` fetches it:

```hocon
dependencies {
  qrcodegen { git = "github.com/sysl-lang/qrcodegen", version = "0.1.0" }
}
```

The coordinate is an identity rather than a URL, so it carries no `https://`, and `version` is the
tag `v0.1.0` here.

Or point at it directly, which needs no fetching and is what this repository's own tests do:

```
sysl build-lib . -o /tmp/qrcodegen.syslib
sysl run yourprogram.sysl --lib /tmp/qrcodegen.syslib

sysl run yourprogram.sysl --lib /path/to/this/repo
```

## The surface

```sysl
enum Ecc
    Low                     // about  7% of the codewords may be corrupt
    Medium                  //     15%
    Quartile                //     25%
    High                    //     30%

enum Mask
    Auto                    // build it eight times and keep the best-scoring
    Pattern(n: int)         // force one, 0 to 7

enum Error
    Short(needed: usize)    // a slice was too small; this is what would do
    TooLong                 // will not fit any version in the range at that level
    InteriorNul(at: usize)  // a zero byte, which mode selection cannot see past
    BadVersion              // outside 1 to 40, or min above max
    BadMask                 // a Pattern outside 0 to 7

struct Qr
    size(self) -> int                       // 21 to 177, always odd
    dark(self, x: int, y: int) -> bool      // light outside the symbol, never an error
    version(self) -> int                    // 1 to 40

buffer_len(version: int) -> usize
scratch_len(payload: usize, max_version: int) -> usize

encode(text: string, scratch: []u8, out: []u8, ecl: Ecc = Medium,
       min_version: int = 1, max_version: int = 40,
       mask: Mask = Auto, boost_ecl: bool = true) -> Result[Qr, Error]

encode_bytes(data: []const u8, …) -> Result[Qr, Error]
encode_binary(data: []const u8, …) -> Result[Qr, Error]
```

**Every call is the same shape: ask how big the answer can be, hand over slices that size, and get
back a view of the one that matters.** Nothing here allocates, so nothing here has to be freed.

`out` is where the symbol goes and needs `buffer_len(max_version)` bytes — 3918 for a symbol of any
size the standard defines, which is the figure that decides whether this fits on a microcontroller. A
payload that will never need a large symbol asks for far less: `buffer_len(4)` is 138, and version 4
holds a sixty-character URL at medium correction.

`scratch` is working space and holds nothing afterwards. It is a second buffer rather than a second
use of `out` because the encoder needs both at once, and it is slightly larger than `out` because the
payload is copied into the front of it — `scratch_len` says how much.

## An example

A package has no way to carry a program of its own — everything under the package root is compiled
*into* the library — so the demo lives here (`design/packages.md § Open h`):

```sysl
import sh.sysl.qrcodegen.*

var scratch: []u8 = [0; 4096]
var out: []u8 = [0; 4096]

val qr = encode("https://sysl.sh", scratch, out).expect("a URL encodes")

print(f"version ${qr.version()}, ${qr.size()} modules square")

// The standard wants four light modules of margin around a symbol, and `dark` answers light outside
// its own bounds -- so the quiet zone needs no special case, only a wider sweep.
for y in -4..<qr.size() + 4
    var line = ""

    for x in -4..<qr.size() + 4
        line += if qr.dark(x, y) then "██" else "  "

    print(line)
```

```
version 2, 25 modules square




        ██████████████  ██  ████████    ██  ██████████████
        ██          ██  ████  ████  ██████  ██          ██
        ██  ██████  ██  ████      ████  ██  ██  ██████  ██
        ██  ██████  ██  ██████  ████████    ██  ██████  ██
        ██  ██████  ██  ██████████  ██      ██  ██████  ██
        ██          ██      ████████        ██          ██
        ██████████████  ██  ██  ██  ██  ██  ██████████████
                        ██████████████  ██
          ████  ██  ████    ██  ██  ██████  ██  ██████████
          ████          ██  ██████  ████    ██          ██
          ██      ████    ██    ████    ██    ██    ██████
          ██    ██        ██  ████    ██      ██      ██
            ██      ██  ████████      ██████████  ██  ████
            ████████      ██            ██  ██    ██    ██
        ██  ██  ██  ██    ████████  ██      ████    ██████
          ██████████    ██  ████            ██  ██    ██
        ██    ██    ██████████  ██████  ████████████
                        ██      ██████  ██      ████  ████
        ██████████████  ████      ██  ████  ██  ████  ████
        ██          ██    ██████    ██  ██      ████    ██
        ██  ██████  ██  ██    ██  ██    ████████████  ██
        ██  ██████  ██      ████      ████    ██████████
        ██  ██████  ██  ████  ██████  ██        ██      ██
        ██          ██  ██    ██    ██████  ██  ████  ██
        ██████████████      ██  ██      ██  ████      ████
```

Scan it. It is a real QR code, and the terminal is the cheapest display there is.

## Three ways in, and which one you want

**`encode` is the one to reach for.** It reads the payload and picks the densest of the standard's
modes that will carry it: digits at ten bits per three, upper-case alphanumeric at eleven bits per
two, everything else a byte each. Forty digits go in a version 1 symbol through `encode` and need a
version 2 as bytes, which is the whole reason mode selection is worth reaching.

**`encode_bytes` is the same call for a payload you happen to be holding as a slice** rather than as a
`string`. It still reads them as text and still picks a mode.

**`encode_binary` forces byte mode and is told the length**, so a zero byte is ordinary payload. That
is the difference that matters: mode selection runs over a NUL-terminated string, so `encode` and
`encode_bytes` *refuse* a payload containing a zero rather than silently encoding the prefix before
it — `InteriorNul(at)`, with the offset. A compressed record, a key, a CBOR document goes through
`encode_binary`.

## Why there is a shim at all, when nothing needs wrapping

Thirteen of the fifteen functions in the raw layer are the upstream symbol named directly, which is
unusual here. Two are not:

**`qrcodegen_BUFFER_LEN_FOR_VERSION(n)` is a function-like macro**, so there is no symbol for an
`extern` to name and a `c const` block can only read a constant rather than call one. That is
`15 §7`'s second C-only shape. It matters because it is how a caller learns how much room to hand
over, so restating its arithmetic in sysl would leave the buffer size agreeing with the encoder's
idea of it only by luck.

**`qrcodegen_encodeText` takes a `const char *`**, and a sysl `string` carries a length and has no
terminator (`04 § Passing one to C`), so converting one allocates. An allocation there would cost
this package the one property that makes it interesting. The wrapper takes a pointer and a length
instead, and finds the terminator's byte inside scratch the caller already owns: it splits `scratch`
in two and puts the NUL-terminated copy in front of the temporary buffer the encoder wants. That is
why `scratch_len` is `payload + 1 + buffer_len(max_version)` rather than just the last term.

## What the API survey found

`~/dev/sysl-lang/CLAUDE.md` asks for a survey of a C library's whole surface before any of the
binding is written, sorted into what sysl expresses directly, what needs a shim, and what would need
the language to move. **The third bucket came out empty**, which is the first time in this
organisation that a binding has had nothing to report — and two of the answers were probes rather
than readings:

- **A `c const` block reads an `enum` constant, including a negative one.** Every other package here
  asks for `#define`s; qrcodegen's three enumerations are C enums, and `qrcodegen_Mask_AUTO` is `-1`,
  which a block that read it as unsigned would have wrapped to 255 and quietly turned into "mask 7".
  `c/tests.sysl` pins all three tables against the values ISO/IEC 18004 states.
- **A struct crosses by value in both directions.** `qrcodegen_makeNumeric` returns a 24-byte
  `qrcodegen_Segment` — over AArch64's 16-byte limit, so through `sret` — and `encodeSegments` takes
  an array of them straight back into C. A layout sysl and clang disagreed about would compile, link,
  and give a wrong answer with no diagnostic anywhere, so it is tested rather than assumed.

The four standing questions the survey asks came out: **nobody owns the memory** (there is no
`malloc` in the library at all); **struct-by-value is used and works**; **there are no compile-time
options**, so no ABI to get wrong; and the capability it needs is **none**.

## Testing

```
sysl test .
```

**The two strongest tests compare a whole symbol, module for module, against other implementations of
the standard.** The reference grids in `tests.sysl` are what both
[segno](https://pypi.org/project/segno/) and [qrcode](https://pypi.org/project/qrcode/) produce at a
fixed version, correction level and mask — two Python encoders sharing no code with qrcodegen or with
each other. A binding that passed the wrong correction level, mis-ordered the mode indicators, or let
a mask constant wrap would still produce a plausible-looking symbol, and every structural test would
still pass.

**There are two references because the first pair disagreed.** Sixteen digits is exactly the numeric
capacity of a version 1 quartile symbol, and there segno produces a different grid from the other
two; at fifteen digits all three agree. Which of them is right is segno's question rather than this
package's — what settled it was asking a third encoder, and the lesson worth keeping is that a single
oracle is an opinion.

The rest of the suite is about the seam. **qrcodegen is handed pointers and is never told how long
either slice is**, so every bound in this package is one nothing underneath it would notice — a slice
a byte too short is not a refusal in C, it is a write past the end. Both sizes are therefore asserted
from both sides: the exact figure works, one byte less is `Short` carrying the figure that would have
done.

`c/tests.sysl` is the API survey's probe pass, kept rather than discarded. Its constants are asserted
against what the standard says, not against what the header says — a test that reads a constant and
compares it to itself passes whatever the header holds.

## Licence

Two things under two licences — `qrcodegen.c` and `qrcodegen.h` are MIT and Project Nayuki's,
everything else is ISC and this repository's. See [LICENSE](LICENSE).
