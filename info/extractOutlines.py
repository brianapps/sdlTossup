import xml.etree.ElementTree as ET
import re, sys, math
from typing import List, Dict


# The goal is to convert an SVG path to a simplified set of absolute coordinates with an action.
# The actions are:
# M - move to the given x, y position.
# L - line from last position to the given position.
# A, B, C - Control points of a cubic Bezier spline.
# X - eXit. This is the end of the outline
#
class Node:
    def __init__(self, x, y, action):
        self.x = x
        self.y = y
        self.action = action


def tokenize_svg_path(s : str) -> List:
    # The inkscape SVG file uses units with upto 4 digits of decimal precision. Multiply by 10,000 and store
    # co-ordinates as integers
    return [x if x.isalpha() else int(round(10000* float(x))) for x in 
        re.findall(r'[A-Za-z]|[.0-9-e]+', s)]

def process_path_new(s : str) -> List[Node]:
    state = ''
    toks = tokenize_svg_path(s)
    pos = 0
    lastX = 0
    lastY = 0
    firstX = 0
    firstY = 0
    cp = 0
    nodes : List[Node] =[]
    while pos < len(toks):
        if isinstance(toks[pos], str):
            state = toks[pos]
            pos += 1
            cp = 0

        action = state.upper()
        is_relative = state.islower()

        if action == 'M':
            lastX = toks[pos] + (lastX if is_relative else 0)
            lastY = toks[pos + 1] + (lastY if is_relative else 0)
            pos += 2
            nodes.append(Node(lastX, lastY, 'M'))
            firstX = lastX
            firstY = lastY
            state = 'l' if is_relative else 'L'
        elif action == 'L':
            lastX = toks[pos] + (lastX if is_relative else 0)
            lastY = toks[pos + 1] + (lastY if is_relative else 0)
            pos += 2
            nodes.append(Node(lastX, lastY, 'L'))
        elif action == 'C':
            # Bezier control point. We expect multiples of three points and we update the last point (used for
            # relative position) on the 3rd point
            nextX = toks[pos] + (lastX if is_relative else 0)
            nextY = toks[pos + 1] + (lastY if is_relative else 0)
            pos += 2
            nodes.append(Node(nextX, nextY, chr(ord('A') + cp)))
            cp = (cp + 1) % 3
            if cp == 0:
                lastX = nextX
                lastY = nextY
        elif action == 'H':
            lastX = toks[pos] + (lastX if is_relative else 0)
            pos += 1
            nodes.append(Node(lastX, lastY, 'L'))
        elif action == 'V':
            lastY = toks[pos] + (lastY if is_relative else 0)
            pos += 1
            nodes.append(Node(lastX, lastY, 'L'))
        elif action == 'Z':
            # Close the path with a line from the last point to the first point
            if lastX != firstX or lastY != firstY:
                lastX = firstX
                lastY = firstY
                nodes.append(Node(lastX, lastY, 'L'))
        else:
            raise(Exception('Unknown action ' + action))

    nodes.append(Node(0, 0, 'X'))
    return nodes


svg = ET.parse('outlines.svg')

all_outlines :Dict[str, List[Node]] = {}

for path_elem in svg.findall('{http://www.w3.org/2000/svg}path'):
    label = path_elem.attrib.get('{http://www.inkscape.org/namespaces/inkscape}label')
    if label:
        all_outlines[label] = process_path_new(path_elem.attrib['d'])

all_keys = list(all_outlines.keys())
all_keys.sort()

with open('../src/Outlines.h', 'w') as header:
    header.write("// This file was automatically generated by the extractOutlines.py script. Do not edit this file by hand.\n\n")
    header.write("#ifndef OUTLINES_H_\n")
    header.write("#define OUTLINES_H_\n")
    header.write("\nnamespace Outlines {\n")
    header.write("\nstruct Node {\n    int x;\n    int y;\n    char action;\n};\n\n")
    for i, k in enumerate(all_keys):
        header.write("constexpr size_t {} = {};\n".format(k.upper(), i))
    header.write("constexpr size_t COUNT = {};\n".format(len(all_keys)))
    header.write("extern const Node* ALL_OUTLINES[COUNT];\n")
    header.write("\n}  // namespace Outlines\n\n")
    header.write("#endif  // OUTLINES_H_\n")
    

with open('../src/Outlines.cpp', 'w') as cpp:
    cpp.write("// This file was automatically generated by the extractOutlines.py script. Do not edit this file by hand.\n\n")
    cpp.write("#include \"outlines.h\"\n\n")

    cpp.write("namespace Outlines {\n")

    for k in all_keys:
        outline = all_outlines[k]
        cpp.write("\nconst Node OUTLINE_{}[] = {{".format(k.upper()))
        for i, node in enumerate(outline):
            if i % 4 == 0:
                cpp.write('\n    ')
            cpp.write("{{{:8},{:8}, '{}'}}, ".format(node.x,node.y, node.action))
        cpp.write("\n};\n")

    cpp.write("\nconst Node* ALL_OUTLINES[{}] = {{".format(len(all_keys)))
    for k in all_keys:
        cpp.write("\n    OUTLINE_{},".format(k.upper()))
    cpp.write("\n};\n")
    cpp.write("\n}  // namespace Outlines\n")


