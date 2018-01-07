import re
import xml.etree.cElementTree as ET

ns = {'xmlns': 'http://graphml.graphdrawing.org/xmlns',
      'y': 'http://www.yworks.com/xml/graphml'}

def load(file):
    return ET.parse(file)

def dump(tree, file):
    entityList = {}
    root = tree.getroot()
    for node in root.iterfind('.//xmlns:node', ns):
        entityId = node.attrib['id']
        entityParam, entityBody = parse_label(node.find('.//y:NodeLabel', ns))
        entityList[entityId] = {'type': 'node', 'id': entityId,
                                'children': [], 'precondition': None,
                                'behaviour': split_behaviour_name(entityBody),
                                'param': entityParam, 'body': entityBody}
    for edge in root.iterfind('.//xmlns:edge', ns):
        entityId = edge.attrib['id']
        entityParam, entityBody = parse_label(edge.find('.//y:EdgeLabel', ns))
        entityList[entityId] = {'type': 'edge', 'id': entityId,
                                'param': entityParam, 'body': entityBody}
        if 'source' in edge.attrib and 'target' in edge.attrib:
            targetEntity = entityList[edge.attrib['target']]
            targetEntity['precondition'] = entityList[entityId]
            entityList[edge.attrib['source']]['children'].append(targetEntity)
    fp = open(file, 'wb')
    behaviourWaitingRoom = []
    for entity in entityList.itervalues():
        if entity['type'] == 'node' and not entity['precondition']:
            write_tree_source(fp, entity)
            behaviourWaitingRoom.append(entity)
            break
    while behaviourWaitingRoom:
        entity = behaviourWaitingRoom[0]
        precondition = entity['precondition']
        if precondition and precondition['body']:
            write_precondition_source(fp, precondition)
        sort_behaviour_children(entity)
        write_behaviour_source(fp, entity)
        behaviourWaitingRoom[:1] = entity['children']
    fp.close()

def parse_label(label):
    if label is None:
        return (), ''
    rawText = label.text
    tidyText = re.sub(r'\s+', ' ', rawText.strip())
    if not tidyText.startswith('['):
        return (), tidyText
    paramText, bodyText = tidyText[1:].split(']', 1)
    return re.split(r'\s?,\s?', paramText.strip()), bodyText.lstrip()

def rewrite_precondition(text):
    def replace_possible_method(matchobj):
        if matchobj.group(1) not in ('and', 'or', 'not'):
            return 'obj:' + matchobj.group(1) + '('
        return matchobj.group(1) + ' ('
    tidyText = re.sub(r'\s?\)', ')', re.sub(r'\s?,\s?', ', ', text))
    return re.sub(r'(\w+)\s?\(', replace_possible_method, tidyText)

def rewrite_behaviour(text):
    tidyText = re.sub(r'\s?\)', ')', re.sub(r'\s?,\s?', ', ', text))
    nameText, paramText = re.split(r'\s?\(\s?', tidyText, 1)
    extraText = '(bb, ' if paramText != ')' else '(bb'
    return nameText + extraText + paramText

def split_behaviour_name(text):
    return re.split(r'\s?\(', text, 1)[0]

def sort_behaviour_children(entity):
    def compare(child1, child2):
        precondition1, precondition2 = child1['precondition'], child2['precondition']
        if not precondition1 or not precondition2: return 0
        param1, param2 = precondition1['param'], precondition2['param']
        if not param1 or not param2: return 0
        return int(param1[0]) - int(param2[0])
    if entity['behaviour'] in ('AIPrioritySelector', 'AISequenceSelector', 'AISequenceNode'):
        entity['children'].sort(compare)

def write_precondition_source(fp, entity):
    fp.write('function t.%s(obj)\n' % (entity['id']))
    fp.write('\treturn %s\n' % (rewrite_precondition(entity['body'])))
    fp.write('end\n\n')

def write_behaviour_source(fp, entity):
    def filter_parameter(entity, number):
        param = entity['param']
        if param and len(param) >= number:
            return param[:number]
        return None
    def filter_parameter_when_append_child(entity, child):
        if entity['behaviour'] in ('AIWeightedSelector',):
            precondition = child['precondition']
            if precondition:
                parameter = filter_parameter(precondition, 1)
                if parameter:
                    return parameter
            return (1,)
        return ()
    fp.write('function t.%s()\n' % (entity['id']))
    fp.write('\tlocal node = %s\n' % (rewrite_behaviour(entity['body'])))
    precondition = entity['precondition']
    if precondition and precondition['body']:
        fp.write('\tnode:SetExternalPrecondition(t.%s)\n' % (precondition['id']))
    behaviourName = entity['behaviour']
    if behaviourName == 'AISequenceNode':
        parameter = filter_parameter(entity, 1)
        if parameter:
            fp.write('\tnode:SetLoopParams(%s)\n' % (', '.join(parameter)))
    elif behaviourName == 'AIParallelNode':
        parameter = filter_parameter(entity, 2)
        if parameter:
            fp.write('\tnode:SetParallelParams(%s)\n' % (', '.join(parameter)))
    for child in entity['children']:
        parameter = ['%s()' % (child['id'])]
        parameter.extend(filter_parameter_when_append_child(entity, child))
        fp.write('\tnode:AppendChildNode(t.%s)\n' % (', '.join(parameter)))
    fp.write('\treturn node\n')
    fp.write('end\n\n')

def write_tree_source(fp, entity):
    fp.write('local bb, t = nil, {}\n')
    fp.write('function BuildBehaviorTree(handler)\n')
    fp.write('\tbb = handler:blackboard()\n')
    fp.write('\thandler:SetRootNode(t.%s())\n' % (entity['id']))
    fp.write('end\n\n')
