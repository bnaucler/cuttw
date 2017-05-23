#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define VER "0.1"
#define TWLEN 140
#define TAILLEN 9
#define MXSZ 4096
#define SBCH 1024

typedef struct flag {
	int dfl; // text decoration? (not implemented)
	int pfl; // punctuation break flag
	int vfl; // verbosity (not implemented)
} flag;

typedef struct tweet {
	char txt[TWLEN];
	struct tweet *next;
} tweet;

int usage(char *err, int ret) {

	if(err) fprintf(stderr, "Error: %s\n", err);
	printf("Usage: cuttw [-dpv] file\n"
			"    -d: Text decoration\n"
			"    -p: Break at punctuation if possible\n"
			"    -v: Increase verbosity level\n");
	return ret;
}

void rbuf(FILE *fp, char *buf, flag *f) {

	char *sbuf = calloc(SBCH, sizeof(char));

	while(fgets(sbuf, MXSZ, fp)) strncat(buf, sbuf, SBCH);

	free(sbuf);
}

int rempref(char *str) {

	while(isspace(*str) || *str == '\n') str++;

	return 0;
}

// Fix
int remtail(char *str, char rem) {

	char *eptr = str;
	eptr += (strlen(str) - 1);

	while(*eptr == rem) *eptr-- = 0;

	return 0;
}

// TODO: get rid of unnecessary if-statement
int addtail(tweet *head, int numtw) {

	tweet *trav = head;
	char *tail = calloc(TAILLEN + 1, sizeof(char));
	int ctr = 1;

	while(*trav->txt) {
		snprintf(tail, TAILLEN, " (%d/%d)", ctr, numtw);
		strncat(trav->txt, tail, TAILLEN);
		ctr++;
		if(trav->next) trav = trav->next;
		else break;
	}

	free(tail);
	return 0;
}

int cuttw(tweet *head, char *buf, flag *f) {

	char *eptr = buf, *dptr = head->txt;
	tweet *trav = head;
	int lctr = 0, lcut = 0, hasp = 0, numtw = 1;

	do {
		if(f->pfl && (*eptr == '.' || *eptr == '!' || *eptr == '?')) {
			hasp++;
			lcut = lctr;
		} else if(!hasp && isspace(*eptr)) lcut = lctr;

		if(lctr > (TWLEN - TAILLEN)) {
			dptr -= (TWLEN - TAILLEN) - lcut;
			eptr -= (TWLEN - TAILLEN) - lcut;
			hasp = lctr = *dptr = 0;
			remtail(trav->txt, ' ');
			rempref(trav->txt);
			remtail(trav->txt, '\n'); // TODO
			trav->next = calloc(1, sizeof(tweet));
			trav = trav->next;
			dptr = trav->txt;
			numtw++;
		}

		*dptr++ = *eptr;
		lctr++;

	} while(*++eptr);

	return numtw;
}

int prdata(tweet *head, flag *f) {

	tweet *trav = head;
	int ctr = 1;

	// TODO: get rid of unnecessary if-statement
	while(trav->txt) {
		printf("%d: %s\n\n", ctr++, trav->txt);
		if(trav->next) trav = trav->next;
		else break;
	}

	return 0;
}

int execop(flag *f, int argc, char **argv) {

	char *buf = calloc(MXSZ, sizeof(char));
	tweet *head = calloc(1, sizeof(tweet));
	FILE *fp;
	int numtw = 0;

	do {
		fp = fopen(*argv, "r");
		if(!fp) {
			return usage("Could not read file", 2);
		} else {
			rbuf(fp, buf, f);
			numtw = cuttw(head, buf, f);
			addtail(head, numtw);
			prdata(head, f);
			fclose(fp);
		}

	} while(*++argv);

	free(buf);
	free(head);
	return 0;
}

// Cheap getopt
void setfl(char *oarg, flag *f) {

	char *arg = oarg;

	while(*++arg) {
		printf("arg: %c\n", *arg);
		switch(*arg) {

			case 'p':
				f->pfl++;
				break;

			case 'v':
				f->vfl++;
				break;
		}
	}
}

int main(int argc, char **argv) {

	if(argc < 2) return usage("No filename provided", 1);

	flag *f = calloc(1, sizeof(flag));

	if(argv[1][0] == '-') {
		setfl(argv++[1], f);
		argc--;
	}

	if(argc < 1) return usage("No filename provided", 1);
	else return execop(f, --argc, ++argv);

	return 0;
}
