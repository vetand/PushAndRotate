# this scripts converts .map file and corresponding set of .scene files to a
# set of .xml files which are ready to be parsed by push and rotate algorithm
# and map generator

import sys
import os

if __name__ == "__main__":
    map_file_name = sys.argv[1]
    agents_file_directory = sys.argv[2]
    xml_file_directory = sys.argv[3]

    try:
        os.mkdir(xml_file_directory)
    except Exception:
        pass

    try:
        files_directory = os.listdir(agents_file_directory)
    except Exception:
        print("Cannot open directory!")

    for name in files_directory:
        output_name = name[:len(name) - 5:] + ".xml"
        try:
            os.system("python3 {} {} {} {}".format("converter.py", map_file_name,
                                                    agents_file_directory + "/" + name, 
                                                    xml_file_directory + "/" + output_name))
        except Exception:
            print("Something went wrong with file {}".format(name))