#!/bin/bash

readonly I2C_BUS=0
readonly I2C_MC_ADDRESS=0x08

readonly I2C_CONF_SECOND_ALARM1=27
readonly I2C_CONF_MINUTE_ALARM1=28
readonly I2C_CONF_HOUR_ALARM1=29
readonly I2C_CONF_DAY_ALARM1=30
readonly I2C_CONF_WEEKDAY_ALARM1=31

log()
{
  if [ $# -gt 1 ] ; then
    echo $2 "$1"
  else
    echo "$1"
  fi
}

bcd2dec()
{
  local result=$(($1/16*10+($1&0xF)))
  echo $result
}

dec2bcd()
{
  local result=$((10#$1/10*16+(10#$1%10)))
  echo $result
}

get_sys_timestamp()
{
  echo $(date +%s)
}

i2c_write(){
	i2cset -y $1 $2 $3 $4
}

system_to_rtc()
{
  log '  Writing system time to RTC...'
  local sys_ts=$(get_sys_timestamp)
  local sec=$(date -d @$sys_ts +%S)
  local min=$(date -d @$sys_ts +%M)
  local hour=$(date -d @$sys_ts +%H)
  local day=$(date -d @$sys_ts +%u)
  local date=$(date -d @$sys_ts +%d)
  local month=$(date -d @$sys_ts +%m)
  local year=$(date -d @$sys_ts +%y)
  i2cset -y ${I2C_BUS} $I2C_MC_ADDRESS 58 $(dec2bcd $sec)
  i2cset -y ${I2C_BUS} $I2C_MC_ADDRESS 59 $(dec2bcd $min)
  i2cset -y ${I2C_BUS} $I2C_MC_ADDRESS 60 $(dec2bcd $hour)
  i2cset -y ${I2C_BUS} $I2C_MC_ADDRESS 61 $(dec2bcd $date)
  i2cset -y ${I2C_BUS} $I2C_MC_ADDRESS 62 $(dec2bcd $day)
  i2cset -y ${I2C_BUS} $I2C_MC_ADDRESS 63 $(dec2bcd $month)
  i2cset -y ${I2C_BUS} $I2C_MC_ADDRESS 64 $(dec2bcd $year)
  TIME_UNKNOWN=2
  log '  Done :-)'
}

clear_startup_time()
{
  i2c_write ${I2C_BUS} $I2C_MC_ADDRESS $I2C_CONF_SECOND_ALARM1 0x00
  i2c_write ${I2C_BUS} $I2C_MC_ADDRESS $I2C_CONF_MINUTE_ALARM1 0x00
  i2c_write ${I2C_BUS} $I2C_MC_ADDRESS $I2C_CONF_HOUR_ALARM1 0x00
  i2c_write ${I2C_BUS} $I2C_MC_ADDRESS $I2C_CONF_DAY_ALARM1 0x00
}

set_startup_time()
{
  sec=$(dec2bcd $4)
  i2c_write ${I2C_BUS} $I2C_MC_ADDRESS $I2C_CONF_SECOND_ALARM1 $sec
  min=$(dec2bcd $3)
  i2c_write ${I2C_BUS} $I2C_MC_ADDRESS $I2C_CONF_MINUTE_ALARM1 $min
  hour=$(dec2bcd $2)
  i2c_write ${I2C_BUS} $I2C_MC_ADDRESS $I2C_CONF_HOUR_ALARM1 $hour
  date=$(dec2bcd $1)
  i2c_write ${I2C_BUS} $I2C_MC_ADDRESS $I2C_CONF_DAY_ALARM1 $date
}

system_to_rtc

printf "old alarm: DOW DAY HH:MM:SS %02x %02x %02x:%02x:%02x\n" $(i2cget -y 0 8 31) $(i2cget -y 0 8 30) $(i2cget -y 0 8 29) $(i2cget -y 0 8 28) $(i2cget -y 0 8 27)
# set startup alarm to 09:01:02 (hh:mm:ss)
#i2cset -y 0 8 29 0x14
#i2cset -y 0 8 28 0x35
#i2cset -y 0 8 27 0x02

#i2cset -y 0 8 30 0x$(date +"%d")

set_startup_time $(date -d '+1 day' +"%d") 09 00 00

## disable alarm day and alarm day of week
#i2cset -y 0 8 30 0x80
#i2cset -y 0 8 31 0x80

printf "new alarm: DOW DAY HH:MM:SS %02x %02x %02x:%02x:%02x\n" $(i2cget -y 0 8 31) $(i2cget -y 0 8 30) $(i2cget -y 0 8 29) $(i2cget -y 0 8 28) $(i2cget -y 0 8 27)
