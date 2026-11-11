# Modelos de la arena hechos en Blender 4.2 (público, palo, botella y disco).
# Cada modelo se arma con primitivas de Blender, se une en un solo objeto con una textura
# de colores (paleta de 4 x 4 cuadros) y se exporta a .obj para cargarlo con Assimp.
# Se guarda también el .blend de cada uno para poder abrirlo y modificarlo en Blender.
import bpy, bmesh, math, os, sys
from mathutils import Vector

SALIDA = sys.argv[sys.argv.index('--') + 1] if '--' in sys.argv else '.'
os.makedirs(SALIDA, exist_ok=True)

def limpiar():
    bpy.ops.wm.read_factory_settings(use_empty=True)

# Coordenadas como en OpenGL: (x, arriba, al frente) -> Blender: (x, -frente, arriba)
def P(x, y, z): return Vector((x, -z, y))

def celda_uv(c):  # centro del cuadro c = (columna, fila) de la paleta 4 x 4
    return ((c[0] + 0.5) / 4.0, 1.0 - (c[1] + 0.5) / 4.0)

def pintar(obj, c):
    # Todas las coordenadas de textura de la pieza van al centro de su color en la paleta
    if not obj.data.uv_layers:
        obj.data.uv_layers.new()
    uv = obj.data.uv_layers.active.data
    u, v = celda_uv(c)
    for i in range(len(uv)):
        uv[i].uv = (u, v)

piezas = []
def guardar_pieza(obj, c, suave=False, bisel=0.0):
    if bisel > 0:
        m = obj.modifiers.new('Bisel', 'BEVEL'); m.width = bisel; m.segments = 2
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.modifier_apply(modifier=m.name)
    pintar(obj, c)
    if suave:
        bpy.context.view_layer.objects.active = obj; obj.select_set(True)
        bpy.ops.object.shade_smooth_by_angle(angle=math.radians(45))
        obj.select_set(False)
    piezas.append(obj)
    return obj

def caja(centro, tam, c, bisel=0.0, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(size=1, location=P(*centro))
    o = bpy.context.object
    o.scale = (tam[0], tam[2], tam[1]); o.rotation_euler = rot
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    return guardar_pieza(o, c, bisel=bisel)

def cilindro(a, b, r, c, seg=16, r2=None):
    # Cilindro (o cono si se da r2) que va del punto a al punto b
    a, b = P(*a), P(*b)
    d = b - a
    if r2 is None:
        bpy.ops.mesh.primitive_cylinder_add(vertices=seg, radius=r, depth=d.length, location=(a + b) / 2)
    else:
        bpy.ops.mesh.primitive_cone_add(vertices=seg, radius1=r, radius2=r2, depth=d.length, location=(a + b) / 2)
    o = bpy.context.object
    o.rotation_euler = d.to_track_quat('Z', 'Y').to_euler()
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    return guardar_pieza(o, c, suave=True)

def esfera(centro, r, c, escala=(1, 1, 1)):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=16, ring_count=10, radius=r, location=P(*centro))
    o = bpy.context.object
    o.scale = (escala[0], escala[2], escala[1])
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return guardar_pieza(o, c, suave=True)

def paleta(nombre, colores):
    # colores: {(col, fila): (r, g, b)} en 0-255; imagen de 64 x 64
    img = bpy.data.images.new(nombre, 64, 64, alpha=True)
    px = [0.0] * (64 * 64 * 4)
    for y in range(64):
        for x in range(64):
            col, fila = x // 16, 3 - (y // 16)   # Blender guarda la imagen de abajo hacia arriba
            r, g, b = colores.get((col, fila), (255, 0, 255))
            i = (y * 64 + x) * 4
            px[i:i + 4] = [r / 255, g / 255, b / 255, 1.0]
    img.pixels = px
    img.filepath_raw = os.path.join(SALIDA, nombre + '.png'); img.file_format = 'PNG'; img.save()
    return img

def terminar(nombre, colores):
    global piezas
    img = paleta(nombre, colores)
    mat = bpy.data.materials.new(nombre); mat.use_nodes = True
    bsdf = mat.node_tree.nodes['Principled BSDF']
    tex = mat.node_tree.nodes.new('ShaderNodeTexImage'); tex.image = img; tex.interpolation = 'Closest'
    mat.node_tree.links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])
    bsdf.inputs['Roughness'].default_value = 0.7
    # Unir todas las piezas en un solo objeto
    bpy.ops.object.select_all(action='DESELECT')
    for o in piezas: o.select_set(True)
    bpy.context.view_layer.objects.active = piezas[0]
    bpy.ops.object.join()
    obj = bpy.context.object; obj.name = nombre; obj.data.name = nombre
    obj.data.materials.clear(); obj.data.materials.append(mat)
    bpy.ops.wm.obj_export(filepath=os.path.join(SALIDA, nombre + '.obj'), export_selected_objects=True,
                          export_uv=True, export_normals=True, export_materials=True, apply_modifiers=True,
                          path_mode='STRIP', forward_axis='NEGATIVE_Z', up_axis='Y')
    bpy.ops.file.pack_all()
    os.makedirs(os.path.join(SALIDA, 'blender'), exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(SALIDA, 'blender', nombre + '.blend'))
    piezas = []

# Aficionado sentado en su asiento
def aficionado(nombre, playera, franja):
    limpiar()
    PIEL, PELO, PANT, TENIS, ASIENTO, PLAY, FRAN = (0, 0), (1, 0), (2, 0), (3, 0), (0, 1), (1, 1), (2, 1)
    # Asiento de plástico de la grada
    caja((0, 0.40, 0.10), (0.46, 0.05, 0.42), ASIENTO, bisel=0.015)
    caja((0, 0.70, -0.12), (0.46, 0.55, 0.05), ASIENTO, bisel=0.015, rot=(math.radians(-8), 0, 0))
    caja((0, 0.19, 0.06), (0.10, 0.38, 0.24), ASIENTO, bisel=0.01)
    for lado in (-1, 1):
        x = 0.1 * lado
        cilindro((x, 0.50, -0.02), (x, 0.50, 0.40), 0.075, PANT)          # muslo
        cilindro((x, 0.50, 0.40), (x, 0.09, 0.42), 0.06, PANT)            # pierna
        caja((x, 0.045, 0.46), (0.11, 0.09, 0.24), TENIS, bisel=0.02)     # tenis
        cilindro((0.22 * lado, 1.02, 0.02), (0.24 * lado, 0.75, 0.07), 0.052, PLAY)   # brazo
        cilindro((0.24 * lado, 0.75, 0.07), (0.15 * lado, 0.60, 0.32), 0.042, PIEL)   # antebrazo
        esfera((0.145 * lado, 0.585, 0.34), 0.045, PIEL)                              # mano
    caja((0, 0.83, 0.02), (0.36, 0.56, 0.22), PLAY, bisel=0.05)          # torso
    caja((0, 0.74, 0.02), (0.37, 0.07, 0.23), FRAN, bisel=0.02)          # franja de la playera
    cilindro((0, 1.10, 0.02), (0, 1.17, 0.02), 0.05, PIEL)               # cuello
    esfera((0, 1.25, 0.03), 0.11, PIEL, escala=(0.95, 1.08, 1.0))        # cabeza
    esfera((0, 1.275, 0.005), 0.112, PELO, escala=(0.97, 1.0, 1.0))      # pelo
    terminar(nombre, {PIEL: (226, 180, 140), PELO: (55, 38, 28), PANT: (45, 60, 95), TENIS: (235, 235, 235),
                      ASIENTO: (70, 74, 82), PLAY: playera, FRAN: franja})

# Palo de hockey
def palo():
    limpiar()
    MADERA, CINTA, PALA = (0, 0), (1, 0), (2, 0)
    caja((0, 0.80, 0), (0.032, 1.50, 0.025), MADERA, bisel=0.006)       # mango
    caja((0, 1.47, 0), (0.036, 0.18, 0.029), CINTA, bisel=0.006)        # cinta del puño
    cilindro((0, 1.55, 0), (0, 1.565, 0), 0.02, CINTA)                  # tapón
    caja((0.005, 0.10, 0), (0.034, 0.16, 0.022), MADERA, bisel=0.006, rot=(0, math.radians(-12), 0))  # talón
    caja((0.155, 0.04, 0), (0.30, 0.075, 0.012), PALA, bisel=0.005)     # pala con cinta negra
    terminar('palo', {MADERA: (176, 128, 74), CINTA: (30, 30, 30), PALA: (20, 20, 20)})

# Botella de agua
def botella():
    limpiar()
    CUERPO, ETIQUETA, TAPA = (0, 0), (1, 0), (2, 0)
    cilindro((0, 0.0, 0), (0, 0.165, 0), 0.035, CUERPO, seg=20)
    cilindro((0, 0.06, 0), (0, 0.12, 0), 0.0355, ETIQUETA, seg=20)
    cilindro((0, 0.165, 0), (0, 0.20, 0), 0.035, CUERPO, seg=20, r2=0.018)   # hombro
    cilindro((0, 0.20, 0), (0, 0.24, 0), 0.015, TAPA, seg=12)
    terminar('botella', {CUERPO: (235, 240, 245), ETIQUETA: (25, 70, 170), TAPA: (30, 30, 30)})

# Disco (reglamento: 7.62 cm x 2.54 cm)
def disco():
    limpiar()
    NEGRO = (0, 0)
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=0.0381, depth=0.0254, location=P(0, 0.0127, 0))
    o = bpy.context.object
    guardar_pieza(o, NEGRO, bisel=0.002)
    bpy.context.view_layer.objects.active = o; o.select_set(True)
    bpy.ops.object.shade_smooth_by_angle(angle=math.radians(45)); o.select_set(False)
    terminar('disco', {NEGRO: (18, 18, 20)})

aficionado('aficionado_local', (200, 30, 40), (245, 245, 245))
aficionado('aficionado_visita', (25, 70, 170), (240, 200, 30))
palo(); botella(); disco()
print('MODELOS LISTOS')
