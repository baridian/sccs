#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define MAX_LINE_LEN 300

#define yes 1
#define no 0

typedef int bool;

typedef enum opType
{
	ins, del, same, end, reserved
} opType;

typedef struct op
{
	opType op;
	int lineNum;
} op;

typedef struct sequence
{
	int numOperations;
	op *base;
} sequence;

/*returns the minimum of n arguments, with n defined by count*/
int minimum(int count, ...)
{
	va_list argP;
	int min = INT_MAX;
	int i = 0;
	int currentValue;
	va_start(argP, count);
	for (; i < count; i++)
	{
		currentValue = va_arg(argP, int);
		if (currentValue < min)
			min = currentValue;
	}
	va_end(argP);
	return min;
}

int lineCount(FILE *f)
{
	int lineCount = 1;
	int temp;
	for (; (temp = fgetc(f)) != EOF; lineCount += temp == '\n');
	return lineCount;
}

void getLine(FILE *f, int line, char *writeTo)
{
	int i = 0;
	int temp;
	for (; (temp = fgetc(f)) != EOF && i < line; i += temp == '\n');
	fgets(writeTo, MAX_LINE_LEN, f);
}

void sequenceAppend(sequence *appendTo, opType appending)
{
	appendTo->numOperations++;
	appendTo->base = realloc(appendTo->base, appendTo->numOperations * sizeof(op));
	appendTo->base[appendTo->numOperations - 1].op = appending;
}

sequence getTransformSequence(FILE *old, FILE *new)
{
	int fromLength = lineCount(old);
	int toLength = lineCount(new);
	int **distanceTable = malloc(fromLength * sizeof(int *));
	char oldLine[MAX_LINE_LEN];
	char newLine[MAX_LINE_LEN];
	int i = 0;
	int j;
	int subCost;
	int deletionCost;
	int insertionCost;
	opType toAppend;
	sequence toReturn;
	toReturn.base = malloc((size_t) (fromLength > toLength ? fromLength : toLength) * sizeof(op));
	for (; i < fromLength; i++) /*generate and zero table*/
		distanceTable[i] = calloc((size_t) toLength, sizeof(int));

	/*cost from empty string to non-empty is number of characters in
	non-empty string*/
	for (i = 1; i < fromLength; i++)
		distanceTable[i][0] = i;

	/*cost from empty string to non-empty is number of characters in
	non-empty string*/
	for (j = 1; j < toLength; j++)
		distanceTable[0][j] = j;

	for (i = 1; i < fromLength; i++)
	{
		for (j = 1; j < toLength; j++)
		{
			getLine(old, i - 1, oldLine);
			getLine(new, j - 1, newLine);
			subCost = distanceTable[i - 1][j - 1];
			if (strcmp(oldLine, newLine) != 0) /*if lines are equal*/
				subCost += 2; /* equal to insertion + deletion*/
			insertionCost = distanceTable[i][j - 1] + 1;
			deletionCost = distanceTable[i - 1][j] + 1;
			if (insertionCost < subCost && insertionCost < deletionCost)
				toAppend = ins;
			else if (deletionCost < subCost && deletionCost < insertionCost)
				toAppend = del;
			else
				toAppend = same;
			sequenceAppend(&toReturn, toAppend);


			distanceTable[i][j] = minimum(3, distanceTable[i - 1][j], /*deletion cost*/
										  distanceTable[i][j - 1], /*insertion cost*/
										  distanceTable[i - 1][j - 1] + subCost); /*substitution cost*/
		}
	}

	for (i = 0; i < fromLength; i++) /*free allocated memory*/
		free(distanceTable[i]);
	free(distanceTable);
	return toReturn;
}

void compressSequence(sequence *set)
{
	int newLength = 0;
	int i = 0;
	int last = 0;
	opType old = reserved;
	for (; i < set->numOperations; i++)
	{
		if (set->base[i].op != old)
		{
			if (set->base[i].op != same)
				newLength += 2;
		}
		old = set->base[i].op;
	}

	for (i = 0; i < set->numOperations; i++)
	{
		if (set->base[i].op != old)
		{
			if (old != same && old != reserved)
			{
				set->base[last].op = end;
				set->base[last].lineNum = i;
				last++;
			}
			if (set->base[i].op != same)
			{
				set->base[last].op = set->base[i].op;
				set->base[last].lineNum = i;
				last++;
			}
		}
		old = set->base[i].op;
	}

	set->base = realloc(set->base, newLength * sizeof(op));
	sequenceAppend(set, reserved);
}

bool isControlLine(char *line)
{
	int dummy;
	char controlChar = 'e';
	if(sscanf(line,"^%c%d\n",&controlChar,&dummy) == 2 || sscanf(line,"^e\n"))
	{
		switch(controlChar)
		{
			case 'i':
			case 'd':
			case 'e':
				return yes;
			default:
				return no;
		}
	}
	return no;
}

void interleaveDeltas(sequence compressed, FILE *sccs, FILE *new, int versionNum)
{
	int oldLine = 0;
	int newLine = 0;
	int outputLine = 0;
	char tempString[MAX_LINE_LEN];
	int i = 0;
	int op = same;
	FILE *temp = fopen("temp","w");
	/*rules:
	 N:lower level deletes in deletes
	 N:higher level deletes in deletes
	 Y:lower level inserts in deletes
	 N:higher level inserts in deletes
	 N:lower level deletes in inserts
	 Y:higher level deletes in inserts
	 N:lower level inserts in inserts
	 Y:higher level inserts in inserts*/
	for (; compressed.base[i].op != reserved; i++)
	{

	}
	rename("temp",".sccs");
	fclose(temp);
}

FILE *generateSource(int versionNum, FILE *sccs)
{
	int numLines = lineCount(sccs);
	int i = 0;
	int blockVersionNum;
	FILE *toReturn = fopen("old.sccs", "w");
	char line[MAX_LINE_LEN];
	bool canRead = yes;
	int layersInDelete = 0;
	for (; i < numLines; i++)
	{
		getLine(sccs, i, line);
		if (sscanf(line, "^i%d\n", &blockVersionNum))
		{
			if (blockVersionNum <= versionNum)
			{
				if (layersInDelete > 0)
					layersInDelete++;
				else
					canRead = yes;
			}
			else /*version number too high*/
				canRead = no;
		}
		else if (sscanf(line, "^d%d\n", &blockVersionNum))
		{
			canRead = no;
			if (blockVersionNum <= versionNum)
				layersInDelete++;
		}
		else if (sscanf(line, "^e\n"))
		{
			if (layersInDelete -= layersInDelete > 0, layersInDelete == 0)
				canRead = yes;
		}
		else if (canRead)
			fprintf(toReturn, "%s", line);
	}
}

int main()
{
	return 0;
}