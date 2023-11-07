#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

char *fgets(char *str, int n, FILE *stream)
{
	int first_char = fgetc(stream);
	if (first_char < 0)
		return NULL;
	
	int i = 0;
	str[i] = (char) first_char;
	str[i + 1] = 0;
	i++;
	
	while (i < n - 1)
	{
		int chr = fgetc(stream);
		if (chr < 0)
			break;
		
		if (chr == '\n')
		{
			ungetc(chr, stream);
			break;
		}
		
		str[i] = (char) chr;
		str[i + 1] = 0;
	}
	
	return str;
}

int sscanf(const char* in_str, const char* format, ...)
{
	// minimal implementation
	va_list vl;
	va_start(vl, format);
	
	while (*format)
	{
		if (*format != '%')
		{
			format++;
			break;
		}
		
		format++;
		if (*format)
			break;
		
		switch (*format)
		{
			case 'u':
			{
				uint32_t* value = va_arg(vl, uint32_t*);
				
				bool skipped_whitespace = false;
				uint32_t t = 0;
				
				while (*in_str)
				{
					// note: '\0' isn't counted as a space
					if (!skipped_whitespace)
					{
						skipped_whitespace = true;
						while (isspace(*in_str))
							in_str++;
					}
					
					if (!(*in_str))
						break;
					
					if (*in_str < '0' || *in_str > '9')
						break;
					
					t = t * 10 + (*in_str - '0');
					in_str++;
				}
				
				*value = t;
				break;
			}
			case 'f':
			{
				float* value = va_arg(vl, float*);
				
				bool skipped_whitespace = false;
				float t = 0;
				bool decimal = false;
				float power = 0.1f;
				
				while (*in_str)
				{
					// note: '\0' isn't counted as a space
					if (!skipped_whitespace)
					{
						skipped_whitespace = true;
						while (isspace(*in_str))
							in_str++;
					}
					
					if (!(*in_str))
						break;
					
					if ((*in_str < '0' || *in_str > '9') && *in_str != '.')
						break;
					
					if (*in_str == '.')
					{
						decimal = true;
						in_str++;
						continue;
					}
					
					if (decimal)
					{
						t = t + power * (float)(*in_str - '0');
						power *= 0.1f;
					}
					else
					{
						t = t * 10 + (*in_str - '0');
					}
					in_str++;
				}
				
				*value = t;
				break;
			}
		}
	}
	
	va_end(vl);
	return 0;
}
