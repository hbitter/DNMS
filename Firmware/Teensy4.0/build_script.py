Import("env")
import shutil

# copies the firmware.hex file to a file with a target-specific name
def copy_firmware_hex(source, target, env):
    hexfile_name = f'{env["PROJECT_DIR"]}/{env["PIOENV"]}.hex'
    shutil.copy(target[0].path, hexfile_name)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.hex", copy_firmware_hex)

