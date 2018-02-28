import os
import re
import contextlib
import xml.etree.cElementTree as ET

ns = {'xmlns': 'http://graphml.graphdrawing.org/xmlns',
      'y': 'http://www.yworks.com/xml/graphml'}

def load(file):
    return ET.parse(file)

def dump(cwd, tree, file):
    entityList = {}
    treeRoot = tree.getroot()
    for node in treeRoot.iterfind('.//xmlns:node', ns):
        entityId = node.attrib['id']
        entityParam, entityBody, rawText = parse_label(node.find('.//y:NodeLabel', ns))
        entityList[entityId] = {'type': 'node', 'id': entityId,
                                'children': [], 'precondition': None, 'behaviour': '',
                                'param': entityParam, 'body': entityBody, 'text': rawText}
    for edge in treeRoot.iterfind('.//xmlns:edge', ns):
        entityId = edge.attrib['id']
        entityParam, entityBody, rawText = parse_label(edge.find('.//y:EdgeLabel', ns))
        entityList[entityId] = {'type': 'edge', 'id': entityId,
                                'param': entityParam, 'body': entityBody, 'text': rawText}
        if 'source' in edge.attrib and 'target' in edge.attrib:
            sourceEntity = entityList[edge.attrib['source']]
            targetEntity = entityList[edge.attrib['target']]
            sourceEntity['children'].append(targetEntity)
            targetEntity['precondition'] = entityList[entityId]
    with contextlib.closing(open(file, 'wb')) as fp:
        for entity in entityList.itervalues():
            if entity['type'] == 'node' and not entity['children'] and not entity['precondition']:
                write_whiteboard_data(fp, cwd, entity['text'])
                break
        for entity in entityList.itervalues():
            if entity['type'] == 'node' and entity['children'] and not entity['precondition']:
                write_behaviour_tree(fp, entity)
                break

def write_behaviour_tree(fp, root):
    write_entry_source(fp, root)
    behaviourWaitingRoom = [root]
    while behaviourWaitingRoom:
        entity = behaviourWaitingRoom[0]
        precondition = entity['precondition']
        if precondition and precondition['body']:
            write_precondition_source(fp, precondition)
        parse_behaviour_type(entity)
        sort_behaviour_children(entity)
        write_behaviour_source(fp, entity)
        behaviourWaitingRoom[:1] = entity['children']

def write_whiteboard_data(fp, cwd, text):
    if text.lstrip().startswith('file://'):
        with contextlib.closing(open(os.path.join(cwd, text.strip()[7:]), 'rb')) as fh:
            text = fh.read()
    for matchobj in re.finditer(r'\s*(\w+)\s*=(.+?);', text, re.DOTALL):
        fp.write('local %s = %s\n' % (matchobj.group(1), matchobj.group(2).strip()))
    fp.write('----------------------------------------------------------------\n\n')

def parse_label(label):
    if label is None:
        return (), '', ''
    rawText = label.text
    tidyText = re.sub(r'\s+', ' ', rawText.strip())
    if not tidyText.startswith('['):
        return (), tidyText, rawText
    paramText, bodyText = tidyText[1:].split(']', 1)
    return re.split(r'\s?,\s?', paramText.strip()), bodyText.lstrip(), rawText

def rewrite_precondition(text):
    def replace_possible_method(matchobj):
        if matchobj.group(1) not in ('and', 'or', 'not'):
            return 'obj:' + matchobj.group(1) + '('
        return matchobj.group(1) + ' ('
    tidyText = re.sub(r'\s?\(\s?', ' (', re.sub(r'\s?\)\s?', ') ', text))
    tidyText = re.sub(r'\(\s?\(', '((', re.sub(r'\)\s?\)', '))', tidyText))
    tidyText = re.sub(r'\(\s?\(', '((', re.sub(r'\)\s?\)', '))', tidyText))
    tidyText = re.sub(r'\s?,\s?', ', ', tidyText.strip())
    return re.sub(r'(\w+)\s\(', replace_possible_method, tidyText)

def rewrite_behaviour(text):
    finalText = rewrite_precondition(text).replace('obj:', '', 1)
    nameText, paramText = finalText.split('(', 1)
    extraText = '(bb' if paramText == ')' else '(bb, '
    return nameText + extraText + paramText

def split_behaviour_name(text):
    return text.split('(', 1)[0].rstrip()

def parse_behaviour_type(entity):
    entity['behaviour'] = split_behaviour_name(entity['body'])

def sort_behaviour_children(entity):
    def compare(child1, child2):
        precondition1, precondition2 = child1['precondition'], child2['precondition']
        if not precondition1 or not precondition2: return 0
        param1, param2 = precondition1['param'], precondition2['param']
        if not param1 or not param2: return 0
        return int(param1[0]) - int(param2[0])
    if entity['behaviour'] in ('AIPrioritySelector',):
        entity['children'].sort(compare)

def write_precondition_source(fp, entity):
    fp.write('function t.%s()\n' % (entity['id']))
    fp.write('\treturn %s\n' % (rewrite_precondition(entity['body'])))
    fp.write('end\n\n')

def write_behaviour_source(fp, entity):
    fp.write('function t.%s()\n' % (entity['id']))
    fp.write('\tlocal node = %s\n' % (rewrite_behaviour(entity['body'])))
    precondition = entity['precondition']
    if precondition and precondition['body']:
        fp.write('\tnode:SetExternalPrecondition(t.%s)\n' % (precondition['id']))
    for child in entity['children']:
        fp.write('\tnode:AppendChildNode(t.%s())\n' % (child['id']))
    fp.write('\treturn node\n')
    fp.write('end\n\n')

def write_entry_source(fp, entity):
    fp.write('local obj, bb, t = nil, nil, {}\n')
    fp.write('function BuildBehaviorTree(handler)\n')
    fp.write('\tobj = handler\n')
    fp.write('\tbb = obj:GetBlackboard()\n')
    fp.write('\tobj.Tree:SetRootNode(t.%s())\n' % (entity['id']))
    fp.write('end\n\n')
