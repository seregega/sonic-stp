import re
import click
import subprocess
import utilities_common.cli as clicommon

XSTPCTRL_FILE_PATH = '/usr/bin/xstpctrl'

XSTP_MST_INSTANCE_ID_MIN = 0
XSTP_MST_INSTANCE_ID_MAX = 4094

def is_valid_mst_id(ctx, mst_id):
    if mst_id not in range(XSTP_MST_INSTANCE_ID_MIN, XSTP_MST_INSTANCE_ID_MAX + 1):
        ctx.fail("MST instance ID must be in range {}-{}.".format(XSTP_MST_INSTANCE_ID_MIN, XSTP_MST_INSTANCE_ID_MAX))

#
# This group houses Spanning_tree commands and subgroups
#
@click.group(cls=clicommon.AliasedGroup, invoke_without_command=True, name='spanning-tree')
@clicommon.pass_db
@click.pass_context
def xstp_spanning_tree(ctx, db):
    """Show spanning_tree commands"""
    global_cfg = db.cfgdb.get_entry("XSTP_GLOBAL", "Global")
    if not global_cfg or global_cfg['value'] == 'disabled':
        click.echo("Spanning-tree is not configured")
        exit(0)
    if ctx.invoked_subcommand is None:
        command = '{} show spanning-tree'.format(XSTPCTRL_FILE_PATH)
        clicommon.run_command(command)

@xstp_spanning_tree.command('brief')
@click.pass_context
def show_stp_brief(ctx):
    """Show spanning_tree brief information"""
    command = '{} show spanning-tree brief'.format(XSTPCTRL_FILE_PATH)
    clicommon.run_command(command)

@xstp_spanning_tree.command('interface')
@click.argument('ifname', metavar='<interface_name>', required=True)
@clicommon.pass_db
def show_stp_interface(db, ifname):
    """Show spanning_tree information of the specified interface"""
    if re.match(r'^Ethernet\d+', ifname):
        interface_type = 'e'
        interface_no = re.search(r'\d+', ifname).group()
    elif re.match(r'^PortChannel\d+', ifname):
        app_db = db.appldb
        lag_trunk_id_entry = app_db.get_entry('LAG_TRUNK_ID_TABLE', ifname)
        if lag_trunk_id_entry:
            interface_type = 'p'
            interface_no = lag_trunk_id_entry.get('trunk_id')
        else:
            return
    else:
        click.echo("Invalid interface name.")
        return

    command = '{} show spanning-tree {} {}'.format(XSTPCTRL_FILE_PATH, interface_type, interface_no)
    clicommon.run_command(command)

@xstp_spanning_tree.group('mst')
@clicommon.pass_db
def spanning_tree_mst(db):
    """Show spanning_tree information of the specified MST instance"""
    pass

@spanning_tree_mst.command('instance')
@click.argument('id', metavar='<0-4094>', required=True, type=int)
@clicommon.pass_db
def spanning_tree_mst_instance(db, id):
    """Show spanning_tree information of the specified MST instance"""
    ctx = click.get_current_context()
    is_valid_mst_id(ctx, id)
    command = '{} show spanning-tree m {}'.format(XSTPCTRL_FILE_PATH, id)
    clicommon.run_command(command)

@spanning_tree_mst.command('brief')
@click.argument('id', metavar='<0-4094>', required=True, type=int)
@clicommon.pass_db
def spanning_tree_mst_brief(db, id):
    """Show spanning_tree brief information of the specified MST instance"""
    ctx = click.get_current_context()
    is_valid_mst_id(ctx, id)
    command = '{} show spanning-tree m {} b'.format(XSTPCTRL_FILE_PATH, id)
    clicommon.run_command(command)

@spanning_tree_mst.command('interface')
@click.argument('id', metavar='<0-4094>', required=True, type=int)
@click.argument('ifname', metavar='<interface_name>', required=True)
@clicommon.pass_db
def spanning_tree_mst_interface(db, id, ifname):
    """Show spanning_tree brief information of the specified interface on a MST instance"""
    ctx = click.get_current_context()
    is_valid_mst_id(ctx, id)
    if re.match(r'^Ethernet\d+', ifname):
        interface_type = 'e'
        interface_no = re.search(r'\d+', ifname).group()
    elif re.match(r'^PortChannel\d+', ifname):
        app_db = db.appldb
        lag_trunk_id_entry = app_db.get_entry('LAG_TRUNK_ID_TABLE', ifname)
        if lag_trunk_id_entry:
            interface_type = 'p'
            interface_no = lag_trunk_id_entry.get('trunk_id')
        else:
            return
    else:
        click.echo("Invalid interface name.")
        return

    command = '{} show spanning-tree m {} {} {}'.format(XSTPCTRL_FILE_PATH, id, interface_type, interface_no)
    clicommon.run_command(command)

@spanning_tree_mst.command('configuration')
@clicommon.pass_db
def spanning_tree_mst_configuration(db):
    """Show spanning_tree configuration of the multiple spanning tree"""
    command = '{} show spanning-tree m c'.format(XSTPCTRL_FILE_PATH)
    clicommon.run_command(command)