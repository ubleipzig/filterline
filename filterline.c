// filterline: filter file by line number.
// http://unix.stackexchange.com/a/209470/376
#include <stdio.h>
#include <limits.h>
#include <string.h>

static const char *PROGRAM_VERSION = "0.1.0";
// TODO(miku): allow this to set via env
static const int MAX_LINE_LENGTH = LINE_MAX * 16;

int main (int argc, char *argv[]) {

	FILE *L;
	FILE *F;

	unsigned int to_print;
	unsigned int current = 0;
	char line[MAX_LINE_LENGTH];

	char *VERSION = "-v";

	if (argc == 2 && strcmp(argv[1], VERSION) == 0) {
		printf("%s\n", PROGRAM_VERSION);
		return 0;
	}

	if (argc != 3) {
		printf("Usage: %s FILE1 FILE2\n\n", argv[0]);
		printf("FILE1: line numbers, FILE2: input file\n");
		printf("MAX_LINE_LENGTH=%d\n", MAX_LINE_LENGTH);
		return 0;
	}

	if ((L = fopen(argv[1], "r")) == NULL) {
		return 1;
	} else if ((F = fopen(argv[2], "r")) == NULL) {
		fclose(L);
		return 1;
	} else {
		while (fscanf(L, "%u", &to_print) > 0) {
			while (fgets(line, MAX_LINE_LENGTH, F) != NULL && ++current != to_print);
			if (current == to_print) {
				printf("%s", line);
			}
		}
		fclose(L);
		fclose(F);
		return 0;
	}
}
