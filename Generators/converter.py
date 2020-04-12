# this script converts .map and .scene file taken from 
# https://movingai.com/benchmarks/mapf/index.html to a single .xml file

import xml.dom.minidom as minidom
import xml.etree.ElementTree as ET
from XML_parser import *
import sys

map_info = XmlParser("", "generator")

def parse_map(file_name):
    global map_info
    file = open(file_name, "r")
    map_file = file.readlines()
    file.close()
    map_info.height = int(map_file[1].split()[1])
    map_info.width = int(map_file[2].split()[1])
    for y in range(map_info.height):
        for x in range(map_info.width):
            symbol = map_file[y + 4][x]
            if symbol == '@' or symbol == 'T':
                map_info.grid[y][x] = 1
            else:
                map_info.grid[y][x] = 0

def parse_agents(file_name):
    global map_info
    file = open(file_name, "r")
    agents_file = file.readlines()
    file.close()
    for ind in range(1, len(agents_file)):
        line = agents_file[ind].split()
        map_info.number_of_agents += 1
        map_info.start_x.append(int(line[4]))
        map_info.start_y.append(int(line[5]))
        map_info.finish_x.append(int(line[6]))
        map_info.finish_y.append(int(line[7]))

def write_to_file(file_name):
    global map_info
    root_tag = ET.Element('root')
    map_tag = ET.SubElement(root_tag, 'map')
    width_tag = ET.SubElement(map_tag, 'width')
    width_tag.text = str(map_info.width)
    height_tag = ET.SubElement(map_tag, 'height')
    height_tag.text = str(map_info.height)
    cellsize_tag = ET.SubElement(map_tag, 'cellsize')
    cellsize_tag.text = "1"

    if map_info.number_of_agents == 1:
        startx_tag = ET.SubElement(map_tag, 'startx')
        startx_tag.text = str(map_info.start_x[0])
        starty_tag = ET.SubElement(map_tag, 'starty')
        starty_tag.text = str(map_info.start_y[0])
        finishx_tag = ET.SubElement(map_tag, 'finishx')
        finishx_tag.text = str(map_info.finish_x[0])
        finishy_tag = ET.SubElement(map_tag, 'finishy')
        finishy_tag.text = str(map_info.finish_y[0])
    else:
        agents_tag = ET.SubElement(map_tag, 'agents')
        for ind in range(map_info.number_of_agents):
            agent_tag = ET.SubElement(agents_tag, 'agent')
            startx_tag = ET.SubElement(agent_tag, 'startx')
            startx_tag.text = str(map_info.start_x[ind])
            starty_tag = ET.SubElement(agent_tag, 'starty')
            starty_tag.text = str(map_info.start_y[ind])
            finishx_tag = ET.SubElement(agent_tag, 'finishx')
            finishx_tag.text = str(map_info.finish_x[ind])
            finishy_tag = ET.SubElement(agent_tag, 'finishy')
            finishy_tag.text = str(map_info.finish_y[ind])

    height_tag = ET.SubElement(map_tag, 'grid')
    for y in range(map_info.height):
        new_row_tag = ET.SubElement(height_tag, 'row')
        row_string = ""
        for x in range(map_info.width):
            if map_info.grid[y][x] == 1:
                row_string += "1"
            else:
                row_string += "0";
            if x != map_info.width - 1:
                row_string += " "
        new_row_tag.text = row_string

    algorithm_tag = ET.SubElement(root_tag, 'algorithm')
    search_tag = ET.SubElement(algorithm_tag, 'searchtype')
    search_tag.text = "astar"
    metric_tag = ET.SubElement(algorithm_tag, 'metrictype')
    metric_tag.text = "manhattan"
    breaking_ties_tag = ET.SubElement(algorithm_tag, 'breakingties')
    breaking_ties_tag.text = "g-min"
    hweight_tag = ET.SubElement(algorithm_tag, 'hweight')
    hweight_tag.text = "1"
    diagonal_tag = ET.SubElement(algorithm_tag, 'allowdiagonal')
    diagonal_tag.text = "true"
    corners_tag = ET.SubElement(algorithm_tag, 'cutcorners')
    corners_tag.text = "true"
    squeeze_tag = ET.SubElement(algorithm_tag, 'allowsqueeze')
    squeeze_tag.text = "true"

    options_tag = ET.SubElement(root_tag, 'options')
    loglevel_tag = ET.SubElement(options_tag, 'loglevel')
    loglevel_tag.text = "1"
    logpath_tag = ET.SubElement(options_tag, 'logpath')
    logfile_tag = ET.SubElement(options_tag, 'logfilename')

    file = open(file_name, 'w')
    output = ET.tostring(root_tag).decode()
    file.write(output)
    file.close()

if __name__ == "__main__":
    map_file_name = sys.argv[1]
    agents_file_name = sys.argv[2]
    xml_file_name = sys.argv[3]
    parse_map(map_file_name)
    parse_agents(agents_file_name)
    write_to_file(xml_file_name)

