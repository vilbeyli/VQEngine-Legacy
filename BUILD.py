""" Script for building debug and release configurations and packaging
everything into one output folder
"""
import subprocess

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

##### ENTRY POINT

# print("hello")
# print(RoboCopy_ListDir("./Source/3rdParty/DirectXTex"))

print(SearchFile("./Source", "DirectXTex_Desktop_2015.vcxproj"))
