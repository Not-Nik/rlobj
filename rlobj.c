// rlobj (c) Nikolas Wipper 2021

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0, with exemptions for Ramon Santamaria and the
 * raylib contributors who may, at their discretion, instead license
 * any of the Covered Software under the zlib license. If a copy of
 * the MPL was not distributed with this file, You can obtain one
 * at https://mozilla.org/MPL/2.0/. */

#include "rlobj.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

typedef struct Edge { int vertex; int texcoord; int normal; } Edge;
typedef struct Face { Edge edges[3]; } Face;

typedef struct OBJMesh {
    Mesh mesh;
    unsigned long mat_hash;
} OBJMesh;

typedef struct OBJMat {
    char *base;

    Vector3 ambient, diffuse, specular;
    float opacity;
    char *ambient_map, *diffuse_map, *specular_map, *highlight_map, *alpha_map, *bump_map, *displacement_map, *decal_map;
    unsigned long name_hash;
} OBJMat;

typedef struct ValidFloat { float f; bool valid; } ValidFloat;
typedef struct ValidInt { int i; bool valid; } ValidInt;
typedef struct ValidEdge { Edge e; bool valid; } ValidEdge;
typedef struct ValidVec3 { Vector3 v; bool valid; } ValidVec3;

typedef struct GenericFile {
    char *data;
} GenericFile;

typedef struct OBJFile {
    char *base;

    GenericFile data;
    Vector3 *vertices, *normals;
    Vector2 *texcoords;
    Face *faces;

    int vertex_count;
    int texcoord_count;
    int normal_count;
    int face_count;

    OBJMat *mats;
    int mat_count;
} OBJFile;

unsigned long hash(unsigned char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

char *PutStringOnHeap(const char *string) {
    size_t string_len = strlen(string);
    if (string_len == 0) return NULL;
    char *res = RL_CALLOC(strlen(string) + 1, sizeof(string));
    strcpy(res, string);
    return res;
}

char *AddBase(char *path, const char *base) {
    size_t path_len = strlen(path), base_len = strlen(base);
    path = RL_REALLOC(path, sizeof(char) * (path_len + base_len + 2));
    memmove(path + base_len + 1, path, path_len + 1);
    memcpy(path, base, base_len);
    path[base_len] = '/';
    return path;
}

// Generic reader functions

void IgnoreLine(GenericFile *file) {
    for (; *file->data != '\n' && *file->data != '\0'; file->data++);
    file->data++;
}

// Clears all whitespaces
// Returns false if the line ended
// This helps prevent reading two lines as one statement
bool ClearWhitespace(GenericFile *file) {
    for (; *file->data == '\n' || isspace(*file->data);) {
        if (*file->data == '\n') { // Don't read over unexpected line break, this prevents invalidation of the next line
            return false;
        }
        file->data++;
    }
    return true;
}

char *ReadName(GenericFile *file) {
    ClearWhitespace(file);

    char *data_cpy = file->data;
    for (; *data_cpy != '\n' && *data_cpy != '\0'; data_cpy++);
    char *name = RL_CALLOC(data_cpy - file->data + 1, sizeof(char));

    int i;
    for (i = 0; !isspace(file->data[i]) && file->data[i] != '\0'; i++) {
        name[i] = file->data[i];
    }
    file->data += i;
    return name;
}

float ReadFloat(GenericFile *file) {
    float res = 0.f, fact = 1.f;
    bool point_seen = false;

    if (*file->data == '-') {
        file->data++;
        fact = -1;
    }

    for (; *file->data; file->data++) {
        if (*file->data == '.') {
            point_seen = true;
            continue;
        }

        unsigned char c = *file->data - '0';
        if (c > 9) break; // same as isdigit(*file->data), but faster because we can use c later
        if (point_seen) fact /= 10.f;
        res = res * 10.f + (float) c;
    }
    return res * fact;
}

ValidFloat ReadValidFloat(GenericFile *file) {
    if (!ClearWhitespace(file))
        return (ValidFloat) {.f = 0.f, .valid = false};
    float vX = ReadFloat(file);
    return (ValidFloat) {.f = vX, .valid = true};
}

float ReadFloatDefault(GenericFile *file, float def) {
    if (ClearWhitespace(file) && isdigit(*file->data)) {
        def = ReadFloat(file);
    }
    return def;
}

int ReadInt(GenericFile *file) {
    int res = 0;
    for (; *file->data; file->data++) {
        unsigned char c = *file->data - '0';
        if (c > 9) break; // same as isdigit(*file->data), but faster because we can use c later

        res = res * 10 + c;
    }
    return res;
}

ValidInt ReadValidInt(GenericFile *file) {
    if (!ClearWhitespace(file))
        return (ValidInt) {.i = 0, .valid = false};
    int i = ReadInt(file);
    return (ValidInt) {.i = i, .valid = true};
}

int ReadIntDefault(GenericFile *file, int def) {
    if (ClearWhitespace(file) && isdigit(*file->data)) {
        def = ReadInt(file);
    }
    return def;
}

ValidVec3 ReadColor(GenericFile *file) {

    ValidFloat r = ReadValidFloat(file);
    ValidFloat g = ReadValidFloat(file);
    ValidFloat b = ReadValidFloat(file);

    if (r.valid && g.valid && b.valid)
        return (ValidVec3) {.v = {.x = r.f, .y = g.f, .z = b.f}, .valid = true};

    return (ValidVec3) {.v = {0}, .valid = false};
}

OBJMat LoadMtlMat(GenericFile *file) {
    bool seen_newmtl = false;

    OBJMat mat = {0};
    mat.opacity = 1.f;

    while (*file->data) {
        if (*file->data == 'K') {
            file->data++;
            if (*file->data == 'a') {
                file->data++;
                ValidVec3 a = ReadColor(file);
                if (a.valid) mat.ambient = a.v;
            } else if (*file->data == 'd') {
                file->data++;
                ValidVec3 d = ReadColor(file);
                if (d.valid) mat.diffuse = d.v;
            } else if (*file->data == 's') {
                file->data++;
                ValidVec3 s = ReadColor(file);
                if (s.valid) mat.specular = s.v;
            }
        } else if (*file->data == 'd') {
            file->data++;
            ValidFloat o = ReadValidFloat(file);
            if (o.valid) mat.opacity = o.f;
            if (mat.opacity > 1) mat.opacity = 1;
        } else if (strncmp(file->data, "newmtl", 6) == 0) {
            if (seen_newmtl) break;
            seen_newmtl = true;
            file->data += 6;

            char *name = ReadName(file);
            mat.name_hash = hash((unsigned char *) name);
            RL_FREE(name);
        } else if (strncmp(file->data, "map_", 4) == 0) {
            file->data += 4;

            if (*file->data == 'K') {
                file->data++;
                if (*file->data == 'a') {
                    file->data++;
                    mat.ambient_map = ReadName(file);
                } else if (*file->data == 'd') {
                    file->data++;
                    mat.diffuse_map = ReadName(file);
                } else if (*file->data == 's') {
                    file->data++;
                    mat.specular_map = ReadName(file);
                }
            } else if (*file->data == 'd') {
                file->data++;
                mat.alpha_map = ReadName(file);
            } else if (strncmp(file->data, "Ns", 2) == 0) {
                file->data += 2;
                mat.highlight_map = ReadName(file);
            } else if (strncmp(file->data, "bump", 4) == 0) {
                file->data += 4;
                mat.bump_map = ReadName(file);
            }
        } else if (strncmp(file->data, "bump", 4) == 0) {
            file->data += 4;
            mat.bump_map = ReadName(file);
        } else if (strncmp(file->data, "disp", 4) == 0) {
            file->data += 4;
            mat.displacement_map = ReadName(file);
        } else if (strncmp(file->data, "decal", 5) == 0) {
            file->data += 5;
            mat.decal_map = ReadName(file);
        }
        IgnoreLine(file);
    }

    return mat;
}

char *ReadMtl(OBJFile *file, char *filename) {
    if (file->base) filename = AddBase(filename, file->base);
    char *data = LoadFileText(filename);
    if (!data) { return filename; }

    GenericFile mtl = (GenericFile) {.data = data};
    char *base = GetPrevDirectoryPath(filename);

    while (*mtl.data) {
        file->mats = (OBJMat *) RL_REALLOC(file->mats, sizeof(OBJMat) * ++file->mat_count);
        file->mats[file->mat_count - 1] = LoadMtlMat(&mtl);
        file->mats[file->mat_count - 1].base = PutStringOnHeap(base);
    }

    UnloadFileText(data);
    return filename;
}

// OBJ specific reader functions

void ReadVertex(OBJFile *file) {
    ValidFloat vX = ReadValidFloat(&file->data);
    ValidFloat vY = ReadValidFloat(&file->data);
    ValidFloat vZ = ReadValidFloat(&file->data);

    if (!(vX.valid && vY.valid && vZ.valid)) { return; }

    ReadFloatDefault(&file->data, 1.f); // value ignored

    file->vertices = (Vector3 *) RL_REALLOC(file->vertices, sizeof(Vector3) * ++file->vertex_count);
    int vc = file->vertex_count - 1;
    file->vertices[vc].x = vX.f;
    file->vertices[vc].y = vY.f;
    file->vertices[vc].z = vZ.f;
}

void ReadTextureCoord(OBJFile *file) {
    ValidFloat tU = ReadValidFloat(&file->data);

    float tV = ReadFloatDefault(&file->data, 0.f);
    ReadFloatDefault(&file->data, 0.f); // value ignored

    if (!tU.valid) { return; }

    file->texcoords = (Vector2 *) RL_REALLOC(file->texcoords, sizeof(Vector2) * ++file->texcoord_count);
    int tc = file->texcoord_count - 1;
    file->texcoords[tc].x = tU.f;
    file->texcoords[tc].y = tV;
}

void ReadNormal(OBJFile *file) {
    ValidFloat nX = ReadValidFloat(&file->data);
    ValidFloat nY = ReadValidFloat(&file->data);
    ValidFloat nZ = ReadValidFloat(&file->data);

    if (!(nX.valid && nY.valid && nZ.valid)) { return; }

    file->normals = (Vector3 *) RL_REALLOC(file->normals, sizeof(Vector3) * ++file->normal_count);
    int nc = file->normal_count - 1;
    file->normals[nc].x = nX.f;
    file->normals[nc].y = nY.f;
    file->normals[nc].z = nZ.f;
}

#define InvalidEdge (ValidEdge){{}, false}

ValidEdge ReadEdge(GenericFile *file) {
    Edge e = {0};

    ValidInt v = ReadValidInt(file);
    if (!v.valid) { return InvalidEdge; }
    e.vertex = v.i;
    if (*file->data == '/') {
        file->data++;
        int vt = ReadIntDefault(file, 0);
        if (*file->data == '/') {
            file->data++;
            ValidInt vn = ReadValidInt(file);
            if (!vn.valid) {
                return InvalidEdge;
            }

            e.texcoord = vt == 0 ? 1 : vt; // a 0 index isn't valid, but not setting the value is
            e.normal = vn.i;
        } else {
            if (vt == 0) {  // a 0 index isn't valid and a single '/' without a value isn't either
                return InvalidEdge;
            }
            e.texcoord = 1;
            e.normal = vt;
        }
    }

    return (ValidEdge) {e, true};
}

void ReadFace(OBJFile *file) {
    Face f;

    for (int i = 0; i < 3; i++) {
        ValidEdge vE = ReadEdge(&file->data);
        if (vE.valid)
            f.edges[i] = vE.e;
        else return;
    }

    file->faces = (Face *) RL_REALLOC(file->faces, sizeof(Face) * ++file->face_count);
    int fc = file->face_count - 1;
    file->faces[fc] = f;

    ClearWhitespace(&file->data);

    // Naïve triangulation
    while (isdigit(*file->data.data)) {
        TraceLog(LOG_WARNING, "Triangulation is only very basic. Try doing that in your modeling software.");
        file->faces = (Face *) RL_REALLOC(file->faces, sizeof(Face) * ++file->face_count);
        f.edges[1] = f.edges[2];

        ValidEdge vE = ReadEdge(&file->data);
        if (vE.valid) {
            f.edges[2] = vE.e;
            f.edges[2] = vE.e;
            file->faces[++fc] = f;
        } else return;
        ClearWhitespace(&file->data);
    }
}

OBJMesh LoadObjMesh(OBJFile *file) {
    unsigned long mat_hash;
    bool seen_o = false;

    while (*file->data.data) {
        if (*file->data.data == 'v') {
            if (isspace(*(++file->data.data))) {
                file->data.data++;
                ReadVertex(file);
            } else if (*file->data.data == 't') {
                file->data.data++;
                ReadTextureCoord(file);
            } else if (*file->data.data == 'n') {
                file->data.data++;
                ReadNormal(file);
            }
        } else if (*file->data.data == 'f') {
            file->data.data++;
            ReadFace(file);
        } else if (*file->data.data == 'o') {
            if (seen_o) break;
            seen_o = true;
        } else if (strncmp(file->data.data, "usemtl", 6) == 0) {
            file->data.data += 6;
            char *name = ReadName(&file->data);
            mat_hash = hash((unsigned char *) name);
            RL_FREE(name);
        } else if (strncmp(file->data.data, "mtllib", 6) == 0) {
            file->data.data += 6;
            char *name = ReadName(&file->data);
            RL_FREE(ReadMtl(file, name));
        }
        IgnoreLine(&file->data);
    }

    Mesh m = (Mesh) {0};
    if (file->vertex_count != 0) {
        if (!file->texcoord_count || !file->texcoords) {
            file->texcoords = (Vector2 *) RL_CALLOC(file->vertex_count, sizeof(Vector2)); // all texcoords will be 0,0
        }
        if (!file->normal_count || !file->normals) {
            file->normals = (Vector3 *) RL_CALLOC(file->vertex_count, sizeof(Vector3)); // all normals will be 0,0,0
        }

        m.vertexCount = file->face_count * 3;
        m.triangleCount = (int) file->face_count;

        m.vertices = (float *) RL_CALLOC(m.vertexCount * 3, sizeof(float));
        m.texcoords = (float *) RL_CALLOC(m.vertexCount * 2, sizeof(float));
        m.normals = (float *) RL_CALLOC(m.vertexCount * 3, sizeof(float));

        // Sort vertices, texcoords and normals
        for (int i = 0; i < file->face_count; i++) {
            for (int j = 0; j < 3; j++) {
                int vIn = file->faces[i].edges[j].vertex - 1;
                int tIn = file->faces[i].edges[j].texcoord - 1;
                int nIn = file->faces[i].edges[j].normal - 1;

                // Three vertices per face, three floats per vertex
                m.vertices[i * 9 + j * 3] = file->vertices[vIn].x;
                m.vertices[i * 9 + j * 3 + 1] = file->vertices[vIn].y;
                m.vertices[i * 9 + j * 3 + 2] = file->vertices[vIn].z;

                // Three coords per face, two floats per coords
                m.texcoords[i * 6 + j * 2] = file->texcoords[tIn].x;
                m.texcoords[i * 6 + j * 2 + 1] = file->texcoords[tIn].y;

                // Three normals per face, three floats per normal
                m.normals[i * 9 + j * 3] = file->normals[nIn].x;
                m.normals[i * 9 + j * 3 + 1] = file->normals[nIn].y;
                m.normals[i * 9 + j * 3 + 2] = file->normals[nIn].z;
            }
        }
    }
    return (OBJMesh) {.mesh = m, .mat_hash = mat_hash};
}

Color Vector3ToColor(Vector3 vec, float opacity) {
    return (Color) {
        .r = (char) (vec.x * 255.f), .g = (char) (vec.y * 255.f), .b = (char) (vec.z * 255.f), .a = (char) (opacity * 255.f)
    };
}

Texture LoadTextureBase(char *filename, char *base) {
    Texture res = {0};
    if (filename) {
        if (base) filename = AddBase(filename, base);
        res = LoadTexture(filename);
    }
    RL_FREE(filename);
    return res;
}

// Wrapper around read LoadObjMesh and LoadMtlMat
// Loads model without uploading meshes
Model LoadObjDry(const char *filename) {
    char *data = LoadFileText(filename);
    if (!data) return (Model) {0};

    OBJMesh *meshes = NULL;
    int mesh_count = 0;

    OBJFile file = (OBJFile) {0};
    file.data.data = data;
    file.base = PutStringOnHeap(GetPrevDirectoryPath(filename));

    while (*file.data.data) {
        meshes = (OBJMesh *) RL_REALLOC(meshes, sizeof(OBJMesh) * ++mesh_count);
        meshes[mesh_count - 1] = LoadObjMesh(&file);
    }

    UnloadFileText(data);
    RL_FREE(file.vertices);
    RL_FREE(file.texcoords);
    RL_FREE(file.normals);
    RL_FREE(file.faces);

    Model model = {0};

    model.transform = (Matrix) {
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f
    };
    model.meshes = (Mesh *) RL_CALLOC(mesh_count, sizeof(Mesh));
    model.meshCount = mesh_count;
    model.materialCount = file.mat_count;
    model.materials = RL_CALLOC(file.mat_count, sizeof(Material));
    model.meshMaterial = RL_CALLOC(mesh_count, sizeof(int));

    for (int i = 0; i < file.mat_count; i++) {
        Material m = LoadMaterialDefault();
        OBJMat names = file.mats[i];

        m.maps[MATERIAL_MAP_ALBEDO].color = Vector3ToColor(names.diffuse, names.opacity);
        m.maps[MATERIAL_MAP_METALNESS].color = Vector3ToColor(names.specular, names.opacity);

        m.maps[MATERIAL_MAP_ALBEDO].texture = LoadTextureBase(names.diffuse_map, names.base);
        m.maps[MATERIAL_MAP_METALNESS].texture = LoadTextureBase(names.specular_map, names.base);
        m.maps[MATERIAL_MAP_OCCLUSION].texture = LoadTextureBase(names.highlight_map, names.base);
        m.maps[MATERIAL_MAP_NORMAL].texture = LoadTextureBase(names.bump_map, names.base);

        // Free unused maps (used ones are freed in LoadTextureBase because of reallocates)
        RL_FREE(names.displacement_map);
        RL_FREE(names.ambient_map);
        RL_FREE(names.alpha_map);
        RL_FREE(names.decal_map);

        model.materials[i] = m;
    }

    for (int i = 0; i < mesh_count; i++) {
        for (int j = 0; j < file.mat_count; j++) {
            if (file.mats[j].name_hash == meshes[i].mat_hash)
                model.meshMaterial[i] = j;
        }
        model.meshes[i] = meshes[i].mesh;
    }

    RL_FREE(meshes);
    RL_FREE(file.mats);
    RL_FREE(file.base);

    return model;
}

// Wrapper around LoadObjDry
// This basically does the same job as LoadMaterial does for LoadOBJ
Model LoadObj(const char *filename) {
    Model obj = LoadObjDry(filename);

    if (obj.materialCount == 0) {
        obj.materialCount = 1;
        obj.materials = RL_CALLOC(1, sizeof(Material));
        obj.materials[0] = LoadMaterialDefault();
    }

    for (int i = 0; i < obj.meshCount; i++) {
        UploadMesh(&obj.meshes[i], false);
    }

    return obj;
}
