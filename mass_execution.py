# this script executes push and rotate algorithm with a map and set of .xml files
# output directory name = scene directory name -solved

import sys
import os

if __name__ == "__main__":
    scene_directory = sys.argv[1]
    output_directory = scene_directory + "-solved"

    try:
        files_directory = os.listdir(scene_directory)
    except Exception:
        print("Cannot open directory!")

    success_counter = 0
    failure_counter = 0

    for name in files_directory:
        try:
            print("Start solving {} ...".format(name))
            try:
                os.mkdir(output_directory)
            except Exception:
                pass

            signal = os.system("Bin/PushAndRotate {} {}".format(scene_directory + "/" + name,
                                                                output_directory + "/" + name))
            if signal == 0:
                success_counter += 1
            else:
                failure_counter += 1
            print()
            print()
            print()

        except Exception:
            print("Something went wrong with file {}".format(name))

    print("Successful solves:", success_counter)
    print("Fails:", failure_counter)