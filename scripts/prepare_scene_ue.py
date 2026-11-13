"""Bake the reviewed appearance into the local licensed map before packaging."""
import math
from pathlib import Path
import unreal

assets=unreal.EditorAssetLibrary
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
lib=unreal.MaterialEditingLibrary
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/LunarG1.LunarGameMode'))
dest='/Game/LunarMaterialReview'
def duplicate(source,name):
    path=dest+'/'+name
    return unreal.load_asset(path) if assets.does_asset_exist(path) else assets.duplicate_asset(source,path)
gravel=duplicate('/Game/Moon/Material/Landscape/MI_Landscape','MI_OmniGravel')
rock=duplicate('/Game/Moon/Material/MoonRock/MI_MoonRock','MI_RockReview')
texture_root=Path(__file__).resolve().parents[1]/'private_assets/gravel'
for parameter,suffix,ext in [('Albedo','diff','png'),('Normal','nor_dx','exr'),('Roughness','rough','exr'),('AO','ao','png')]:
    name='T_OmniGravel_'+parameter
    path=dest+'/'+name
    if assets.does_asset_exist(path):
        texture=unreal.load_asset(path)
    else:
        task=unreal.AssetImportTask()
        task.set_editor_property('filename',str(texture_root/f'gravel_stones_{suffix}_4k.{ext}'))
        task.set_editor_property('destination_path',dest)
        task.set_editor_property('destination_name',name)
        task.set_editor_property('automated',True)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        texture=unreal.load_asset(path)
        assert texture, f'Missing texture: {path}; run import_assets.py first'
    texture.set_editor_property('srgb',parameter=='Albedo')
    if parameter=='Albedo':texture.set_editor_property('adjust_brightness',.82)
    if parameter=='Normal':
        texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
        texture.set_editor_property('flip_green_channel',False)
    assets.save_loaded_asset(texture)
    lib.set_material_instance_texture_parameter_value(gravel,parameter,texture)
lib.set_material_instance_scalar_parameter_value(gravel,'normal',.8)
lib.set_material_instance_texture_parameter_value(rock,'Roughness',unreal.load_asset('/Game/Moon/Texture/MoonRock/T_CaveWall_A_2_5x2_5_8K_roughness'))
for k,v in [('normal',.8),('Brightness',.62),('Desaturation',1.)]:lib.set_material_instance_scalar_parameter_value(rock,k,v)
lib.update_material_instance(rock);assets.save_loaded_asset(rock)
if not assets.does_asset_exist('/Game/MuJoCo/RobotMat'):
    material=unreal.AssetToolsHelpers.get_asset_tools().create_asset('RobotMat','/Game/MuJoCo',unreal.Material,unreal.MaterialFactoryNew())
    color=lib.create_material_expression(material,unreal.MaterialExpressionVectorParameter)
    color.set_editor_property('parameter_name','BaseColor')
    color.set_editor_property('default_value',unreal.LinearColor(.3,.3,.3,1))
    lib.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
    for name,value,prop in [('Roughness',.85,unreal.MaterialProperty.MP_ROUGHNESS),('Metallic',0.,unreal.MaterialProperty.MP_METALLIC)]:
        node=lib.create_material_expression(material,unreal.MaterialExpressionScalarParameter)
        node.set_editor_property('parameter_name',name);node.set_editor_property('default_value',value)
        lib.connect_material_property(node,'',prop)
    lib.recompile_material(material);assets.save_loaded_asset(material)
for name,value in [('Tilling',.175),('Far Tilling',.025)]:lib.set_material_instance_scalar_parameter_value(gravel,name,value)
assets.save_loaded_asset(gravel)
cube_path='/Game/LunarMaterialReview/T_NeutralSky'
cube=unreal.load_asset(cube_path) if assets.does_asset_exist(cube_path) else assets.duplicate_asset('/Engine/EditorMaterials/AssetViewer/T_GreyAmbient',cube_path)
for actor in actors.get_all_level_actors():
    if isinstance(actor,unreal.LandscapeProxy):actor.set_editor_property('landscape_material',gravel)
    if isinstance(actor,unreal.DirectionalLight):
        actor.set_actor_rotation(unreal.Rotator(pitch=-35,yaw=-48.495),False)
        actor.light_component.set_intensity(13.)
        actor.light_component.set_editor_property('forward_shading_priority',1)
    if isinstance(actor,unreal.SkyLight):
        c=actor.light_component;c.set_real_time_capture(False)
        c.set_editor_property('source_type',unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
        c.set_cubemap(cube);c.set_intensity(.35)
    if isinstance(actor,unreal.PostProcessVolume):
        s=actor.get_editor_property('settings')
        values=dict(auto_exposure_method=unreal.AutoExposureMethod.AEM_MANUAL,
                    auto_exposure_apply_physical_camera_exposure=False,auto_exposure_bias=-.6,
                    color_saturation=unreal.Vector4(1.1,1.1,1.1,1),color_contrast=unreal.Vector4(1.1,1.1,1.1,1),
                    color_gain=unreal.Vector4(.78,.831333,1,1),color_contrast_shadows=unreal.Vector4(1.1,1.1,1.1,1),
                    color_gain_shadows=unreal.Vector4(.794666,.815972,1,1),color_gamma_shadows=unreal.Vector4(1,1,1,1),
                    color_offset_shadows=unreal.Vector4(0,0,0,0),film_toe=.45,film_black_clip=0.,
                    dynamic_global_illumination_method=unreal.DynamicGlobalIlluminationMethod.LUMEN,
                    lumen_scene_lighting_quality=2.,lumen_final_gather_quality=2.)
        for k,v in values.items():s.set_editor_property('override_'+k,True);s.set_editor_property(k,v)
        actor.set_editor_property('settings',s)
    label=actor.get_actor_label()
    if label=='BP_Earth':
        actor.set_actor_scale3d(unreal.Vector(600,600,600))
        distance=500000.;az=math.radians(10);el=math.radians(9)
        actor.set_actor_location(unreal.Vector(distance*math.cos(el)*math.cos(az),distance*math.cos(el)*math.sin(az),distance*math.sin(el)),False,False)
    if label in ['BP_Earth','sky']:
        for c in actor.get_components_by_class(unreal.StaticMeshComponent):
            current=c.get_material(0)
            name='MI_EarthRelease' if label=='BP_Earth' else 'MI_SkyRelease'
            path='/Game/LunarMaterialReview/'+name
            material=unreal.load_asset(path) if assets.does_asset_exist(path) else unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,'/Game/LunarMaterialReview',unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
            lib.set_material_instance_parent(material,current.get_editor_property('parent'))
            values={'BRIGHTNESS':1.,'CityLight':.03} if label=='BP_Earth' else {'Stars brightness':.5,'Cloud opacity':0.}
            for k,v in values.items():lib.set_material_instance_scalar_parameter_value(material,k,v)
            lib.update_material_instance(material);assets.save_loaded_asset(material);c.set_material(0,material)
for p in assets.list_assets('/Game/Moon/Meshes',recursive=False):
    if '/SM_Rock_' in p:
        mesh=unreal.load_asset(p)
        for i in range(len(mesh.static_materials)):mesh.set_material(i,rock)
        assets.save_loaded_asset(mesh)
assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
print('RELEASE_SCENE_SAVED')
