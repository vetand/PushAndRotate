# this script executes push and rotate algorithm with a map and set of .xml files
# output directory name = scene directory name -solved

import sys
import os
import xml.dom.minidom as minidom

def collect_log(directory, files_in_directory):
    moving_time = 0
    post_time = 0
    total_steps = 0
    total_length = 0
    for name in files_in_directory:
        try:
            doc = minidom.parse(directory + "/" + name)
            log_tag = doc.getElementsByTagName("log")[0]
            general_tag = doc.getElementsByTagName("general")[0]
            steps_tag = general_tag.getElementsByTagName("makespan")[0]
            quality_tag = general_tag.getElementsByTagName("summ-of-costs")[0]
            moving_time_tag = general_tag.getElementsByTagName("moving-phase-time")[0]
            post_time_tag = general_tag.getElementsByTagName("post-processing-time")[0]
            total_steps += int(steps_tag.childNodes[0].data)
            total_length += int(quality_tag.childNodes[0].data)
            moving_time += int(moving_time_tag.childNodes[0].data[:-2:])
            post_time += int(post_time_tag.childNodes[0].data[:-2:])
        except Exception:
            pass

    simulations = len(files_in_directory)
    print("Average makespan: {}".format(total_steps // simulations))
    print("Average summ of costs: {}".format(total_length // simulations))
    print("Average time: {}ms".format((moving_time + post_time) // simulations))

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
            if len(sys.argv) >= 3:
                if sys.argv[2] == "parallel":
                    command = "Bin/PushAndRotate {} {} parallel".format(
                                                            scene_directory + "/" + name,
                                                            output_directory + "/" + name)
                if sys.argv[2] == "priorities":
                    command = "Bin/PushAndRotate {} {} priorities".format(
                                                            scene_directory + "/" + name,
                                                            output_directory + "/" + name)
            else:
                command = "Bin/PushAndRotate {} {}".format(scene_directory + "/" + name,
                                                           output_directory + "/" + name)
            signal = os.system(command)
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
    collect_log(output_directory, os.listdir(output_directory))