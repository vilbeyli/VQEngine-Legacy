""" This python program, given a working directory,
traverses all the folders in the working directory to find the .cpp & .h files
and writes copyright notice to the beginning of each file that doesn't already
contain it (currently it checkts to see if it starts with '/')

WARNING:    run this on a backup and then diff to make sure no content is lost.
"""

import os

# exclude directories
EXCLUDE_DIRS = ['3rdParty']         

# copyright string: feel free to change this part if you're modifying this source
COPYRIGHT_STR = """//	DX11Renderer - VDemo | DirectX11 Renderer
//	Copyright(C) 2016  - Volkan Ilbeyli
//
//	This program is free software : you can redistribute it and / or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.If not, see <http://www.gnu.org/licenses/>.
//
//	Contact: volkanilbeyli@gmail.com
"""

def generate_new_path(file_name):
    """ appends '_licensed' to the end of file name, before extension - test"""
    split_file_name = file_name.split('.')
    new_file_name = split_file_name[0] + '_licensed.' + split_file_name[1]
    new_path = os.path.join(os.getcwd(), new_file_name)
    print(new_path)
    return new_path

def edit_headers(file_name):
    """ opens the file and writes the copyright if it doesn't already exist """
    full_path = os.path.join(os.getcwd(), file_name)        # build full path of the file
    source_file = open(full_path, 'r')                      # open the file in read mode
    first_line = source_file.readline()                     # read first line

    # TODO: improve this line to detect if the file contains copyright
    #       it currently only checks if the file starts with a '/' character
    contains_copyright = first_line[0] == '/'               # see if it starts with a comment '/'

    if not contains_copyright:
        contents = source_file.read()                   # save contents of the source code in memory
        source_file.close()                             # close the read mode
        os.remove(full_path)                            # remove the file (needed for appending to beginning)
        #new_path = generate_new_path(file_name)         
        source_file = open(full_path, 'w')              # open the file in write mode
        #print(full_path)
        source_file.seek(0)                             # seek to the beginning of the file
        source_file.write(COPYRIGHT_STR + "\n")         # write the copyright
        source_file.write(first_line + contents)        # and add the original contents

    source_file.close()
    return

def traverse_folder(curr_folder):
    """ recursively traverses all child folders in a directory """
    #print(curr_folder)
    os.chdir(curr_folder)
    for dir_entry in os.listdir():
        if os.path.isdir(dir_entry) and not dir_entry in EXCLUDE_DIRS:      # folders
            if  dir_entry[0] != '.':                              # that are not hidden
                traverse_folder(dir_entry)
        elif os.path.isfile(dir_entry):                           # consider files
            file_name = str(dir_entry).split('.')
            if len(file_name) == 2:                               # consider filename.ext files
                if file_name[1] == 'cpp' or file_name[1] == 'h' or file_name[1] == 'hlsl':
                    edit_headers(dir_entry)
    os.chdir("..\\")
    return

# ENTRY POINT
os.chdir("..\\")
DIRS = os.listdir()                 # get the directories
for entry in DIRS:                  # traverrse directories
    if os.path.isdir(entry) and entry[0] != '.': # only if its a directory and not hidden (git)
        cwd = os.getcwd()            
        traverse_folder(entry)
