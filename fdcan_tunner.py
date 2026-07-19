import os
import shutil
import glob
import cantools
from cantools.database.can.c_source import generate

# ==========================================
# CONFIGURACIÓN DE RUTAS
# ==========================================
DBC_FOLDER = "." 

# Carpetas destino relativas a donde está el script
INC_TARGET_DIR = os.path.join('Core', 'Inc')
SRC_TARGET_DIR = os.path.join('Core', 'Src')

def sync_dbc_to_stm32():
    try:
        # 1. Buscar archivos .dbc en el directorio actual
        dbc_files = glob.glob(os.path.join(DBC_FOLDER, "*.dbc"))
        
        if not dbc_files:
            raise IndexError
            
        dbc_path = dbc_files[0]
        dbc_filename = os.path.basename(dbc_path)
        base_name = os.path.splitext(dbc_filename)[0]
        
        print(f"📦 Archivo DBC detectado: {dbc_filename}")
        print("🔄 Cargando base de datos CAN en memoria...")

        # 2. Cargar el archivo DBC usando la API nativa de cantools
        db = cantools.database.load_file(dbc_path)
        
        print(f"👤 Nodos detectados en el DBC: {[node.name for node in db.nodes]}")
        print("🛠️ Generando código fuente UNIFICADO C y H...")
        
        generated_h = f"{base_name}.h"
        generated_c = f"{base_name}.c"
        fuzzer_c = f"{base_name}_fuzzer.c"

        # 3. Utilizar el generador de código unificado pasando los argumentos por POSICIÓN
        # Esto evita incompatibilidades con los nombres internos de las variables de cantools
        header, source, f_header, f_source = generate(
            db,              # 1er arg: Objeto de la base de datos
            base_name,       # 2do arg: Nombre de la base de datos
            generated_h,     # 3er arg: Nombre del archivo de cabecera
            generated_c,     # 4to arg: Nombre del archivo fuente
            fuzzer_c,        # 5to arg: Nombre del fuzzer
            True             # 6to arg: use_float (Optimizado para STM32 FPU)
        )

        # 4. Asegurar la existencia de las carpetas destino en el proyecto STM32
        os.makedirs(INC_TARGET_DIR, exist_ok=True)
        os.makedirs(SRC_TARGET_DIR, exist_ok=True)

        # 5. Escribir directamente el archivo .h a Core/Inc (Evita el paso temporal)
        dest_h = os.path.join(INC_TARGET_DIR, generated_h)
        if os.path.exists(dest_h):
            os.remove(dest_h)
        with open(dest_h, "w", encoding="utf-8") as f:
            f.write(header)
        print(f"✔️ Cabecera única guardada en: {dest_h}")

        # 6. Escribir directamente el archivo .c a Core/Src
        dest_c = os.path.join(SRC_TARGET_DIR, generated_c)
        if os.path.exists(dest_c):
            os.remove(dest_c)
        with open(dest_c, "w", encoding="utf-8") as f:
            f.write(source)
        print(f"✔️ Fuente único guardado en: {dest_c}")
        
        print("🚀 ¡Sincronización unificada completada con éxito!")

    except IndexError:
        print("❌ Error: No se encontró ningún archivo .dbc en la carpeta donde se ejecuta el script.")
    
    except (cantools.database.errors.ParseError, ValueError) as e:
        print(f"❌ Error de Sintaxis en DBC: Cantools no pudo procesar el archivo. Detalle: {e}")
        print("💡 Revisa el archivo .dbc con un editor de texto o una herramienta de diseño CAN.")
    
    except FileNotFoundError as e:
        print(f"❌ Error de Sistema: No se encontró un archivo o directorio intermedio. {e}")
    
    except PermissionError:
        print("❌ Error de Permisos: STM32CubeIDE o el sistema bloquean los archivos.")
    
    except Exception as e:
        print(f"❌ Ocurrió un error inesperado de tipo [{type(e).__name__}]: {e}")

if __name__ == "__main__":
    sync_dbc_to_stm32()
