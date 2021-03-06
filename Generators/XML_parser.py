import xml.dom.minidom as minidom
import xml.etree.ElementTree as ET
import tkinter as tk
from tkinter import messagebox

class XmlParser:
    def __init__(self, file_name, mode):
        self.MAX_SIZE = 400
        self.start_x = []
        self.start_y = []
        self.finish_x = []
        self.finish_y = []
        if mode == "generator":
            self.number_of_agents = 0
            self.height = 0
            self.width = 0
            self.grid = []
            for y in range(self.MAX_SIZE):
                self.grid.append([])
                for x in range(self.MAX_SIZE):
                    self.grid[y].append(0)
            return

        if file_name == "":
            messagebox.showerror("Ultimate trace tool", "File name is empty!")
            self.correct = False
            return
        try:
            doc = minidom.parse(file_name)
            self.correct = True
        except Exception:
            messagebox.showerror("Ultimate trace tool", "Cannot parse file!")
            self.correct = False
            return
        try:
            map_tag = doc.getElementsByTagName("map")[0]
        except Exception:
            messagebox.showerror("Ultimate trace tool", "No map tag!")
            self.correct = False
            return
        try:
            self.width = int(map_tag.getElementsByTagName("width")[0].childNodes[0].data)
            self.height = int(map_tag.getElementsByTagName("height")[0].childNodes[0].data)
            if self.width > self.MAX_SIZE or self.height > self.MAX_SIZE:
                messagebox.showerror("Ultimate trace tool", "Too big map (200x200 max)!")
                self.correct = False
                return
        except Exception:
            messagebox.showerror("Ultimate trace tool", "Incorrect map size!")
            self.correct = False
            return
        self.number_of_agents = 0
        try:
            agents_tag = map_tag.getElementsByTagName("agents")
            if len(agents_tag) == 0:
                start_x_tag = map_tag.getElementsByTagName("startx")[0]
                start_y_tag = map_tag.getElementsByTagName("starty")[0]
                finish_x_tag = map_tag.getElementsByTagName("finishx")[0]
                finish_y_tag = map_tag.getElementsByTagName("finishy")[0]
                self.start_x.append(int(start_x_tag.childNodes[0].data))
                self.start_y.append(int(start_y_tag.childNodes[0].data))
                self.finish_x.append(int(finish_x_tag.childNodes[0].data))
                self.finish_y.append(int(finish_y_tag.childNodes[0].data))
                self.number_of_agents = 1
            else:
                agent_tags = agents_tag[0].getElementsByTagName("agent")
                self.number_of_agents = len(agent_tags)
                for ind in range(self.number_of_agents):
                    agent_tag = agent_tags[ind]
                    start_x_tag = agent_tag.getElementsByTagName("startx")[0]
                    start_y_tag = agent_tag.getElementsByTagName("starty")[0]
                    finish_x_tag = agent_tag.getElementsByTagName("finishx")[0]
                    finish_y_tag = agent_tag.getElementsByTagName("finishy")[0]
                    self.start_x.append(int(start_x_tag.childNodes[0].data))
                    self.start_y.append(int(start_y_tag.childNodes[0].data))
                    self.finish_x.append(int(finish_x_tag.childNodes[0].data))
                    self.finish_y.append(int(finish_y_tag.childNodes[0].data))
        except Exception:
            messagebox.showerror("Ultimate trace tool", "Incorrect agent`(s) position(s)!")
            self.correct = False
            return
        try:
            row_tags = map_tag.getElementsByTagName("grid")[0].getElementsByTagName("row")
            self.grid = []
            for row in range(self.height):
                self.grid.append([])
                row_info = row_tags[row].childNodes[0].data.split()
                for col in range(self.width):
                    self.grid[row].append(int(row_info[col]))
        except Exception:
            messagebox.showerror("Ultimate trace tool", "Incorrect map!")
            self.correct = False
            return
        try:
            self.diag = map_tag.getElementsByTagName("allowdiagonal")[0].childNodes[0].data
            self.corners = map_tag.getElementsByTagName("cutcorners")[0].childNodes[0].data
            self.squeeze = map_tag.getElementsByTagName("allowsqueeze")[0].childNodes[0].data
        except Exception:
            messagebox.showerror("Ultimate trace tool", "Incorrect movement options!")
            self.correct = False
            return
        if mode == "load":
            return
        log_tag = doc.getElementsByTagName("log")[0]
        paths_tag = log_tag.getElementsByTagName("paths")[0]
        agent_tags = paths_tag.getElementsByTagName("agent")
        self.turns = []
        current_id = 0
        for agent_tag in agent_tags:
            movement_tags = agent_tag.getElementsByTagName("movement")
            for movement in movement_tags:
                list_ = movement.attributes.items()
                for item in list_:
                    if item[0] == 'x':
                        x_attribute = int(item[1])
                    elif item[0] == 'y':
                        y_attribute = int(item[1])
                    elif item[0] == 'step':
                        step_attribute = int(item[1])
                self.turns.append((step_attribute, current_id, x_attribute, y_attribute))
            current_id += 1
        self.turns.sort()