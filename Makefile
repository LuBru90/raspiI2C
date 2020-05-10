default:
	gcc readAdc.c -l curl -o main

new:
	gcc newreadAdc.c -l wiringPi -o main

