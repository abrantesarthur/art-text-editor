/*** includes ***/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/*** data  ***/
struct config {
	int screenrows;
	int screencols;
	struct termios orig_termios;
};

struct config E;



/*** terminal ***/

/*
 * die(s)
 *	print error message 's' and exit
*/
void die(const char *s) {
	write(STDOUT_FILENO, "\x1b[2J", 4); // clear screen
	write(STDOUT_FILENO, "\x1b[H", 3);			// reposition cursor at top-left corner
	perror(s);
	exit(1);
}


// restore original termios attribute
void disableRawMode() {
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) {
		die("tcsetattr");
	}
}

/*
 enableRawMode();
		disabled local modes
			ECHO: echoes typed characters
			ICANON: reads line-by-line instead of byte-by-byte
			ISIG: activates ctrl+c and ctrl+z signals
			IEXTEN: enables ctrl+ V
		disable output modes
			OPOST: enables output processing (adding carriage return before newline)
		disabled input modes
			IXON: enables ctrl+S and ctrl+Q
			ICRNL: translates carriage return to newline on input
*/
void enableRawMode() {
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
	atexit(disableRawMode);	

	struct termios raw = E.orig_termios;
	tcgetattr(STDIN_FILENO, &raw);
	// disable input modes
	raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
	// disable output modes
	raw.c_oflag &= ~(OPOST);
	// disable control modes
	raw.c_cflag &= ~(CS8);
	// disable local modes 
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);	
	// set timeout
	raw.c_cc[VMIN] = 0; // minimum bytes before read can return
	raw.c_cc[VTIME] = 1; // maximum time to wait before read returns
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

// readKey()
//	waits for keypress and returns it
char readKey() {
	int nread;
	char c;
	while((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if(nread == -1 && errno != EAGAIN) die("read");
	}
	return c;
}

int getWindowSize(int *rows, int *cols) {
	struct winsize sz;

	if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		return -1;
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

/*** output ***/

void drawRows() {
	int y;
	for(y = 0; y < 24; y++) {
		write(STDOUT_FILENO, "~\r\n", 3);
	}
}

/* refreshScreen()
 * refreshes screen by writing scape sequence to terminal
*/
void refreshScreen() {
	write(STDOUT_FILENO, "\x1b[2J", 4); // clear screen
	write(STDOUT_FILENO, "\x1b[H", 3);	// reposition cursor at top-left corner

	drawRows();

	// reposition cursor
	write(STDOUT_FILENO, "\x1b[H", 3);
}

/*** input ***/
// processKeypress()
//	
void processKeypress() {
	char c = readKey();

	switch(c) {
		case CTRL_KEY('q'):
			exit(0);
			break;
	}
}

int main() {
	enableRawMode();

	while(1) {
		refreshScreen();
		processKeypress();
	}
	return 0;
}
