#!/bin/sh

sec_to_time() {

    [ -z "$1" ] && return 1

    h=$(echo "( $1 / 3600 )" | bc)
    m=$(echo "( ( $1 / 60 ) % 60 )" | bc)
#    s=$(echo "( $1 % 60 )" | bc)
    
#    printf "%02d:%02d:%02d\n" $h $m $s
    printf "%02d:%02d\n" $h $m
    return 0
}

first="true"
for batt_id in `ls /sys/class/power_supply/`
do

    [ -n "$1" ] && [ "$1" '!=' "$batt_id" ] && continue
    [ -z "$first" ] && echo "" || first=""

    if [ -f "/sys/class/power_supply/$batt_id/present" ]
    then

	for info in $(cat "/sys/class/power_supply/$batt_id/uevent");
        do
	    key="`echo $info | tr '=' ' ' | awk '{print $1}'`"
	    value="`echo $info | tr '=' ' ' | awk '{print $2}'`"


            [ "$key" = POWER_SUPPLY_CURRENT_NOW ] && present_rate="$value"
            [ "$key" = POWER_SUPPLY_POWER_NOW ] && present_rate="$value"

	    [ "$key" = POWER_SUPPLY_ENERGY_NOW ] && remaining_cap="$value"
	    [ "$key" = POWER_SUPPLY_CHARGE_NOW ] && remaining_cap="$value"

	    [ "$key" = POWER_SUPPLY_ENERGY_FULL ] && full_cap="$value"
	    [ "$key" = POWER_SUPPLY_CHARGE_FULL ] && full_cap="$value"

	    [ "$key" = POWER_SUPPLY_STATUS ] && batt_status="$value"

        done

	if [ "$batt_status" = "Discharging" ]
	then
	    secs=`echo "$remaining_cap / $present_rate * 3600" | bc -l |  sed 's/\(.*\)\..*/\1/'`
	    remaining_time=`sec_to_time $secs`
	elif [ "$batt_status" = "Charging" ]
	then
	    secs=`echo "($full_cap - $remaining_cap) / $present_rate * 3600" | bc -l |  sed 's/\(.*\)\..*/\1/'`
	    remaining_time=`sec_to_time $secs`
	fi

	cap_percent="`echo "$remaining_cap / $full_cap * 100" | bc -l | sed 's/\(.*\)\..*/\1/'`"

	echo ID: $batt_id
	echo Present: Yes
	echo Charge: $cap_percent%
	[ -n "$remaining_time" ] && echo Temps restant: $remaining_time
	echo Status: $batt_status
	
    else
	echo ID: $batt_id
	echo Present: No
    fi
done
