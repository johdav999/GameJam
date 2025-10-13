import unreal

# ==========================================================
# Create a 4-layer landscape material (Grass, Rock, Dirt, Snow)
# Compatible with Unreal Engine 5.6
# ==========================================================

asset_path = "/Game/Materials/M_Landscape_4Layer"
material_name = "M_Landscape_4Layer"
package_path = "/Game/Materials"

mel = unreal.MaterialEditingLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

# --- Remove existing material (avoids overwrite dialog) ---
if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
    unreal.EditorAssetLibrary.delete_asset(asset_path)

# --- Create new material asset (UE 5.6 way) ---
material_factory = unreal.MaterialFactoryNew()
material = asset_tools.create_asset(material_name, package_path, unreal.Material, material_factory)

# Save & compile baseline
unreal.EditorAssetLibrary.save_loaded_asset(material)
mel.recompile_material(material)

# ==========================================================
# Shared / global parameters
# ==========================================================

texcoord = mel.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, 0, 0)

tiling = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, 200, 0)
tiling.set_editor_property("ParameterName", "Tiling")
tiling.set_editor_property("DefaultValue", 1.0)

uv_scale = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, 400, 0)
mel.connect_material_expressions(texcoord, "", uv_scale, "A")
mel.connect_material_expressions(tiling, "", uv_scale, "B")

global_tint = mel.create_material_expression(material, unreal.MaterialExpressionVectorParameter, 0, 200)
global_tint.set_editor_property("ParameterName", "GlobalTint")
global_tint.set_editor_property("DefaultValue", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))

roughness_param = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, 0, 400)
roughness_param.set_editor_property("ParameterName", "Roughness")
roughness_param.set_editor_property("DefaultValue", 0.6)

# ==========================================================
# Create 4 layer sets: Base / Detail / Normal
# ==========================================================

layer_defs = ["Grass", "Rock", "Dirt", "Snow"]
layers_base = {}
layers_normal = {}

x_offset = 600
y_offset = 0

for i, name in enumerate(layer_defs):
    base = mel.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, x_offset, y_offset + i * 400)
    base.set_editor_property("ParameterName", f"{name}_Base")

    detail = mel.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, x_offset + 400, y_offset + i * 400)
    detail.set_editor_property("ParameterName", f"{name}_Detail")

    normal = mel.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, x_offset + 800, y_offset + i * 400)
    normal.set_editor_property("ParameterName", f"{name}_Normal")

    mul_detail = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, x_offset + 1200, y_offset + i * 400)
    mel.connect_material_expressions(base, "", mul_detail, "A")
    mel.connect_material_expressions(detail, "", mul_detail, "B")

    layers_base[name] = mul_detail
    layers_normal[name] = normal

# --- Compile intermediate state ---
mel.recompile_material(material)

# ==========================================================
# Landscape Layer Blends
# ==========================================================

# BaseColor blend
layerblend_base = mel.create_material_expression(material, unreal.MaterialExpressionLandscapeLayerBlend, 2000, 0)
for name, expr in layers_base.items():
    entry = unreal.LayerBlendInput()
    entry.LayerName = name
    entry.BlendType = unreal.LandscapeLayerBlendType.WEIGHT_BLEND
    layerblend_base.add_layer(entry)
    mel.connect_material_expressions(expr, "", layerblend_base, name)

# Normal blend
layerblend_normal = mel.create_material_expression(material, unreal.MaterialExpressionLandscapeLayerBlend, 2000, 800)
for name, expr in layers_normal.items():
    entry = unreal.LayerBlendInput()
    entry.LayerName = name
    entry.BlendType = unreal.LandscapeLayerBlendType.WEIGHT_BLEND
    layerblend_normal.add_layer(entry)
    mel.connect_material_expressions(expr, "", layerblend_normal, name)

# ==========================================================
# Final output wiring
# ==========================================================

mul_tint = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, 2600, 0)
mel.connect_material_expressions(layerblend_base, "", mul_tint, "A")
mel.connect_material_expressions(global_tint, "", mul_tint, "B")

mel.connect_material_expressions(mul_tint, "", None, "BaseColor")
mel.connect_material_expressions(roughness_param, "", None, "Roughness")
mel.connect_material_expressions(layerblend_normal, "", None, "Normal")

# ==========================================================
# Save and finish
# ==========================================================
mel.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)

print(f"✅ Created {asset_path}")
