import base64
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gltf
import models


class GltfExportTest(unittest.TestCase):
    def test_converts_coordinate_system_and_quad_winding(self):
        face = models.Face(prim=0, v=(0, 1, 2, 3), rgb=(64, 128, 255))
        bank = models.Bank(count=1, vertex_off=0, normal_off=0, model_offs=[])
        bank.vertices = [(0, 1, 2), (3, 4, 5), (6, 7, 8), (9, 10, 11)]
        bank.normals = [(0, 1, 0)]
        bank.models = [models.Model(index=7, offset=0, faces=[face])]

        doc = gltf.bank_to_gltf(bank)

        self.assertEqual("2.0", doc["asset"]["version"])
        self.assertEqual(1, len(doc["meshes"]))
        primitive = doc["meshes"][0]["primitives"][0]
        self.assertEqual(6, doc["accessors"][primitive["indices"]]["count"])
        position = primitive["attributes"]["POSITION"]
        self.assertEqual([0.0, -10.0, -11.0],
                         doc["accessors"][position]["min"])

    def test_textured_faces_reference_exported_png_material(self):
        face = models.Face(prim=1, v=(0, 1, 2, 3),
                           uv=((16, 32), (17, 32), (17, 33), (16, 33)),
                           tpage=0x123, clut=0x45)
        bank = models.Bank(count=1, vertex_off=0, normal_off=0, model_offs=[])
        bank.vertices = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        bank.models = [models.Model(index=0, offset=0, faces=[face])]

        doc = gltf.bank_to_gltf(bank, [{"file": "textures/test.png",
                                        "tpage": 0x123, "clut": 0x45}])

        primitive = doc["meshes"][0]["primitives"][0]
        self.assertEqual(0, primitive["material"])
        self.assertEqual("../textures/test.png", doc["images"][0]["uri"])
        accessor = doc["accessors"][primitive["attributes"]["TEXCOORD_0"]]
        view = doc["bufferViews"][accessor["bufferView"]]
        blob = base64.b64decode(doc["buffers"][0]["uri"].split(",", 1)[1])
        actual = struct.unpack_from("<2f", blob, view["byteOffset"])
        self.assertAlmostEqual(16 / 256.0, actual[0])
        self.assertAlmostEqual(32 / 256.0, actual[1])

    def test_texture_window_is_baked_into_normalized_uvs(self):
        face = models.Face(prim=1, v=(0, 1, 2, 3),
                           uv=((241, 250), (0, 0), (0, 0), (0, 0)),
                           texwin=(64, 32, 128, 64))
        bank = models.Bank(count=1, vertex_off=0, normal_off=0, model_offs=[])
        bank.vertices = [(0, 0, 0)] * 4
        bank.models = [models.Model(index=0, offset=0, faces=[face])]

        doc = gltf.bank_to_gltf(bank)

        primitive = doc["meshes"][0]["primitives"][0]
        accessor = doc["accessors"][primitive["attributes"]["TEXCOORD_0"]]
        view = doc["bufferViews"][accessor["bufferView"]]
        blob = base64.b64decode(doc["buffers"][0]["uri"].split(",", 1)[1])
        actual = struct.unpack_from("<2f", blob, view["byteOffset"])
        self.assertAlmostEqual(177 / 256.0, actual[0])
        self.assertAlmostEqual(90 / 256.0, actual[1])


if __name__ == "__main__":
    unittest.main()
