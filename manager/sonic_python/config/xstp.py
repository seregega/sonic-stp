#!/usr/sbin/env python
#
# 'spanning-tree' group ('config spanning-tree ...')
#

import click
import utilities_common.cli as clicommon
from natsort import natsorted
import logging
import utilities_common.stp_util as stp_common
import re

XSTP_FORWARD_DELAY_MIN = 4
XSTP_FORWARD_DELAY_MAX = 30
XSTP_FORWARD_DELAY_DFLT = 15

XSTP_HELLO_INTERVAL_MIN = 1
XSTP_HELLO_INTERVAL_MAX = 10
XSTP_HELLO_INTERVAL_DFLT = 2

XSTP_MAX_AGE_MIN = 6
XSTP_MAX_AGE_MAX = 40
XSTP_MAX_AGE_DFLT = 20

XSTP_PRIORITY_MIN = 0
XSTP_PRIORITY_MAX = 61440

XSTP_TRANSMISSION_LIMIT_MIN = 1
XSTP_TRANSMISSION_LIMIT_MAX = 10

XSTP_MST_MAX_HOPS_MIN = 1
XSTP_MST_MAX_HOPS_MAX = 40

XSTP_MST_REVISION_MIN = 0
XSTP_MST_REVISION_MAX = 65535

XSTP_MST_NAME_MAX_LEN = 32

XSTP_MST_INSTANCE_ID_MIN = 0
XSTP_MST_INSTANCE_ID_MAX = 4094

XSTP_MST_INSTANCE_VLAN_MIN = 1
XSTP_MST_INSTANCE_VLAN_MAX = 4094

XSTP_INTERFACE_PRIORITY_MIN = 0
XSTP_INTERFACE_PRIORITY_MAX = 240

XSTP_INTERFACE_COST_MIN = 1
XSTP_INTERFACE_COST_MAX = 200000000

XSTP_INTERFACE_MST_PRIORITY_MIN = XSTP_INTERFACE_PRIORITY_MIN
XSTP_INTERFACE_MST_PRIORITY_MAX = XSTP_INTERFACE_PRIORITY_MAX

XSTP_INTERFACE_MST_COST_MIN = XSTP_INTERFACE_COST_MIN
XSTP_INTERFACE_MST_COST_MAX = XSTP_INTERFACE_COST_MAX

XSTP_INTERFACE_AUTO_RECOVERY_INTERVAL_MIN = 30
XSTP_INTERFACE_AUTO_RECOVERY_INTERVAL_MAX = 86400

XSTP_MST_INSTANCE_PRIORITY_MIN = 0
XSTP_MST_INSTANCE_PRIORITY_MAX = 61440

XSTP_MODE_DFLT = 'rstp'
XSTP_MAX_MST_VLAN_MAPPING_NUM = 64

STP_DEFAULT_ROOT_GUARD_TIMEOUT = 30
STP_DEFAULT_FORWARD_DELAY = 15
STP_DEFAULT_HELLO_INTERVAL = 2
STP_DEFAULT_MAX_AGE = 20
STP_DEFAULT_BRIDGE_PRIORITY = 32768
STP_DEFAULT_LBD_SHUTDOWN_INTERVAL = 60

##################################
# STP parameter validations
##################################

def is_valid_forward_delay(ctx, forward_delay):
    if forward_delay not in range(XSTP_FORWARD_DELAY_MIN, XSTP_FORWARD_DELAY_MAX + 1):
        ctx.fail("STP forward delay value must be in range {}-{}.".format(XSTP_FORWARD_DELAY_MIN, XSTP_FORWARD_DELAY_MAX))

def is_valid_hello_interval(ctx, hello_interval):
    if hello_interval not in range(XSTP_HELLO_INTERVAL_MIN, XSTP_HELLO_INTERVAL_MAX + 1):
        ctx.fail("STP hello timer must be in range {}-{}.".format(XSTP_HELLO_INTERVAL_MIN, XSTP_HELLO_INTERVAL_MAX))

def is_valid_max_age(ctx, max_age):
    if max_age not in range(XSTP_MAX_AGE_MIN, XSTP_MAX_AGE_MAX + 1):
        ctx.fail("STP max age value must be in range {}-{}.".format(XSTP_MAX_AGE_MIN, XSTP_MAX_AGE_MAX))

def is_valid_transmission_limit(ctx, transmission_limit):
    if transmission_limit not in range(XSTP_TRANSMISSION_LIMIT_MIN, XSTP_TRANSMISSION_LIMIT_MAX + 1):
        ctx.fail("STP transmission limit value must be in range {}-{}.".format(XSTP_TRANSMISSION_LIMIT_MIN, XSTP_TRANSMISSION_LIMIT_MAX))

def is_valid_bridge_priority(ctx, priority):
    if priority not in range(XSTP_PRIORITY_MIN, XSTP_PRIORITY_MAX + 1):
        ctx.fail("STP bridge priority must be in range {}-{}.".format(XSTP_PRIORITY_MIN, XSTP_PRIORITY_MAX))
    if priority % 4096 != 0:
        ctx.fail("STP bridge priority must be multiple of 4096.")

def is_valid_mst_max_hops(ctx, max_hops):
    if max_hops not in range(XSTP_MST_MAX_HOPS_MIN, XSTP_MST_MAX_HOPS_MAX + 1):
        ctx.fail("MST max hops value must be in range {}-{}.".format(XSTP_MST_MAX_HOPS_MIN, XSTP_MST_MAX_HOPS_MAX))

def is_valid_mst_name(ctx, name):
    if XSTP_MST_NAME_MAX_LEN < len(name):
        ctx.fail("The max length of MST name is {}.".format(XSTP_MST_NAME_MAX_LEN))

def is_valid_mst_revision(ctx, revision):
    if revision not in range(XSTP_MST_REVISION_MIN, XSTP_MST_REVISION_MAX + 1):
        ctx.fail("MST revision number must be in range {}-{}.".format(XSTP_MST_REVISION_MIN, XSTP_MST_REVISION_MAX))

def is_valid_mst_id(ctx, mst_id):
    if mst_id not in range(XSTP_MST_INSTANCE_ID_MIN, XSTP_MST_INSTANCE_ID_MAX + 1):
        ctx.fail("MST instance ID must be in range {}-{}.".format(XSTP_MST_INSTANCE_ID_MIN, XSTP_MST_INSTANCE_ID_MAX))

def is_valid_mst_instance_bridge_priority(ctx, priority):
    if priority not in range(XSTP_MST_INSTANCE_PRIORITY_MIN, XSTP_MST_INSTANCE_PRIORITY_MAX + 1):
        ctx.fail("MST bridge priority of a spanning tree instance must be in range {}-{}.".format(XSTP_MST_INSTANCE_PRIORITY_MIN, XSTP_MST_INSTANCE_PRIORITY_MAX))
    if priority % 4096 != 0:
        ctx.fail("MST bridge priority of a spanning tree instance must be multiple of 4096.")

def is_valid_mst_instance_vlan(ctx, vlan):
    if vlan not in range(XSTP_MST_INSTANCE_VLAN_MIN, XSTP_MST_INSTANCE_VLAN_MAX + 1):
        ctx.fail("MST VLAN ID must be in range {}-{}.".format(XSTP_MST_INSTANCE_VLAN_MIN, XSTP_MST_INSTANCE_VLAN_MAX))

def is_valid_interface_auto_recover_interval(ctx, interval):
    if interval not in range(XSTP_INTERFACE_AUTO_RECOVERY_INTERVAL_MIN, XSTP_INTERFACE_AUTO_RECOVERY_INTERVAL_MAX + 1):
        ctx.fail("BPDU guard auto-recovery value must be in range {}-{}.".format(XSTP_INTERFACE_AUTO_RECOVERY_INTERVAL_MIN, XSTP_INTERFACE_AUTO_RECOVERY_INTERVAL_MAX))

def is_valid_interface_priority(ctx, priority):
    if priority not in range(XSTP_INTERFACE_PRIORITY_MIN, XSTP_INTERFACE_PRIORITY_MAX + 1):
        ctx.fail("Interface bridge priority must be in range {}-{}.".format(XSTP_INTERFACE_PRIORITY_MIN, XSTP_INTERFACE_PRIORITY_MAX))

def is_valid_interface_cost(ctx, cost):
    if cost not in range(XSTP_INTERFACE_COST_MIN, XSTP_INTERFACE_COST_MAX + 1):
        ctx.fail("Interface cost must be in range {}-{}.".format(XSTP_INTERFACE_COST_MIN, XSTP_INTERFACE_COST_MAX))

def is_valid_interface_mst_instance_priority(ctx, priority):
    if priority not in range(XSTP_INTERFACE_MST_PRIORITY_MIN, XSTP_INTERFACE_MST_PRIORITY_MAX + 1):
        ctx.fail("Interface MST bridge priority must be in range {}-{}.".format(XSTP_INTERFACE_MST_PRIORITY_MIN, XSTP_INTERFACE_MST_PRIORITY_MAX))

def is_valid_interface_mst_instance_cost(ctx, cost):
    if cost not in range(XSTP_INTERFACE_MST_COST_MIN, XSTP_INTERFACE_MST_COST_MAX + 1):
        ctx.fail("Interface MST cost must be in range {}-{}.".format(XSTP_INTERFACE_MST_COST_MIN, XSTP_INTERFACE_MST_COST_MAX))

def validate_transmission_params(forward_delay, max_age, hello_time):
    if (2 * (int(forward_delay) - 1)) >= int(max_age) >= (2 * (int(hello_time) + 1) ):
        return True
    return False

def is_valid_transmission_params(ctx, db, forward_delay = None, max_age = None, hello_time = None):
    forward_delay = forward_delay or db.get_entry('XSTP_GLOBAL', 'forward-time').get('value')
    max_age = max_age or db.get_entry('XSTP_GLOBAL', 'max-age').get('value')
    hello_time = hello_time or db.get_entry('XSTP_GLOBAL', 'hello-time').get('value')

    if validate_transmission_params(forward_delay, max_age, hello_time) != True:
        ctx.fail("2*(forward_delay-1) >= max_age >= 2*(hello_time +1 ) not met")

def check_if_vlan_exist_in_db(db, ctx, vid):
    vlan_name = 'Vlan{}'.format(vid)
    vlan = db.get_entry('VLAN', vlan_name)
    if len(vlan) == 0:
        ctx.fail("{} doesn't exist".format(vlan_name))

def check_if_global_stp_enabled(db, ctx):
    if get_global_status(db) != 'enabled':
        ctx.fail("Global STP is not enabled.")

def is_global_xstp_enabled(db):
    if get_global_status(db) != 'enabled':
        return False
    return True

def is_stp_enabled_for_interface(db, intf_name):
    stp_entry = db.get_entry('XSTP_INTERFACE', intf_name)
    stp_enabled = stp_entry.get("spanning-disable")
    if stp_enabled == "disabled":
        return True
    else:
        return False

def check_if_stp_enabled_for_interface(ctx, db, intf_name):
    if not is_stp_enabled_for_interface(db, intf_name):
        ctx.fail("STP is not enabled for interface {}".format(intf_name))

def get_vlan_list_for_interface(db, interface_name):
    vlan_intf_info = db.get_table('VLAN_MEMBER')
    vlan_list = []
    for line in vlan_intf_info:
        if interface_name == line[1]:
            vlan_name = line[0]
            vlan_list.append(vlan_name)
    return vlan_list

def get_pc_member_port_list(db):
    pc_member_info = db.get_table('PORTCHANNEL_MEMBER')
    pc_member_port_list = []
    for line in pc_member_info:
        intf_name = line[1]
        pc_member_port_list.append(intf_name)
    return pc_member_port_list

def is_vlan_configured_interface(db, interface_name):
    intf_to_vlan_list = get_vlan_list_for_interface(db, interface_name)
    if intf_to_vlan_list:  # if empty
        return True
    else:
        return False

def is_portchannel_member_port(db, interface_name):
    pc_member_port_list = get_pc_member_port_list(db)
    if interface_name in pc_member_port_list:
        return True
    else:
        return False

def check_if_interface_is_valid(ctx, db, interface_name):
    from config.main import interface_name_is_valid
    if interface_name_is_valid(db, interface_name) is False:
        ctx.fail("Interface name is invalid. Please enter a valid interface name!!")
    for key in db.get_table('INTERFACE'):
        if type(key) != tuple:
            continue
        if key[0] == interface_name:
            ctx.fail(" {} has ip address {} configured - It's not a L2 interface".format(interface_name, key[1]))
    if is_portchannel_member_port(db, interface_name):
        ctx.fail(" {} is a portchannel member port - STP can't be configured".format(interface_name))
    if not is_vlan_configured_interface(db, interface_name):
        ctx.fail(" {} has no VLAN configured - It's not a L2 interface".format(interface_name))

def get_mst_mapping_count(db):
    count = 0
    mst_dict = db.cfgdb.get_table('XSTP_MST')
    for id in mst_dict.keys():
        vlan_str = db.cfgdb.get_entry('XSTP_MST', id).get('vlan_str')
        vid_list = expand_and_sort(vlan_str.split(','))
        count += len(vid_list)
    return count

def get_interface_admin_edge_port_status(interface_name):
    int_info = clicommon.run_command('show spanning-tree interface {}'.format(interface_name), return_cmd=True)
    admin_edge_port = re.search(r'Admin Edge Port\s+:\s+(\w+)', int_info).group(1)
    return admin_edge_port

def is_admin_edge_port_enabled_for_interface(db, interface_name):
    admin_edge_port_status = get_interface_admin_edge_port_status(interface_name)
    if admin_edge_port_status == "Disabled":
        return False
    else:
        return True


##################################
# STP common functions
##################################

def expand_and_sort(num_list):
    nums = []
    nums_str = sorted(set(num_list))
    for i in nums_str:
        if '-' in i:
            start_i = int(i.split('-')[0])
            end_i = int(i.split('-')[1])
            for num in range(start_i, end_i+1):
                nums.append(int(num))
        else:
            nums.append(int(i))
    nums = sorted(set(nums))
    return nums

def join_range(nums):
    ranges = []
    start = end = nums[0]
    for n in nums[1:] + [None]:
        if n != end + 1:
            ranges.append(str(start) if start == end else f"{start}-{end}")
            start = n
        end = n
    return ",".join(ranges)

def range_string(num_list):
    nums_str = sorted(set(num_list))
    if len(num_list) <= 1:
        return nums_str[0]

    nums = expand_and_sort(num_list)
    ranges = join_range(nums)
    return ranges

def get_global_status(db):
    entry = db.get_entry('XSTP_GLOBAL', 'Global')
    return entry.get('value')

def get_global_stp_mode(db):
    entry = db.get_entry('XSTP_GLOBAL', 'mode')
    return entry.get('value')

def set_interface_enable_stp(db, ifname):
    db.set_entry('XSTP_INTERFACE', ifname, {'spanning-disable': 'disabled'})

def disable_spanning_tree(db):
    db.delete_table('XSTP_MST_INTERFACE')
    db.delete_table('XSTP_INTERFACE')
    db.delete_table('XSTP_MST')
    db.delete_table('XSTP_MST_GLOBAL')
    db.delete_table('XSTP_GLOBAL')

def disable_interface_bpdu_guard(db, interface_name):
    cur_data = db.get_entry('XSTP_INTERFACE', interface_name)
    if 'auto-recovery' in cur_data.keys():
        del cur_data['auto-recovery']
    if 'auto-recovery-interval' in cur_data.keys():
        del cur_data['auto-recovery-interval']
    cur_data['bpdu-guard'] = 'disabled'
    db.set_entry('XSTP_INTERFACE', interface_name, cur_data)


###############################################
# STP commands implementation
###############################################

@click.group(cls=clicommon.AbbreviationGroup, name='spanning-tree')
def xstp_spanning_tree():
    """STP command line"""
    pass


###############################################
# STP Global commands implementation
###############################################

# cmd: config spanning-tree enable {mstp | rstp | stp | pvst}
@xstp_spanning_tree.command('enable')
@click.argument('mode', metavar='<mstp, rstp, stp, pvst>', required=True, type=click.Choice(['mstp', 'rstp', 'stp', 'pvst']))
@clicommon.pass_db
def spanning_tree_enable(db, mode):
    """Enable STP"""
    if mode == 'pvst':
        click.confirm("The current configurations will be saved, and the device will restart. Continue?", abort=True)

        disable_spanning_tree(db.cfgdb)
        fvs = {'mode': mode,
           'rootguard_timeout': STP_DEFAULT_ROOT_GUARD_TIMEOUT,
           'forward_delay': STP_DEFAULT_FORWARD_DELAY,
           'hello_time': STP_DEFAULT_HELLO_INTERVAL,
           'max_age': STP_DEFAULT_MAX_AGE,
           'priority': STP_DEFAULT_BRIDGE_PRIORITY
           }
        db.cfgdb.set_entry('STP', "GLOBAL", fvs)
        stp_common.enable_stp_for_interfaces(db.cfgdb)
        stp_common.enable_stp_for_vlans(db.cfgdb)

        stp_common.save_and_backup_config()
        clicommon.run_command('config reload -y -f', display_cmd=True)
        return

    ctx = click.get_current_context()
    current_global_status = get_global_status(db.cfgdb)
    if current_global_status == 'enabled':
        if get_global_stp_mode(db.cfgdb) == mode:
            ctx.fail('{} is already configured.'.format(mode.upper()))

    # change mode or first time
    if current_global_status != 'enabled':
        db.cfgdb.set_entry('XSTP_GLOBAL', 'Global', {'value': 'enabled'})

    db.cfgdb.set_entry('XSTP_GLOBAL', 'mode',   {'value': mode})

    if db.cfgdb.get_entry('XSTP_GLOBAL', 'forward-time').get('value') == None:
        db.cfgdb.set_entry('XSTP_GLOBAL', 'forward-time',   {'value': XSTP_FORWARD_DELAY_DFLT})

    if db.cfgdb.get_entry('XSTP_GLOBAL', 'hello-time').get('value') == None:
        db.cfgdb.set_entry('XSTP_GLOBAL', 'hello-time', {'value': XSTP_HELLO_INTERVAL_DFLT})

    if db.cfgdb.get_entry('XSTP_GLOBAL', 'max-age').get('value') == None:
        db.cfgdb.set_entry('XSTP_GLOBAL', 'max-age',    {'value': XSTP_MAX_AGE_DFLT})

    members = [ v for k, v in db.cfgdb.get_table('VLAN_MEMBER')]
    members = list(dict.fromkeys(members))
    for member in members:
        set_interface_enable_stp(db.cfgdb, member)

# cmd: config spanning-tree disable [mstp | rstp | stp | pvst]
@xstp_spanning_tree.command('disable')
@click.argument('mode', metavar='<mstp, rstp, stp, pvst>', required=False, type=click.Choice(['mstp', 'rstp', 'stp', 'pvst']))
@clicommon.pass_db
def spanning_tree_disable(db, mode):
    """Disable STP"""
    ctx = click.get_current_context()
    if get_global_status(db.cfgdb) == 'enabled':
        if mode != None:
            current_mode = get_global_stp_mode(db.cfgdb)
            if mode != current_mode:
                ctx.fail("Current spanning-tree mode is " + current_mode + ", please specify correct mode. ")
        disable_spanning_tree(db.cfgdb)
        db.cfgdb.set_entry('XSTP_GLOBAL', 'Global', {'value': 'disabled'})

# cmd: config spanning-tree forward_delay <value>
@xstp_spanning_tree.command('forward_delay')
@click.argument('forward_delay', metavar='<4-30 seconds>', required=True, type=int)
@clicommon.pass_db
def stp_global_forward_delay(db, forward_delay):
    """Configure STP global forward delay"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    is_valid_forward_delay(ctx, forward_delay)
    is_valid_transmission_params(ctx, db.cfgdb, forward_delay = forward_delay)
    db.cfgdb.mod_entry('XSTP_GLOBAL', 'forward-time', {'value': forward_delay})

# cmd: config spanning-tree hello <value>
@xstp_spanning_tree.command('hello')
@click.argument('hello_interval', metavar='<1-10 seconds>', required=True, type=int)
@clicommon.pass_db
def stp_global_hello_interval(db, hello_interval):
    """Configure STP global hello interval"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    is_valid_hello_interval(ctx, hello_interval)
    is_valid_transmission_params(ctx, db.cfgdb, hello_time = hello_interval)
    db.cfgdb.mod_entry('XSTP_GLOBAL', 'hello-time', {'value': hello_interval})

# cmd: config spanning-tree max_age <value>
@xstp_spanning_tree.command('max_age')
@click.argument('max_age', metavar='<6-40 seconds>', required=True, type=int)
@clicommon.pass_db
def stp_global_max_age(db, max_age):
    """Configure STP global max_age"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    is_valid_max_age(ctx, max_age)
    is_valid_transmission_params(ctx, db.cfgdb, max_age = max_age)
    db.cfgdb.mod_entry('XSTP_GLOBAL', 'max-age', {'value': max_age})

# cmd: config spanning-tree priority <value>
@xstp_spanning_tree.command('priority')
@click.argument('priority', metavar='<0-61440>', required=True, type=int)
@clicommon.pass_db
def stp_global_priority(db, priority):
    """Configure STP global bridge priority"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    is_valid_bridge_priority(ctx, priority)
    db.cfgdb.mod_entry('XSTP_GLOBAL', 'priority', {'value': priority})

# # (option) cmd: config spanning-tree transmission_limit <value>
# @xstp_spanning_tree.command('transmission_limit')
# @click.argument('transmission_limit', metavar='<1-10>', required=True, type=int)
# @clicommon.pass_db
# def stp_global_transmission_limit(db, transmission_limit):
#     """Configure the minimum interval between the transmission of consecutive RSTP/MSTP BPDUs"""
#     ctx = click.get_current_context()
#     check_if_global_stp_enabled(db.cfgdb, ctx)
#     is_valid_transmission_limit(ctx, transmission_limit)
#     db.cfgdb.mod_entry('XSTP_GLOBAL', 'transmission-limit', {'value': transmission_limit})

# # (option) cmd: config spanning-tree pathcost method {long | short}
# @xstp_spanning_tree.command('pathcost')
# @xstp_spanning_tree.command('method')
# @click.argument('method', metavar='<long, short>', required=True, type=click.Choice(['long', 'short']))
# @clicommon.pass_db
# def stp_global_pathcost_method(db, method):
#     """Configure the path cost method used for Rapid Spanning Tree and Multiple Spanning Tree"""
#     ctx = click.get_current_context()
#     check_if_global_stp_enabled(db.cfgdb, ctx)
#     db.cfgdb.set_entry('XSTP_GLOBAL', 'method', {'value': method})


###############################################
# STP MST commands implementation
###############################################

@xstp_spanning_tree.group('mst')
@clicommon.pass_db
def spanning_tree_mst(db):
    """Configure MST settings"""
    pass

# cmd: config spanning-tree mst max_hops <value>
@spanning_tree_mst.command('max_hops')
@click.argument('max_hops', metavar='<1-40>', required=True, type=int)
@clicommon.pass_db
def stp_mst_global_max_hops(db, max_hops):
    """Configure the maximum number of hops in the region before a BPDU is discarded"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    is_valid_mst_max_hops(ctx, max_hops)
    db.cfgdb.mod_entry('XSTP_MST_GLOBAL', 'max-hops', {'value': max_hops})

# cmd: config spanning-tree mst name <value>
@spanning_tree_mst.command('name')
@click.argument('name', required=True, type=str)
@clicommon.pass_db
def stp_mst_global_name(db, name):
    """Configure the name for the multiple spanning tree region in which this switch is located"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    is_valid_mst_name(ctx, name)
    db.cfgdb.mod_entry('XSTP_MST_GLOBAL', 'name', {'value': name})

# cmd: config spanning-tree mst revision <value>
@spanning_tree_mst.command('revision')
@click.argument('revision', metavar='<0-65535>', required=True, type=int)
@clicommon.pass_db
def stp_mst_global_revision(db, revision):
    """Configure the revision number for this multiple spanning tree configuration of this switch"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    is_valid_mst_revision(ctx, revision)
    db.cfgdb.mod_entry('XSTP_MST_GLOBAL', 'revision', {'value': revision})

# cmd: config spanning-tree mst priority <id> <value>
@spanning_tree_mst.command('priority')
@click.argument('id', metavar='<0-4094>', required=True, type=int)
@click.argument('priority', metavar='<0-61440>', required=True, type=int)
@clicommon.pass_db
def stp_mst_instance_priority(db, id, priority):
    """Configure the priority of a MST instance"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    is_valid_mst_id(ctx, id)
    is_valid_mst_instance_bridge_priority(ctx, priority)
    db.cfgdb.mod_entry('XSTP_MST', id, {'priority': priority})

@spanning_tree_mst.group(cls=clicommon.AbbreviationGroup, name='vlan')
@click.pass_context
def stp_mst_instance_vlan(ctx):
    """Configure VLANs of a MST instance"""
    pass

# cmd: config spanning-tree mst vlan add <id> <value>
@stp_mst_instance_vlan.command('add', short_help='Add VLAN to MSTI')
@click.argument('id', metavar='<0-4094>', required=True, type=int)
@click.argument('vlan', metavar='<1-4094>', required=True, type=int)
@clicommon.pass_db
def add(db, id, vlan):
    """Configure VLANs of a MST instance"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    is_valid_mst_id(ctx, id)
    is_valid_mst_instance_vlan(ctx, vlan)

    count = get_mst_mapping_count(db)
    if count == XSTP_MAX_MST_VLAN_MAPPING_NUM:
        ctx.fail("Exceeded the maximum limit of MSTI-VLAN mappings.")

    vlan_str_entry = db.cfgdb.get_entry('XSTP_MST', id).get('vlan_str')
    if vlan_str_entry:
        vid_list = '{},{}'.format(vlan, vlan_str_entry)
    else:
        vid_list = str(vlan)
    vlan_str = range_string(vid_list.split(','))

    db.cfgdb.mod_entry('XSTP_MST', id, {'vlan_str': vlan_str})

# cmd: config spanning-tree mst vlan del <id> <value>
@stp_mst_instance_vlan.command('del', short_help='Remove VLAN from MSTI')
@click.argument('id', metavar='<0-4094>', required=True, type=int)
@click.argument('vlan', metavar='<1-4094>', required=True, type=int)
@clicommon.pass_db
def delete(db, id, vlan):
    """Configure VLANs of a MST instance"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    is_valid_mst_id(ctx, id)
    is_valid_mst_instance_vlan(ctx, vlan)

    vlan_str_entry = db.cfgdb.get_entry('XSTP_MST', id).get('vlan_str')
    if vlan_str_entry:
        vid_list = expand_and_sort(vlan_str_entry.split(','))
        if vlan in vid_list:
            vid_list.remove(vlan)
        else:
            ctx.fail("mst id and vlan is not mapped")
    else:
        ctx.fail("mst id is not exist")
    if len(vid_list) > 0:
        vlan_str = join_range(vid_list)
        db.cfgdb.mod_entry('XSTP_MST', id, {'vlan_str': vlan_str})
    else:
        db.cfgdb.mod_entry('XSTP_MST', id, None)

    members = [ v for k, v in db.cfgdb.get_table('VLAN_MEMBER') if k == 'Vlan{}'.format(vlan) ]
    members = list(dict.fromkeys(members))
    for member in members:
        mst_key = '{}|{}'.format(id, member)
        if db.cfgdb.get_entry('XSTP_MST_INTERFACE', mst_key):
            db.cfgdb.set_entry('XSTP_MST_INTERFACE', mst_key, None)
###############################################
# STP interface commands implementation
###############################################

@xstp_spanning_tree.group('interface')
@clicommon.pass_db
def spanning_tree_interface(db):
    """Configure STP for interface"""
    pass

# cmd: config spanning-tree interface enable <interface_name>
@spanning_tree_interface.command('enable')
@click.argument('interface_name', required=True, type=str)
@clicommon.pass_db
def stp_interface_enable(db, interface_name):
    """Enable STP for interface"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
    if is_stp_enabled_for_interface(db.cfgdb, interface_name):
        ctx.fail("STP is already enabled for " + interface_name)
    db.cfgdb.set_entry('XSTP_INTERFACE', interface_name, {'spanning-disable': 'disabled'})

# cmd: config spanning-tree interface disable <interface_name>
@spanning_tree_interface.command('disable')
@click.argument('interface_name', required=True, type=str)
@clicommon.pass_db
def stp_interface_disable(db, interface_name):
    """Disable STP for interface"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
    db.cfgdb.set_entry('XSTP_INTERFACE', interface_name, {'spanning-disable': 'enabled'})

# cmd: config spanning-tree interface priority <interface_name> <value>
@spanning_tree_interface.command('priority')
@click.argument('interface_name', required=True, type=str)
@click.argument('priority', metavar='<0-240>', required=True, type=int)
@clicommon.pass_db
def stp_interface_priority(db, interface_name, priority):
    """Configure priority for interface"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
    check_if_stp_enabled_for_interface(ctx, db.cfgdb, interface_name)
    is_valid_interface_priority(ctx, priority)
    db.cfgdb.mod_entry('XSTP_INTERFACE', interface_name, {'port-priority': priority})

# cmd: config spanning-tree interface cost <interface_name> <value>
@spanning_tree_interface.command('cost')
@click.argument('interface_name', required=True, type=str)
@click.argument('cost', metavar='<1-200000000>', required=True, type=int)
@clicommon.pass_db
def stp_interface_cost(db, interface_name, cost):
    """Configure cost for interface"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
    check_if_stp_enabled_for_interface(ctx, db.cfgdb, interface_name)
    is_valid_interface_cost(ctx, cost)
    db.cfgdb.mod_entry('XSTP_INTERFACE', interface_name, {'cost': cost})

# # (option) cmd: config spanning-tree interface link_type {auto | point-to-point | shared} <interface_name>
# @spanning_tree_interface.command('link_type')
# @click.argument('type', metavar='<auto, point-to-point, shared>', required=True, type=click.Choice(['auto', 'point-to-point', 'shared']))
# @click.argument('interface_name', required=True, type=str)
# @clicommon.pass_db
# def stp_interface_link_type(db, link_type, interface_name):
#     """Configure the link type for Rapid Spanning Tree and Multiple Spanning Tree"""
#     ctx = click.get_current_context()
#     check_if_global_stp_enabled(db.cfgdb, ctx)
#     check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
#     check_if_stp_enabled_for_interface(ctx, db.cfgdb, interface_name)
#     db.cfgdb.mod_entry('XSTP_INTERFACE', interface_name, {'link-type': link_type})

# # (option) cmd: config spanning-tree interface tc_prop_stop {enable | disable} <interface_name>
# @spanning_tree_interface.command('tc_prop_stop')
# @click.argument('type', metavar='<enable, disable>', required=True, type=click.Choice(['enable', 'disable']))
# @click.argument('interface_name', required=True, type=str)
# @clicommon.pass_db
# def stp_interface_tc_prop_stop(db, tc_prop_stop, interface_name):
#     """Configure to stop the propagation of topology change notifications (TCN)"""
#     ctx = click.get_current_context()
#     check_if_global_stp_enabled(db.cfgdb, ctx)
#     check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
#     check_if_stp_enabled_for_interface(ctx, db.cfgdb, interface_name)
#     db.cfgdb.mod_entry('XSTP_INTERFACE', interface_name, {'tc-prop-stop': tc_prop_stop})

# # (option) cmd: config spanning-tree interface link_type {auto | point-to-point | shared} <interface_name>
# @spanning_tree_interface.command('protocol_migration')
# @click.argument('interface_name', required=True, type=str)
# @clicommon.pass_db
# def stp_interface_protocol_migration(db, interface_name):
#     """Re-check the appropriate BPDU format to send on the selected interface"""
#     ctx = click.get_current_context()
#     check_if_global_stp_enabled(db.cfgdb, ctx)
#     check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
#     check_if_stp_enabled_for_interface(ctx, db.cfgdb, interface_name)
#     # call xstpctl to execute cmd


# STP interface bpdu filter
@spanning_tree_interface.group('bpdu_filter')
@clicommon.pass_db
def spanning_tree_interface_bpdu_filter(db):
    """Configure STP bpdu filter for interface"""
    pass

# cmd: config spanning-tree interface bpdu_filter enable <interface_name>
@spanning_tree_interface_bpdu_filter.command('enable')
@click.argument('interface_name', required=True, type=str)
@clicommon.pass_db
def stp_interface_enable_bpdu_filter(db, interface_name):
    """Enable bpdu filter for interface"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
    check_if_stp_enabled_for_interface(ctx, db.cfgdb, interface_name)
    if is_admin_edge_port_enabled_for_interface(db.cfgdb, interface_name) == False:
        ctx.fail("Not allow to enable bpdu-filter function since the edge-port status is diabled on {}.".format(interface_name))
    db.cfgdb.mod_entry('XSTP_INTERFACE', interface_name, {'bpdu-filter': 'enabled'})

# cmd: config spanning-tree interface bpdu_filter disable <interface_name>
@spanning_tree_interface_bpdu_filter.command('disable')
@click.argument('interface_name', required=True, type=str)
@clicommon.pass_db
def stp_interface_disable_bpdu_filter(db, interface_name):
    """Disable bpdu filter for interface"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
    check_if_stp_enabled_for_interface(ctx, db.cfgdb, interface_name)
    db.cfgdb.mod_entry('XSTP_INTERFACE', interface_name, {'bpdu-filter': 'disabled'})

# STP interface root guard
@spanning_tree_interface.group('root_guard')
@clicommon.pass_db
def spanning_tree_interface_root_guard(db):
    """Configure STP root guard for interface"""
    pass

# cmd: config spanning-tree interface root_guard enable <interface_name>
@spanning_tree_interface_root_guard.command('enable')
@click.argument('interface_name', required=True, type=str)
@clicommon.pass_db
def stp_interface_enable_root_guard(db, interface_name):
    """Enable root guard for interface"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
    check_if_stp_enabled_for_interface(ctx, db.cfgdb, interface_name)
    db.cfgdb.mod_entry('XSTP_INTERFACE', interface_name, {'root-guard': 'enabled'})

# cmd: config spanning-tree interface root_guard disable <interface_name>
@spanning_tree_interface_root_guard.command('disable')
@click.argument('interface_name', required=True, type=str)
@clicommon.pass_db
def stp_interface_disable_root_guard(db, interface_name):
    """Enable root guard for interface"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
    check_if_stp_enabled_for_interface(ctx, db.cfgdb, interface_name)
    db.cfgdb.mod_entry('XSTP_INTERFACE', interface_name, {'root-guard': 'disabled'})

# STP interface bpdu guard
@spanning_tree_interface.group('bpdu_guard')
@clicommon.pass_db
def spanning_tree_interface_bpdu_guard(db):
    """Configure STP bpdu guard for interface"""
    pass

# cmd: config spanning-tree interface bpdu_guard enable <interface_name> [--auto-recovery <value>]
@spanning_tree_interface_bpdu_guard.command('enable')
@click.argument('interface_name', required=True, type=str)
@click.option('--auto-recovery', help="enabling the port after a specified timer has expired", type=click.IntRange(30, 86400), required=False)
@clicommon.pass_db
def stp_interface_enable_bpdu_guard(db, interface_name, auto_recovery):
    """Enable bpdu guard for interface"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
    check_if_stp_enabled_for_interface(ctx, db.cfgdb, interface_name)
    if is_admin_edge_port_enabled_for_interface(db.cfgdb, interface_name) == False:
        ctx.fail("Not allow to enable bpdu-guard function since the edge-port status is diabled on {}.".format(interface_name))

    fvs = {
        'bpdu-guard': 'enabled',
        'auto-recovery': 'disabled'
    }

    if auto_recovery:
        is_valid_interface_auto_recover_interval(ctx, auto_recovery)
        fvs['auto-recovery'] = 'enabled'
        fvs['auto-recovery-interval'] = auto_recovery

    db.cfgdb.mod_entry('XSTP_INTERFACE', interface_name, fvs)

# cmd: config spanning-tree interface bpdu_guard disable <interface_name>
@spanning_tree_interface_bpdu_guard.command('disable')
@click.argument('interface_name', required=True, type=str)
@clicommon.pass_db
def stp_interface_disable_bpdu_guard(db, interface_name):
    """Disable bpdu guard for interface"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
    check_if_stp_enabled_for_interface(ctx, db.cfgdb, interface_name)
    disable_interface_bpdu_guard(db.cfgdb, interface_name)

# STP interface edge port
@spanning_tree_interface.group('edge_port')
@clicommon.pass_db
def spanning_tree_interface_edge_port(db):
    """Configure STP edge port for interface"""
    pass

# cmd: config spanning-tree interface edge_port enable <interface_name>
@spanning_tree_interface_edge_port.command('enable')
@click.argument('interface_name', required=True, type=str)
@clicommon.pass_db
def stp_interface_enable_edge_port(db, interface_name):
    """Enable edge port for interface"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
    check_if_stp_enabled_for_interface(ctx, db.cfgdb, interface_name)
    db.cfgdb.mod_entry('XSTP_INTERFACE', interface_name, {'edge-port': 'true'})

# cmd: config spanning-tree interface edge_port disable <interface_name>
@spanning_tree_interface_edge_port.command('disable')
@click.argument('interface_name', required=True, type=str)
@clicommon.pass_db
def stp_interface_disable_edge_port(db, interface_name):
    """Disable edge port for interface"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
    check_if_stp_enabled_for_interface(ctx, db.cfgdb, interface_name)
    db.cfgdb.mod_entry('XSTP_INTERFACE', interface_name, {'edge-port': 'false'})
    # If the admin edge-configure disable, this BPDU filtering function will be disabled, too.
    db.cfgdb.mod_entry('XSTP_INTERFACE', interface_name, {'bpdu-filter': 'disabled'})
    # If the admin edge is changed to be disabled, this BPDU guard function shall become disabled automatically.
    disable_interface_bpdu_guard(db.cfgdb, interface_name)

# cmd: config spanning-tree interface edge_port auto <interface_name>
@spanning_tree_interface_edge_port.command('auto')
@click.argument('interface_name', required=True, type=str)
@clicommon.pass_db
def stp_interface_auto_edge_port(db, interface_name):
    """Set edge port for interface to auto"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
    check_if_stp_enabled_for_interface(ctx, db.cfgdb, interface_name)
    db.cfgdb.mod_entry('XSTP_INTERFACE', interface_name, {'edge-port': 'auto'})

# STP interface mst
@spanning_tree_interface.group('mst')
@clicommon.pass_db
def spanning_tree_interface_mst(db):
    """Configure STP settings for MST instance"""
    pass

# cmd: config spanning-tree interface mst priority <interface_name> <id> <value>
@spanning_tree_interface_mst.command('priority')
@click.argument('interface_name', required=True, type=str)
@click.argument('id', metavar='<0-4094>', required=True, type=int)
@click.argument('priority', metavar='<0-240>', required=True, type=int)
@clicommon.pass_db
def stp_interface_mst_priority(db, interface_name, id, priority):
    """Configure priority for interface of MST instance"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
    check_if_stp_enabled_for_interface(ctx, db.cfgdb, interface_name)
    is_valid_mst_id(ctx, id)
    is_valid_interface_mst_instance_priority(ctx, priority)
    mst_key = '{}|{}'.format(id, interface_name)
    db.cfgdb.mod_entry('XSTP_MST_INTERFACE', mst_key, {'port-priority': priority})

# cmd: config spanning-tree interface mst cost <interface_name> <id> <value>
@spanning_tree_interface_mst.command('cost')
@click.argument('interface_name', required=True, type=str)
@click.argument('id', metavar='<0-4094>', required=True, type=int)
@click.argument('cost', metavar='<1-200000000>', required=True, type=int)
@clicommon.pass_db
def stp_interface_mst_cost(db, interface_name, id, cost):
    """Configure cost for interface of MST instance"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
    check_if_stp_enabled_for_interface(ctx, db.cfgdb, interface_name)
    is_valid_mst_id(ctx, id)
    is_valid_interface_mst_instance_cost(ctx, cost)
    mst_key = '{}|{}'.format(id, interface_name)
    db.cfgdb.mod_entry('XSTP_MST_INTERFACE', mst_key, {'cost': cost})

# STP interface loopback_detection
@spanning_tree_interface.group('loopback_detection')
@clicommon.pass_db
def spanning_tree_interface_lb(db):
    """Configure STP loopback detection for interface"""
    pass

# cmd: config spanning-tree loopback_detection enable <interface_name>
@spanning_tree_interface_lb.command('enable')
@click.argument('interface_name', required=True, type=str)
@clicommon.pass_db
def stp_interface_lb_enable(db, interface_name):
    """Enable STP loopback detection for interface"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
    check_if_stp_enabled_for_interface(ctx, db.cfgdb, interface_name)
    db.cfgdb.mod_entry('XSTP_INTERFACE', interface_name, {'loopback-detection': 'enabled'})

# cmd: config spanning-tree loopback_detection disable <interface_name>
@spanning_tree_interface_lb.command('disable')
@click.argument('interface_name', required=True, type=str)
@clicommon.pass_db
def stp_interface_lb_disable(db, interface_name):
    """Disable STP loopback detection for interface"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
    check_if_stp_enabled_for_interface(ctx, db.cfgdb, interface_name)
    db.cfgdb.mod_entry('XSTP_INTERFACE', interface_name, {'loopback-detection': 'disabled'})

# cmd: config spanning-tree loopback_detection action {block | shutdown} <interface_name> [--shutdown-interval <value>]
@spanning_tree_interface_lb.command('action')
@click.argument('action', metavar='<block, shutdown>', required=True, type=click.Choice(['block', 'shutdown']))
@click.argument('interface_name', required=True, type=str)
@click.option('--shutdown-interval', help="enabling the port after a specified timer has expired", type=click.IntRange(60, 86400), required=False)
@clicommon.pass_db
def stp_interface_lb_action(db, action, interface_name, shutdown_interval):
    """Configure the action of loopback detection for interface"""
    ctx = click.get_current_context()
    check_if_global_stp_enabled(db.cfgdb, ctx)
    check_if_interface_is_valid(ctx, db.cfgdb, interface_name)
    check_if_stp_enabled_for_interface(ctx, db.cfgdb, interface_name)

    if action == 'shutdown' and shutdown_interval == None:
        shutdown_interval = STP_DEFAULT_LBD_SHUTDOWN_INTERVAL

    fvs = {
        'lbd-action': action,
        'lbd-shutdown-interval': shutdown_interval
    }

    db.cfgdb.mod_entry('XSTP_INTERFACE', interface_name, fvs)

# if __name__ == '__main__':
#     xstp_spanning_tree()
