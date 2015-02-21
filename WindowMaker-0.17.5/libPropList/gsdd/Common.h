#ifndef COMMON_H
#define COMMON_H

#define GIVEUP(x, y) { char buf[255]; \
                       fprintf(stderr, "gsdd: %s:\n", x); \
		       sprintf(buf, "gsdd: %s", y); \
                       perror(buf); \
		       fprintf(stderr, "gsdd: Giving up.\n"); \
		       BailOut(); }

#endif /* COMMON_H */
