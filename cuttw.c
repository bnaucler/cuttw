#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <termios.h>
#include <sys/ioctl.h>

#define VER "0.1"
#define TWLEN 140
#define TAILLEN 9
#define BBCH 4096 // Should be enough for any sane twittrer
#define MBCH 1024
#define SBCH 256

#define WHT				"\033[1m\033[37m"
#define RED				"\x1B[1m\033[31m"
#define RESET			"\033[0m"

typedef struct flag {
	int cfl; // color output
	int dfl; // text decoration
	int pfl; // punctuation break flag
	int vfl; // verbosity
	int row; // terminal rows
	int col; // terminal columns
} flag;

typedef struct tweet {
	char txt[TWLEN];
	struct tweet *next;
} tweet;

int usage(char *err, const int ret) {

	if(err) fprintf(stderr, "Error: %s\n", err);
	printf("Usage: cuttw [-cdpv] file\n"
			"    -c: Colored output\n"
			"    -d: Text decoration\n"
			"    -p: Break at punctuation if possible\n"
			"    -v: Increase verbosity level\n");
	return ret;
}

void rbuf(FILE *fp, char *buf) {

	char *sbuf = calloc(MBCH, sizeof(char));

	while(fgets(sbuf, BBCH, fp)) strncat(buf, sbuf, MBCH);

	free(sbuf);
}

void remsp(char *str) {

	char *ptr = str;
	int len = strlen(str);
	unsigned int ctr = 0;

	while(isspace(*ptr) || *ptr == '\n') {
		ctr++;
		ptr++;
	}

	memmove(str, str + ctr, len - ctr);

	ptr = str + (strlen(str) - 1);

	while(isspace(*ptr) || *ptr == '\n') *ptr-- = 0;
}

// TODO: Color
void addtail(tweet *head, const int numtw, const flag *f) {

	tweet *trav = head;
	char *tail = calloc(TAILLEN + 1, sizeof(char));
	unsigned int ctr = 0;

	while(ctr++ < numtw) {
		snprintf(tail, TAILLEN, " (%d/%d)", ctr, numtw);
		strncat(trav->txt, tail, TAILLEN);
		trav = trav->next;
	}

	free(tail);
}

int cuttw(tweet *head, char *buf, const flag *f) {

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
			remsp(trav->txt);
			trav->next = calloc(1, sizeof(tweet));
			trav = trav->next;
			dptr = trav->txt;
			numtw++;
		}

		*dptr++ = *eptr;
		lctr++;

	} while(*++eptr);

	remsp(trav->txt);

	return numtw;
}

char *spstr(char *str, const int len) {

	char *eptr = str, *lptr = str;
	unsigned int ctr = 0;

	do {
		if(isspace(*eptr)) lptr = eptr;
		if(ctr >= len) {
			*lptr = '\n';	
			eptr = lptr + 1;
			ctr = 0;
		}
		ctr++;
	
	} while(*++eptr);

	return str;
}

// TODO:	odd termsize alignment
//			varying alignment if numtw > 9
char *mkhdr(char *str, char *box, const int boxsz, const int ctr,
	const int numtw, const flag *f) {

	int sub = 7; // magic number (space taken by number + decoration)
	if(numtw > 9) sub += 2;

	int blen = (f->col - sub) / 2;

	char *tln = calloc(blen + 1, sizeof(char)), *ttrav = tln;
	unsigned int a = 0;
	
	for(a = 0; a < blen; a++) *ttrav++ = '-';

	if(f->cfl)
		snprintf(box, boxsz, RED "\n%c%s " WHT "(%d)" RED " %s%c\n" RESET "%s\n",
				'+', tln, ctr, tln, '+', spstr(str, f->col));
	else
		snprintf(box, boxsz, "\n%c%s (%d) %s%c\n%s\n",
				'+', tln, ctr, tln, '+', spstr(str, f->col));

	free(tln);
	return box;
}

int prdata(tweet *head, const int numtw, const flag *f) {

	char *box = calloc(MBCH, sizeof(char));
	tweet *trav = head;
	unsigned int ctr = 0;

	if(f->vfl > 1 && f->dfl)
		printf("DEBUG rows: %d, cols: %d\n\n", f->row, f->col);

	while(ctr++ < numtw) {
		if(f->dfl)
			printf("%s", mkhdr(trav->txt, box, MBCH, ctr, numtw, f));
		else
			printf("%s\n\n", trav->txt);

		trav = trav->next;
	}

	if(box) free(box);
	return 0;
}

int execop(const flag *f, char **argv) {

	char *buf = calloc(BBCH, sizeof(char));
	tweet *head = calloc(1, sizeof(tweet));
	FILE *fp;
	int numtw = 0;

	do {
		fp = fopen(*argv, "r");
		if(!fp) {
			return usage("Could not open file", 2);
		} else {
			rbuf(fp, buf);
			if(!strlen(buf)) {
				if(f->vfl) fprintf(stderr, "%s: empty file\n", *argv);
				continue;
			} else if(f->vfl) printf("%s:\n", *argv);
			numtw = cuttw(head, buf, f);
			addtail(head, numtw, f);
			prdata(head, numtw, f);
			fclose(fp);
		}

	} while(*++argv);

	free(buf);
	free(head);
	return 0;
}

// Cheap getopt
void setfl(char *arg, flag *f) {

	while(*++arg) {
		switch(*arg) {

			case 'c':
				f->cfl++;
				break;

			case 'd':
				f->dfl++;
				break;

			case 'p':
				f->pfl++;
				break;

			case 'v':
				f->vfl++;
				break;
		}
	}
}

void gettsz(flag *f) {

	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	f->row = w.ws_row;
	f->col = w.ws_col;
}

int main(int argc, char **argv) {

	flag *f = calloc(1, sizeof(flag));

	if(argc < 2 || (argc == 2 && argv[1][0] == '-'))
		return usage("No filename provided", 1);
	else if (argv[1][0] == '-')
		setfl(argv++[1], f);

	if(f->dfl) gettsz(f);

	return execop(f, ++argv);

	return 0;
}
