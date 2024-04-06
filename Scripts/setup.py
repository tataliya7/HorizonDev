import os
import sys
if sys.version_info.major == 2:
    sys.exit("Error: Horizon Engine does not support Python 2. Please use a supported version of Python 3 instead.")

import ctypes
if not (ctypes.sizeof(ctypes.c_voidp) == 8):
    sys.exit("Error: 64bit python not found. Please install it.")

import argparse
import shutil
import zipfile
import requests
import contextlib
import traceback
import urllib

@contextlib.contextmanager
def current_working_dir(dir):
    curdir = os.getcwd()
    os.chdir(dir)
    try:
        yield
    finally:
        os.chdir(curdir)

def download_url(url, dir, force = False):
    max_retries = 5
    last_error = None
    filename = None
    with current_working_dir(dir):
        for i in range(max_retries):
            try:
                r = urllib.request.urlopen(url)
                filename = r.info().get_filename()

                if force and os.path.exists(filename):
                    os.remove(filename)

                if os.path.exists(filename):
                    print("Info: {0} already exists, skipping download".format(os.path.abspath(filename)))
                else:
                    print("Info: Downloading {0} to {1}".format(url, os.path.abspath(filename)))

                    tmpfile = filename + ".tmp"
                    if os.path.exists(tmpfile):
                        os.remove(tmpfile)

                    with open(tmpfile, "wb") as outfile:
                        outfile.write(r.read())

                    shutil.move(tmpfile, filename)
                break
            except Exception as e:
                print("Error: Retrying download due to error: {0}, Number of attempts remaining: {1}\n".format(e, max_retries - i - 1))
                last_error = e
        else:
            msg = str(last_error)
            raise RuntimeError("Failed to download {0}: {1}".format(url, msg))
        return os.path.abspath(filename)

def extract_zip_file(src_path, dst_dir, tmp_dir = None, force = False):
    name, _ = os.path.splitext(os.path.basename(src_path))
    archive = None
    try:
        if zipfile.is_zipfile(src_path):
            archive = zipfile.ZipFile(src_path)
        else:
            raise RuntimeError("Unrecognized archive file type")

        with archive:
            dst_path = os.path.join(os.path.abspath(dst_dir), name)
            if force and os.path.isdir(dst_path):
                shutil.rmtree(dst_path)

            if os.path.isdir(dst_path):
                print("Info: Directory {0} already exists, skipping extract".format(dst_path))
            else:
                print("Info: Extracting archive to {0}".format(dst_dir))

                if tmp_dir is None:
                    tmp_dir = os.path.join(os.path.dirname(src_path), name)

                tmp_path = os.path.abspath(tmp_dir)
                if os.path.isdir(tmp_path):
                    shutil.rmtree(tmp_path)

                archive.extractall(tmp_path)

                shutil.move(tmp_path, dst_path)

            return dst_path
    except Exception as e:
        shutil.move(src_path, src_path + ".corrupt")
        raise RuntimeError("Failed to extract archive {0}: {1}".format(src_path, e))

class dependency(object):
    def __init__(self, name, install, url, dst, args = None):
        self.name = name
        self.install = install
        self.url = url
        self.dst = dst
        self.args = args

class dependency_manager:
    def __init__(self, args):
        self.install_dir = os.path.abspath(args.install_dir)
        self.download_dir = os.path.abspath(args.download_dir)
        self.dependencies = dict()
        if not os.path.exists(self.download_dir):
            os.makedirs(self.download_dir)
    def add_dependency(self, dep):
        self.dependencies[dep.name] = dep
    def install_dependency(self, name):
        if name not in self.dependencies:
            return False
        dep = self.dependencies[name]
        dep.install(self, dep, dep.args)
        return True

def install_glfw(manager, dep, args = None):
    dst_dir = os.path.join(manager.install_dir, dep.dst)
    downloaded_file = download_url(dep.url, manager.download_dir, False)
    extract_dir = extract_zip_file(downloaded_file, dst_dir)

dependency_glfw = dependency(
    name = "glfw",
    install = install_glfw,
    url = "https://github.com/glfw/glfw/releases/download/3.3.9/glfw-3.3.9.bin.WIN64.zip",
    dst = "glfw")

def install_imgui(manager, dep, args = None):
    dst_dir = os.path.join(manager.install_dir, dep.dst)
    downloaded_file = download_url(dep.url, manager.download_dir, False)
    extract_dir = extract_zip_file(downloaded_file, dst_dir)
    shutil.copy(os.path.join(dst_dir, "imconfig.h"), os.path.join(extract_dir, "imconfig.h"))

dependency_imgui = dependency(
    name = "imgui",
    install = install_imgui,
    url = "https://github.com/ocornut/imgui/archive/refs/tags/v1.89.9-docking.zip",
    dst = "imgui")

def install_ImGuizmo(manager, dep, args = None):
    downloaded_file = download_url(dep.url, manager.download_dir, False)
    extract_zip_file(downloaded_file, os.path.join(manager.install_dir, dep.dst))
    #with current_working_dir():
    #   return os.getcwd()

dependency_ImGuizmo = dependency(
    name = "ImGuizmo",
    install = install_ImGuizmo,
    url = "https://github.com/CedricGuillemet/ImGuizmo/archive/refs/tags/1.83.zip",
    dst = "ImGuizmo")

def install_googletest(manager, dep, args = None):
    dst_dir = os.path.join(manager.install_dir, dep.dst)
    downloaded_file = download_url(dep.url, manager.download_dir, False)
    extract_dir = extract_zip_file(downloaded_file, dst_dir)

dependency_googletest = dependency(
    name = "googletest",
    install = install_googletest,
    url = "https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip",
    dst = "googletest")

def install_optick(manager, dep, args = None):
    dst_dir = os.path.join(manager.install_dir, dep.dst)
    downloaded_file = download_url(dep.url, manager.download_dir, False)
    extract_dir = extract_zip_file(downloaded_file, dst_dir)

dependency_optick = dependency(
    name = "optick",
    install = install_optick,
    url = "https://github.com/bombomby/optick/releases/download/1.4.0.0/Optick_1.4.0.zip",
    dst = "optick")

def install_dxc(manager, dep, args = None):
    dst_dir = os.path.join(manager.install_dir, dep.dst)
    downloaded_file = download_url(dep.url, manager.download_dir, False)
    extract_dir = extract_zip_file(downloaded_file, dst_dir)

dependency_dxc = dependency(
    name = "dxc",
    install = install_dxc,
    url = "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.8.2403.1/dxc_2024_03_22.zip",
    dst = "dxc")

# parser = argparse.ArgumentParser()
# parser.add_argument(
#    "install_dir",
#    type = str,
#    default = os.path.dirname(os.path.abspath(os.getcwd())).join("ThirdParty"),
#    help = "Directory where dependencies will be installed")
# parser.add_argument(
#    "download_dir",
#    type = str,
#    default = os.path.dirname(os.path.abspath(os.getcwd())).join("Download"),
#    help = "Directory where dependencies will be downloaded")
# args = parser.parse_args()

class args_(object):
    def __init__(self, install_dir, download_dir):
        self.install_dir = install_dir
        self.download_dir = download_dir
args = args_("ThirdParty", "Download")

manager = dependency_manager(args)
manager.add_dependency(dependency_glfw)
manager.add_dependency(dependency_imgui)
manager.add_dependency(dependency_ImGuizmo)
manager.add_dependency(dependency_googletest)
manager.add_dependency(dependency_optick)
manager.add_dependency(dependency_dxc)

required_dependencies = list()
required_dependencies.append("glfw")
required_dependencies.append("imgui")
required_dependencies.append("ImGuizmo")
required_dependencies.append("googletest")
required_dependencies.append("optick")
required_dependencies.append("dxc")

try:
    for dependency_name in required_dependencies:
        dep = manager.dependencies[dependency_name]
        print("Installing {0}...".format(dep.name))
        if manager.install_dependency(dep.name) == False:
            print("Failed to install {0}...".format(dep.name))
except Exception as e:
    traceback.print_exc()
    print ("Error:", str(e))
    sys.exit(1)

# def unzip_file(filename, path):
#     try:
#         with zipfile.ZipFile(filename, 'r') as zip_ref:
#             zip_ref.extractall(path)
#         return True
#     except:
#         print("Unzip failed")
#         return False

# def delete_file(filename):
#     try:
#         os.remove(filename)
#         return True
#     except:
#         print("Delete failed")
#         return False

# def delete_folder(folder):
#     try:
#         shutil.rmtree(folder)
#         return True
#     except:
#         print("Delete failed")
#         return False

# working_dir = os.getcwd()
# working_dir = ""

# if not os.path.exists(os.path.join(working_dir, "Temp")):
#     os.makedirs(os.path.join(working_dir, "Temp"))

# if not os.path.exists(os.path.join(working_dir, "ThirdParty/OpenUSD")):
#     succeed = download_file(os.path.join(working_dir, "Temp/OpenUSD.zip"), "https://github.com/PixarAnimationStudios/OpenUSD/archive/refs/tags/v23.08.zip")
#     if succeed:
#         unzip_file(os.path.join(working_dir, "Temp/OpenUSD.zip"), os.path.join(working_dir, "ThirdParty"))
#         os.rename(os.path.join(working_dir, "ThirdParty/OpenUSD-23.08"), os.path.join(working_dir, "ThirdParty/OpenUSD"))
#         delete_file(os.path.join(working_dir, "Temp/OpenUSD.zip"))

# delete_folder(os.path.join(working_dir, "Temp"))