// rlobj (c) Nikolas Wipper 2021

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0, with exemptions for Ramon Santamaria and the
 * raylib contributors who may, at their discretion, instead license
 * any of the Covered Software under the zlib license. If a copy of
 * the MPL was not distributed with this file, You can obtain one
 * at https://mozilla.org/MPL/2.0/. */

#ifndef RLOBJ_LIBRARY_H
#define RLOBJ_LIBRARY_H

#include <raylib.h>

#ifdef __cplusplus
extern "C" {
#endif

// Naming is important to avoid linker errors
// raylib has a LoadOBJ
Model LoadObj(const char *filename);

#ifdef __cplusplus
};
#endif

#endif //RLOBJ_LIBRARY_H
