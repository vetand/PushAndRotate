from tkinter import filedialog
from tkinter import messagebox
import copy
import random
import time
import tkinter as tk
import xml.dom.minidom as minidom
import sys

from XML_parser import *

BUTTON_COLUMN = 150
EPS = 0.001
FRAME = 10
ENTRY_COLOR = "grey80"
LABEL_COLOR = "DarkOrange1"
MAIN_MENU_COLOR = "grey20"

class Application(tk.Frame):
    def __init__(self, master):
        super().__init__(master)
        self.WID_NUM = 4
        self.master = master
        self.master.geometry("400x600")
        self.master.resizable(0, 0)
        self.master['background'] = MAIN_MENU_COLOR
        self.create_main_menu()
        self.master.title("Main menu")

    def call_manual_generator(self):
        self.child_menu = ManualGenerator(tk.Toplevel())

    def call_xml_only_mode(self):
        self.child_menu = XmlOnlyMenu(tk.Toplevel())

    def create_main_menu(self):
        widget_counter = 0
        self.title = tk.Label(self.master, text="Ultimate trace tool",
                              anchor = "w",
                              fg = LABEL_COLOR, background = MAIN_MENU_COLOR,
                              font=("Arial bold", 28))
        self.title.place(x=5*FRAME, y=4*FRAME)

        widget_counter += 1
        self.xml_only = tk.Button(self.master, bg=LABEL_COLOR, fg="black",
                                      width=20, height=2, command=self.call_xml_only_mode,
                                      text="Xml only mode")
        self.xml_only.place(x = 200, anchor = "n",
                            y = (600 * widget_counter) // (self.WID_NUM+1))

        widget_counter += 1
        self.manual_button = tk.Button(self.master, bg=LABEL_COLOR, fg="black",
                                      width=20, height=2, command=self.call_manual_generator,
                                      text="Manual map generator")
        self.manual_button.place(x = 200, anchor = "n",
                                 y = (600 * widget_counter) // (self.WID_NUM+1))

        widget_counter += 1
        self.quit_button = tk.Button(self.master, bg=LABEL_COLOR, fg="red",
                                      width=20, height=2, command=self.quit_,
                                      text="Quit")
        self.quit_button.place(x = 200, anchor = "n",
                               y = (600 * widget_counter) // (self.WID_NUM+1))

    def quit_(self):
        try:
            self.child_menu.quit_()
        except Exception:
            pass
        self.master.destroy()

class Map:
    def __init__(self, master, cell_size, map_info, options):
        self.master = master
        self.width = map_info.width
        self.height = map_info.height
        self.cell_size = cell_size
        self.map_info = map_info
        self.agent_point = None
        self.last_actions = []
        self.redo_actions = []
        self.corresponding_widget = dict()
        self.buffer_actions = []
        self.end_points = set()
        self.agents = []
        self.canvas = tk.Canvas(master,
                                width=cell_size * self.width,
                                height=cell_size * self.height,
                                background="black")
        self.canvas.bind('<ButtonRelease-1>', self.left_release)
        self.canvas.bind('<ButtonRelease-3>', self.right_release)
        self.obstacles = set()
        self.active_agents = []
        self.number_of_agents = 0
        self.current_step = 0
        self.check_options = options['check_movements']
        self.grid = []
        self.cells = set()
        self.last_pressed = None
        for x in range(-1, self.width + 1):
            self.obstacles.add((x, -1))
            self.obstacles.add((x, self.height))
        for y in range(-1, self.height + 1):
            self.obstacles.add((-1, y))
            self.obstacles.add((self.width, y))
        self.make_cells()
        for y in range(self.height):
            for x in range(self.width):
                if self.map_info.grid[y][x]:
                    self.add_obstacle(x, y)
        for ind in range(self.map_info.number_of_agents):
            self.add_agent(self.map_info.start_x[ind],
                           self.map_info.start_y[ind], ind)
            self.add_agent(self.map_info.finish_x[ind],
                           self.map_info.finish_y[ind], ind)
        self.place_ten_delimiters()

    def add_agent(self, x, y, place):
        if x >= self.map_info.width or y >= self.map_info.height or x < 0 or y < 0:
            return
        rectangle = self.grid[y][x]
        if self.canvas.itemcget(rectangle, 'fill') == "goldenrod4":
            return
        if self.agent_point == None:
            if self.canvas.itemcget(rectangle, 'fill') == "RoyalBlue3":
                return
            self.agent_point = (x, y)
            self.canvas.itemconfig(rectangle, fill="RoyalBlue3")
            self.setting_agent = True
            self.buffer_actions.append([2, self.agent_point])
        else:
            if (x, y) in self.end_points:
                return
            start = self.agent_point
            finish = (x, y)
            tag_name = "point" + str(x) + '.' + str(y)
            tag_name_label = "label1" + str(x) + '.' + str(y)
            point = self.canvas.create_rectangle(x * self.cell_size + self.cell_size // 3,
                                                 y * self.cell_size + self.cell_size // 3,
                                                 (x+1) * self.cell_size - self.cell_size // 3,
                                                 (y+1) * self.cell_size - self.cell_size // 3,
                                                 fill="green", tags = tag_name)
            self.canvas.tag_bind(tag_name, '<ButtonPress-3>', self.right_press)
            label = self.canvas.create_text(start[0] * self.cell_size + self.cell_size // 2, 
                                            start[1] * self.cell_size + self.cell_size // 2, 
                                            text=str(place + 1), 
                                            justify='c', font="Verdana " + 
                                            str((self.cell_size*2)//3), tags = tag_name_label)
            self.canvas.tag_bind(tag_name_label, '<Shift-1>', self.shift_left)
            agent = GeneratedAgent(start, finish, point, label)
            self.agents.insert(place, agent)
            for ind in range(len(self.agents)):
                agent = self.agents[ind]
                if agent.start == finish:
                    self.canvas.delete(agent.label)
                    agent.label = self.canvas.create_text(
                                            finish[0] * self.cell_size + self.cell_size // 2, 
                                            finish[1] * self.cell_size + self.cell_size // 2, 
                                            text=str(ind + 1), 
                                            justify='c', font="Verdana " + 
                                            str((self.cell_size*2)//3), tags = tag_name_label)
                    self.canvas.tag_bind(tag_name_label, '<ButtonPress-3>', self.right_press)
                    tags = tag_name_label
                    self.corresponding_widget[agent.label] = point

                if agent.finish == start:
                    tag_name_label2 = "label2" + str(start[0]) + '.' + str(start[1])
                    self.canvas.itemconfig(label, tags=tag_name_label2)
                    self.canvas.tag_bind(tag_name_label2, '<ButtonPress-3>', self.right_press)
                    self.corresponding_widget[label] = agent.point
            self.agent_point = None  
            self.end_points.add((x, y))
            self.setting_agent = False
            self.buffer_actions[0][0] = 3
            self.buffer_actions[0][1] = [self.buffer_actions[0][1], (x, y)]
            self.last_actions.append(self.buffer_actions)
            self.redo_actions = []
            self.buffer_actions = []
            self.corresponding_widget[point] = point

    def add_obstacle(self, x, y):
        self.canvas.itemconfig(self.grid[y][x], fill = 'goldenrod4')
        self.obstacles.add((x, y))

    def check_obstacle(self, x, y):
        return (x, y) in self.obstacles

    def delete_agent(self, ind):
        self.canvas.delete(self.agents[ind].point)
        self.canvas.delete(self.agents[ind].label)
        start = self.agents[ind].start
        finish = self.agents[ind].finish
        self.canvas.itemconfig(self.grid[start[1]][start[0]], fill="khaki1")
        self.end_points.remove(finish)
        self.agents.pop(ind)

    def left_press(self, event):
        return

    def left_release(self, event):
        return

    def make_cells(self):
        for y in range(self.height):
            self.grid.append([])
            for x in range(self.width):
                tag_name = str(y) + "." + str(x)
                cell = self.canvas.create_rectangle(
                                      x * self.cell_size, y * self.cell_size,
                                      (x+1) * self.cell_size, (y+1) * self.cell_size,
                                      fill = "khaki1", tags=tag_name)
                self.grid[y].append(cell)
                self.cells.add(cell)
                self.canvas.tag_bind(tag_name, '<B1-Motion>', self.motion)
                self.canvas.tag_bind(tag_name, '<ButtonPress-1>', self.left_press)
                self.canvas.tag_bind(tag_name, '<ButtonPress-3>', self.right_press)
                self.canvas.tag_bind(tag_name, '<Shift-1>', self.shift_left)

    def motion(self, event):
        return

    def place_ten_delimiters(self):
        for x in range(0, self.width, 10):
            self.canvas.create_line(x * self.cell_size, 0,
                                    x * self.cell_size, self.height * self.cell_size,
                                    width=3)
        for y in range(0, self.height, 10):
            self.canvas.create_line(0, y * self.cell_size,
                                    self.width * self.cell_size, y * self.cell_size,
                                    width=3)

    def right_release(self, event):
        rectangle = self.last_pressed
        if rectangle == None:
            return
        if self.canvas.itemcget(rectangle, 'fill') == "orange":
            self.canvas.itemconfig(rectangle, fill="goldenrod4")
        elif self.canvas.itemcget(rectangle, 'fill') == "deep sky blue":
            self.canvas.itemconfig(rectangle, fill="RoyalBlue3")
        elif self.canvas.itemcget(rectangle, 'fill') == "green yellow":
            self.canvas.itemconfig(rectangle, fill="green")
        else:
            self.canvas.itemconfig(rectangle, fill="khaki1")
        self.cell_label.destroy()
        self.last_pressed = None
        self.master.update()

    def right_press(self, event):
        rectangle = event.widget.find_closest(event.x, event.y)[0]
        if rectangle in self.corresponding_widget.keys():
            rectangle = self.corresponding_widget[rectangle]
        x = event.x // self.cell_size
        y = event.y // self.cell_size
        for ind in range(len(self.agents)):
            if rectangle == self.agents[ind].point:
                self.canvas.itemconfig(rectangle, fill="green yellow")
                self.last_pressed = rectangle
                self.cell_label = tk.Label(self.master, text = "Agent №" + str(ind + 1),
                                            bg=ENTRY_COLOR , font=("Helvetica", 16))
                self.cell_label.place(x = event.x + self.SIDEBAR,
                                      y = event.y, anchor="ne")

        if rectangle not in self.cells:
            return
        if self.canvas.itemcget(rectangle, 'fill') == "goldenrod4":
            self.canvas.itemconfig(rectangle, fill="orange")
        elif self.canvas.itemcget(rectangle, 'fill') == "RoyalBlue3":
            self.canvas.itemconfig(rectangle, fill="deep sky blue")
        else:
            self.canvas.itemconfig(rectangle, fill="yellow")
        self.last_pressed = rectangle
        self.cell_label = tk.Label(self.master, text = "(" + str(x) + ", " + str(y) +")" +
                                                 ", id = " + str(y * self.map_info.width + x),
                                                 bg=ENTRY_COLOR , font=("Helvetica", 16))
        self.cell_label.place(x = event.x + self.SIDEBAR,
                              y = event.y, anchor="ne")

    def shift_left(self, event):
        return

    def undo(self, event):
        if self.buffer_actions != []:
            if self.buffer_actions[0][0] >= 2:
                x, y = self.agent_point[0], self.agent_point[1]
                self.canvas.itemconfig(self.grid[y][x], fill="Khaki1")
                self.agent_point = None
            elif self.buffer_actions[0][0] == 1:
                for ind in range(len(self.buffer_actions)):
                    pos = self.buffer_actions[ind][1]
                    self.canvas.itemconfig(self.grid[pos[1]][pos[0]], fill="goldenrod4")
                    self.map_info.grid[pos[1]][pos[0]] = 1
            else:
                for ind in range(len(self.buffer_actions)):
                    pos = self.buffer_actions[ind][1]
                    self.canvas.itemconfig(self.grid[pos[1]][pos[0]], fill="khaki1")
                    self.map_info.grid[pos[1]][pos[0]] = 0
            self.redo_actions.append(self.buffer_actions)
            self.buffer_actions = []
        else:
            if self.last_actions == []:
                return
            if self.last_actions[-1][0][0] >= 2:
                self.delete_agent(-1)
            elif self.last_actions[-1][0][0] == 1:
                for ind in range(len(self.last_actions[-1])):
                    pos = self.last_actions[-1][ind][1]
                    self.canvas.itemconfig(self.grid[pos[1]][pos[0]], fill="goldenrod4")
                    self.map_info.grid[pos[1]][pos[0]] = 1
            else:
                for ind in range(len(self.last_actions[-1])):
                    pos = self.last_actions[-1][ind][1]
                    self.canvas.itemconfig(self.grid[pos[1]][pos[0]], fill="khaki1")
                    self.map_info.grid[pos[1]][pos[0]] = 0
            self.redo_actions.append(self.last_actions[-1])
            self.last_actions.pop()
        self.pattern = None

class ManualGenerator(tk.Frame):
    def __init__(self, master):
        super().__init__(master)
        self.WID_NUM = 10
        self.master = master
        self.SIDEBAR = 190
        self.master.geometry(str(self.SIDEBAR) + "x600")
        self.master.resizable(0, 0)
        self.master['background'] = MAIN_MENU_COLOR
        self.map = None
        self.ops_menu = None
        self.create_menu()
        self.master.title("Ultimate trace tool")
        self.map_info = XmlParser("", "generator")
        self.options = dict()
        self.options['check_movements'] = False
        self.output_file = None
        self.change_map()
        self.algo_ops = dict()
        self.init_algo_ops()

    def basic_map_check(self):
        if self.map == None:
            messagebox.showerror("Ultimate trace tool", "Empty map!")
            return False

        if len(self.map.agents) == 0:
            messagebox.showerror("Ultimate trace tool", "No agents on the map!")
            return False
        return True

    def change_map(self):
        if self.map != None:
            self.map.canvas.destroy()
        try:
            self.options['size_X'] = min(max(int(self.desktop_x.get()), 600), 2000)
            self.options['size_Y'] = min(max(int(self.desktop_y.get()), 600), 2000)
        except Exception:
            messagebox.showerror("Ultimate trace tool", "Incorrect desktop resolution")
            return
        try:
            max_size_x = self.options['size_X'] // 6
            max_size_y = self.options['size_Y'] // 6
            self.map_info.width = min(max(int(self.width_entry.get()), 1), max_size_x)
            self.map_info.height = min(max(int(self.height_entry.get()), 1), max_size_y)
        except Exception:
            messagebox.showerror("Ultimate trace tool", "Incorrect map size")
            return
        coef_1 = (self.options['size_X'] - self.SIDEBAR - 2*FRAME) // self.map_info.width
        coef_2 = (self.options['size_Y'] - 2*FRAME) // self.map_info.height
        coef = min(coef_1, coef_2)
        for y in range(self.map_info.height):
            for x in range(self.map_info.width):
                self.map_info.grid[y][x] = 0
        self.map = MapGenerator(self.master, coef, self.map_info, self.options, self.SIDEBAR)
        self.master.bind('<Control-z>', self.map.undo)
        self.master.bind('<Control-x>', self.map.redo)
        self.master.bind('<Control-s>', self.save_bind)
        self.master.geometry(str(coef*self.map_info.width+2*FRAME+self.SIDEBAR)+ 'x' +
                             str(max(coef*self.map_info.height+2*FRAME, 600)))
        self.map.canvas.place(x=self.SIDEBAR+FRAME, y=FRAME)

    def clear(self):
        if self.map != None:
            event = DummyEvent(0, 0, self.map.canvas)
            while self.map.last_actions != []:
                self.map.undo(event)

    def create_menu(self):
        widget_counter = 0
        self.title = tk.Label(self.master, text="Manual generator",
                              fg = LABEL_COLOR, background = MAIN_MENU_COLOR,
                              font=("Arial bold", 16),
                              justify="center", anchor = "center")
        self.title.place(x=FRAME, y=FRAME)

        widget_counter += 1
        self.width_entry = tk.Entry(self.master, bg = ENTRY_COLOR, fg = "black",
                                  width=20)
        self.width_entry.insert(0, "30")
        self.width_entry.place(x = self.SIDEBAR//2, anchor="n",
                               y = (600 * widget_counter) // (self.WID_NUM+1))
        self.width_label = tk.Label(self.master, bg=MAIN_MENU_COLOR, fg=LABEL_COLOR,
                                        text="Map width")
        self.width_label.place(x = 3*FRAME,
                               y = (600 * widget_counter) // (self.WID_NUM+1) + 20)

        widget_counter += 1
        self.height_entry = tk.Entry(self.master, bg = ENTRY_COLOR, fg = "black",
                                  width=20)
        self.height_entry.insert(0, "30")
        self.height_entry.place(x = self.SIDEBAR//2, anchor="n",
                                y = (600 * widget_counter) // (self.WID_NUM+1))
        self.height_label = tk.Label(self.master, bg=MAIN_MENU_COLOR, fg=LABEL_COLOR,
                                        text="Map height")
        self.height_label.place(x = 3*FRAME,
                                y = (600 * widget_counter) // (self.WID_NUM+1) + 20)

        widget_counter += 1
        self.desktop_x = tk.Entry(self.master, bg = ENTRY_COLOR, fg = "black",
                                  width=20)
        self.desktop_x.insert(0, "800")
        self.desktop_x.place(x = self.SIDEBAR//2, anchor="n",
                             y = (600 * widget_counter) // (self.WID_NUM+1))
        self.desktop_x_label = tk.Label(self.master, bg=MAIN_MENU_COLOR, fg=LABEL_COLOR,
                                        text="Desktop resolution X")
        self.desktop_x_label.place(x = 3*FRAME,
                                   y = (600 * widget_counter) // (self.WID_NUM+1) + 20)

        widget_counter += 1
        self.desktop_y = tk.Entry(self.master, bg = ENTRY_COLOR, fg = "black",
                                  width=20)
        self.desktop_y.insert(0, "600")
        self.desktop_y.place(x = self.SIDEBAR//2, anchor="n",
                             y = (600 * widget_counter) // (self.WID_NUM+1))
        self.desktop_y_label = tk.Label(self.master, bg=MAIN_MENU_COLOR, fg=LABEL_COLOR,
                                        text="Desktop resolution Y")
        self.desktop_y_label.place(x = 3*FRAME,
                                   y = (600 * widget_counter) // (self.WID_NUM+1) + 20)

        widget_counter += 1
        self.start_button = tk.Button(self.master, bg=LABEL_COLOR, fg="black",
                                      width=17, height=1, command=self.change_map,
                                      text="Change params")
        self.start_button.place(x = self.SIDEBAR//2, anchor="n",
                                y = (600 * widget_counter) // (self.WID_NUM+1))

        widget_counter += 1
        self.ops_button = tk.Button(self.master, bg=LABEL_COLOR, fg="black",
                                    width=17, height=1, command=self.special_ops_menu,
                                    text="Special options")
        self.ops_button.place(x = self.SIDEBAR//2, anchor="n",
                              y = (600 * widget_counter) // (self.WID_NUM+1))

        widget_counter += 1
        self.clear_button = tk.Button(self.master, bg=LABEL_COLOR, fg="black",
                                      width=17, height=1, command=self.clear,
                                      text="Clear")
        self.clear_button.place(x = self.SIDEBAR//2, anchor="n",
                                y = (600 * widget_counter) // (self.WID_NUM+1))

        widget_counter += 1
        self.load_button = tk.Button(self.master, bg=LABEL_COLOR, fg="black",
                                      width=17, height=1, command=self.load,
                                      text="Load")
        self.load_button.place(x = self.SIDEBAR//2, anchor="n",
                               y = (600 * widget_counter) // (self.WID_NUM+1))

        widget_counter += 1
        self.save_button = tk.Button(self.master, bg=LABEL_COLOR, fg="black",
                                      width=7, height=1, command=self.save_button,
                                      text="Save")
        self.save_button.place(x = self.SIDEBAR//3, anchor="n",
                               y = (600 * widget_counter) // (self.WID_NUM+1))
        self.saveas_button = tk.Button(self.master, bg=LABEL_COLOR, fg="black",
                                      width=7, height=1, command=self.select_file,
                                      text="Save as")
        self.saveas_button.place(x = (self.SIDEBAR*2)//3, anchor="n",
                                 y = (600 * widget_counter) // (self.WID_NUM+1))

        widget_counter += 1
        self.quit_button = tk.Button(self.master, bg=LABEL_COLOR, fg="red",
                                      width=17, height=1, command=self.quit_,
                                      text="Quit")
        self.quit_button.place(x = self.SIDEBAR//2, anchor="n",
                               y = (600 * widget_counter) // (self.WID_NUM+1))

    def init_algo_ops(self):
        self.algo_ops['algorithm'] = tk.StringVar()
        self.algo_ops['algorithm'].set('astar')
        self.algo_ops['metric'] = tk.StringVar()
        self.algo_ops['metric'].set('diagonal')
        self.algo_ops['breakingties'] = tk.StringVar()
        self.algo_ops['breakingties'].set('g-max')
        self.algo_ops['hweight'] = tk.StringVar()
        self.algo_ops['hweight'].set('1')
        self.algo_ops['cellsize'] = tk.StringVar()
        self.algo_ops['cellsize'].set('1')
        self.algo_ops['allowdiagonal'] = tk.StringVar()
        self.algo_ops['allowdiagonal'].set('true')
        self.algo_ops['cutcorners'] = tk.StringVar()
        self.algo_ops['cutcorners'].set('true')
        self.algo_ops['allowsqueeze'] = tk.StringVar()
        self.algo_ops['allowsqueeze'].set('true')
        self.algo_ops['loglevel'] = tk.StringVar()
        self.algo_ops['loglevel'].set('1')

    def load(self):
        file_name = filedialog.askopenfilename(initialdir = "/",
                                               title = "Select .xml file",
                                               filetypes = [('XML files', '*.xml')])
        map_info = XmlParser(file_name, "load")
        if map_info.correct == False:
            return
        self.algo_ops['algorithm'].set('astar')
        self.algo_ops['metric'].set('diagonal')
        self.algo_ops['breakingties'].set('g-max')
        self.algo_ops['hweight'].set('1')
        self.algo_ops['cellsize'].set('1')
        self.algo_ops['allowdiagonal'].set(map_info.diag)
        self.algo_ops['cutcorners'].set(map_info.corners)
        self.algo_ops['allowsqueeze'].set(map_info.squeeze)
        self.algo_ops['loglevel'].set('1')
        self.width_entry.delete(0, len(self.width_entry.get()))
        self.height_entry.delete(0, len(self.height_entry.get()))
        self.width_entry.insert(0, map_info.width)
        self.height_entry.insert(0, map_info.height)
        self.change_map()
        for y in range(map_info.height):
            for x in range(map_info.width):
                if map_info.grid[y][x] == 1:
                    event_x = x * self.map.cell_size + self.map.cell_size // 2
                    event_y = y * self.map.cell_size + self.map.cell_size // 2
                    event_widget = self.map.canvas
                    event = DummyEvent(event_x, event_y, event_widget)
                    self.map.left_press(event)
        event = DummyEvent(0, 0, self.map.canvas)
        self.map.left_release(event)

        for ind in range(map_info.number_of_agents):
            x1 = map_info.start_x[ind]
            y1 = map_info.start_y[ind]
            x2 = map_info.finish_x[ind]
            y2 = map_info.finish_y[ind]
            event_x = x1 * self.map.cell_size + self.map.cell_size // 2
            event_y = y1 * self.map.cell_size + self.map.cell_size // 2
            event_widget = self.map.canvas
            event = DummyEvent(event_x, event_y, event_widget)
            self.map.shift_left(event);
            event_x = x2 * self.map.cell_size + self.map.cell_size // 2
            event_y = y2 * self.map.cell_size + self.map.cell_size // 2
            event = DummyEvent(event_x, event_y, event_widget)
            self.map.shift_left(event)

        self.output_file = file_name

    def save_bind(self, event):
        correct = self.basic_map_check()
        if not correct:
            return

        if self.output_file == None:
            self.select_file()
            return
        self.save()

    def save_button(self):
        correct = self.basic_map_check()
        if not correct:
            return

        if self.output_file == None:
            self.select_file()
            return
        self.save()  

    def save(self):
        root_tag = ET.Element('root')
        map_tag = ET.SubElement(root_tag, 'map')
        width_tag = ET.SubElement(map_tag, 'width')
        width_tag.text = str(self.map_info.width)
        height_tag = ET.SubElement(map_tag, 'height')
        height_tag.text = str(self.map_info.height)
        cellsize_tag = ET.SubElement(map_tag, 'cellsize')
        cellsize_tag.text = self.algo_ops['cellsize'].get()

        if len(self.map.agents) == 1:
            startx_tag = ET.SubElement(map_tag, 'startx')
            startx_tag.text = str(self.map.agents[0].start[0])
            starty_tag = ET.SubElement(map_tag, 'starty')
            starty_tag.text = str(self.map.agents[0].start[1])
            finishx_tag = ET.SubElement(map_tag, 'finishx')
            finishx_tag.text = str(self.map.agents[0].finish[0])
            finishy_tag = ET.SubElement(map_tag, 'finishy')
            finishy_tag.text = str(self.map.agents[0].finish[1])
        else:
            agents_tag = ET.SubElement(map_tag, 'agents')
            for agent in self.map.agents:
                agent_tag = ET.SubElement(agents_tag, 'agent')
                startx_tag = ET.SubElement(agent_tag, 'startx')
                startx_tag.text = str(agent.start[0])
                starty_tag = ET.SubElement(agent_tag, 'starty')
                starty_tag.text = str(agent.start[1])
                finishx_tag = ET.SubElement(agent_tag, 'finishx')
                finishx_tag.text = str(agent.finish[0])
                finishy_tag = ET.SubElement(agent_tag, 'finishy')
                finishy_tag.text = str(agent.finish[1])

        height_tag = ET.SubElement(map_tag, 'grid')
        for y in range(self.map_info.height):
            new_row_tag = ET.SubElement(height_tag, 'row')
            row_string = ""
            for x in range(self.map_info.width):
                if self.map_info.grid[y][x] == 1:
                    row_string += "1"
                else:
                    row_string += "0";
                if x != self.map_info.width - 1:
                    row_string += " "
            new_row_tag.text = row_string

        algorithm_tag = ET.SubElement(root_tag, 'algorithm')
        search_tag = ET.SubElement(algorithm_tag, 'searchtype')
        search_tag.text = self.algo_ops['algorithm'].get()
        metric_tag = ET.SubElement(algorithm_tag, 'metrictype')
        metric_tag.text = self.algo_ops['metric'].get()
        breaking_ties_tag = ET.SubElement(algorithm_tag, 'breakingties')
        breaking_ties_tag.text = self.algo_ops['breakingties'].get()
        hweight_tag = ET.SubElement(algorithm_tag, 'hweight')
        hweight_tag.text = self.algo_ops['hweight'].get()
        diagonal_tag = ET.SubElement(algorithm_tag, 'allowdiagonal')
        diagonal_tag.text = self.algo_ops['allowdiagonal'].get()
        corners_tag = ET.SubElement(algorithm_tag, 'cutcorners')
        corners_tag.text = self.algo_ops['cutcorners'].get()
        squeeze_tag = ET.SubElement(algorithm_tag, 'allowsqueeze')
        squeeze_tag.text = self.algo_ops['allowsqueeze'].get()

        options_tag = ET.SubElement(root_tag, 'options')
        loglevel_tag = ET.SubElement(options_tag, 'loglevel')
        loglevel_tag.text = self.algo_ops['loglevel'].get()
        logpath_tag = ET.SubElement(options_tag, 'logpath')
        logfile_tag = ET.SubElement(options_tag, 'logfilename')

        output = ET.tostring(root_tag).decode()
        try:
            file = open(self.output_file, 'w')
        except Exception:
            messagebox.showerror("Ultimate trace tool", "Cannot open file!")
            return
        file.write(output)
        file.close()

    def select_file(self):
        correct = self.basic_map_check()
        if not correct:
            return
        self.output_file = tk.filedialog.asksaveasfilename(initialdir = "/", 
                                                       title = "Select output",
                                                       filetypes = [('XML files', '*.xml')])
        if self.output_file == "":
            return
        self.save()

    def special_ops_menu(self):
        self.ops_menu = SpecialOpsMenu(tk.Toplevel(self.master), self.algo_ops)
        place_x = self.master.winfo_x()
        place_y = self.master.winfo_y()
        self.ops_menu.master.geometry('350' + 'x400+' + 
                                      str(place_x + self.SIDEBAR) + '+' 
                                    + str(place_y + self.SIDEBAR//2))

    def quit_(self):
        try:
            self.ops_menu.destroy()
        except Exception:
            pass
        self.master.destroy()

class SpecialOpsMenu(tk.Frame):
    def __init__(self, master, algo_ops):
        super().__init__(master)
        self.master = master
        self.algo_ops = algo_ops
        self.width = 350
        self.WID_NUM = 8
        self.master.title('Additional options')
        self.master['background'] = MAIN_MENU_COLOR
        self.master.resizable(0, 0)
        self.create_menu()

    def confirm(self):
        try:
            number = int(self.algo_ops['hweight'].get())
            number = max(number, 0)
            self.algo_ops['hweight'].set(str(number))
        except Exception:
            messagebox.showerror("Ultimate trace tool", "Incorrect heuristic weight!")
            return
        
        try:
            number = int(self.algo_ops['cellsize'].get())
            number = max(number, 0)
            self.algo_ops['cellsize'].set(str(number))
        except Exception:
            messagebox.showerror("Ultimate trace tool", "Incorrect cell size!")
            return

        if self.algo_ops['loglevel'].get() not in ["0", "0.0", "0.5", "1.0", 
                                                   "1", "1.5", "2.0", "2"]:
            messagebox.showerror("Ultimate trace tool", "Incorrect log level!")
            return

        self.master.destroy()

    def create_menu(self):
        widget_counter = 0
        self.cellsize_button = tk.Entry(self.master, 
                                 bg = ENTRY_COLOR, width = 8,
                                 fg = "black", textvariable=self.algo_ops['cellsize'])
        self.cellsize_button.place(x = self.width // 3,
                                   y = (400 * widget_counter) // (self.WID_NUM+1) + 20)
        self.cellsize_label = tk.Label(self.master, bg=MAIN_MENU_COLOR, fg=LABEL_COLOR,
                                   text="Cell size:", justify = "left")
        self.cellsize_label.place(x = self.width//12,
                                 y = (400 * widget_counter) // (self.WID_NUM+1)-2 + 20)

        self.hweight_button = tk.Entry(self.master, 
                                 bg = ENTRY_COLOR, width = 8,
                                 fg = "black", textvariable=self.algo_ops['hweight'])
        self.hweight_button.place(x = (self.width*5) // 7,
                                   y = (400 * widget_counter) // (self.WID_NUM+1) + 20)
        self.hweight_label = tk.Label(self.master, bg=MAIN_MENU_COLOR, fg=LABEL_COLOR,
                                      text="Heuristic\nweight:", justify = "left")
        self.hweight_label.place(x = self.width//2 + 10,
                                 y = (400 * widget_counter) // (self.WID_NUM+1)-10 + 20)

        widget_counter += 1
        self.astar_button = tk.Radiobutton(self.master, text="A*", bg = MAIN_MENU_COLOR, 
                                 fg = LABEL_COLOR, variable=self.algo_ops['algorithm'], 
                                 value="astar")
        self.astar_button.place(x = self.width//3,
                                y = (400 * widget_counter) // (self.WID_NUM+1) + 5)
        self.dijkstra_button = tk.Radiobutton(self.master, text="Dijkstra", 
                                 bg = MAIN_MENU_COLOR, 
                                 fg = LABEL_COLOR, variable=self.algo_ops['algorithm'], 
                                 value="dijkstra")
        self.dijkstra_button.place(x = (self.width*4) // 7,
                                   y = (400 * widget_counter) // (self.WID_NUM+1) + 5)
        self.algo_label = tk.Label(self.master, bg=MAIN_MENU_COLOR, fg=LABEL_COLOR,
                                   text="Algorithm:")
        self.algo_label.place(x = self.width//12,
                              y = (400 * widget_counter) // (self.WID_NUM+1)+1 + 5)

        widget_counter += 1
        self.diagonal_button = tk.Radiobutton(self.master, text="Diagonal", 
                                 bg = MAIN_MENU_COLOR,
                                 fg = LABEL_COLOR, variable=self.algo_ops['metric'], 
                                 value="diagonal")
        self.diagonal_button.place(x = self.width//3,
                                   y = (400 * widget_counter) // (self.WID_NUM+1) - 15)
        self.euclid_button = tk.Radiobutton(self.master, text="Euclidean", 
                                 bg = MAIN_MENU_COLOR, 
                                 fg = LABEL_COLOR, variable=self.algo_ops['metric'], 
                                 value="euclidean")
        self.euclid_button.place(x = (self.width*4) // 7,
                                 y = (400 * widget_counter) // (self.WID_NUM+1) - 15)
        self.manhattan_button = tk.Radiobutton(self.master, text="Manhattan", 
                                 bg = MAIN_MENU_COLOR,
                                 fg = LABEL_COLOR, variable=self.algo_ops['metric'], 
                                 value="manhattan")
        self.manhattan_button.place(x = self.width//3,
                                    y = (400 * widget_counter) // (self.WID_NUM+1) + 25 - 15)
        self.chebyshev_button = tk.Radiobutton(self.master, text="Chebyshev", 
                                 bg = MAIN_MENU_COLOR, 
                                 fg = LABEL_COLOR, variable=self.algo_ops['metric'], 
                                 value="chebyshev")
        self.chebyshev_button.place(x = (self.width*4) // 7,
                                    y = (400 * widget_counter) // (self.WID_NUM+1) + 25 - 15)
        self.metric_label = tk.Label(self.master, bg=MAIN_MENU_COLOR, fg=LABEL_COLOR,
                                     text="Metric type:")
        self.metric_label.place(x = self.width//12,
                                y = (400 * widget_counter) // (self.WID_NUM+1)+13 - 15)

        widget_counter += 1
        self.gmax_button = tk.Radiobutton(self.master, text="g-max", bg = MAIN_MENU_COLOR,
                                 fg = LABEL_COLOR, variable=self.algo_ops['breakingties'], 
                                 value="g-max")
        self.gmax_button.place(x = self.width//3,
                               y = (400 * widget_counter) // (self.WID_NUM+1))
        self.gmin_button = tk.Radiobutton(self.master, text="g-min", 
                                 bg = MAIN_MENU_COLOR, 
                                 fg = LABEL_COLOR, variable=self.algo_ops['breakingties'], 
                                 value="g-min")
        self.gmin_button.place(x = (self.width*4) // 7,
                               y = (400 * widget_counter) // (self.WID_NUM+1))
        self.algo_label = tk.Label(self.master, bg=MAIN_MENU_COLOR, fg=LABEL_COLOR,
                                   text="Breaking ties:")
        self.algo_label.place(x = self.width//12,
                              y = (400 * widget_counter) // (self.WID_NUM+1)+1)

        widget_counter += 1
        self.diag1_button = tk.Radiobutton(self.master, text="Yes", bg = MAIN_MENU_COLOR,
                                 fg = LABEL_COLOR, variable=self.algo_ops['allowdiagonal'], 
                                 value="true")
        self.diag1_button.place(x = self.width//3,
                                y = (400 * widget_counter) // (self.WID_NUM+1))
        self.diag0_button = tk.Radiobutton(self.master, text="No", 
                                 bg = MAIN_MENU_COLOR, 
                                 fg = LABEL_COLOR, variable=self.algo_ops['allowdiagonal'], 
                                 value="false")
        self.diag0_button.place(x = (self.width*4) // 7,
                                y = (400 * widget_counter) // (self.WID_NUM+1))
        self.diag_label = tk.Label(self.master, bg=MAIN_MENU_COLOR, fg=LABEL_COLOR,
                                   text="Allow diagonal\nmoves:", justify = "left")
        self.diag_label.place(x = self.width//12,
                              y = (400 * widget_counter) // (self.WID_NUM+1)-6)

        widget_counter += 1
        self.corner1_button = tk.Radiobutton(self.master, text="Yes", bg = MAIN_MENU_COLOR,
                                 fg = LABEL_COLOR, variable=self.algo_ops['cutcorners'], 
                                 value="true")
        self.corner1_button.place(x = self.width//3,
                                  y = (400 * widget_counter) // (self.WID_NUM+1))
        self.corner0_button = tk.Radiobutton(self.master, text="No", 
                                 bg = MAIN_MENU_COLOR, 
                                 fg = LABEL_COLOR, variable=self.algo_ops['cutcorners'], 
                                 value="false")
        self.corner0_button.place(x = (self.width*4) // 7,
                                  y = (400 * widget_counter) // (self.WID_NUM+1))
        self.corner_label = tk.Label(self.master, bg=MAIN_MENU_COLOR, fg=LABEL_COLOR,
                                   text="Allow cutting\ncorners:", justify = "left")
        self.corner_label.place(x = self.width//12,
                                y = (400 * widget_counter) // (self.WID_NUM+1)-6)

        widget_counter += 1
        self.squeeze1_button = tk.Radiobutton(self.master, text="Yes", bg = MAIN_MENU_COLOR,
                                 fg = LABEL_COLOR, variable=self.algo_ops['allowsqueeze'], 
                                 value="true")
        self.squeeze1_button.place(x = self.width//3,
                                  y = (400 * widget_counter) // (self.WID_NUM+1))
        self.squeeze0_button = tk.Radiobutton(self.master, text="No", 
                                 bg = MAIN_MENU_COLOR, 
                                 fg = LABEL_COLOR, variable=self.algo_ops['allowsqueeze'], 
                                 value="false")
        self.squeeze0_button.place(x = (self.width*4) // 7,
                                   y = (400 * widget_counter) // (self.WID_NUM+1))
        self.squeeze_label = tk.Label(self.master, bg=MAIN_MENU_COLOR, fg=LABEL_COLOR,
                                   text="Allow squeeze\nmoves:", justify = "left")
        self.squeeze_label.place(x = self.width//12,
                                 y = (400 * widget_counter) // (self.WID_NUM+1)-6)

        widget_counter += 0.5
        self.loglevel_button = tk.Entry(self.master, 
                                 bg = ENTRY_COLOR, width = 8,
                                 fg = "black", textvariable=self.algo_ops['loglevel'])
        self.loglevel_button.place(x = self.width // 3,
                                   y = (400 * widget_counter) // (self.WID_NUM+1) + 20)
        self.loglevel_label = tk.Label(self.master, bg=MAIN_MENU_COLOR, fg=LABEL_COLOR,
                                   text="Log level:", justify = "left")
        self.loglevel_label.place(x = self.width//12,
                                  y = (400 * widget_counter) // (self.WID_NUM+1)-2 + 20)

        widget_counter += 1.25
        self.confirm_button = tk.Button(self.master, bg=LABEL_COLOR, fg="black",
                                        width=8, height=1, command=self.confirm,
                                        text="Confirm")
        self.confirm_button.place(x = self.width // 3, anchor="n",
                                  y = (400 * widget_counter) // (self.WID_NUM+1))
        self.back_button = tk.Button(self.master, bg=LABEL_COLOR, fg="black",
                                        width=8, height=1, command=self.default,
                                        text="Default")
        self.back_button.place(x = (self.width*2) // 3, anchor="n",
                               y = (400 * widget_counter) // (self.WID_NUM+1))

    def default(self):
        self.algo_ops['algorithm'].set('astar')
        self.algo_ops['metric'].set('diagonal')
        self.algo_ops['breakingties'].set('g-max')
        self.algo_ops['hweight'].set('1')
        self.algo_ops['cellsize'].set('1')
        self.algo_ops['allowdiagonal'].set('true')
        self.algo_ops['cutcorners'].set('true')
        self.algo_ops['allowsqueeze'].set('true')
        self.algo_ops['loglevel'].set('1')

class GeneratedAgent:
    def __init__(self, start, finish, point, label):
        self.start = start
        self.finish = finish
        self.point = point
        self.label = label

class DummyEvent:
    def __init__(self, x, y, widget):
        self.x = x
        self.y = y
        self.widget = widget

class MapGenerator(Map):
    def __init__(self, master, cell_size, map_info, options, SIDEBAR):
        super().__init__(master, cell_size, map_info, options)
        self.SIDEBAR = SIDEBAR
        self.setting_agent = False
        self.pattern = None

    def left_release(self, event):
        self.pattern = None
        if self.buffer_actions != [] and self.buffer_actions[-1][0] != 2:
            self.last_actions.append(self.buffer_actions)
            self.redo_actions = []
            self.buffer_actions = []

    def left_press(self, event):
        if self.setting_agent == True:
            self.shift_left(event)
            return
        rectangle = event.widget.find_closest(event.x, event.y)
        if rectangle[0] not in self.cells:
            return
        x = event.x // self.cell_size
        y = event.y // self.cell_size
        if x >= self.map_info.width or y >= self.map_info.height or x < 0 or y < 0:
            return
        if (x, y) in self.end_points:
            return
        if self.pattern == None:
            if self.canvas.itemcget(rectangle, 'fill') == "khaki1":
                self.map_info.grid[y][x] = 1
                self.canvas.itemconfig(rectangle, fill="goldenrod4")
                self.pattern = "goldenrod4"
                self.buffer_actions.append((0, (x, y)))
            elif self.canvas.itemcget(rectangle, 'fill') == "goldenrod4":
                self.map_info.grid[y][x] = 0
                self.canvas.itemconfig(rectangle, fill="khaki1")
                self.pattern = "khaki1"
                self.buffer_actions.append((1, (x, y)))
        elif self.pattern == "khaki1" and self.canvas.itemcget(rectangle, 'fill')=="goldenrod4":
            self.map_info.grid[y][x] = 0
            self.canvas.itemconfig(rectangle, fill="khaki1")
            self.buffer_actions.append((0, (x, y)))
        elif self.pattern == "goldenrod4" and self.canvas.itemcget(rectangle, 'fill')=="khaki1":
            self.map_info.grid[y][x] = 1
            self.canvas.itemconfig(rectangle, fill="goldenrod4")
            self.buffer_actions.append((1, (x, y)))

    def motion(self, event):
        if not self.setting_agent:
            self.left_press(event)

    def redo(self, event):
        if self.redo_actions == []:
            return
        if self.redo_actions[-1][0][0] == 3:
            start = self.redo_actions[-1][0][1][0]
            finish = self.redo_actions[-1][0][1][1]
            redo_actions_copy = copy.deepcopy(self.redo_actions)
            event1 = DummyEvent(self.cell_size*start[0]+self.cell_size//2, 
                                self.cell_size*start[1]+self.cell_size//2, self.canvas)
            event2 = DummyEvent(self.cell_size*finish[0]+self.cell_size//2, 
                                self.cell_size*finish[1]+self.cell_size//2, self.canvas)
            self.shift_left(event1)
            self.shift_left(event2)
            self.redo_actions = copy.deepcopy(redo_actions_copy)
        elif self.redo_actions[-1][0][0] == 2:
            start = self.redo_actions[-1][0][1]
            redo_actions_copy = copy.deepcopy(self.redo_actions)
            event = DummyEvent(self.cell_size*start[0]+self.cell_size//2, 
                               self.cell_size*start[1]+self.cell_size//2, self.canvas)
            self.shift_left(event)
            self.redo_actions = copy.deepcopy(redo_actions_copy)
        elif self.redo_actions[-1][0][0] == 1:
            for ind in range(len(self.redo_actions[-1])):
                pos = self.redo_actions[-1][ind][1]
                self.canvas.itemconfig(self.grid[pos[1]][pos[0]], fill="khaki1")
                self.map_info.grid[pos[1]][pos[0]] = 0
            self.last_actions.append(self.redo_actions[-1])
        else:
            for ind in range(len(self.redo_actions[-1])):
                pos = self.redo_actions[-1][ind][1]
                self.canvas.itemconfig(self.grid[pos[1]][pos[0]], fill="goldenrod4")
                self.map_info.grid[pos[1]][pos[0]] = 1
            self.last_actions.append(self.redo_actions[-1])
        self.redo_actions.pop()
        self.pattern = None

    def shift_left(self, event):
        x = event.x // self.cell_size
        y = event.y // self.cell_size
        self.add_agent(x, y, len(self.agents))

class XmlOnlyMenu(tk.Frame):
    def __init__(self, master):
        super().__init__(master)
        self.WID_NUM = 6
        self.master = master
        self.master.geometry("395x600")
        self.master.resizable(0, 0)
        self.master['background'] = MAIN_MENU_COLOR
        self.create_menu()
        self.master.title("Ultimate trace tool")

    def create_menu(self):

        self.title = tk.Label(self.master, text="Xml only mode",
                              fg = LABEL_COLOR, background = MAIN_MENU_COLOR,
                              font=("Arial bold", 22),
                              justify="center", anchor = "center")
        self.title.place(x=5*FRAME, y=4*FRAME)

        widget_counter = 1.25
        self.file_entry = tk.Entry(self.master, bg = ENTRY_COLOR, fg = "black",
                                   width=40)
        self.file_entry.place(x = 5*FRAME,
                              y = (600 * widget_counter) // (self.WID_NUM+1))
        self.file_button = tk.Button(self.master, text="...", command=self.get_file_name,
                                     height=1, width = 3, background = LABEL_COLOR)
        self.file_button.place(x = 400 - 8*FRAME,
                               y = (600 * widget_counter) // (self.WID_NUM+1) - 3)
        self.file_label = tk.Label(self.master, bg=MAIN_MENU_COLOR, fg=LABEL_COLOR,
                                   text="Your .xml file name:")
        self.file_label.place(x = 5*FRAME,
                              y = (600 * widget_counter) // (self.WID_NUM+1) + 20)

        widget_counter += 1
        self.frequency_entry = tk.Entry(self.master, bg = ENTRY_COLOR, fg = "black",
                                        width=20)
        self.frequency_entry.insert(0, "8")
        self.frequency_entry.place(x = 5*FRAME,
                                   y = (600 * widget_counter) // (self.WID_NUM+1))
        self.frequency_label = tk.Label(self.master, bg=MAIN_MENU_COLOR, fg=LABEL_COLOR,
                                        text="Steps per second")
        self.frequency_label.place(x = 5*FRAME,
                                   y = (600 * widget_counter) // (self.WID_NUM+1) + 20)

        widget_counter += 1
        self.options = dict()
        self.show_ok = tk.BooleanVar()
        self.show_ok.set(1)
        self.logs = tk.Radiobutton(self.master, text="Yes", bg = MAIN_MENU_COLOR, fg = LABEL_COLOR,
                                   variable=self.show_ok, value=1)
        self.no_logs = tk.Radiobutton(self.master, text="No", bg = MAIN_MENU_COLOR, fg = LABEL_COLOR,
                                   variable=self.show_ok, value=0)
        self.logs.place(x = 5*FRAME, y = (600 * widget_counter) // (self.WID_NUM+1)+FRAME)
        self.no_logs.place(x = 15*FRAME, y = (600 * widget_counter) // (self.WID_NUM+1)+FRAME)
        self.logs_label = tk.Label(self.master, bg=MAIN_MENU_COLOR, fg=LABEL_COLOR,
                                        text='Show "simulation finished" messages?')
        self.logs_label.place(x = 5*FRAME,
                              y = (600 * widget_counter) // (self.WID_NUM+1)-FRAME//2)

        widget_counter += 1
        self.check_options = tk.BooleanVar()
        self.check_options.set(0)
        self.check = tk.Radiobutton(self.master, text="Yes", bg = MAIN_MENU_COLOR, fg = LABEL_COLOR,
                                   variable=self.check_options, value=1)
        self.no_check = tk.Radiobutton(self.master, text="No", bg = MAIN_MENU_COLOR, fg = LABEL_COLOR,
                                   variable=self.check_options, value=0)
        self.check.place(x = 5*FRAME, y = (600 * widget_counter) // (self.WID_NUM+1)+FRAME)
        self.no_check.place(x = 15*FRAME, y = (600 * widget_counter) // (self.WID_NUM+1)+FRAME)
        self.check_label = tk.Label(self.master, bg=MAIN_MENU_COLOR, fg=LABEL_COLOR,
                                        text='Check movement options?')
        self.check_label.place(x = 5*FRAME,
                               y = (600 * widget_counter) // (self.WID_NUM+1)-FRAME//2)

        widget_counter += 1
        self.desktop_x = tk.Entry(self.master, bg = ENTRY_COLOR, fg = "black",
                                  width=20)
        self.desktop_x.insert(0, "800")
        self.desktop_x.place(x = 5*FRAME,
                             y = (600 * widget_counter) // (self.WID_NUM+1))
        self.desktop_y = tk.Entry(self.master, bg = ENTRY_COLOR, fg = "black",
                                  width=20)
        self.desktop_y.insert(0, "600")
        self.desktop_y.place(x = 20*FRAME,
                             y = (600 * widget_counter) // (self.WID_NUM+1))
        self.desktop_x_label = tk.Label(self.master, bg=MAIN_MENU_COLOR, fg=LABEL_COLOR,
                                        text="Desktop resolution X")
        self.desktop_x_label.place(x = 5*FRAME,
                                   y = (600 * widget_counter) // (self.WID_NUM+1) + 20)
        self.desktop_y_label = tk.Label(self.master, bg=MAIN_MENU_COLOR, fg=LABEL_COLOR,
                                        text="Desktop resolution Y")
        self.desktop_y_label.place(x = 20*FRAME,
                                   y = (600 * widget_counter) // (self.WID_NUM+1) + 20)

        widget_counter += 1
        self.start_button = tk.Button(self.master, bg=LABEL_COLOR, fg="black",
                                      width=15, height=1, command=self.start_simulation,
                                      text="Start simulation")
        self.start_button.place(x = 5*FRAME,
                                y = (600 * widget_counter) // (self.WID_NUM+1))
        self.quit_button = tk.Button(self.master, bg=LABEL_COLOR, fg="red",
                                      width=8, height=1, command=self.quit_,
                                      text="Quit")
        self.quit_button.place(x = 26*FRAME,
                               y = (600 * widget_counter) // (self.WID_NUM+1))

    def get_file_name(self):
        self.file_entry.delete(0, len(self.file_entry.get()))
        file_name = filedialog.askopenfilename(initialdir = "/",
                                               title = "Select .xml file",
                                               filetypes = [('XML files', '*.xml')])
        self.file_entry.insert(0, file_name)

    def quit_(self):
        try:
            self.child_window.quit_()
        except Exception:
            pass
        self.master.destroy()

    def start_simulation(self):
        map_info = XmlParser(self.file_entry.get(), "tracer")
        if not map_info.correct:
            return
        try:
            mini_size = max(map_info.width, map_info.height) * 6
            self.options['size_X'] = min(max(int(self.desktop_x.get()), mini_size), 2000)
            self.options['size_Y'] = min(max(int(self.desktop_y.get()), mini_size), 2000)
        except Exception:
            messagebox.showerror("Ultimate trace tool", "Incorrect desktop resolution")
            return
        self.options['show_messages'] = self.show_ok.get()
        self.options['check_movements'] = self.check_options.get()
        try:
            self.options['frequency'] = 1 / max(EPS, int(self.frequency_entry.get()))
        except Exception:
            messagebox.showerror("Ultimate trace tool", "Incorrect frequrncy!")
            return
        self.child_window = Simulation(tk.Toplevel(), self.options, map_info)

class Simulation(tk.Frame):
    def __init__(self, master, options, parsed_doc):
        super().__init__(master)
        self.WID_NUM = 6
        self.master = master
        self.options = options
        self.map_info = parsed_doc
        self.master.resizable(0, 0)
        self.master['background'] = MAIN_MENU_COLOR
        self.create_map_widget()
        self.master.geometry(str(self.options['size_X']) + 'x' + str(self.options['size_Y']))
        self.master.title("Simulation in progress")
        self.create_buttons()
        self.active = False
        self.total_steps = self.map_info.turns[-1][3] + 1
        self.step_counter.configure(text = "Steps: 0/" + str(self.total_steps))

    def continue_(self):
        self.undo_button['state'] = 'disabled'
        self.next_button['state'] = 'disabled'
        self.active = True
        self.loop()

    def create_buttons(self):
        button_counter = 1
        self.continue_button = tk.Button(self.master, text="Start/Continue",
                                command=self.continue_,
                                height=2, width = 15, background = LABEL_COLOR)
        self.continue_button.place(x = self.options['size_X'] - BUTTON_COLUMN//2, anchor = 'n',
                               y = (self.options['size_Y'] * button_counter) // (self.WID_NUM+1))

        button_counter += 1
        self.stop_button = tk.Button(self.master, text="Stop",
                                command=self.stop,
                                height=2, width = 15, background = LABEL_COLOR)
        self.stop_button.place(x = self.options['size_X'] - BUTTON_COLUMN//2, anchor = 'n',
                               y = (self.options['size_Y'] * button_counter) // (self.WID_NUM+1))

        button_counter += 1
        self.next_button = tk.Button(self.master, text="Next step",
                                command=self.perform_step,
                                height=2, width = 15, background = LABEL_COLOR)
        self.next_button.place(x = self.options['size_X'] - BUTTON_COLUMN//2, anchor = 'n',
                               y = (self.options['size_Y'] * button_counter) // (self.WID_NUM+1))

        button_counter += 1
        self.undo_button = tk.Button(self.master, text="Undo",
                                command=self.undo,
                                height=2, width = 15, background = LABEL_COLOR)
        self.undo_button.place(x = self.options['size_X'] - BUTTON_COLUMN//2, anchor = 'n',
                               y = (self.options['size_Y'] * button_counter) // (self.WID_NUM+1))

        button_counter += 1
        self.reset_button = tk.Button(self.master, text="Reset",
                                command=self.reset,
                                height=2, width = 15, background = LABEL_COLOR)
        self.reset_button.place(x = self.options['size_X'] - BUTTON_COLUMN//2, anchor = 'n',
                                y = (self.options['size_Y'] * button_counter) // (self.WID_NUM+1))

        button_counter += 1
        self.quit_button = tk.Button(self.master, text="Close window",
                                command=self.quit_, fg = "red",
                                height=2, width = 15, background = LABEL_COLOR)
        self.quit_button.place(x = self.options['size_X'] - BUTTON_COLUMN//2, anchor = 'n',
                               y = (self.options['size_Y'] * button_counter) // (self.WID_NUM+1))

    def create_map_widget(self):
        coef_1 = (self.options['size_X'] - BUTTON_COLUMN - 2*FRAME) // self.map_info.width
        coef_2 = (self.options['size_Y'] - 6*FRAME) // self.map_info.height
        coef = min(coef_1, coef_2)
        self.options['size_Y'] = coef * self.map_info.height + 6*FRAME
        self.options['size_X'] = coef * self.map_info.width + 2*FRAME + BUTTON_COLUMN
        self.map = MapPlay(self.master, coef, self.map_info, self.options)
        self.map.canvas.place(x=FRAME, y=5*FRAME)
        self.step_counter = tk.Label(self.master, font=("Helvetica", 20), bg = MAIN_MENU_COLOR,
                                                                          fg = ENTRY_COLOR)
        self.step_counter.place(x=2*FRAME, y=FRAME)

    def move_fail(self):
        self.stop()
        self.continue_button['state'] = 'disabled'
        self.next_button['state'] = 'disabled'
        self.step_counter.configure(text = "Steps: " + str(self.map.current_step) + "/" +
                                           str(self.total_steps))

    def loop(self):
        for step in range(self.map.current_step, self.total_steps):
            success = self.map.perform_step()
            if not success:
                self.move_fail()
                return
            time.sleep(self.options['frequency'])
            if not self.active:
                return
            self.step_counter.configure(text = "Steps: " + str(self.map.current_step) + "/" +
                                               str(self.total_steps))
            self.map.canvas.update()

        incomplete = []
        for ind in range(self.map_info.number_of_agents):
            x, y = self.map.agents[ind].start
            if x != self.map_info.finish_x[ind] or y != self.map_info.finish_y[ind]:
                incomplete.append(ind + 1)
        if len(incomplete) > 0:
            self.stop()
            answer = "Simulation finished without any bugs, but some"
            answer += " agent(s) didn`t reach their finish point(s): "
            for num in range(len(incomplete) - 1):
                answer += str(incomplete[num]) + ", "
            answer += str(incomplete[-1])
            messagebox.showerror("Ultimate trace tool", answer)
            return
        self.stop()
        if self.options['show_messages']:
            messagebox.showinfo("Ultimate trace tool",
                                "Simulation finished without any bugs")

    def quit_(self):
        self.stop()
        self.master.destroy()

    def perform_step(self):
        if self.map.current_step < self.total_steps:
            success = self.map.perform_step()
            if not success:
                self.move_fail()
                return
        self.step_counter.configure(text = "Steps: " + str(self.map.current_step) + "/" +
                                           str(self.total_steps))

    def reset(self):
        self.map.current_step = 0
        self.map.current_turn = 0
        self.next_button['state'] = 'normal'
        self.continue_button['state'] = 'normal'
        self.undo_button['state'] = 'normal'
        while (len(self.map.agents) != 0):
            self.map.delete_agent(-1)
        self.current_step = 0
        for ind in range(self.map_info.number_of_agents):
            self.map.add_agent(self.map_info.start_x[ind],
                               self.map_info.start_y[ind], ind)
            self.map.add_agent(self.map_info.finish_x[ind],
                               self.map_info.finish_y[ind], ind)
        self.map.canvas.update()
        self.active = False
        self.step_counter.configure(text = "Steps: " + str(self.map.current_step) + "/"
                                         + str(self.total_steps))

    def stop(self):
        self.undo_button['state'] = 'normal'
        self.next_button['state'] = 'normal'
        self.active = False

    def undo(self):
        if self.map.current_step == 0:
            return
        self.next_button['state'] = 'normal'
        self.continue_button['state'] = 'normal'
        self.map.undo()
        self.map.canvas.update()
        self.step_counter.configure(text = "Steps: " + str(self.map.current_step) + "/"
                                         + str(self.total_steps))

class Agent:
    def __init__(self, x, y):
        self.x = x
        self.y = y

class MapPlay(Map):
    def __init__(self, master, cell_size, map_info, options):
        super().__init__(master, cell_size, map_info, options)
        self.SIDEBAR = 2 * FRAME
        self.current_turn = 0
        self.current_step = 0
        self.turns = map_info.turns
        self.last_position = []
        for ind in range(self.map_info.number_of_agents):
            self.last_position.append([])

    def check_movement(self, cur_pos, dx, dy, first_ind):
        x, y = cur_pos[0], cur_pos[1]
        if cur_pos[0] + dx < 0 or cur_pos[0] + dx >= self.map_info.width:
            return "Agent " + str(first_ind + 1) + " goes off the map!"
        if cur_pos[1] + dy < 0 or cur_pos[1] + dy >= self.map_info.height:
            return "Agent " + str(first_ind + 1) + " goes off the map!"
        if self.check_obstacle(x + dx, y + dy):
            return "Collision between agent " + str(first_ind + 1) + " and obstacle!"
        new_pos = (cur_pos[0] + dx, cur_pos[1] + dy)
        for ind in range(len(self.agents)):
            agent = self.agents[ind]
            if agent.start == new_pos:
                return "Collision between agents " + str(first_ind+1) + " and " + str(ind+1) + "!"
        if abs(dx) > 1 or abs(dy) > 1:
            return "Too long jump from agent number " + str(first_ind + 1) + "!"
        if dx == 0 or dy == 0:
            return "OK"
        if dx != 0 and dy != 0 and self.map_info.diag == "false":
            return "allowDiagonal=false rule is not respected!"
        obstacles = 0
        if self.check_obstacle(x + dx, y):
            obstacles += 1
        if self.check_obstacle(x, y + dy):
            obstacles += 1
        if self.map_info.corners == "false" and obstacles > 0:
            return "cutCorners=false rule is not respected!"
        if self.map_info.squeeze == "false" and obstacles == 2:
            return "allowSqueeze=false rule is not respected!"
        return "OK"

    def perform_step(self):
        moved = set()
        self.current_step += 1
        while self.current_turn < len(self.turns):
            if self.turns[self.current_turn][3] == self.current_step:
                break
            turn = self.turns[self.current_turn]
            if turn[0] in moved:
                log = "Step number {}, agent {} was moved twice!".format(self.current_step, 
                                                                                   turn[0])
                messagebox.showerror("Ultimate trace tool", log)
                return False
            moved.add(turn[0])
            current_pos = self.agents[turn[0]].start
            self.last_position[turn[0]].append(current_pos)
            dx = turn[1] - current_pos[0]
            dy = turn[2] - current_pos[1]
            log = self.check_movement(current_pos, dx, dy, turn[0])
            if log != "OK":
                messagebox.showerror("Ultimate trace tool", log)
                return False
            self.delete_agent(turn[0])
            self.add_agent(turn[1], turn[2], turn[0])
            self.add_agent(self.map_info.finish_x[turn[0]], 
                           self.map_info.finish_y[turn[0]], turn[0])
            self.current_turn += 1
        return True

    def undo(self):
        if self.current_step == 0:
            return
        self.current_step -= 1
        self.current_turn -= 1
        while self.turns[self.current_turn][3] == self.current_step:
            turn = self.turns[self.current_turn]
            new_pos = self.last_position[turn[0]][-1]
            self.last_position[turn[0]].pop()
            self.delete_agent(turn[0])
            self.add_agent(new_pos[0], new_pos[1], turn[0])
            self.add_agent(self.map_info.finish_x[turn[0]], self.map_info.finish_y[turn[0]], 
                                                                                    turn[0])
            if self.current_turn == 0:
                return
            self.current_turn -= 1
        self.current_turn += 1

if __name__ == "__main__":
    root = tk.Tk()
    app = Application(root)
    app.master.mainloop()
