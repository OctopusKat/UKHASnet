require 'serialport'

port_str="/dev/tty.usbmodem1a21"
baud_rate=9600
data_bits=8
stop_bits=1
parity=SerialPort::NONE

sp = SerialPort.new(port_str, baud_rate, data_bits, stop_bits, parity)

#just read forever  
 while true do  
   sp_char = sp.readline  
   if sp_char  
	dataFile = File.open('/Library/WebServer/Documents/data.txt', 'a')
	time = Time.new
	total_String = "#{time.strftime("%Y-%m-%d %H:%M:%S")}:- #{sp_char}"
	puts total_String
	dataFile.write(total_String)
	dataFile.close
   end  
 end  
