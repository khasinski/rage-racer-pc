import base64
import struct
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gltf
import models
import rmesh
import extract


class GltfExportTest(unittest.TestCase):
    def test_track_vram_keeps_boot_car_textures_before_track_uploads(self):
        shared = b"shared-car-textures"
        track = struct.pack("<5i", 20, 24, 28, 32, 36) + b"track-data"
        extractor = extract.Extractor.__new__(extract.Extractor)
        extractor.names = [f"asset-{i}" for i in range(135)]
        extractor.names[87] = "BIG1.1ST"
        extractor.data = lambda index: shared if index == 5 else track

        class FakeVram:
            def __init__(self):
                self.loads = []

            def load(self, buf, block):
                self.loads.append((buf, block))

            def clone(self):
                result = FakeVram()
                result.loads = list(self.loads)
                return result

        def image_asset(buf, offset):
            if buf is shared:
                return ["shared"]
            return [f"track-{offset}"]

        with mock.patch.object(extract.images, "Vram", FakeVram), \
             mock.patch.object(extract.images, "parse_image_asset",
                               side_effect=image_asset), \
             mock.patch.object(extract.images, "_parse_chunk",
                               return_value=["track-28"]):
            vram, alternate, sources = extractor.track_vrams(88)

        self.assertEqual(["CAR.TMS", "BIG1.1ST"], sources)
        self.assertEqual(["shared", "track-20", "track-24", "track-28",
                          "track-32", "track-36"],
                         [block for _buf, block in vram.loads])
        self.assertEqual(["shared", "track-20", "track-24", "track-28",
                          "track-32"],
                         [block for _buf, block in alternate.loads])

    def test_track_vram_loads_selected_car_pack_before_track_uploads(self):
        shared = b"shared-car-textures"
        car_pack = struct.pack("<5i", 20, 24, 28, 32, 36) + b"car-pack"
        track = struct.pack("<5i", 20, 24, 28, 32, 36) + b"track-data"
        extractor = extract.Extractor.__new__(extract.Extractor)
        extractor.names = [f"asset-{i}" for i in range(135)]
        extractor.names[29] = "CAR_30.2ND"
        extractor.names[87] = "BIG1.1ST"
        extractor.data = lambda index: {
            5: shared, 29: car_pack, 87: track}[index]

        class FakeVram:
            def __init__(self):
                self.loads = []

            def load(self, buf, block):
                self.loads.append((buf, block))

            def clone(self):
                result = FakeVram()
                result.loads = list(self.loads)
                return result

        def image_asset(buf, offset):
            if buf is shared:
                return ["shared"]
            if buf is car_pack:
                return [f"car-{offset}"]
            return [f"track-{offset}"]

        with mock.patch.object(extract.images, "Vram", FakeVram), \
             mock.patch.object(extract.images, "parse_image_asset",
                               side_effect=image_asset), \
             mock.patch.object(extract.images, "_parse_chunk",
                               return_value=["track-28"]):
            vram, alternate, sources = extractor.track_vrams(88, 29)

        self.assertEqual(["CAR.TMS", "CAR_30.2ND", "BIG1.1ST"], sources)
        self.assertEqual(
            ["shared", "car-36", "track-20", "track-24", "track-28",
             "track-32", "track-36"],
            [block for _buf, block in vram.loads])
        self.assertEqual(
            ["shared", "car-36", "track-20", "track-24", "track-28",
             "track-32"],
            [block for _buf, block in alternate.loads])

    def test_runtime_index_maps_archive_asset_and_geometry_kind(self):
        model = {"runtimeMesh": "models/a.rmesh",
                 "runtimeMaterials": "models/a.rmat"}
        records = [
            {"index": 10, "model": model},
            {"index": 91, "banks": [
                {"sub": 3, **model}, {"sub": 6, **model},
                {"sub": 5, **model}, {"sub": 7, **model},
            ]},
        ]

        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            extract.write_runtime_index(root, records)
            rows = (root / "runtime-index.txt").read_text().splitlines()

        self.assertEqual("# rage-rmesh-index v2", rows[0])
        self.assertEqual("10 model models/a.rmesh models/a.rmat", rows[1])
        self.assertEqual("91 track-model-1 models/a.rmesh models/a.rmat", rows[2])
        self.assertEqual("91 track-model-2 models/a.rmesh models/a.rmat", rows[3])
        self.assertEqual("91 course models/a.rmesh models/a.rmat", rows[4])
        self.assertEqual("91 terrain models/a.rmesh models/a.rmat", rows[5])

    def test_runtime_material_sidecar_is_renderer_neutral(self):
        face = models.Face(
            prim=1, v=(0, 1, 2, 3),
            uv=((0, 0), (1, 0), (1, 1), (0, 1)),
            tpage=0x142, clut=0x137,
            texwin=(64, 32, 128, 64))
        bank = models.Bank(count=1, vertex_off=0, normal_off=0,
                           model_offs=[])
        bank.vertices = [(0, 0, 0)] * 4
        bank.models = [models.Model(index=0, offset=0, faces=[face])]
        textures = [{
            "file": "textures/source.png",
            "runtimePixels": "textures/source.rgba",
            "tpage": face.tpage,
            "clut": face.clut,
            "texwin": list(face.texwin),
        }]

        with tempfile.TemporaryDirectory() as temp:
            extractor = extract.Extractor.__new__(extract.Extractor)
            extractor.out = Path(temp)
            (extractor.out / "models").mkdir()
            with mock.patch.object(extract.gltf, "write_bank"), \
                 mock.patch.object(extract.rmesh, "write_bank"):
                extractor.emit_bank(bank, "source", textures=textures)
            sidecar = (extractor.out / "models/source.rmat").read_text()

        self.assertEqual(
            "# rage-rmat v4\n"
            "0 textures/source.rgba textures/source.rgba\n",
            sidecar)

    def test_runtime_material_sidecar_attaches_semantic_paint_mask(self):
        bank = models.Bank(count=0, vertex_off=0, normal_off=0,
                           model_offs=[])
        bank.vertices = []
        bank.models = []
        textures = [{
            "runtimePixels": "textures/car.rgba",
            "runtimePaintMask": "textures/car.rpaint",
        }]
        with tempfile.TemporaryDirectory() as temp:
            extractor = extract.Extractor.__new__(extract.Extractor)
            extractor.out = Path(temp)
            (extractor.out / "models").mkdir()
            with mock.patch.object(extract.gltf, "write_bank"), \
                 mock.patch.object(extract.rmesh, "write_bank"):
                extractor.emit_bank(bank, "car", textures=textures)
            sidecar = (extractor.out / "models/car.rmat").read_text()
        self.assertEqual(
            "# rage-rmat v5\n"
            "0 textures/car.rgba textures/car.rgba | textures/car.rpaint\n",
            sidecar)

    def test_car_paint_mask_labels_palette_sampled_texels(self):
        vram = extract.images.Vram()
        # 4bpp page 1 begins at x=64. Its first byte contains palette indices
        # 0 and 1; only the second palette cell has a semantic label.
        offset = (0 * extract.images.VRAM_W + 64) * 2
        vram.buf[offset] = 0x10
        mask = extract.images.decode_texpage_labels(
            vram, 1, 0, {(1, 0): extract.CAR_PAINT_FIRST[6]})
        self.assertEqual((0, extract.CAR_PAINT_FIRST[6]),
                         tuple(mask[:2]))
        labels = extract.car_paint_vram_labels()
        self.assertIn(extract.CAR_PAINT_FIRST[0], labels.values())
        self.assertIn(extract.CAR_PAINT_SECOND[6], labels.values())

    def test_terrain_materials_bake_page_and_environment_variants(self):
        face = models.Face(
            prim=0, v=(0, 1, 2, 3),
            uv=((0, 0), (1, 0), (1, 1), (0, 1)),
            tpage=0x142, clut=0x137)
        bank = models.Bank(count=1, vertex_off=0, normal_off=0,
                           model_offs=[])
        bank.vertices = [(0, 0, 0)] * 4
        bank.models = [models.Model(index=0, offset=0, faces=[face])]
        page_a, page_b = object(), object()

        def pixels(vram, _tpage, clut):
            value = 1 + (vram is page_b) * 2 + (clut - face.clut)
            return bytes([value]) * (256 * 256 * 4)

        with tempfile.TemporaryDirectory() as temp:
            extractor = extract.Extractor.__new__(extract.Extractor)
            extractor.out = Path(temp)
            (extractor.out / "textures").mkdir()
            with mock.patch.object(extract.images, "decode_texpage",
                                   side_effect=pixels), \
                 mock.patch.object(extract.png, "write_rgba"):
                textures = extractor.emit_textures(
                    page_a, bank, "terrain",
                    variant_vrams=(page_a, page_b),
                    variant_clut_offsets=(0, 1))

        variants = textures[0]["runtimePixelVariants"]
        self.assertEqual(4, len(variants))
        self.assertEqual(4, len(set(variants)))

    def test_runtime_mesh_is_indexed_and_has_no_ps1_state(self):
        face = models.Face(prim=1, v=(0, 1, 2, 3), rgb=(1, 2, 3),
                           uv=((0, 0), (255, 0), (255, 255), (0, 255)),
                           tpage=0x42, clut=0x13)
        bank = models.Bank(count=1, vertex_off=0, normal_off=0, model_offs=[])
        bank.vertices = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        bank.models = [models.Model(index=0, offset=0, faces=[face])]

        blob = rmesh.bank_to_bytes(bank, [{"tpage": 0x42, "clut": 0x13}])

        magic, version, meshes, vertices, indices = rmesh.HEADER.unpack_from(blob)
        self.assertEqual(rmesh.MAGIC, magic)
        self.assertEqual(1, version)
        self.assertEqual((1, 4, 6), (meshes, vertices, indices))
        vertex = rmesh.VERTEX.unpack_from(blob, rmesh.HEADER.size + 8)
        self.assertEqual((1, 2, 3, 255), vertex[6:10])
        self.assertEqual(0, vertex[-1])

    def test_runtime_mesh_marks_only_scrolling_course_faces(self):
        scrolling = models.Face(
            prim=3, v=(0, 1, 2, 3),
            uv=((0, 0), (63, 0), (63, 63), (0, 63)),
            tpage=0x42, clut=0x13)
        bank = models.Bank(count=1, vertex_off=0, normal_off=0,
                           model_offs=[])
        bank.vertices = [(0, 0, 0), (1, 0, 0),
                         (1, 1, 0), (0, 1, 0)]
        bank.models = [models.Model(index=0, offset=0,
                                    faces=[scrolling])]
        textures = [{"tpage": 0x42, "clut": 0x13}]

        ordinary = rmesh.bank_to_bytes(bank, textures)
        course = rmesh.bank_to_bytes(
            bank, textures, scrolling_primitives=True)
        vertex_offset = rmesh.HEADER.size + 8
        self.assertEqual(0, rmesh.VERTEX.unpack_from(
            ordinary, vertex_offset)[-1])
        self.assertEqual(rmesh.MATERIAL_SCROLL_U,
                         rmesh.VERTEX.unpack_from(
                             course, vertex_offset)[-1])

    def test_runtime_mesh_preserves_authored_depth_bias(self):
        face = models.Face(
            prim=1, v=(0, 1, 2, 3), otbias=-4,
            uv=((0, 0), (63, 0), (63, 63), (0, 63)),
            tpage=0x42, clut=0x13)
        bank = models.Bank(count=1, vertex_off=0, normal_off=0,
                           model_offs=[])
        bank.vertices = [(0, 0, 0), (1, 0, 0),
                         (1, 1, 0), (0, 1, 0)]
        bank.models = [models.Model(index=0, offset=0, faces=[face])]
        blob = rmesh.bank_to_bytes(
            bank, [{"tpage": 0x42, "clut": 0x13}])
        material = rmesh.VERTEX.unpack_from(
            blob, rmesh.HEADER.size + 8)[-1]
        self.assertEqual(
            rmesh.MATERIAL_METADATA | (0xFC <<
                                       rmesh.MATERIAL_DEPTH_BIAS_SHIFT),
            material)

    def test_fixed_terrain_modes_select_the_authored_clut_row(self):
        self.assertEqual([0, 0, 0, 1, 0, 1], [
            models.terrain_fixed_clut_offset(mode) for mode in range(6)
        ])

    def test_runtime_mesh_preserves_terrain_visibility_and_palette_flags(self):
        face = models.Face(
            prim=0, v=(0, 1, 2, 3), otbias=0,
            uv=((0, 0), (63, 0), (63, 63), (0, 63)),
            tpage=0x42, clut=0x13)
        face.flags = 2
        bank = models.Bank(count=1, vertex_off=0, normal_off=0,
                           model_offs=[])
        bank.vertices = [(0, 0, 0), (1, 0, 0),
                         (1, 1, 0), (0, 1, 0)]
        bank.models = [models.Model(index=0, offset=0, faces=[face])]
        blob = rmesh.bank_to_bytes(
            bank, [{"tpage": 0x42, "clut": 0x13}],
            terrain_primitives=True)
        material = rmesh.VERTEX.unpack_from(
            blob, rmesh.HEADER.size + 8)[-1]
        self.assertEqual(
            rmesh.MATERIAL_METADATA |
            rmesh.MATERIAL_TERRAIN_NEAR_ONLY |
            rmesh.MATERIAL_TERRAIN_ENV_CLUT,
            material)

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
        self.assertAlmostEqual(16.5 / 256.0, actual[0])
        self.assertAlmostEqual(32.5 / 256.0, actual[1])

    def test_texture_window_keeps_original_uvs_for_baked_material(self):
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
        self.assertAlmostEqual(241.5 / 256.0, actual[0])
        self.assertAlmostEqual(250.5 / 256.0, actual[1])

    def test_texture_window_is_expanded_per_texel(self):
        rgba = b"".join(bytes((x, y, 0, 255))
                        for y in range(256) for x in range(256))
        baked = extract.Extractor.apply_texture_window(rgba, (64, 32, 128, 64))

        def pixel(x, y):
            offset = (y * 256 + x) * 4
            return tuple(baked[offset:offset + 4])

        self.assertEqual((128, 64, 0, 255), pixel(0, 0))
        self.assertEqual((177, 90, 0, 255), pixel(241, 250))
        self.assertEqual((128, 64, 0, 255), pixel(64, 32))

    def test_runtime_material_variants_deduplicate_identical_pixels(self):
        face = models.Face(prim=1, v=(0, 1, 2, 3),
                           uv=((0, 0), (1, 0), (1, 1), (0, 1)),
                           tpage=1, clut=2)
        bank = models.Bank(count=1, vertex_off=0, normal_off=0,
                           model_offs=[])
        bank.vertices = [(0, 0, 0)] * 4
        bank.models = [models.Model(index=0, offset=0, faces=[face])]
        first, second, duplicate = object(), object(), object()
        pixels_a = bytes((1, 2, 3, 255)) * (256 * 256)
        pixels_b = bytes((4, 5, 6, 255)) * (256 * 256)

        with tempfile.TemporaryDirectory() as temp:
            extractor = extract.Extractor.__new__(extract.Extractor)
            extractor.out = Path(temp)
            (extractor.out / "textures").mkdir()

            def decode(vram, _tpage, _clut):
                return pixels_b if vram is second else pixels_a

            with mock.patch.object(extract.images, "decode_texpage",
                                   side_effect=decode), \
                 mock.patch.object(extract.png, "write_rgba"):
                textures = extractor.emit_textures(
                    first, bank, "variant", variant_vrams=[
                        first, second, duplicate])

            paths = textures[0]["runtimePixelVariants"]
            self.assertEqual(paths[0], paths[2])
            self.assertNotEqual(paths[0], paths[1])
            self.assertTrue((extractor.out / paths[0]).is_file())
            self.assertTrue((extractor.out / paths[1]).is_file())

    def test_rival_material_variants_include_body_palette_offset(self):
        face = models.Face(prim=1, v=(0, 1, 2, 3),
                           uv=((0, 0), (1, 0), (1, 1), (0, 1)),
                           tpage=1, clut=20)
        bank = models.Bank(count=1, vertex_off=0, normal_off=0,
                           model_offs=[])
        bank.vertices = [(0, 0, 0)] * 4
        bank.models = [models.Model(index=0, offset=0, faces=[face])]
        first, second = object(), object()
        calls = []

        with tempfile.TemporaryDirectory() as temp:
            extractor = extract.Extractor.__new__(extract.Extractor)
            extractor.out = Path(temp)
            (extractor.out / "textures").mkdir()

            def decode(vram, _tpage, clut):
                calls.append((vram, clut))
                value = clut + (10 if vram is second else 0)
                return bytes((value, 0, 0, 255)) * (256 * 256)

            with mock.patch.object(extract.images, "decode_texpage",
                                   side_effect=decode), \
                 mock.patch.object(extract.png, "write_rgba"):
                textures = extractor.emit_textures(
                    first, bank, "palette", variant_vrams=[first, second],
                    variant_clut_offsets=(0, 1, 2))

        self.assertEqual(6, len(textures[0]["runtimePixelVariants"]))
        self.assertEqual(
            [(first, 20), (first, 21), (first, 22),
             (second, 20), (second, 21), (second, 22)],
            calls[1:])


if __name__ == "__main__":
    unittest.main()
