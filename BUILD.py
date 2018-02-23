# 	DX11Renderer - VDemo | DirectX11 Renderer
# 	Copyright(C) 2018  - Volkan Ilbeyli
# 
# 	This program is free software : you can redistribute it and / or modify
# 	it under the terms of the GNU General Public License as published by
# 	the Free Software Foundation, either version 3 of the License, or
# 	(at your option) any later version.
# 
# 	This program is distributed in the hope that it will be useful,
# 	but WITHOUT ANY WARRANTY; without even the implied warranty of
# 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
# 	GNU General Public License for more details.
# 
# 	You should have received a copy of the GNU General Public License
# 	along with this program.If not, see <http://www.gnu.org/licenses/>.
# 
# 	Contact: volkanilbeyli@gmail.com

""" Script for building the release configuration of the project 
and packaging everything into one output folder in ARTIFACTS_PATH defined below
 so that the user can double click the Application.exe and run it from one folder.
"""

import subprocess
import winreg
import os
import shutil

#------------------- GLOBAL VARIABLES/CONSTANTS -------------------
# https://developercommunity.visualstudio.com/content/problem/2813/cant-find-registry-entries-for-visual-studio-2017.html
VS_REG_PATH = r"SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7"
DEVENV_RELATIVE_PATH = r"Common7\IDE\devenv.exe"
MSBUILD_RELATIVE_PATH = r"MSBuild\15.0\Bin\MSBuild.exe"

VS_PATH = r"PATH_NOT_FOUND"         # to be assigned
DEVENV_PATH = r"PATH_NOT_FOUND"     # to be assigned
MSBUILD_PATH = r"PATH_NOT_FOUND"    # to be assigned    

LOG_DEVENV = r"./LogFiles/devenv_out.log"
LOG_MSBUILD_S = r"./LogFiles/msbuild_out_st.log"
LOG_MSBUILD_M = r"./LogFiles/msbuild_out_mt.log"

SOLUTION_FILE = r"VDemo.sln"
ARTIFACTS_PATH = r"./Build/_artifacts"  # <-- Output directory 

#--------------------------- ROBOCOPY & PROCESSES ----------------------------
def RunSubprocess(cmd, bPrintOut):
    try:
        ret = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True).decode("utf-8")
        if bPrintOut:
            print(ret)
        return 0
    except subprocess.CalledProcessError as e:
        # handle robocopy exit codes
        bIsRunningRobocopy = cmd.split(' ')[0].lower() == 'robocopy'
        if bIsRunningRobocopy and e.returncode >= 8:
            print(e.output.decode("utf-8"))
        # rest of the processes' exit codes
        else:
            if bPrintOut:
                print(e.output.decode("utf-8"))

        return e.returncode


# assumes folder with no '/' or '\' in the end
def RobocopyRemoveFolder(folder, bPrintOutput):
    empty = "./Empty"
    os.makedirs(empty)
    cmd = "Robocopy " + empty + " " + folder + " /purge"
    ret = RunSubprocess(cmd, bPrintOutput)
    os.rmdir(empty)
    os.rmdir(folder)
    return ret < 8  # success condition


# assumes folder with no '/' or '\' in the end
def RobocopyCopyFolder(src, dst, bPrintOutput):
    _src = "\"" + src + "\""
    _dst = "\"" + dst + "\""

    cmd = "Robocopy " + _src + " " + _dst + " /E /MT:8"
    ret = RunSubprocess(cmd, bPrintOutput)
    return ret < 8  # success condition

#------------------------------ BUILD FUNCTIONS -------------------------------
# https://developercommunity.visualstudio.com/content/problem/2813/cant-find-registry-entries-for-visual-studio-2017.html
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
    print(" ")

# CLEAN
def CleanProjectFolders(bPrintOutput):
    print("Robocopy:      Cleaning up project folders...")
    #TODO: handle log files. either append date or delete previous

    return RobocopyRemoveFolder("./Build", bPrintOutput)

# MSBUILD
def BuildProject_msbuild():
    print("MSBuild:       Building the project...")
    global MSBUILD_PATH, LOG_MSBUILD_S, LOG_MSBUILD_M
    cmd = "\"" + MSBUILD_PATH + "\" /p:Configuration=Release /p:BuildInParallel=TRUE" # + LOG_MSBUILD_S
    ret = RunSubprocess(cmd, True)
    
    return ret == 0  # success condition

# DEVENV
def BuildProject_devenv(bPrintOutput):
    print("devenv:        Building the project...")
    global DEVENV_PATH, LOG_DEVENV
    cmd = "\"" + DEVENV_PATH + "\" " + SOLUTION_FILE + " /build RELEASE /out " + LOG_DEVENV
    ret = RunSubprocess(cmd, bPrintOutput)
    
    return ret == 0  # success condition

# PACKAGE
def PackageBinaries(bPrintOutput):
    print("Robocopy:      Packaging the build artifacts...")
    global ARTIFACTS_PATH
    
    # cleanup artifacts
    if os.path.exists(ARTIFACTS_PATH):
        bCleanSuccess = RobocopyRemoveFolder(ARTIFACTS_PATH, False)
        if not bCleanSuccess:
            print("Cleaning up artifacts folder (" + ARTIFACTS_PATH + ")  failed.")
            exit()

    os.makedirs(ARTIFACTS_PATH)
    
    # copy all the executables in ./Build to ARTIFACTS_PATH
    for path, subdirs, files in os.walk("./Build"):
        # print("path= " + path.replace('\\', '/'))
        if path.replace('\\', '/') == ARTIFACTS_PATH:
            continue

        for name in files:
            filename = os.path.join(path, name)
            extension = filename.split('.')[-1].lower()
            # print("Searching file: " + filename + " -> ." + extension)
            if  extension == "exe" or extension == "dll":
                if bPrintOutput:
                    print("copy " + filename + " to " + ARTIFACTS_PATH)
                shutil.copy2(filename, ARTIFACTS_PATH + "/" + name)

    # copy Data
    if not RobocopyCopyFolder("./Data", ARTIFACTS_PATH + "/Data", bPrintOutput):
        print("Error copying ./Data folder")
        return False

    # copy Shaders
    shadersPath = "/Source/Shaders"
    if not RobocopyCopyFolder("." + shadersPath, ARTIFACTS_PATH + shadersPath, bPrintOutput):
        print("Error copying ./Source/Shaders folder")
        return False

    return True


#---------------------------- ENTRY POINT ----------------------------
if CheckVisualStudio():
    # PrintPaths()

    bPrintLogs = False
    if not CleanProjectFolders(bPrintLogs):
        print("Clean Failed. TODO: output")
        exit()
    
    bPrintLogs = True
    if not BuildProject_devenv(bPrintLogs):
        print("Build Failed. TODO: output")
        exit()

    bPrintLogs = False
    if not PackageBinaries(bPrintLogs):
        print("Packaging failed. TODO output")
        exit()

    print("\nBuild Finished.\n" + ARTIFACTS_PATH + " contains the release version of the project executable.")

else:   # VS2017 Not Found
    print("Build Failed: Cannot find Visual Studio 2017.")