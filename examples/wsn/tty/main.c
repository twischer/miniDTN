/* 
 * Programm: tty
 *
 * This Programm reads commands from a given device
 * and simulate key presses
 *
 * it's written as part of an presenter
 *
 * code Licenced under Public Domain
 * Karsten Hinz <k.hinz@tu-bs.de>
 */
#include <stdlib.h> 
#include <stdio.h>
#define LINE_LENGTH 80

/*
 * usage:
 * ./tty <device>
 *
 * <device> could be /dev/ttyUSB0
 */
int main (int argc, char *argv[])
{
	char line[LINE_LENGTH];
	FILE *datei;

	datei = fopen (argv[1], "r");

	if (datei == NULL)
	{
		printf("Fehler beim oeffnen der Datei.");
		return 1;
	}

	while (1) {
		 	printf(":%s",fgets(line, LINE_LENGTH, datei));
			switch(line[0]) {
				case 'r': 
						 system("xdotool click 3");
					break;
				case 'l': 
						 system("xdotool click 1");
				case 'c': 
						 system("xdotool click 1");
					break;
				case 'q':
					//bla
					break;
			}
	}

	fclose (datei);
	return 0;
}

