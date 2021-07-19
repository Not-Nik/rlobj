# rlobj

rlobj is a drop-in replacement for raylib's obj loader, which uses an outdated version of tinyobj-loader-c.

## Why a new loader?

As mentioned above the old OBJ loader is outdated, because some design decisions in its newer versions
make integration into raylib complicated. The old, currently in-use version has some bugs regarding
material assignment and multiple objects within one OBJ file, which are fixed in this loader. Additionally,
this loader speeds loading up by around 2.7 times, likely because it doesn't bother reading
anything raylib can't use, instead of being as compliant to the standard as possible.

# Licensing

This software is subject to terms of the Mozilla Public License, v. 2.0, with exemptions for
[Ramon Santamaria and the raylib contributors](https://github.com/raysan5/raylib/graphs/contributors)
who may, at their discretion, instead license any of the Covered Software under the [zlib license](https://en.wikipedia.org/wiki/Zlib_License).
