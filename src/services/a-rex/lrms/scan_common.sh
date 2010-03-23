#!/usr/bin/env bash

# This file contains functions that are used througout the scan-*-job scripts.

# This function takes a time interval formatted as 789:12:34:56 (with days) or
# 12:34:56 (without days) and transforms it to seconds. It returns the result in
# the return_interval_seconds variable.
interval_to_seconds () {
    _interval_dhms=$1
    _interval_size=`echo $_interval_dhms | grep -o : | wc -l`
    if [ $_interval_size -eq 2 ]; then
        return_interval_seconds=`echo $_interval_dhms | tr : ' ' | awk '{print $1*60*60+$2*60+$3;}'`
    elif [ $_interval_size -eq 3 ]; then
        return_interval_seconds=`echo $_interval_dhms | tr : ' ' | awk '{print $1*24*60*60+$2*60*60+$3*60+$4;}'`
    else
        echo "Bad formatting of time interval: $_interval_dhms" >&2
        return_interval_seconds=0
    fi
    unset _interval_dhms _interval_size
}

# This function takes a date string in the form recognized by the date utility
# and transforms it into seconds in UTC time.  It returns the result in the
# return_date_seconds variable.
date_to_utc_seconds () {
    _date_string=$1
    _date_seconds=`date -d "$_date_string" +%s`
    date_seconds_to_utc "$_date_seconds"
    unset _date_string _date_seconds
}

# This function takes a timestamp as seconds in local time and transforms it into
# seconds in UTC time.  It returns the result in the return_date_seconds variable.
date_seconds_to_utc () {
    _date_seconds=$1
    _offset_hms=`date +"%::z"`
    _offset_seconds=`echo $_offset_hms | tr ':' ' ' | awk '{ print $1*60*60+$2*60+$3; }'`
    return_date_seconds=$(( $_date_seconds - ($_offset_seconds) ))
    unset _date_seconds _offset_hms _offset_seconds
}

# This function takes a timestamp as seconds and transforms it to Mds date
# format (YYYYMMDDHHMMSSZ).  It returns the result in the return_mds_date
# variable.
seconds_to_mds_date () {
    _date_seconds=$1
    return_mds_date=`date -d "@$_date_seconds" +"%Y%m%d%H%M%SZ"`
    unset _date_seconds
}

