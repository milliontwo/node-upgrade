# node-upgrade

Command line tool to upgrade AVR nodes running the millionboot bootloader in the milliontwo project.

## Usage
* Put all devices into firmware upgrade mode: 
`./node-upgrade -e`
* Upgrade the firmware on a device that is in firmware upgrade mode: `./node-upgrade -a {i2c address} -f {filepath of hex file}`


**Note:** This tool defaults to the i2c port `/dev/i2c-1`. You can select a different path with the `-d` option