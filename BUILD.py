""" Script for building debug and release configurations and packaging
everything into one output folder
"""

import subprocess
import winreg
import os



#### TEST FUNCTIONS #####
def RoboCopy_ListDir(dir):
    try:
        subprocess.check_output("robocopy " + dir + " ./new/tmp /E /L", stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        return e.output.decode("utf-8")

def SearchFile(dir, file):
    try:
        return subprocess.check_output("robocopy " + dir + " ./new/tmp /E /L | findstr /c:" + file, stderr=subprocess.STDOUT, shell=True).decode("utf-8")
    except subprocess.CalledProcessError as e:
        return e.output.decode("utf-8")
#### TEST FUNCTIONS #####



# https://developercommunity.visualstudio.com/content/problem/2813/cant-find-registry-entries-for-visual-studio-2017.html
VS_REG_PATH = r"SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7"
DEVENV_RELATIVE_PATH = r"Common7\IDE\devenv.exe"
MSBUILD_RELATIVE_PATH = r"MSBuild\15.0\Bin\MSBuild.exe"

VS_PATH = r""
DEVENV_PATH = r""
MSBUILD_PATH = r""

# reads registry to find VS2017 path
def GetVS2017Path():
    vs_registry_key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, VS_REG_PATH, 0, winreg.KEY_READ)
    value, regtype = winreg.QueryValueEx(vs_registry_key, "15.0")
    winreg.CloseKey(vs_registry_key)
    return value

# returns true if devenv.exe is found
def CheckVisualStudio():
    global VS_PATH, DEVENV_PATH, MSBUILD_PATH
    VS_PATH = GetVS2017Path()
    DEVENV_PATH = VS_PATH + DEVENV_RELATIVE_PATH
    MSBUILD_PATH = VS_PATH + MSBUILD_RELATIVE_PATH
    return os.path.isfile(DEVENV_PATH) and os.path.isfile(MSBUILD_PATH)

def PrintPaths():
    print("VS Path:\t\t", VS_PATH)
    print("devenv.exe Path:\t", DEVENV_PATH)
    print("msbuild.exe Path:\t", MSBUILD_PATH)


##### ENTRY POINT #####
if CheckVisualStudio():
    PrintPaths()
else:
    print("Cannot find Visual Studio. Compilation Aborted.")