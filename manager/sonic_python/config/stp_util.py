import click
import datetime
import subprocess
import os
import itertools
import utilities_common.cli as clicommon
import logging

from natsort import natsorted

PVST_MAX_INSTANCES = 255

SONIC_CFGGEN_PATH = '/usr/local/bin/sonic-cfggen'
DEFAULT_CONFIG_FILE_DIR = '/etc/sonic'
DEFAULT_CONFIG_FILE_PATH = '{}/config_db.json'.format(DEFAULT_CONFIG_FILE_DIR)

def _get_intf_list_in_vlan_member_table(config_db):
    """
    Get info from REDIS ConfigDB and create interface to vlan mapping
    """
    get_int_vlan_configdb_info = config_db.get_table('VLAN_MEMBER')
    int_list = []
    for key in get_int_vlan_configdb_info:
        interface = key[1]
        if interface not in int_list:
            int_list.append(interface)
    return int_list

def save_and_backup_config():
    now = datetime.datetime.now()
    formatted_date = now.strftime("%Y-%m-%d_%H:%M:%S")
    backup_file_path = '{}/config_db_{}.json'.format(DEFAULT_CONFIG_FILE_DIR, formatted_date)

    click.echo("backuping configuration...")
    backup_cmd = 'cp {} {}'.format(DEFAULT_CONFIG_FILE_PATH, backup_file_path)
    clicommon.run_command(backup_cmd, display_cmd=True)

    click.echo("saving configuration...")
    save_cmd = "{} -d --print-data > {}".format(SONIC_CFGGEN_PATH, DEFAULT_CONFIG_FILE_PATH)
    clicommon.run_command(save_cmd, display_cmd=True)

def get_global_stp_forward_delay(db):
    stp_entry = db.get_entry('STP', "GLOBAL")
    forward_delay = stp_entry.get("forward_delay")
    return forward_delay

def get_global_stp_hello_time(db):
    stp_entry = db.get_entry('STP', "GLOBAL")
    hello_time = stp_entry.get("hello_time")
    return hello_time

def get_global_stp_max_age(db):
    stp_entry = db.get_entry('STP', "GLOBAL")
    max_age = stp_entry.get("max_age")
    return max_age

def get_global_stp_priority(db):
    stp_entry = db.get_entry('STP', "GLOBAL")
    priority = stp_entry.get("priority")
    return priority

def get_max_stp_instances():
    return PVST_MAX_INSTANCES

def enable_stp_for_interfaces(db):
    fvs = {'enabled': 'true',
           'root_guard': 'false',
           'bpdu_guard': 'false',
           'bpdu_guard_do_disable': 'false',
           'portfast': 'false',
           'uplink_fast': 'false'
           }
    port_dict = natsorted(db.get_table('PORT'))
    intf_list_in_vlan_member_table = _get_intf_list_in_vlan_member_table(db)

    for port_key in port_dict:
        if port_key in intf_list_in_vlan_member_table:
            db.set_entry('STP_PORT', port_key, fvs)

    po_ch_dict = natsorted(db.get_table('PORTCHANNEL'))
    for po_ch_key in po_ch_dict:
        if po_ch_key in intf_list_in_vlan_member_table:
            db.set_entry('STP_PORT', po_ch_key, fvs)

def enable_stp_for_vlans(db):
    vlan_count = 0
    fvs = {'enabled': 'true',
           'forward_delay': get_global_stp_forward_delay(db),
           'hello_time': get_global_stp_hello_time(db),
           'max_age': get_global_stp_max_age(db),
           'priority': get_global_stp_priority(db)
           }
    vlan_dict = natsorted(db.get_table('VLAN'))
    max_stp_instances = get_max_stp_instances()
    for vlan_key in vlan_dict:
        if vlan_count >= max_stp_instances:
            logging.warning("Exceeded maximum STP configurable VLAN instances for {}".format(vlan_key))
            break
        db.set_entry('STP_VLAN', vlan_key, fvs)
        vlan_count += 1
