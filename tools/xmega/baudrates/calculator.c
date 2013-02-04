#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double __pow(double base, int exp)
{
	double r;
	if(exp == 0)
		return 1;
	
	if(exp < 0)
	{
		exp *= -1;

		r = base;
		while(--exp > 0)
		{
			r *= base;
		}

		return 1.0f / r;
	}	

	r = base;
	while(--exp > 0)
	{
		r *= base;
	}

	return r;
}

void calculateBaud(unsigned int f_cpu, unsigned int baud, unsigned char summary)
{
	double baud_error_ok = 0.2;
	char bscale;
	char bscale_signed;
	double bsel;
	int bsel_rounded;
	int pow;
	double bsel_diff;
	unsigned int baud_actual;
	double baud_error;
	double best_baud_error = 100.0;
	int best_baud_upper;
	int best_baud_lower;
		
	if(!summary)
	{
		printf("BAUD = %d\n", baud);
		printf("\n");
		printf("  BSCALE |         BSEL |   BAUD |   ERROR | BAUDCTRLB | BAUDCTRLA \n --------|--------------|--------|---------|-----------|-----------\n");
	}

	// loop trough possible BSCALE values
	for(bscale=0;bscale<=0xf;bscale++)
	{
		if(bscale < 8)
		{
			bscale_signed = 0;
			bsel = (double) f_cpu / ( __pow(2.0f, bscale) * 16.0f * (double)baud) - 1.0f;
		}
		else if(bscale > 8)
		{
			bscale_signed = 0 - (16 + ~bscale) - 1;
			bsel = 1.0f / __pow(2.0f, bscale_signed) * ( (double) f_cpu / ( 16.0f * (double)baud) - 1.0f );
		}
		
		bsel_rounded = (int) bsel; // cut off the floating points
		bsel_diff = (double) bsel_rounded - bsel;
		
		// if bsel is < 0 and we cut off floating points, 

		if(bsel_diff <= -0.5)
		{
			bsel_rounded += 1;
		}
		else if(bsel_diff >= 0.5)
		{
			bsel_rounded -= 1;
		}
		
		if(bscale_signed == 0)
		{
			baud_actual = (int) ((double) f_cpu / (__pow(2.0f, bscale) * 16.0f * (bsel_rounded + 1.0)) );
		}
		else
		{
			baud_actual = ( (double) f_cpu / ( 16.0f * ( __pow(2.0f, bscale_signed) * bsel_rounded  + 1.0) )  ) ;
		}

		if(baud > baud_actual)
		{
			baud_error = baud_actual / (double) baud;
		}
		else
		{
			baud_error = (double) baud / baud_actual;
		}

		baud_error = (1 - baud_error) * 100;

		// printf("baud_error: %f", (1 - baud_error) * 100);

		// BAUDCTRLB: BSACLE[3:0] BSEL[11:8]
		// BAUDCTRLA:             BSEL[7:0]
		
		// @NOTE if bsel == 0 --> bsel = 2^BSCALE-1

		if(!summary)
		{
			printf("%8.2X | ", (int) bscale);
			if(bsel_rounded <= 0)
			{
				printf("   bsel < 0  | ");
			}
			else if(bsel >= 4096)
			{
				printf(" bsel > 4096 | ");
			}
			else
			{
				printf(" %11d | %6d | %5.2f %%", bsel_rounded, baud_actual, baud_error);

				if(baud_error <= baud_error_ok)
				{
					printf(" | %9.2X | %9.2X", (bscale << 4) | ((bsel_rounded & 0xFF00) >> 8), bsel_rounded & 0xFF);
				}
			}

			printf("\n");
		}

		if(bsel_rounded > 0 && bsel <= 4095 && baud_error < best_baud_error && baud_error < baud_error_ok)
		{
			best_baud_error = baud_error;
			best_baud_upper = (bscale << 4) | ((bsel_rounded & 0xFF00) >> 8);
			best_baud_lower = (bsel_rounded & 0xFF);
		}
		
	}

	if(summary)
	{
		printf("#define USART_BAUD_%d 0x%.2X%.2X\n", baud, best_baud_upper, best_baud_lower);
	}
}

int main(int argc, char** argv)
{
	unsigned int f_cpu = 0;
	unsigned int baud = 0;
	unsigned char summary = 0;
	unsigned char list = 0;
	int a;
	unsigned int baudrates[14] = {2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600, 76800, 115200, 230400, 250000, 500000, 1000000};
	

	for(a=1; a<argc; a++)
	{
		if(strstr(argv[a], "-f_cpu=") != NULL)
		{
			f_cpu = atoi(&argv[a][7]);
		}
		else if(strstr(argv[a], "-baud=") != NULL)
		{
			baud = atoi(&argv[a][6]);
		}
		else if(strstr(argv[a], "-summary") != NULL)
		{
			summary = 1;
		}
		else if(strstr(argv[a], "-list") != NULL)
		{
			list = 1;
		}
	}

	if(f_cpu == 0)
	{
		printf("F_CPU required: -f_cpu=<frequency>\n");	
	
		return -1;
	}

	printf("F_CPU = %d\n", f_cpu);

	if(list == 1)
	{
		for(a=0;a<14;a++)
		{
			calculateBaud(f_cpu, baudrates[a], summary);
		}
	}
	else
	{
		if(baud == 0)
		{
			printf("Baud Rate required: -baud=<baud_rate>\n");	

			return -1;
		}

		calculateBaud(f_cpu, baud, summary);
	}
	
	return 0;
}
