from calendar import c
from math import e
import sys
from os.path import basename
import os
import json
import shutil

Import('env')

print("Current CLI targets", COMMAND_LINE_TARGETS)
print("Current Build targets", BUILD_TARGETS)

FIRMWARE_PUBLISH_DIR = "docu"

def load_json(filename):
    with open(filename, "r", encoding="utf-8") as infile:
        return json.load(infile)

def get_firmware_version() -> str:
    return env.GetProjectConfig().get("common", "firmware_version").strip("'").strip('"')

def isNewFirmwareVersion(manifest_file_name : str):
    manifest = load_json(manifest_file_name)
    return manifest["version"] != get_firmware_version()

def build_release_firmware(source, target, env):
        print(f"Merging firmware for release {get_firmware_version()}")
        firmware_fn = os.path.join(FIRMWARE_PUBLISH_DIR, "firmware_tmp.bin")
        manifest_fn = os.path.join(FIRMWARE_PUBLISH_DIR, "manifest_firmware.json")
        manifest = load_json(manifest_fn)
#        print(env.Dump())
        extra_targets = ""
        for et in env.get("FLASH_EXTRA_IMAGES", []):
            extra_targets += f"{et[0]} {et[1]} "
        extra_targets += f"{env['ESP32_APP_OFFSET']} $BUILD_DIR/firmware.bin"
        print("Extra targets", extra_targets)
        cmd = " ".join([
        env['OBJCOPY'], "--chip", "esp32", "merge_bin", "-o", firmware_fn, "--flash_mode","dio", "--flash_size", "4MB"]) + " " + extra_targets
        if env.Execute(cmd) != 0:
            print("Error merging firmware")
            os.remove(firmware_fn)
            sys.exit(1)
        manifest["version"] = get_firmware_version()
        with open(manifest_fn, "w", encoding="utf-8") as outfile:
            json.dump(manifest, outfile, indent=4)
        shutil.copyfile(firmware_fn, os.path.join(FIRMWARE_PUBLISH_DIR, f"firmware.bin"))
        shutil.copyfile(firmware_fn, os.path.join(FIRMWARE_PUBLISH_DIR, f"firmware_v{get_firmware_version()}.bin"))
        os.remove(firmware_fn)
        # Merge SPIFFS image
        spiffs_fn =  os.path.join(FIRMWARE_PUBLISH_DIR, f"spiffs_v{get_firmware_version()}.bin")
        if not os.path.exists(spiffs_fn):
            print("No SPIFFS image found")
            return
        cmd += f" 0x290000 {spiffs_fn}"
        if env.Execute(cmd) != 0:
            print("Error merging SPIFFS firmware")
            os.remove(firmware_fn)
            sys.exit(1)
        manifest_fn = os.path.join(FIRMWARE_PUBLISH_DIR, "manifest.json")
        manifest = load_json(manifest_fn)
        manifest["version"] = get_firmware_version()
        with open(manifest_fn, "w", encoding="utf-8") as outfile:
            json.dump(manifest, outfile, indent=4)
        shutil.copyfile(firmware_fn, os.path.join(FIRMWARE_PUBLISH_DIR, f"merged-firmware.bin"))
        shutil.copyfile(firmware_fn, os.path.join(FIRMWARE_PUBLISH_DIR, f"merged-firmware_v{get_firmware_version()}.bin"))
        os.remove(firmware_fn)

env.AddCustomTarget(
    "build_release_firmware",
    "$BUILD_DIR/firmware.bin",
    build_release_firmware,    
    title="Build release firmware",
    description="Build merged firmware(s) and update manifest files",
)

def post_program_action(source, target, env):
    manifest_fn = os.path.join(FIRMWARE_PUBLISH_DIR, "manifest.json")
    print(f"Checking for version {get_firmware_version()}!")
    program_path = target[0].get_abspath()
    if isNewFirmwareVersion(manifest_fn):
        print("New version detected, copy SPIFF files")
        shutil.copyfile(program_path, os.path.join(FIRMWARE_PUBLISH_DIR, f"spiffs_v{get_firmware_version()}.bin"))
        shutil.copyfile(program_path, os.path.join(FIRMWARE_PUBLISH_DIR, "spiffs.bin"))

env.AddPostAction("$BUILD_DIR/spiffs.bin", post_program_action)