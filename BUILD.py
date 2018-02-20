""" Script for building debug and release configurations and packaging
everything into one output folder
"""

import subprocess
import winreg
import os



#----------------------- TEST FUNCTIONS -----------------------
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



#----------------------- GLOBAL CONSTANTS -----------------------
# https://developercommunity.visualstudio.com/content/problem/2813/cant-find-registry-entries-for-visual-studio-2017.html
VS_REG_PATH = r"SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7"
DEVENV_RELATIVE_PATH = r"Common7\IDE\devenv.exe"
MSBUILD_RELATIVE_PATH = r"MSBuild\15.0\Bin\MSBuild.exe"

VS_PATH = r""
DEVENV_PATH = r""
MSBUILD_PATH = r""

LOG_DEVENV = r"./LogFiles/devenv_out.log"
LOG_MSBUILD_S = r"./LogFiles/msbuild_out_st.log"
LOG_MSBUILD_M = r"./LogFiles/msbuild_out_mt.log"


APP_DEBUG_ARGUMENTS  = r"/GS /W3 /Zc:wchar_t /ZI /Gm /Od /sdl /Fd\"C:/Users/volka/Documents/GitHub/VQEngine/Build/Temp/Application/x64/Debug/vc141.pdb\" /Zc:inline /fp:precise /D \"_MBCS\" /errorReport:prompt /WX- /Zc:forScope /RTC1 /Gd /MDd /Fa\"C:/Users/volka/Documents/GitHub/VQEngine/Build/Temp/Application/x64/Debug/\" /EHsc /nologo /Fo\"C:/Users/volka/Documents/GitHub/VQEngine/Build/Temp/Application/x64/Debug/\" /Fp\"C:/Users/volka/Documents/GitHub/VQEngine/Build/Temp/Application/x64/Debug/Application.pch\" /diagnostics:classic"
UTIL_DEBUG_ARGUMENTS = r"/GS /W3 /Zc:wchar_t /ZI /Gm /Od /sdl /Fd\"C:/Users/volka/Documents/GitHub/VQEngine/Build/Temp/Utilities/x64/Debug/Utilities.pdb\" /Zc:inline /fp:precise /D \"_MBCS\" /errorReport:prompt /WX- /Zc:forScope /RTC1 /Gd /MDd /Fa\"C:/Users/volka/Documents/GitHub/VQEngine/Build/Temp/Utilities/x64/Debug/\" /EHsc /nologo /Fo\"C:/Users/volka/Documents/GitHub/VQEngine/Build/Temp/Utilities/x64/Debug/\" /Fp\"C:/Users/volka/Documents/GitHub/VQEngine/Build/Temp/Utilities/x64/Debug/Utilities.pch\" /diagnostics:classic"



#----------------------- FUNCTIONS -----------------------
# reads registry to find VS2017 path
def GetVS2017Path():
    vs_registry_key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, VS_REG_PATH, 0, winreg.KEY_READ)
    value, regtype = winreg.QueryValueEx(vs_registry_key, "15.0")
    winreg.CloseKey(vs_registry_key)
    return value

# returns true if devenv.exe & msbuild.eze are both found
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

def BuildProject_msbuild():
    global MSBUILD_PATH
    cmd = "\"" + MSBUILD_PATH + "\" " + UTIL_DEBUG_ARGUMENTS
    print("todo")

def BuildProject_devenv():
    global DEVENV_PATH, LOG_DEVENV
    cmd = "\"" + DEVENV_PATH + "\" " + "VDemo.sln /build RELEASE /out " + LOG_DEVENV
    try:
        ret = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True).decode("utf-8")
        print(ret)
    except subprocess.CalledProcessError as e:
        print(e.output.decode("utf-8"))

##### ENTRY POINT #####
if CheckVisualStudio():
    PrintPaths()

    BuildProject_devenv()

    # BuildProject_msbuild()

else:
    print("Cannot find Visual Studio. Compilation Aborted.")