#!/usr/bin/env bash

# This file contains functions that are used througout the scan-*-job scripts.

#This function takes a timestamp on the formats: 789:12:34:56 (with days) or 12:34:56 (without days)
#and transforms the time to seconds. It is returned in the return_clock_seconds variable.
get_seconds_clock () {
        clock=$1
        clock_space=`echo $clock | tr ':' ' '`
        clock_size=`echo $clock | grep -o ":" | wc -l`
        if [ $clock_size -eq 2 ]; then
                return_clock_seconds=`echo $clock_space | awk '{print $1*60*60+$2*60+$3;}'`
        elif [ $clock_size -eq 3 ]; then
                return_clock_seconds=`echo $clock_space | awk '{print $1*24*60*60+$2*60*60+$3*60+$4;}'`
        else
                echo "Bad formatting of clock" >&2
                return_clock_seconds=0
        fi
}

#This function takes a date on the form of the date utility, 
#checks the current timezone and transforms the time to utc.
#It returns the value in the return_date_seconds variable
get_seconds_utc () {
        date=$1
        timezoneoffset=`date +"%::z"`
        sign=${timezoneoffset:0:1}
        timezone=${timezoneoffset:1}
        timezone_sec=`echo $timezone | tr ':' ' ' | awk '{ print $1*60*60+$2*60+$3; }'`
        date_sec=`date -d "$date" +"%s"`
        return_date_sec=""
        if [ "x$sign" = "x+" ]; then
                return_date_seconds=$(( $date_sec - $timezone_sec ))
        else
                return_date_seconds=$(( $date_sec + $timezone_sec ))
        fi
}

#This function takes a timestamp as seconds and transforms it to YYYYMMDDHHMMSSZ.
#It returns the value in the return_date variable.
get_date_from_seconds () {
        return_date_sec=$1
        return_date=`date -d "@$return_date_sec" +"%Y%m%d%H%M%SZ"`
}
