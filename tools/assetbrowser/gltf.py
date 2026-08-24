"""Export decoded Rage Racer model banks as ordinary glTF 2.0 meshes.

This is an offline conversion step.  Runtime renderers consume mesh/material
assets; they must never decode PS1 ordering-table packets or GTE output.
"""

from __future__ import annotations

import base64
import json
import struct
from pathlib import Path


def _align(data: bytearray) -> None:
    while len(data) & 3:
        data.append(0)


def _accessor(doc, blob, values, fmt, component_type, value_type, minimum=None,
              maximum=None):
    _align(blob)
    offset = len(blob)
    for value in values:
        blob.extend(struct.pack(fmt, *value))
    view = len(doc["bufferViews"])
    doc["bufferViews"].append({"buffer": 0, "byteOffset": offset,
                               "byteLength": len(blob) - offset})
    accessor = {"bufferView": view, "componentType": component_type,
                "count": len(values), "type": value_type}
    if minimum is not None:
        accessor["min"] = minimum
        accessor["max"] = maximum
    doc["accessors"].append(accessor)
    return len(doc["accessors"]) - 1


def _model_geometry(bank, model, material_for_face):
    """Return one independent primitive for each imported material.

    Keeping the material boundary here, rather than as a field in a bespoke
    Rage format, makes the generated file usable by standard glTF tooling and
    gives the runtime a renderer-neutral mesh/material contract.
    """
    groups = {}
    for face in model.faces:
        positions, normals, colors, uvs, indices = [], [], [], [], []
        material = material_for_face(face)
        group = groups.setdefault(material, {
            "positions": [], "normals": [], "colors": [], "uvs": [],
            "indices": [],
        })
        first = len(group["positions"])
        color = face.rgb or (255, 255, 255)
        for corner, vertex_index in enumerate(face.v):
            x, y, z = bank.vertices[vertex_index]
            # PS1 (+Y down, +Z forward) -> glTF (+Y up, -Z forward).
            group["positions"].append((float(x), float(-y), float(-z)))
            if face.n:
                nx, ny, nz = bank.normals[face.n[corner]]
                group["normals"].append((float(nx), float(-ny), float(-nz)))
            else:
                group["normals"].append((0.0, 1.0, 0.0))
            group["colors"].append((color[0] / 255.0, color[1] / 255.0,
                                    color[2] / 255.0, 1.0))
            if face.uv:
                u, v = face.uv[corner]
                if face.texwin is not None:
                    width_u, width_v, off_u, off_v = face.texwin
                    u = (u % width_u) + off_u
                    v = (v % width_v) + off_v
                # Rage stores texel addresses (0..255); glTF stores
                # normalized coordinates.  The decoded PNG is the original
                # 256x256 texture page, so no PS1 texture state reaches the
                # runtime renderer.
                group["uvs"].append((u / 256.0, v / 256.0))
            else:
                group["uvs"].append((0.0, 0.0))
        # The handedness conversion reverses winding.
        group["indices"].extend((first, first + 2, first + 1,
                                 first + 1, first + 2, first + 3))
    return groups


def bank_to_gltf(bank, textures=None) -> dict:
    """Return a self-contained glTF document for one parsed model bank."""
    doc = {"asset": {"version": "2.0", "generator": "rage-assetbrowser"},
           "buffers": [], "bufferViews": [], "accessors": [], "meshes": [],
           "nodes": [], "scenes": [{"nodes": []}], "scene": 0}
    texture_by_key = {(item["tpage"], item["clut"]): item
                      for item in (textures or [])}
    material_by_key = {}

    def material_for_face(face):
        if not face.uv:
            return None
        key = (face.tpage, face.clut)
        if key in material_by_key:
            return material_by_key[key]
        texture = texture_by_key.get(key)
        if texture is None:
            return None
        image = len(doc.setdefault("images", []))
        doc["images"].append({"uri": "../" + texture["file"]})
        texture_index = len(doc.setdefault("textures", []))
        doc["textures"].append({"source": image})
        material = len(doc.setdefault("materials", []))
        doc["materials"].append({
            "name": "tpage-%04x-clut-%04x" % key,
            "pbrMetallicRoughness": {
                "baseColorTexture": {"index": texture_index},
                "metallicFactor": 0.0,
                "roughnessFactor": 1.0,
            },
        })
        material_by_key[key] = material
        return material

    blob = bytearray()
    for model in bank.models:
        groups = _model_geometry(bank, model, material_for_face)
        if not groups:
            continue
        primitives = []
        for material, group in groups.items():
            positions = group["positions"]
            mins = [min(p[axis] for p in positions) for axis in range(3)]
            maxs = [max(p[axis] for p in positions) for axis in range(3)]
            attributes = {
                "POSITION": _accessor(doc, blob, positions, "<3f", 5126, "VEC3", mins, maxs),
                "NORMAL": _accessor(doc, blob, group["normals"], "<3f", 5126, "VEC3"),
                "COLOR_0": _accessor(doc, blob, group["colors"], "<4f", 5126, "VEC4"),
                "TEXCOORD_0": _accessor(doc, blob, group["uvs"], "<2f", 5126, "VEC2"),
            }
            primitive = {"attributes": attributes,
                         "indices": _accessor(doc, blob,
                                              [(i,) for i in group["indices"]],
                                              "<I", 5125, "SCALAR"),
                         "mode": 4}
            if material is not None:
                primitive["material"] = material
            primitives.append(primitive)
        mesh_index = len(doc["meshes"])
        doc["meshes"].append({"name": f"model-{model.index}",
                              "primitives": primitives})
        doc["nodes"].append({"mesh": mesh_index, "name": f"model-{model.index}"})
        doc["scenes"][0]["nodes"].append(len(doc["nodes"]) - 1)
    doc["buffers"].append({"byteLength": len(blob),
                           "uri": "data:application/octet-stream;base64,"
                                  + base64.b64encode(blob).decode("ascii")})
    return doc


def write_bank(path: Path, bank, textures=None) -> None:
    path.write_text(json.dumps(bank_to_gltf(bank, textures), separators=(",", ":")))

