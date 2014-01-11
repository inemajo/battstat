#!/usr/bin/python3

import  os
import  sys


class BattException(Exception):
    def __init__(self, reason):
        self.reason = reason
     
    def __str__(self):
        return self.reason

def     get_battlist():
    """Battlist() -> list
    
    Return all directorys at /sys/class/power_supply
    """

    battlist = []
    for f in os.listdir('/sys/class/power_supply'):
        battlist.append(f)
    return (battlist)

def     is_present(battname):
    """is_present(battname) -> Boolean

    Return True if battery "battname" is present
    """    
    if os.path.isfile("/sys/class/power_supply/%s/present" % (battname)):
        return (True)
    else:
        return (False)

def     seconds_to_time(sec):
    return ("%d:%d" % (sec / 3600, (sec / 60) % 60))
    

def     exist(battname):
    return os.path.isdir("/sys/class/power_supply/%s/" % (battname))

def     get_battinfo(battname):
    if not exist(battname):
        raise BattException("Battery name \"%s\" not exist" % (battname))
    battinfo={}
    battinfo["name"] = battname

    battinfo["path"] = "/sys/class/power_supply/%s/uevent" % (battname)
        
    if is_present(battname):
        battinfo["present"] = True
    else:
        battinfo["present"] = False
        return battinfo

    with open(battinfo["path"], 'r') as f:
        for line in f.readlines():
            key = line.split('=')[0]
            value = line[len(key)+1:-1]
            
            if key == "POWER_SUPPLY_CURRENT_NOW" or key == "POWER_SUPPLY_POWER_NOW":
                present_rate = int(value)
            elif key == "POWER_SUPPLY_ENERGY_NOW" or key == "POWER_SUPPLY_CHARGE_NOW":
                current = int(value)
            elif key == "POWER_SUPPLY_ENERGY_FULL" or key == "POWER_SUPPLY_CHARGE_FULL":
                full = int(value)
            elif key == "POWER_SUPPLY_STATUS":
                status = value

    if status == "Discharging":
        battinfo["time_remaining"] = int(float(current / present_rate * 3600))
    elif status == "Charging":
        battinfo["time_remaining"] = int(float((full - current)) / float(present_rate) * 3600)
    
    battinfo["status"] = status
    battinfo["charge_purcent"] = int(float(current) / float(full) * 100)
    return battinfo

if __name__ == "__main__":

    def         print_battstat(info):
        print("Name: %s" % (info["name"]))
        print("Present: %s" % (info["present"]))
        if info["present"]:
            print("Charge: %d%%" % (info["charge_purcent"]))
            print("Status: %s" % (info["status"]))
            if info["status"] == "Discharging" or info["status"] == "Charging":
                print("Remaining time %s" % (seconds_to_time(info["time_remaining"])))

    if len(sys.argv) == 1:
        for battname in get_battlist():
            print_battstat(get_battinfo(battname))
            print("")
    else:
        print_battstat(get_battinfo(sys.argv[1]))
