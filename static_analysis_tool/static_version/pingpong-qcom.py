# Boomerang-finder for QSEE -- static version
# @author Eric "subwire" Gustafson
#
# This script finds ways to reach the qsee_register_shared_buffer() function in a
# QSEE TA.  This one does it statically, by tracing the callgraph backward, and 
# doing a few fancy tricks to get around some weird stuff like indirect jumps
# Ideally, the traced paths in the callgraph should reach one single point;
# However, bugs in angr make doing this for every binary difficult, and whether
# this works or not is a good measure of the completeness of angr's CFG and CG for 
# real ARM/THUMB binaries. Also note, of course since this is static, we only know that a possible
# path exists.  This is much, much faster than symbolic execution, but doens't give you 
# the same kind of guarantees and detailed contextual information.
#
# Note that, reaching 0x0 isn't the goal; these TAs use the 0x0 part to initialize a callback, found later in the binary,
# which is triggered when the normal world makes an SMC call.
# 
# You'll need some QSEE TAs to do anything with this.  They're around, if you know where to look
#
# While this isn't very useful to a lot of people, it's provided anyway as a nice example of using
# angr for something other than symbolic execution


import angr
import sys
from copy import deepcopy
def find_validator(p):
    for f in p.kb.functions:
        for b in p.kb.functions[f].blocks:
            for inst in b.capstone.insns:
                if inst.bytes == b'o\xf0\xfb\x00':
                    return p.kb.functions[f]

def find_handler(p):
    # HACK FIXME: when the CFG is complete
    # Bugs in angr make finding the actual handler a problem
    # For now, just give it some nonsense so that it goes until it can't anymore
    return p.kb.functions[0x0]

def print_chain(chain):
    c = "%x" % chain[0]
    for addr in chain[1:]:
        c += "-> %x" % addr
    print c

def find_previous_functions(p,cfg,addr):
    """
    Given an addr, find the previous BB in a function in the CFG
    """
    funcs = []
    addrs_stack = [addr]
    while addrs_stack:
        cur_addr = addrs_stack.pop()
        cfg_preds = cfg.get_node(cur_addr).predecessors
        if cfg_preds:
           for pr in cfg_preds:
               try:
                   f = p.kb.functions[pr.function_address]
                   funcs.append(f.addr)
                   #print "Found function", f.addr
               except KeyError:
                   #print "Not in a function yet ", pr.addr
                   addrs_stack.append(pr.addr)
    return funcs

def trace_it(p,cfg,validator,handler):
    cg = p.kb.callgraph
    callchains = []
    next_stack = [[validator.addr]]
    while next_stack:
        cur_chain = next_stack.pop()
        preds = cg.predecessors(cur_chain[-1])
        if not preds:
            preds = find_previous_functions(p,cfg,cur_chain[1])
        if not preds:
            print_chain(cur_chain)
        for pred in preds:
            if pred in cur_chain and len(preds) > 1:
                continue
            new_chain = deepcopy(cur_chain)
            new_chain.append(pred)
            next_stack.append(new_chain)

if __name__ == '__main__':
    print "Loading binary..."
    p = angr.Project(sys.argv[1])
    print "Computing CFG..."
    cfg = p.analyses.CFGFast(resolve_indirect_jumps=True,collect_data_references=True,normalize=True,show_progressbar=True)
    print "Locating qsee_register_shared_buffer()..."
    validator = find_validator(p)
    print "Found qee_register_shared_buffer at %0x" % validator.addr
    print "Locating dispatcher..."
    handler = find_handler(p)
    print "Found dispatcher at %0x" % handler.addr
    print "Tracing callchains..."
    trace_it(p,cfg,validator,handler)
