/* gs1.c - Verifies GS1 data */

/*
    libzint - the open source barcode library
    Copyright (C) 2009 Robin Stuart <robin@zint.org.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "common.h"

/* This code does some checks on the integrity of GS1 data. It is not intended
   to be bulletproof, nor does it report very accurately what problem was found
   or where, but should prevent some of the more common encoding errors */

void itostr(char ai_string[], int ai_value)
{
	int thou, hund, ten, unit;
	char temp[2];
	
	strcpy(ai_string, "(");
	thou = ai_value / 1000;
	hund = (ai_value - (1000 * thou)) / 100;
	ten = (ai_value - ((1000 * thou) + (100 * hund))) / 10;
	unit = ai_value - ((1000 * thou) + (100 * hund) + (10 * ten));
	
	temp[1] = '\0';
	if(ai_value >= 1000) { temp[0] = itoc(thou); concat(ai_string, temp); }
	if(ai_value >= 100) { temp[0] = itoc(hund); concat(ai_string, temp); }
	temp[0] = itoc(ten);
	concat(ai_string, temp);
	temp[0] = itoc(unit);
	concat(ai_string, temp);
	concat(ai_string, ")");
}

int gs1_verify(struct zint_symbol *symbol, unsigned char source[], char reduced[])
{
	int i, j, last_ai, ai_latch;
	char ai_string[6];
	int bracket_level, max_bracket_level, ai_length, max_ai_length, min_ai_length;
	int ai_value[100], ai_location[100], ai_count, data_location[100], data_length[100];
	int error_latch;
	
	/* Detect extended ASCII characters */
	for(i = 0; i <  ustrlen(source); i++) {
		if(source[i] >=128) {
			strcpy(symbol->errtxt, "Extended ASCII characters are not supported by GS1");
			return ERROR_INVALID_DATA;
		}
	}
	
	if(source[0] != '[') {
		strcpy(symbol->errtxt, "Data does not start with an AI");
		return ERROR_INVALID_DATA;
	}
	
	/* Check the position of the brackets */
	bracket_level = 0;
	max_bracket_level = 0;
	ai_length = 0;
	max_ai_length = 0;
	min_ai_length = 5;
	j = 0;
	for(i = 0; i < ustrlen(source); i++) {
		ai_length += j;
		if(source[i] == '[') { bracket_level++; j = 1; }
		if(source[i] == ']') {
			bracket_level--;
			if(ai_length < min_ai_length) { min_ai_length = ai_length; }
			j = 0;
			ai_length = 0; }
		if(bracket_level > max_bracket_level) { max_bracket_level = bracket_level; }
		if(ai_length > max_ai_length) { max_ai_length = ai_length; }
	}
	min_ai_length--;
	
	if(bracket_level != 0) {
		/* Not all brackets are closed */
		strcpy(symbol->errtxt, "Malformed AI in input data (brackets don\'t match)");
		return ERROR_INVALID_DATA;
	}
	
	if(max_bracket_level > 1) {
		/* Nested brackets */
		strcpy(symbol->errtxt, "Found nested brackets in input data");
		return ERROR_INVALID_DATA;
	}
	
	if(max_ai_length > 4) {
		/* AI is too long */
		strcpy(symbol->errtxt, "Invalid AI in input data (AI too long)");
		return ERROR_INVALID_DATA;
	}
	
	if(min_ai_length <= 1) {
		/* AI is too short */
		strcpy(symbol->errtxt, "Invalid AI in input data (AI too short)");
		return ERROR_INVALID_DATA;
	}
	
	if(ai_latch == 1) {
		/* Non-numeric data in AI */
		strcpy(symbol->errtxt, "Invalid AI in input data (non-numeric characters in AI)");
		return ERROR_INVALID_DATA;
	}
	
	ai_count = 0;
	for(i = 1; i < ustrlen(source); i++) {
		if(source[i - 1] == '[') {
			ai_location[ai_count] = i;
			j = 0;
			do {
				ai_string[j] = source[i + j];
				j++;
			} while (ai_string[j - 1] != ']');
			ai_string[j - 1] = '\0';
			ai_value[ai_count] = atoi(ai_string);
			ai_count++;
		}
	}
	
	for(i = 0; i < ai_count; i++) {
		data_location[i] = ai_location[i] + 3;
		if(ai_value[i] >= 100) { data_location[i]++; }
		if(ai_value[i] >= 1000) { data_location[i]++; }
		data_length[i] = 0;
		do {
			data_length[i]++;
		} while ((source[data_location[i] + data_length[i] - 1] != '[') && (source[data_location[i] + data_length[i] - 1] != '\0'));
		data_length[i]--;
	}
	
	error_latch = 0;
	strcpy(ai_string, "");
	for(i = 0; i < ai_count; i++) {
		switch (ai_value[i]) {
			case 0: if(data_length[i] != 18) { error_latch = 1; } break;
			case 1: if(data_length[i] != 14) { error_latch = 1; } break;
			case 2: if(data_length[i] != 14) { error_latch = 1; } break;
			case 3: if(data_length[i] != 14) { error_latch = 1; } break;
			case 4: if(data_length[i] != 16) { error_latch = 1; } break;
			case 11: if(data_length[i] != 6) { error_latch = 1; } break;
			case 12: if(data_length[i] != 6) { error_latch = 1; } break;
			case 13: if(data_length[i] != 6) { error_latch = 1; } break;
			case 14: if(data_length[i] != 6) { error_latch = 1; } break;
			case 15: if(data_length[i] != 6) { error_latch = 1; } break;
			case 16: if(data_length[i] != 6) { error_latch = 1; } break;
			case 17: if(data_length[i] != 6) { error_latch = 1; } break;
			case 18: if(data_length[i] != 6) { error_latch = 1; } break;
			case 19: if(data_length[i] != 6) { error_latch = 1; } break;
			case 20: if(data_length[i] != 2) { error_latch = 1; } break;
			case 23: error_latch = 2; break;
			case 24: error_latch = 2; break;
			case 25: error_latch = 2; break;
			case 39: error_latch = 2; break;
			case 40: error_latch = 2; break;
			case 41: error_latch = 2; break;
			case 42: error_latch = 2; break;
			case 70: error_latch = 2; break;
			case 80: error_latch = 2; break;
			case 81: error_latch = 2; break;
		}
		if((ai_value[i] >= 100) && (ai_value[i] <= 179)) { error_latch = 2; }
		if((ai_value[i] >= 1000) && (ai_value[i] <= 1799)) { error_latch = 2; }
		if((ai_value[i] >= 200) && (ai_value[i] <= 229)) { error_latch = 2; }
		if((ai_value[i] >= 2000) && (ai_value[i] <= 2299)) { error_latch = 2; }
		if((ai_value[i] >= 300) && (ai_value[i] <= 309)) { error_latch = 2; }
		if((ai_value[i] >= 3000) && (ai_value[i] <= 3099)) { error_latch = 2; }
		if((ai_value[i] >= 31) && (ai_value[i] <= 36)) { error_latch = 2; }
		if((ai_value[i] >= 310) && (ai_value[i] <= 369)) { error_latch = 2; }
		if((ai_value[i] >= 3100) && (ai_value[i] <= 3699)) { if(data_length[i] != 6) { error_latch = 1; } }
		if((ai_value[i] >= 370) && (ai_value[i] <= 379)) { error_latch = 2; }
		if((ai_value[i] >= 3700) && (ai_value[i] <= 3799)) { error_latch = 2; }
		if((ai_value[i] >= 410) && (ai_value[i] <= 415)) { if(data_length[i] != 13) { error_latch = 1; } }
		if((ai_value[i] >= 4100) && (ai_value[i] <= 4199)) { error_latch = 2; }
		if((ai_value[i] >= 700) && (ai_value[i] <= 703)) { error_latch = 2; }
		if((ai_value[i] >= 800) && (ai_value[i] <= 810)) { error_latch = 2; }
		if((ai_value[i] >= 900) && (ai_value[i] <= 999)) { error_latch = 2; }
		if((ai_value[i] >= 9000) && (ai_value[i] <= 9999)) { error_latch = 2; }
		if((error_latch < 4) && (error_latch > 0)) {
			/* error has just been detected: capture AI */
			itostr(ai_string, ai_value[i]);
			error_latch += 4;
		}
	}
	
	if(error_latch == 5) {
		strcpy(symbol->errtxt, "Invalid data length for AI ");
		concat(symbol->errtxt, ai_string);
		return ERROR_INVALID_DATA;
	}
	
	if(error_latch == 6) {
		strcpy(symbol->errtxt, "Invalid AI value ");
		concat(symbol->errtxt, ai_string);
		return ERROR_INVALID_DATA;
	}

	/* Resolve AI data - put resulting string in 'reduced' */
	j = 0;
	last_ai = 0;
	ai_latch = 1;
	for(i = 0; i < ustrlen(source); i++) {
		if((source[i] != '[') && (source[i] != ']')) {
			reduced[j] = source[i];
			j++;
		}
		if(source[i] == '[') {
			/* Start of an AI string */
			if(ai_latch == 0) {
				reduced[j] = '[';
				j++;
			}
			ai_string[0] = source[i + 1];
			ai_string[1] = source[i + 2];
			ai_string[2] = '\0';
			last_ai = atoi(ai_string);
			ai_latch = 0;
			/* The following values from "GS-1 General Specification version 8.0 issue 2, May 2008"
			figure 5.4.8.2.1 - 1 "Element Strings with Pre-Defined Length Using Application Identifiers" */
			if((last_ai >= 0) && (last_ai <= 4)) { ai_latch = 1; }
			if((last_ai >= 11) && (last_ai <= 20)) { ai_latch = 1; }
			if(last_ai == 23) { ai_latch = 1; } /* legacy support - see 5.3.8.2.2 */
			if((last_ai >= 31) && (last_ai <= 36)) { ai_latch = 1; }
			if(last_ai == 41) { ai_latch = 1; }
		}
		/* The ']' character is simply dropped from the input */
	}
	reduced[j] = '\0';
	
	/* the character '[' in the reduced string refers to the FNC1 character */
	return 0;
}